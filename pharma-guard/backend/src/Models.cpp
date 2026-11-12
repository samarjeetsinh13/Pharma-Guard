#include "Models.h"

#include <ostream>

#include "Text.h"

// ------------------------------------------------------------------ Record

std::atomic<int> Record::count_{0};  // the single shared counter lives here

Record::Record() : name_("unnamed"), id_("") { ++count_; }

Record::Record(const std::string& id, const std::string& name) : name_(name), id_(id) { ++count_; }

Record::Record(const Record& other) : name_(other.name_), id_(other.id_) { ++count_; }

Record::~Record() { --count_; }

Record& Record::setName(const std::string& newName) {
    this->name_ = newName;  // "this" points at the object the function was called on
    return *this;           // returning *this allows chaining
}

std::string Record::describe() const { return "Record[" + id_ + "] " + name_; }

int Record::instanceCount() { return count_.load(); }

// -------------------------------------------------------------- Searchable

void Searchable::addAlias(const std::string& alias) {
    std::string clean = Text::normalize(alias);
    if (!clean.empty()) aliases_.push_back(clean);
}

bool Searchable::matches(const std::string& query) const {
    std::string q = Text::normalize(query);
    if (q.empty()) return false;
    if (Text::normalize(name_) == q) return true;
    for (const std::string& alias : aliases_) {
        if (alias == q) return true;
    }
    return false;
}

// ---------------------------------------------------------------- Medicine

Medicine::Medicine() : Record("", "unnamed medicine") {}

// With virtual inheritance the MOST DERIVED class (Medicine) must construct the
// shared Record itself - that is why Record(id, name) is called right here.
Medicine::Medicine(const std::string& id, const std::string& name, const std::string& genericName,
                   const std::string& drugClass, const std::string& use)
    : Record(id, name), genericName_(genericName), drugClass_(drugClass), use_(use) {}

std::string Medicine::describe() const {
    return "Medicine[" + id() + "] " + name_ + " (" + genericName_ + "), class: " + drugClass_ +
           ", use: " + use_;
}

std::string Medicine::summary() const { return name_ + " - " + drugClass_; }

bool Medicine::matches(const std::string& query) const {
    if (Searchable::matches(query)) return true;  // reuse the base-class logic first
    return Text::normalize(genericName_) == Text::normalize(query);
}

bool Medicine::operator==(const Medicine& other) const { return id() == other.id(); }

bool Medicine::operator<(const Medicine& other) const {
    return Text::toLower(name_) < Text::toLower(other.name_);
}

std::ostream& operator<<(std::ostream& os, const Medicine& m) {
    // A friend function may read private members such as genericName_.
    return os << m.name_ << " (" << m.genericName_ << ")";
}

// ----------------------------------------------------------------- Illness

Illness::Illness() : Record("", "unnamed illness") {}

Illness::Illness(const std::string& id, const std::string& name, const std::string& category,
                 const std::string& description)
    : Record(id, name), category_(category), description_(description) {}

std::string Illness::describe() const {
    return "Illness[" + id() + "] " + name_ + " - " + category_ + ": " + description_;
}

std::string Illness::summary() const { return name_ + " (" + category_ + ")"; }

// ---------------------------------------------------------------- Severity

std::string Severity::label() const {
    switch (level_) {
        case 1: return "MINOR";
        case 2: return "MODERATE";
        case 3: return "MAJOR";
        default: return "NONE";
    }
}

bool Severity::tryParse(const std::string& label, Severity& out) {
    std::string up = Text::normalizeCode(label);
    if (up == "MINOR") out = Severity(1);
    else if (up == "MODERATE") out = Severity(2);
    else if (up == "MAJOR") out = Severity(3);
    else return false;
    return true;
}

// ------------------------------------------------------------- BatchNumber

BatchNumber::BatchNumber(const std::string& raw) : display_(Text::trim(raw)), key_(Text::normalizeCode(raw)) {}

BatchNumber::BatchNumber(const char* raw) : BatchNumber(std::string(raw)) {}  // delegating constructor

std::ostream& operator<<(std::ostream& os, const BatchNumber& b) { return os << b.display_; }
