// Checkers.h - the three modules of Pharma Guard.
//
// CONCEPTS SHOWN HERE
//   * Inheritance + virtual functions + abstract class: Checker is the base class;
//     each module overrides execute(). Code can hold a  Checker*  (base pointer)
//     that points at a derived object and still call the right version.
//   * "final": CounterfeitChecker cannot be inherited from, and
//     InteractionChecker::moduleName() cannot be overridden any further.
//   * Static variable/function: Checker::runs_ / Checker::totalRuns().
//   * Threads + Runnable abstraction: PairScanTask implements Runnable
//     ("implements Runnable"); ThreadRunner runs it on a real std::thread.
//   * Mutex synchronization: SharedFindings is written by several worker threads
//     at once, so every access is protected by a std::mutex.
#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "Concurrency.h"
#include "Database.h"
#include "Results.h"

// Everything a request can contain. Each module reads only the fields it needs.
struct CheckRequest {
    std::string illness;                // suitability
    std::vector<std::string> medicines; // suitability + interaction
    std::string medicine;               // counterfeit
    std::string batch;                  // counterfeit
    std::string manufacturer;           // counterfeit
};

// Abstract base class of all modules.
class Checker {
public:
    explicit Checker(const Database& db) : db_(db) {}
    virtual ~Checker() = default;  // virtual destructor: deleting through Checker* is safe

    virtual std::string moduleName() const = 0;
    virtual std::unique_ptr<CheckResult> execute(const CheckRequest& request) = 0;

    static int totalRuns();  // static function: how many checks ran since the program started

protected:
    void markRun();          // called by every execute()
    const Database& db_;

private:
    static std::atomic<int> runs_;  // static variable shared by all checkers
};

// Factory: returns a BASE pointer to a DERIVED object (polymorphism).
// moduleName is "suitability", "interaction" or "counterfeit"; returns nullptr otherwise.
std::unique_ptr<Checker> makeChecker(const std::string& moduleName, const Database& db);

// ------------------------------------------------------------ module 1
class SuitabilityChecker : public Checker {
public:
    using Checker::Checker;  // inherit the constructor
    std::string moduleName() const override { return "suitability"; }
    std::unique_ptr<CheckResult> execute(const CheckRequest& request) override;
};

// ------------------------------------------------------------ module 2
// Collects what the worker threads find. All public functions lock the mutex first.
class SharedFindings {
public:
    void addFinding(const InteractionFinding& finding);
    void addCleanPair(int pairIndex, const std::string& text);
    void countComparison();

    std::vector<InteractionFinding> findings() const;
    std::vector<std::pair<int, std::string>> cleanPairs() const;
    int comparisons() const;
    int lockAcquisitions() const;

private:
    mutable std::mutex mutex_;
    std::vector<InteractionFinding> findings_;
    std::vector<std::pair<int, std::string>> clean_;
    int comparisons_ = 0;
    int locks_ = 0;
};

// "implements Runnable": one worker's job = check its share of the medicine pairs.
class PairScanTask : public Runnable {
public:
    static constexpr int kLookupDelayMs = 20;  // pretend each lookup takes a little time

    PairScanTask(const Database& db, const std::vector<const Medicine*>& medicines, int workerIndex,
                 int workerCount, SharedFindings& shared, const std::string& threadName)
        : db_(db), medicines_(medicines), workerIndex_(workerIndex), workerCount_(workerCount),
          shared_(shared), threadName_(threadName) {}

    void run() override;

private:
    const Database& db_;
    std::vector<const Medicine*> medicines_;
    int workerIndex_;
    int workerCount_;
    SharedFindings& shared_;
    std::string threadName_;
};

class InteractionChecker : public Checker {
public:
    static constexpr int kWorkers = 2;  // number of worker threads

    using Checker::Checker;
    std::string moduleName() const override final { return "interaction"; }  // "final": no further overriding
    std::unique_ptr<CheckResult> execute(const CheckRequest& request) override;
};

// ------------------------------------------------------------ module 3
class CounterfeitChecker final : public Checker {  // "final": nobody may inherit from this class
public:
    using Checker::Checker;
    std::string moduleName() const override { return "counterfeit"; }
    std::unique_ptr<CheckResult> execute(const CheckRequest& request) override;
};
