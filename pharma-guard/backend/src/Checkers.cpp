#include "Checkers.h"

#include <algorithm>
#include <chrono>

#include "Text.h"

// ------------------------------------------------------------------ Checker

std::atomic<int> Checker::runs_{0};

int Checker::totalRuns() { return runs_.load(); }

void Checker::markRun() { ++runs_; }

std::unique_ptr<Checker> makeChecker(const std::string& moduleName, const Database& db) {
    if (moduleName == "suitability") return std::make_unique<SuitabilityChecker>(db);
    if (moduleName == "interaction") return std::make_unique<InteractionChecker>(db);
    if (moduleName == "counterfeit") return std::make_unique<CounterfeitChecker>(db);
    return nullptr;
}

// ------------------------------------------------------ SuitabilityChecker

std::unique_ptr<CheckResult> SuitabilityChecker::execute(const CheckRequest& request) {
    markRun();
    const Illness* illness = db_.findIllness(request.illness);
    auto result = std::make_unique<SuitabilityResult>(request.illness, illness ? illness->name() : request.illness,
                                                      illness != nullptr);
    if (!illness) {
        result->addNote("Hint", "Open the Database tab to see which illnesses are supported.");
    }

    std::vector<std::string> seen;  // to skip the same medicine typed twice
    for (const std::string& input : request.medicines) {
        SuitabilityResult::Item item;
        item.input = input;

        const Medicine* medicine = db_.findMedicine(input);
        if (!medicine) {
            item.medicine = input;
            item.verdict = "NOT_FOUND";
            item.reason = "This medicine is not in the demo database.";
            result->addItem(item);
            continue;
        }
        if (std::find(seen.begin(), seen.end(), medicine->id()) != seen.end()) {
            result->addNote("Duplicate ignored", medicine->name());
            continue;
        }
        seen.push_back(medicine->id());

        item.medicine = medicine->name();
        item.drugClass = medicine->drugClass();
        if (!illness) {
            item.verdict = "UNKNOWN";
            item.reason = "The illness is not in the database, so no rule can be checked.";
        } else if (const SuitabilityRule* rule = db_.findRule(illness->id(), medicine->id())) {
            item.verdict = rule->verdict;
            item.reason = rule->reason;
        } else {
            item.verdict = "UNKNOWN";
            item.reason = "No rule for this illness and medicine in the demo knowledge base. Ask a doctor or pharmacist.";
        }
        result->addItem(item);
    }
    return result;  // unique_ptr<SuitabilityResult> converts to unique_ptr<CheckResult>
}

// --------------------------------------------------------- SharedFindings

void SharedFindings::addFinding(const InteractionFinding& finding) {
    std::lock_guard<std::mutex> guard(mutex_);  // only one thread at a time may pass this line
    ++locks_;
    findings_.push_back(finding);
}

void SharedFindings::addCleanPair(int pairIndex, const std::string& text) {
    std::lock_guard<std::mutex> guard(mutex_);
    ++locks_;
    clean_.emplace_back(pairIndex, text);
}

void SharedFindings::countComparison() {
    std::lock_guard<std::mutex> guard(mutex_);
    ++locks_;
    ++comparisons_;
}

std::vector<InteractionFinding> SharedFindings::findings() const {
    std::lock_guard<std::mutex> guard(mutex_);
    return findings_;
}

std::vector<std::pair<int, std::string>> SharedFindings::cleanPairs() const {
    std::lock_guard<std::mutex> guard(mutex_);
    return clean_;
}

int SharedFindings::comparisons() const {
    std::lock_guard<std::mutex> guard(mutex_);
    return comparisons_;
}

int SharedFindings::lockAcquisitions() const {
    std::lock_guard<std::mutex> guard(mutex_);
    return locks_;
}

// ----------------------------------------------------------- PairScanTask

void PairScanTask::run() {
    // Every pair of medicines gets a running number (pairIndex). Worker k handles the
    // pairs whose number leaves remainder k when divided by the worker count, so the
    // work is split with no overlap.
    int pairIndex = 0;
    for (std::size_t i = 0; i < medicines_.size(); ++i) {
        for (std::size_t j = i + 1; j < medicines_.size(); ++j, ++pairIndex) {
            if (pairIndex % workerCount_ != workerIndex_) continue;  // not my pair

            ThreadRunner::sleepMs(kLookupDelayMs);  // sleep: simulate a slow lookup

            const Medicine& a = *medicines_[i];
            const Medicine& b = *medicines_[j];
            const InteractionRule* rule = db_.findInteraction(a, b);  // reading the database is safe: it is const
            shared_.countComparison();

            if (rule) {
                InteractionFinding finding;
                finding.pairIndex = pairIndex;
                finding.medicineA = a.name();
                finding.medicineB = b.name();
                finding.severity = rule->severity;
                finding.effect = rule->effect;
                finding.advice = rule->advice;
                finding.foundBy = threadName_;
                shared_.addFinding(finding);  // synchronized write
            } else {
                shared_.addCleanPair(pairIndex, a.name() + " + " + b.name());
            }
            ThreadRunner::yieldNow();  // yield: let the other worker run
        }
    }
}

// ----------------------------------------------------- InteractionChecker

std::unique_ptr<CheckResult> InteractionChecker::execute(const CheckRequest& request) {
    markRun();
    auto result = std::make_unique<InteractionResult>();

    // 1. Resolve what the user typed into known medicines (no duplicates).
    std::vector<const Medicine*> medicines;
    for (const std::string& input : request.medicines) {
        const Medicine* medicine = db_.findMedicine(input);
        if (!medicine) {
            result->unknown.push_back(input);
            continue;
        }
        bool duplicate = false;
        for (const Medicine* existing : medicines) {
            if (*existing == *medicine) duplicate = true;  // Medicine::operator==
        }
        if (duplicate) {
            result->addNote("Duplicate ignored", medicine->name());
            continue;
        }
        medicines.push_back(medicine);
    }
    for (const Medicine* m : medicines) result->medicines.push_back(m->name());

    if (!result->unknown.empty()) {
        result->addNote("Not in database (skipped)", Text::join(result->unknown));
    }
    if (medicines.size() < 2) {
        result->addNote("Tip", "Enter at least two medicines that exist in the database.");
        return result;
    }

    // 2. Start worker threads. Each one checks its share of the pairs.
    const int count = static_cast<int>(medicines.size());
    const int pairCount = count * (count - 1) / 2;
    const int workerCount = pairCount >= kWorkers ? kWorkers : 1;

    TraceLog trace;
    SharedFindings shared;
    std::vector<std::unique_ptr<PairScanTask>> tasks;
    std::vector<std::unique_ptr<ThreadRunner>> threads;

    const auto startedAt = std::chrono::steady_clock::now();
    trace.add("main: " + std::to_string(pairCount) + " pair(s) to check, creating " +
              std::to_string(workerCount) + " worker thread(s)");

    for (int w = 0; w < workerCount; ++w) {
        std::string name = "worker-" + std::to_string(w + 1);
        tasks.push_back(std::make_unique<PairScanTask>(db_, medicines, w, workerCount, shared, name));
        threads.push_back(std::make_unique<ThreadRunner>(name, tasks.back().get(), &trace));
    }
    for (auto& t : threads) t->start();           // NEW -> RUNNABLE -> RUNNING
    trace.add("main: workers started, calling join() to wait for them");
    for (auto& t : threads) t->join();            // main thread waits here
    trace.add("main: all workers joined (TERMINATED)");

    // 3. Collect the answers. Thread timing is random, so sort by pair number
    //    to give the user the same order every time.
    std::vector<InteractionFinding> findings = shared.findings();
    std::sort(findings.begin(), findings.end(),
              [](const InteractionFinding& x, const InteractionFinding& y) { return x.pairIndex < y.pairIndex; });
    std::vector<std::pair<int, std::string>> clean = shared.cleanPairs();
    std::sort(clean.begin(), clean.end());

    result->findings = findings;
    for (const auto& c : clean) result->cleanPairs.push_back(c.second);
    result->pairsChecked = shared.comparisons();
    result->workers = workerCount;
    result->lockAcquisitions = shared.lockAcquisitions();
    result->trace = trace.snapshot();
    result->elapsedMs = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startedAt).count());
    return result;
}

// ----------------------------------------------------- CounterfeitChecker

std::unique_ptr<CheckResult> CounterfeitChecker::execute(const CheckRequest& request) {
    markRun();
    auto result = std::make_unique<CounterfeitResult>();

    BatchNumber batch = request.batch;  // BASIC -> CLASS conversion (std::string becomes BatchNumber)
    const BatchRecord* record = db_.findBatch(batch);

    if (!record) {
        result->verdict = "NOT_FOUND";
        result->message = "Batch " + batch.display() + " is not in the demo registry, so this pack cannot be verified.";
        result->matchPercent = 0;
        CounterfeitResult::Check c;
        c.field = "Batch number";
        c.provided = batch.display();
        c.expected = "(no such batch)";
        c.match = false;
        result->checks.push_back(c);
        result->addNote("What this means", "An unknown batch number can be a typing mistake or a fake label.");
        return result;
    }

    result->hasRecord = true;
    result->record = *record;

    // Compare the three fields one by one.
    CounterfeitResult::Check batchCheck{"Batch number", batch.display(), record->batch.display(), true};

    const Medicine* typed = db_.findMedicine(request.medicine);  // accepts brand names / aliases too
    bool medicineMatches = typed ? (typed->id() == record->medicineId)
                                 : (Text::normalize(request.medicine) == Text::normalize(record->medicineName));
    CounterfeitResult::Check medicineCheck{"Medicine name", request.medicine, record->medicineName, medicineMatches};

    // Ignore spaces, dots and capital letters in company names: "Pvt. Ltd." == "pvt ltd".
    bool makerMatches = Text::normalizeCode(request.manufacturer) == Text::normalizeCode(record->manufacturer);
    CounterfeitResult::Check makerCheck{"Manufacturer", request.manufacturer, record->manufacturer, makerMatches};

    result->checks = {batchCheck, medicineCheck, makerCheck};
    int matches = 0;
    std::vector<std::string> wrong;
    for (const CounterfeitResult::Check& c : result->checks) {
        if (c.match) ++matches;
        else wrong.push_back(c.field);
    }
    result->matchPercent = matches * 100 / static_cast<int>(result->checks.size());

    if (!wrong.empty()) {
        result->verdict = "MISMATCH";
        result->message = "Possible counterfeit: " + Text::join(wrong, " and ") +
                          (wrong.size() == 1 ? " does" : " do") + " not match the registry record for this batch.";
    } else if (record->status == "RECALLED") {
        result->verdict = "RECALLED";
        result->message = "Details match, but this batch has been RECALLED. Do not use it.";
    } else if (record->expiryDate < Text::today()) {  // ISO dates compare correctly as text
        result->verdict = "EXPIRED";
        result->message = "Details match the registry, but this batch expired on " + record->expiryDate + ".";
    } else {
        result->verdict = "VERIFIED";
        result->message = "All details match the registry record. Batch is valid until " + record->expiryDate + ".";
    }
    result->addNote("Manufactured", record->mfgDate);
    result->addNote("Expires", record->expiryDate);
    return result;
}
