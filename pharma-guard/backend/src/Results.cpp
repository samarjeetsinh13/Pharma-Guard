#include "Results.h"

#include "Json.h"
#include "Repository.h"
#include "Text.h"

namespace {
const char* kDisclaimer = "Educational demo only - uses a small local demo database. Not medical advice.";
}

// ------------------------------------------------------------- CheckResult

CheckResult& CheckResult::addNote(const std::string& note) {
    notes_.push_back(note);
    return *this;
}

CheckResult& CheckResult::addNote(const std::string& label, const std::string& value) {
    return addNote(label + ": " + value);  // the two-argument version reuses the one-argument version
}

CheckResult& CheckResult::operator<<(const std::string& note) { return addNote(note); }

CheckResult::operator ResultSummary() const { return ResultSummary(status(), headline()); }

std::string CheckResult::notesJson() const { return JsonArray::ofStrings(notes_).str(); }

// -------------------------------------------------------- SuitabilityResult

SuitabilityResult::SuitabilityResult(const std::string& illnessInput, const std::string& illnessName,
                                     bool illnessFound)
    : CheckResult("suitability"),
      illnessInput_(illnessInput),
      illnessName_(illnessName),
      illnessFound_(illnessFound) {}

int SuitabilityResult::count(const std::string& verdict) const {
    int n = 0;
    for (const Item& item : items_) {
        if (item.verdict == verdict) ++n;
    }
    return n;
}

std::string SuitabilityResult::status() const {
    if (!illnessFound_) return "ILLNESS_NOT_FOUND";
    if (count("NOT_SUITABLE") > 0) return "NOT_SUITABLE_FOUND";
    if (count("CAUTION") > 0 || count("UNKNOWN") > 0 || count("NOT_FOUND") > 0) return "REVIEW_NEEDED";
    return "ALL_SUITABLE";
}

std::string SuitabilityResult::headline() const {
    if (!illnessFound_) return "The illness \"" + illnessInput_ + "\" is not in the demo database.";
    std::string text = std::to_string(count("SUITABLE")) + " suitable, " + std::to_string(count("CAUTION")) +
                       " need caution, " + std::to_string(count("NOT_SUITABLE")) + " not suitable";
    int other = count("UNKNOWN") + count("NOT_FOUND");
    if (other > 0) text += ", " + std::to_string(other) + " unknown";
    return text + " for " + illnessName_ + ".";
}

std::string SuitabilityResult::toJson() const {
    JsonArray items;
    for (const Item& item : items_) {
        JsonObject o;
        o.add("input", item.input)
            .add("medicine", item.medicine)
            .add("drugClass", item.drugClass)
            .add("verdict", item.verdict)
            .add("reason", item.reason);
        items.addRaw(o.str());
    }
    JsonObject counts;
    counts.add("suitable", count("SUITABLE"))
        .add("caution", count("CAUTION"))
        .add("notSuitable", count("NOT_SUITABLE"))
        .add("unknown", count("UNKNOWN") + count("NOT_FOUND"));
    JsonObject root;
    root.add("module", module_)
        .add("status", status())
        .add("headline", headline())
        .add("illnessInput", illnessInput_)
        .add("illness", illnessName_)
        .add("illnessFound", illnessFound_)
        .addRaw("counts", counts.str())
        .addRaw("items", items.str())
        .addRaw("notes", notesJson())
        .add("disclaimer", kDisclaimer);
    return root.str();
}

// -------------------------------------------------------- InteractionResult

Severity InteractionResult::overall() const {
    Severity worst;  // starts at NONE
    for (const InteractionFinding& f : findings) worst = maxOf(worst, f.severity);  // function template
    return worst;
}

std::string InteractionResult::status() const {
    if (medicines.size() < 2) return "NEED_MORE_MEDICINES";
    return findings.empty() ? "NO_KNOWN_INTERACTIONS" : "INTERACTIONS_FOUND";
}

std::string InteractionResult::headline() const {
    if (medicines.size() < 2) return "At least two known medicines are needed to check interactions.";
    if (findings.empty()) {
        return "No known interactions in the demo database among " + std::to_string(pairsChecked) + " pair(s).";
    }
    return std::to_string(findings.size()) + " interaction(s) found among " + std::to_string(pairsChecked) +
           " pair(s); highest severity: " + overall().label() + ".";
}

std::string InteractionResult::toJson() const {
    JsonArray found;
    for (const InteractionFinding& f : findings) {
        JsonObject o;
        o.add("medicineA", f.medicineA)
            .add("medicineB", f.medicineB)
            .add("severity", f.severity.label())
            .add("severityLevel", static_cast<int>(f.severity))  // class-to-basic conversion
            .add("effect", f.effect)
            .add("advice", f.advice)
            .add("foundBy", f.foundBy);
        found.addRaw(o.str());
    }
    JsonObject threads;
    threads.add("workers", workers)
        .add("pairsChecked", pairsChecked)
        .add("mutexLocks", lockAcquisitions)
        .add("elapsedMs", elapsedMs)
        .addRaw("trace", JsonArray::ofStrings(trace).str());
    JsonObject root;
    root.add("module", module_)
        .add("status", status())
        .add("headline", headline())
        .add("overallSeverity", overall().label())
        .addRaw("medicines", JsonArray::ofStrings(medicines).str())
        .addRaw("unknownMedicines", JsonArray::ofStrings(unknown).str())
        .addRaw("interactions", found.str())
        .addRaw("cleanPairs", JsonArray::ofStrings(cleanPairs).str())
        .addRaw("thread", threads.str())
        .addRaw("notes", notesJson())
        .add("disclaimer", kDisclaimer);
    return root.str();
}

// -------------------------------------------------------- CounterfeitResult

std::string CounterfeitResult::toJson() const {
    JsonArray rows;
    for (const Check& c : checks) {
        JsonObject o;
        o.add("field", c.field).add("provided", c.provided).add("expected", c.expected).add("match", c.match);
        rows.addRaw(o.str());
    }
    std::string recordJson = "null";
    if (hasRecord) {
        JsonObject r;
        r.add("medicine", record.medicineName)
            .add("batch", record.batch.display())
            .add("manufacturer", record.manufacturer)
            .add("mfgDate", record.mfgDate)
            .add("expiryDate", record.expiryDate)
            .add("status", record.status);
        recordJson = r.str();
    }
    JsonObject root;
    root.add("module", module_)
        .add("status", verdict)
        .add("headline", message)
        .add("matchPercent", matchPercent)
        .addRaw("checks", rows.str())
        .addRaw("registryRecord", recordJson)
        .addRaw("notes", notesJson())
        .add("disclaimer", "Educational demo only - compares against a small fictional registry. Not a real verification service.");
    return root.str();
}
