// ConceptDemo.h - runs small, REAL experiments that show each C++ concept.
//
// Open the "OOP Concepts" tab in the web app (or run  pharma_guard --demo)
// and every line you see was produced by actual code in ConceptDemo.cpp.
#pragma once

#include <string>
#include <vector>

#include "Database.h"

struct DemoEntry {
    std::string topic;    // e.g. "Object slicing"
    std::string where;    // which files/classes demonstrate it
    std::string output;   // what the experiment printed
};

class ConceptDemo {
public:
    static std::vector<DemoEntry> runAll(const Database& db);
    static std::string toJson(const std::vector<DemoEntry>& entries);
    static std::string toText(const std::vector<DemoEntry>& entries);
};
