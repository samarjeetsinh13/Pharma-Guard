// Repository.h - TEMPLATES.
//
// A template is a "blueprint" that the compiler turns into real code for each type
// you use it with. Repository<Medicine> and Repository<Illness> are two different
// classes generated from the SAME blueprint.
#pragma once

#include <cstddef>
#include <sstream>
#include <string>
#include <vector>

// CLASS TEMPLATE: a simple typed container with a search function.
template <typename T>
class Repository {
public:
    void add(const T& item) { items_.push_back(item); }
    void clear() { items_.clear(); }
    std::size_t size() const { return items_.size(); }
    const std::vector<T>& all() const { return items_; }

    // MEMBER FUNCTION TEMPLATE: Pred can be any callable (a lambda, for example).
    // Returns a pointer to the first item the predicate accepts, or nullptr.
    template <typename Pred>
    const T* findIf(Pred predicate) const {
        for (const T& item : items_) {
            if (predicate(item)) return &item;
        }
        return nullptr;
    }

private:
    std::vector<T> items_;
};

// FUNCTION TEMPLATE: works for any type that supports operator<
// (int, std::string, and even our Severity class through its int conversion).
template <typename T>
T maxOf(const T& a, const T& b) {
    return (a < b) ? b : a;
}

// FUNCTION TEMPLATE: joins any printable values with a separator.
template <typename T>
std::string joinValues(const std::vector<T>& values, const std::string& separator) {
    std::ostringstream out;
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i > 0) out << separator;
        out << values[i];
    }
    return out.str();
}
