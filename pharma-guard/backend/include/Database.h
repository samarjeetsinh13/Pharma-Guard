// Database.h - loads the hand-built text database from the data/ folder.
//
// Five plain-text tables (pipe-separated) are read at start-up:
//   medicines.txt, illnesses.txt, suitability.txt, interactions.txt, registry.txt
//
// CONCEPTS SHOWN HERE
//   * Nested class: Database::Stats lives INSIDE Database
//   * Templates in use: Repository<Medicine> and Repository<Illness>
//   * Method overloading: findInteraction(ids) and findInteraction(Medicine, Medicine)
//
// THREAD SAFETY: after load() the database is never modified, so several
// worker threads may call the const find...() functions at the same time.
#pragma once

#include <string>
#include <vector>

#include "Models.h"
#include "Repository.h"

class Database {
public:
    // NESTED CLASS: only makes sense together with Database, so it lives inside it.
    class Stats {
    public:
        int medicines = 0;
        int illnesses = 0;
        int suitabilityRules = 0;
        int interactions = 0;
        int batches = 0;
        std::string toJson() const;
    };

    // Reads and validates all five files. On failure returns false and sets `error`;
    // the previously loaded data (if any) stays untouched.
    bool load(const std::string& dataDir, std::string& error);

    // Look-ups. They accept names, generic names or aliases, in any letter case.
    const Medicine* findMedicine(const std::string& nameOrAlias) const;
    const Illness* findIllness(const std::string& nameOrAlias) const;
    const Medicine* medicineById(const std::string& id) const;
    const Illness* illnessById(const std::string& id) const;
    const SuitabilityRule* findRule(const std::string& illnessId, const std::string& medicineId) const;

    // METHOD OVERLOADING: same job, two ways to call it.
    const InteractionRule* findInteraction(const std::string& idA, const std::string& idB) const;
    const InteractionRule* findInteraction(const Medicine& a, const Medicine& b) const;

    const BatchRecord* findBatch(const BatchNumber& batch) const;

    const Repository<Medicine>& medicines() const { return medicines_; }
    const Repository<Illness>& illnesses() const { return illnesses_; }
    Stats stats() const;
    const std::string& dataDir() const { return dataDir_; }

    // Everything in the database as JSON (for the "Database" tab in the web app).
    std::string toJson() const;

private:
    Repository<Medicine> medicines_;
    Repository<Illness> illnesses_;
    std::vector<SuitabilityRule> suitability_;
    std::vector<InteractionRule> interactions_;
    std::vector<BatchRecord> registry_;
    std::string dataDir_;
};
