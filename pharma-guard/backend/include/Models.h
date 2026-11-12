// Models.h - the "things" in our pharmacy world: medicines, illnesses, severities, batches.
//
// CONCEPTS SHOWN HERE
//   * Inheritance, access specifiers (public / protected / private)
//   * Constructors: default, parameterized, copy
//   * VIRTUAL INHERITANCE: Searchable and Printable both derive virtually from Record,
//     so a Medicine contains exactly ONE Record (no "diamond problem").
//   * Virtual functions: describe(), matches(), summary()
//   * Static variable + static function: Record::count_ / Record::instanceCount()
//   * "this" pointer: Record::setName returns *this so calls can be chained
//   * Operator overloading (member): Medicine::operator== and operator<
//   * Friend operator overloading: operator<< for Medicine, operator== for BatchNumber
//   * Basic-to-class conversion:  BatchNumber(const std::string&), Severity(int)
//   * Class-to-basic conversion:  BatchNumber::operator std::string(), Severity::operator int()
#pragma once

#include <atomic>
#include <iosfwd>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Record: the shared base class. Every medicine / illness has an id and a name.
// ---------------------------------------------------------------------------
class Record {
public:
    Record();                                                // default constructor
    Record(const std::string& id, const std::string& name);  // parameterized constructor
    Record(const Record& other);                             // copy constructor
    Record& operator=(const Record& other) = default;
    virtual ~Record();                                       // virtual: safe delete through base pointer

    const std::string& id() const { return id_; }
    const std::string& name() const { return name_; }

    // "this" pointer: returning *this lets us write  rec.setName("A").setName("B");
    Record& setName(const std::string& newName);

    // Virtual function: derived classes replace this with their own text.
    virtual std::string describe() const;

    // Static function: belongs to the class, reads the static counter below.
    static int instanceCount();

protected:               // visible to derived classes (Medicine, Illness) but not to outsiders
    std::string name_;

private:                 // visible only inside Record itself
    std::string id_;
    static std::atomic<int> count_;  // static variable: ONE copy shared by all Records
};

// ---------------------------------------------------------------------------
// Searchable and Printable: two "abilities". Both inherit VIRTUALLY from Record.
// ---------------------------------------------------------------------------
class Searchable : virtual public Record {
public:
    void addAlias(const std::string& alias);
    const std::vector<std::string>& aliases() const { return aliases_; }

    // True when the query equals the name or one of the aliases (ignoring case).
    virtual bool matches(const std::string& query) const;

protected:
    std::vector<std::string> aliases_;
};

class Printable : virtual public Record {
public:
    virtual std::string summary() const = 0;  // pure virtual: derived classes MUST provide it
};

// ---------------------------------------------------------------------------
// Medicine: inherits from BOTH abilities (multiple inheritance).
// ---------------------------------------------------------------------------
class Medicine : public Searchable, public Printable {
public:
    Medicine();
    Medicine(const std::string& id, const std::string& name, const std::string& genericName,
             const std::string& drugClass, const std::string& use);

    const std::string& genericName() const { return genericName_; }
    const std::string& drugClass() const { return drugClass_; }
    const std::string& use() const { return use_; }

    std::string describe() const override;
    std::string summary() const override;
    bool matches(const std::string& query) const override;

    // Member operator overloading: compare two medicines by id, order them by name.
    bool operator==(const Medicine& other) const;
    bool operator<(const Medicine& other) const;

    // Friend operator overloading: lets us write  std::cout << medicine;
    friend std::ostream& operator<<(std::ostream& os, const Medicine& m);

private:
    std::string genericName_;
    std::string drugClass_;
    std::string use_;
};

class Illness : public Searchable, public Printable {
public:
    Illness();
    Illness(const std::string& id, const std::string& name, const std::string& category,
            const std::string& description);

    const std::string& category() const { return category_; }
    const std::string& description() const { return description_; }

    std::string describe() const override;
    std::string summary() const override;

private:
    std::string category_;
    std::string description_;
};

// ---------------------------------------------------------------------------
// Severity: how serious an interaction is. A tiny wrapper around an int.
// ---------------------------------------------------------------------------
class Severity {
public:
    // BASIC -> CLASS conversion (not explicit on purpose):  Severity s = 3;
    Severity(int level = 0) : level_(level < 0 ? 0 : (level > 3 ? 3 : level)) {}

    // CLASS -> BASIC conversion:  int n = severity;
    operator int() const { return level_; }

    std::string label() const;                                  // "NONE", "MINOR", "MODERATE", "MAJOR"
    static bool tryParse(const std::string& label, Severity& out);

private:
    int level_;
};

// ---------------------------------------------------------------------------
// BatchNumber: a batch code typed by a person. "pcm 2501-a12" == "PCM-2501-A12".
// ---------------------------------------------------------------------------
class BatchNumber {
public:
    BatchNumber() = default;
    BatchNumber(const std::string& raw);  // BASIC -> CLASS: std::string becomes a BatchNumber
    BatchNumber(const char* raw);         // lets us write  BatchNumber b = "PCM-2501-A12";

    operator std::string() const { return display_; }  // CLASS -> BASIC: BatchNumber becomes std::string

    const std::string& display() const { return display_; }  // as typed
    const std::string& key() const { return key_; }          // normalized for comparing
    bool empty() const { return key_.empty(); }

    friend bool operator==(const BatchNumber& a, const BatchNumber& b) { return a.key_ == b.key_; }
    friend std::ostream& operator<<(std::ostream& os, const BatchNumber& b);

private:
    std::string display_;
    std::string key_;
};

// ---------------------------------------------------------------------------
// Plain data rows loaded from the text database.
// ---------------------------------------------------------------------------
struct SuitabilityRule {
    std::string illnessId, medicineId, verdict, reason;
};

struct InteractionRule {
    std::string medicineA, medicineB;  // medicine ids
    Severity severity;
    std::string effect, advice;
};

struct BatchRecord {
    std::string medicineId, medicineName;
    BatchNumber batch;
    std::string manufacturer, mfgDate, expiryDate, status;
};
