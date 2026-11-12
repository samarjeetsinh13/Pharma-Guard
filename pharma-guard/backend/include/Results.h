// Results.h - what each of the three modules returns.
//
// CONCEPTS SHOWN HERE
//   * Inheritance + virtual functions: CheckResult is an abstract base class;
//     SuitabilityResult, InteractionResult and CounterfeitResult override toJson().
//   * Class-to-class conversion: any CheckResult converts to a ResultSummary
//     through CheckResult::operator ResultSummary().
//   * Method overloading: CheckResult::addNote(note) and addNote(label, value).
//   * Operator overloading (member): result << "note"
//   * Nested classes: SuitabilityResult::Item and CounterfeitResult::Check
//   * "this" pointer: addNote returns *this so calls can be chained.
//   * Templates in use: maxOf<Severity> picks the worst interaction.
#pragma once

#include <string>
#include <vector>

#include "Models.h"

// A tiny summary object: "what happened" in one status word and one sentence.
class ResultSummary {
public:
    ResultSummary(const std::string& status = "", const std::string& headline = "")
        : status_(status), headline_(headline) {}
    const std::string& status() const { return status_; }
    const std::string& headline() const { return headline_; }

private:
    std::string status_;
    std::string headline_;
};

// Abstract base class (it has pure virtual functions, so you cannot create a CheckResult directly).
class CheckResult {
public:
    explicit CheckResult(const std::string& module) : module_(module) {}
    virtual ~CheckResult() = default;

    virtual std::string toJson() const = 0;      // each module writes its own JSON
    virtual std::string status() const = 0;      // e.g. "VERIFIED"
    virtual std::string headline() const = 0;    // one human sentence

    // METHOD OVERLOADING
    CheckResult& addNote(const std::string& note);
    CheckResult& addNote(const std::string& label, const std::string& value);

    // OPERATOR OVERLOADING:  result << "first" << "second";
    CheckResult& operator<<(const std::string& note);

    // CLASS-TO-CLASS CONVERSION:  ResultSummary s = someResult;
    operator ResultSummary() const;

    const std::string& module() const { return module_; }

protected:
    std::string notesJson() const;
    std::string module_;
    std::vector<std::string> notes_;
};

// ------------------------------------------------------------ module 1
class SuitabilityResult : public CheckResult {
public:
    // NESTED CLASS: one row of the answer (one medicine).
    struct Item {
        std::string input;     // what the user typed
        std::string medicine;  // official name (or the input if unknown)
        std::string drugClass;
        std::string verdict;   // SUITABLE, CAUTION, NOT_SUITABLE, UNKNOWN or NOT_FOUND
        std::string reason;
    };

    SuitabilityResult(const std::string& illnessInput, const std::string& illnessName, bool illnessFound);
    void addItem(const Item& item) { items_.push_back(item); }

    std::string toJson() const override;
    std::string status() const override;
    std::string headline() const override;

private:
    int count(const std::string& verdict) const;
    std::string illnessInput_;
    std::string illnessName_;
    bool illnessFound_;
    std::vector<Item> items_;
};

// ------------------------------------------------------------ module 2
struct InteractionFinding {
    int pairIndex = 0;
    std::string medicineA, medicineB;
    Severity severity;
    std::string effect, advice, foundBy;  // foundBy = name of the worker thread
};

class InteractionResult : public CheckResult {
public:
    InteractionResult() : CheckResult("interaction") {}

    std::vector<std::string> medicines;   // official names that were checked
    std::vector<std::string> unknown;     // inputs that are not in the database
    std::vector<InteractionFinding> findings;
    std::vector<std::string> cleanPairs;  // pairs with no known interaction
    int pairsChecked = 0;
    int workers = 0;
    int lockAcquisitions = 0;             // how many times the shared mutex was locked
    int elapsedMs = 0;
    std::vector<std::string> trace;       // thread lifecycle messages

    Severity overall() const;

    std::string toJson() const override;
    std::string status() const override;
    std::string headline() const override;
};

// ------------------------------------------------------------ module 3
class CounterfeitResult : public CheckResult {
public:
    // NESTED CLASS: one compared field.
    struct Check {
        std::string field, provided, expected;
        bool match = false;
    };

    CounterfeitResult() : CheckResult("counterfeit") {}

    std::string verdict = "NOT_FOUND";  // VERIFIED, MISMATCH, RECALLED, EXPIRED, NOT_FOUND
    std::string message;
    int matchPercent = 0;
    std::vector<Check> checks;
    bool hasRecord = false;
    BatchRecord record;

    std::string toJson() const override;
    std::string status() const override { return verdict; }
    std::string headline() const override { return message; }
};
