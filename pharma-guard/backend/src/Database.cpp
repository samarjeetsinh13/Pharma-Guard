#include "Database.h"

#include <fstream>
#include <set>
#include <utility>

#include "Json.h"
#include "Text.h"

namespace {

struct Row {
    int line;
    std::vector<std::string> cols;
};

// Reads one pipe-separated file. Skips blank lines and lines that start with '#'.
bool readTable(const std::string& path, std::size_t expectedCols, std::vector<Row>& rows,
               std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "Cannot open data file: " + path;
        return false;
    }
    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        if (lineNo == 1 && line.size() >= 3 && static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB && static_cast<unsigned char>(line[2]) == 0xBF) {
            line.erase(0, 3);  // remove a UTF-8 byte-order mark added by some editors
        }
        std::string trimmed = Text::trim(line);
        if (trimmed.empty() || trimmed[0] == '#') continue;
        std::vector<std::string> cols = Text::split(trimmed, '|');
        for (std::string& c : cols) c = Text::trim(c);
        if (cols.size() != expectedCols) {
            error = path + " line " + std::to_string(lineNo) + ": expected " +
                    std::to_string(expectedCols) + " columns separated by '|' but found " +
                    std::to_string(cols.size());
            return false;
        }
        rows.push_back(Row{lineNo, cols});
    }
    return true;
}

std::string where(const std::string& path, const Row& row) {
    return path + " line " + std::to_string(row.line) + ": ";
}

// Generic helper (a function template) used for both medicines and illnesses.
template <typename T>
const T* findByQuery(const Repository<T>& repo, const std::string& query, bool tryStripDose) {
    std::string q = Text::normalize(query);
    if (q.empty()) return nullptr;
    const T* hit = repo.findIf([&](const T& item) { return item.matches(q); });
    if (hit) return hit;
    if (tryStripDose) {
        std::string stripped = Text::stripDose(q);  // "paracetamol 500mg" -> "paracetamol"
        if (stripped != q) return repo.findIf([&](const T& item) { return item.matches(stripped); });
    }
    return nullptr;
}

}  // namespace

bool Database::load(const std::string& dataDir, std::string& error) {
    const std::string dir = dataDir.empty() ? "." : dataDir;
    auto pathOf = [&](const char* file) { return dir + "/" + file; };

    // Everything is built in local variables first and only committed at the very end,
    // so a mistake in a text file can never leave the database half-loaded.
    Repository<Medicine> medicines;
    Repository<Illness> illnesses;
    std::vector<SuitabilityRule> suitability;
    std::vector<InteractionRule> interactions;
    std::vector<BatchRecord> registry;
    std::vector<Row> rows;

    // ---- medicines.txt : id | name | generic | class | use | aliases
    {
        const std::string path = pathOf("medicines.txt");
        if (!readTable(path, 6, rows, error)) return false;
        std::set<std::string> ids, names;
        for (const Row& r : rows) {
            if (r.cols[0].empty() || r.cols[1].empty()) {
                error = where(path, r) + "id and name must not be empty";
                return false;
            }
            if (!ids.insert(r.cols[0]).second) {
                error = where(path, r) + "duplicate id " + r.cols[0];
                return false;
            }
            if (!names.insert(Text::normalize(r.cols[1])).second) {
                error = where(path, r) + "duplicate medicine name " + r.cols[1];
                return false;
            }
            Medicine m(r.cols[0], r.cols[1], r.cols[2], r.cols[3], r.cols[4]);
            for (const std::string& alias : Text::split(r.cols[5], ',')) m.addAlias(alias);
            medicines.add(m);
        }
        if (medicines.size() == 0) {
            error = path + ": no medicines found";
            return false;
        }
    }

    // ---- illnesses.txt : id | name | category | description | aliases
    {
        const std::string path = pathOf("illnesses.txt");
        rows.clear();
        if (!readTable(path, 5, rows, error)) return false;
        std::set<std::string> ids, names;
        for (const Row& r : rows) {
            if (r.cols[0].empty() || r.cols[1].empty()) {
                error = where(path, r) + "id and name must not be empty";
                return false;
            }
            if (!ids.insert(r.cols[0]).second) {
                error = where(path, r) + "duplicate id " + r.cols[0];
                return false;
            }
            if (!names.insert(Text::normalize(r.cols[1])).second) {
                error = where(path, r) + "duplicate illness name " + r.cols[1];
                return false;
            }
            Illness ill(r.cols[0], r.cols[1], r.cols[2], r.cols[3]);
            for (const std::string& alias : Text::split(r.cols[4], ',')) ill.addAlias(alias);
            illnesses.add(ill);
        }
        if (illnesses.size() == 0) {
            error = path + ": no illnesses found";
            return false;
        }
    }

    // ---- suitability.txt : illness | medicine | verdict | reason
    {
        const std::string path = pathOf("suitability.txt");
        rows.clear();
        if (!readTable(path, 4, rows, error)) return false;
        std::set<std::string> seen;
        for (const Row& r : rows) {
            const Illness* ill = findByQuery(illnesses, r.cols[0], false);
            const Medicine* med = findByQuery(medicines, r.cols[1], false);
            if (!ill) {
                error = where(path, r) + "unknown illness '" + r.cols[0] + "' (add it to illnesses.txt)";
                return false;
            }
            if (!med) {
                error = where(path, r) + "unknown medicine '" + r.cols[1] + "' (add it to medicines.txt)";
                return false;
            }
            const std::string verdict = r.cols[2];
            if (verdict != "SUITABLE" && verdict != "CAUTION" && verdict != "NOT_SUITABLE") {
                error = where(path, r) + "verdict must be SUITABLE, CAUTION or NOT_SUITABLE";
                return false;
            }
            if (!seen.insert(ill->id() + "/" + med->id()).second) {
                error = where(path, r) + "duplicate rule for " + ill->name() + " + " + med->name();
                return false;
            }
            suitability.push_back(SuitabilityRule{ill->id(), med->id(), verdict, r.cols[3]});
        }
    }

    // ---- interactions.txt : medicine A | medicine B | severity | effect | advice
    {
        const std::string path = pathOf("interactions.txt");
        rows.clear();
        if (!readTable(path, 5, rows, error)) return false;
        std::set<std::string> seen;
        for (const Row& r : rows) {
            const Medicine* a = findByQuery(medicines, r.cols[0], false);
            const Medicine* b = findByQuery(medicines, r.cols[1], false);
            if (!a || !b) {
                error = where(path, r) + "unknown medicine '" + (a ? r.cols[1] : r.cols[0]) +
                        "' (add it to medicines.txt)";
                return false;
            }
            if (a->id() == b->id()) {
                error = where(path, r) + "a medicine cannot interact with itself";
                return false;
            }
            Severity severity;
            if (!Severity::tryParse(r.cols[2], severity)) {
                error = where(path, r) + "severity must be MINOR, MODERATE or MAJOR";
                return false;
            }
            std::string key = a->id() < b->id() ? a->id() + "/" + b->id() : b->id() + "/" + a->id();
            if (!seen.insert(key).second) {
                error = where(path, r) + "duplicate pair " + a->name() + " + " + b->name();
                return false;
            }
            interactions.push_back(InteractionRule{a->id(), b->id(), severity, r.cols[3], r.cols[4]});
        }
    }

    // ---- registry.txt : medicine | batch | manufacturer | mfg date | expiry date | status
    {
        const std::string path = pathOf("registry.txt");
        rows.clear();
        if (!readTable(path, 6, rows, error)) return false;
        std::set<std::string> batchKeys;
        for (const Row& r : rows) {
            const Medicine* med = findByQuery(medicines, r.cols[0], false);
            if (!med) {
                error = where(path, r) + "unknown medicine '" + r.cols[0] + "' (add it to medicines.txt)";
                return false;
            }
            BatchNumber batch = r.cols[1];  // basic-to-class conversion
            if (batch.empty()) {
                error = where(path, r) + "batch number must not be empty";
                return false;
            }
            if (!batchKeys.insert(batch.key()).second) {
                error = where(path, r) + "duplicate batch number " + r.cols[1];
                return false;
            }
            if (r.cols[2].empty()) {
                error = where(path, r) + "manufacturer must not be empty";
                return false;
            }
            if (!Text::isIsoDate(r.cols[3]) || !Text::isIsoDate(r.cols[4])) {
                error = where(path, r) + "dates must look like YYYY-MM-DD";
                return false;
            }
            if (r.cols[5] != "GENUINE" && r.cols[5] != "RECALLED") {
                error = where(path, r) + "status must be GENUINE or RECALLED";
                return false;
            }
            registry.push_back(BatchRecord{med->id(), med->name(), batch, r.cols[2], r.cols[3], r.cols[4], r.cols[5]});
        }
    }

    // Commit. Moving whole containers is cheap and cannot fail half-way.
    medicines_ = std::move(medicines);
    illnesses_ = std::move(illnesses);
    suitability_ = std::move(suitability);
    interactions_ = std::move(interactions);
    registry_ = std::move(registry);
    dataDir_ = dir;
    return true;
}

const Medicine* Database::findMedicine(const std::string& nameOrAlias) const {
    return findByQuery(medicines_, nameOrAlias, true);
}

const Illness* Database::findIllness(const std::string& nameOrAlias) const {
    return findByQuery(illnesses_, nameOrAlias, false);
}

const Medicine* Database::medicineById(const std::string& id) const {
    return medicines_.findIf([&](const Medicine& m) { return m.id() == id; });
}

const Illness* Database::illnessById(const std::string& id) const {
    return illnesses_.findIf([&](const Illness& i) { return i.id() == id; });
}

const SuitabilityRule* Database::findRule(const std::string& illnessId, const std::string& medicineId) const {
    for (const SuitabilityRule& rule : suitability_) {
        if (rule.illnessId == illnessId && rule.medicineId == medicineId) return &rule;
    }
    return nullptr;
}

const InteractionRule* Database::findInteraction(const std::string& idA, const std::string& idB) const {
    for (const InteractionRule& rule : interactions_) {
        if ((rule.medicineA == idA && rule.medicineB == idB) || (rule.medicineA == idB && rule.medicineB == idA)) {
            return &rule;
        }
    }
    return nullptr;
}

const InteractionRule* Database::findInteraction(const Medicine& a, const Medicine& b) const {
    return findInteraction(a.id(), b.id());  // the Medicine version just forwards to the id version
}

const BatchRecord* Database::findBatch(const BatchNumber& batch) const {
    for (const BatchRecord& record : registry_) {
        if (record.batch == batch) return &record;  // uses the friend operator==
    }
    return nullptr;
}

std::string Database::Stats::toJson() const {
    JsonObject o;
    o.add("medicines", medicines)
        .add("illnesses", illnesses)
        .add("suitabilityRules", suitabilityRules)
        .add("interactions", interactions)
        .add("batches", batches);
    return o.str();
}

Database::Stats Database::stats() const {
    Stats s;
    s.medicines = static_cast<int>(medicines_.size());
    s.illnesses = static_cast<int>(illnesses_.size());
    s.suitabilityRules = static_cast<int>(suitability_.size());
    s.interactions = static_cast<int>(interactions_.size());
    s.batches = static_cast<int>(registry_.size());
    return s;
}

std::string Database::toJson() const {
    JsonArray meds;
    for (const Medicine& m : medicines_.all()) {
        JsonObject o;
        o.add("id", m.id())
            .add("name", m.name())
            .add("generic", m.genericName())
            .add("drugClass", m.drugClass())
            .add("use", m.use())
            .addRaw("aliases", JsonArray::ofStrings(m.aliases()).str());
        meds.addRaw(o.str());
    }
    JsonArray ills;
    for (const Illness& i : illnesses_.all()) {
        JsonObject o;
        o.add("id", i.id())
            .add("name", i.name())
            .add("category", i.category())
            .add("description", i.description())
            .addRaw("aliases", JsonArray::ofStrings(i.aliases()).str());
        ills.addRaw(o.str());
    }
    JsonArray rules;
    for (const SuitabilityRule& r : suitability_) {
        const Illness* ill = illnessById(r.illnessId);
        const Medicine* med = medicineById(r.medicineId);
        JsonObject o;
        o.add("illness", ill ? ill->name() : r.illnessId)
            .add("medicine", med ? med->name() : r.medicineId)
            .add("verdict", r.verdict)
            .add("reason", r.reason);
        rules.addRaw(o.str());
    }
    JsonArray inter;
    for (const InteractionRule& r : interactions_) {
        const Medicine* a = medicineById(r.medicineA);
        const Medicine* b = medicineById(r.medicineB);
        JsonObject o;
        o.add("medicineA", a ? a->name() : r.medicineA)
            .add("medicineB", b ? b->name() : r.medicineB)
            .add("severity", r.severity.label())
            .add("effect", r.effect)
            .add("advice", r.advice);
        inter.addRaw(o.str());
    }
    JsonArray reg;
    const std::string today = Text::today();
    for (const BatchRecord& r : registry_) {
        JsonObject o;
        o.add("medicine", r.medicineName)
            .add("batch", r.batch.display())
            .add("manufacturer", r.manufacturer)
            .add("mfgDate", r.mfgDate)
            .add("expiryDate", r.expiryDate)
            .add("status", r.status)
            .add("expired", r.expiryDate < today);
        reg.addRaw(o.str());
    }
    JsonObject root;
    root.add("today", today)
        .add("dataDir", dataDir_)
        .addRaw("stats", stats().toJson())
        .addRaw("medicines", meds.str())
        .addRaw("illnesses", ills.str())
        .addRaw("suitability", rules.str())
        .addRaw("interactions", inter.str())
        .addRaw("registry", reg.str());
    return root.str();
}
