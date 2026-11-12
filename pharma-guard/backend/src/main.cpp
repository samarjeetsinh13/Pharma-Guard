// main.cpp - starts Pharma Guard: loads the database and serves the JSON API.
//
//   pharma_guard                     start the server on port 8080
//   pharma_guard --port 9000         use another port
//   pharma_guard --data ../data      use another data folder
//   pharma_guard --demo              print the C++ concept demos and exit
//   pharma_guard --check             load + validate the database files and exit

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "Checkers.h"
#include "ConceptDemo.h"
#include "Database.h"
#include "HttpServer.h"
#include "Json.h"
#include "Text.h"

namespace {

const std::size_t kMaxItems = 10;   // at most 10 medicines per request
const std::size_t kMaxLength = 80;  // at most 80 characters per typed value

HttpResponse ok(const std::string& resultJson) {
    JsonObject o;
    o.add("ok", true).addRaw("result", resultJson);
    return HttpResponse(200, "application/json", o.str());
}

HttpResponse fail(int status, const std::string& message) {
    JsonObject o;
    o.add("ok", false).add("error", message);
    return HttpResponse(status, "application/json", o.str());
}

// Parses the request body and checks that it is a JSON object.
bool parseBody(const HttpRequest& request, JsonValue& doc, std::string& error) {
    if (!JsonValue::parse(request.body, doc, error)) {
        error = "Request body is not valid JSON: " + error;
        return false;
    }
    if (doc.type != JsonValue::Type::Object) {
        error = "Request body must be a JSON object";
        return false;
    }
    return true;
}

// Reads a required text field and trims it.
bool readText(const JsonValue& doc, const std::string& field, const std::string& label, std::string& out,
              std::string& error) {
    const JsonValue* v = doc.get(field);
    if (!v || v->type != JsonValue::Type::String) {
        error = "Field '" + field + "' (" + label + ") is required and must be text";
        return false;
    }
    out = Text::trim(v->text);
    if (out.empty()) {
        error = "Please enter the " + label;
        return false;
    }
    if (out.size() > kMaxLength) {
        error = "The " + label + " is too long (maximum " + std::to_string(kMaxLength) + " characters)";
        return false;
    }
    return true;
}

// Reads an array of text values with a minimum and maximum count.
bool readList(const JsonValue& doc, const std::string& field, std::size_t minCount, std::vector<std::string>& out,
              std::string& error) {
    const JsonValue* v = doc.get(field);
    if (!v || v->type != JsonValue::Type::Array) {
        error = "Field '" + field + "' must be a list of medicine names";
        return false;
    }
    out.clear();
    for (const JsonValue& item : v->items) {
        if (item.type != JsonValue::Type::String) {
            error = "Every entry in '" + field + "' must be text";
            return false;
        }
        std::string name = Text::trim(item.text);
        if (name.empty()) continue;
        if (name.size() > kMaxLength) {
            error = "A medicine name is too long (maximum " + std::to_string(kMaxLength) + " characters)";
            return false;
        }
        out.push_back(name);
    }
    if (out.size() < minCount) {
        error = "Please enter at least " + std::to_string(minCount) + " medicine" + (minCount > 1 ? "s" : "");
        return false;
    }
    if (out.size() > kMaxItems) {
        error = "Please enter at most " + std::to_string(kMaxItems) + " medicines at a time";
        return false;
    }
    return true;
}

bool fileExists(const std::string& path) { return std::ifstream(path).good(); }

// Finds the data folder: the --data option, or the first of data, ../data, ../../data that exists.
std::string findDataDir(const std::string& requested) {
    if (!requested.empty()) return requested;
    for (const char* candidate : {"data", "../data", "../../data"}) {
        if (fileExists(std::string(candidate) + "/medicines.txt")) return candidate;
    }
    return "data";
}

}  // namespace

int main(int argc, char** argv) {
    int port = 8080;
    std::string dataArg;
    bool demoOnly = false;
    bool checkOnly = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc) {
            port = std::atoi(argv[++i]);
            if (port <= 0 || port > 65535) {
                std::cerr << "Invalid port number.\n";
                return 1;
            }
        } else if (arg == "--data" && i + 1 < argc) {
            dataArg = argv[++i];
        } else if (arg == "--demo") {
            demoOnly = true;
        } else if (arg == "--check") {
            checkOnly = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: pharma_guard [--port N] [--data FOLDER] [--demo] [--check]\n";
            return 0;
        } else {
            std::cerr << "Unknown option: " << arg << "  (try --help)\n";
            return 1;
        }
    }

    Database db;
    std::string error;
    const std::string dataDir = findDataDir(dataArg);
    if (!db.load(dataDir, error)) {
        std::cerr << "DATABASE ERROR: " << error << "\n"
                  << "Run the program from the project folder, or pass  --data <folder-with-the-txt-files>\n";
        return 1;
    }
    const Database::Stats stats = db.stats();
    std::cout << "Pharma Guard database loaded from '" << dataDir << "': " << stats.medicines << " medicines, "
              << stats.illnesses << " illnesses, " << stats.suitabilityRules << " suitability rules, "
              << stats.interactions << " interactions, " << stats.batches << " registry batches.\n";

    if (checkOnly) {
        std::cout << "Database files are valid.\n";
        return 0;
    }
    if (demoOnly) {
        std::cout << "\n" << ConceptDemo::toText(ConceptDemo::runAll(db));
        return 0;
    }

    HttpServer server(port);

    server.route("GET", "/api/health", [&](const HttpRequest&) {
        JsonObject o;
        o.add("app", "Pharma Guard").add("status", "running").add("today", Text::today()).addRaw("stats", db.stats().toJson());
        return ok(o.str());
    });

    server.route("GET", "/api/database", [&](const HttpRequest&) { return ok(db.toJson()); });

    // Re-reads the .txt files so edits show up without restarting the server.
    server.route("POST", "/api/reload", [&](const HttpRequest&) {
        std::string reloadError;
        if (!db.load(dataDir, reloadError)) return fail(400, "Reload failed, old data kept: " + reloadError);
        JsonObject o;
        o.add("reloaded", true).addRaw("stats", db.stats().toJson());
        return ok(o.str());
    });

    // ---- Module 1: exactly ONE illness + one or more medicines
    server.route("POST", "/api/suitability", [&](const HttpRequest& request) {
        JsonValue doc;
        std::string err;
        if (!parseBody(request, doc, err)) return fail(400, err);

        CheckRequest check;
        const JsonValue* illnessField = doc.get("illness");
        if (illnessField && illnessField->type == JsonValue::Type::Array) {
            // Accept ["Fever"] but reject ["Fever","Cold"]: exactly one illness is allowed.
            if (illnessField->items.size() != 1 || illnessField->items[0].type != JsonValue::Type::String) {
                return fail(400, "Please provide exactly one illness");
            }
            check.illness = Text::trim(illnessField->items[0].text);
        } else if (!readText(doc, "illness", "illness", check.illness, err)) {
            return fail(400, err);
        }
        if (check.illness.empty()) return fail(400, "Please enter the illness");
        if (check.illness.find_first_of(",;|") != std::string::npos) {
            return fail(400, "Please enter exactly one illness (no commas or semicolons)");
        }
        if (!readList(doc, "medicines", 1, check.medicines, err)) return fail(400, err);

        std::unique_ptr<Checker> checker = makeChecker("suitability", db);  // base pointer -> derived object
        return ok(checker->execute(check)->toJson());
    });

    // ---- Module 2: two or more medicines, checked on separate worker threads
    server.route("POST", "/api/interactions", [&](const HttpRequest& request) {
        JsonValue doc;
        std::string err;
        if (!parseBody(request, doc, err)) return fail(400, err);

        CheckRequest check;
        if (!readList(doc, "medicines", 2, check.medicines, err)) return fail(400, err);

        std::unique_ptr<Checker> checker = makeChecker("interaction", db);
        return ok(checker->execute(check)->toJson());
    });

    // ---- Module 3: medicine name + batch number + manufacturer
    server.route("POST", "/api/counterfeit", [&](const HttpRequest& request) {
        JsonValue doc;
        std::string err;
        if (!parseBody(request, doc, err)) return fail(400, err);

        CheckRequest check;
        if (!readText(doc, "medicine", "medicine name", check.medicine, err)) return fail(400, err);
        if (!readText(doc, "batch", "batch number", check.batch, err)) return fail(400, err);
        if (!readText(doc, "manufacturer", "manufacturer", check.manufacturer, err)) return fail(400, err);

        std::unique_ptr<Checker> checker = makeChecker("counterfeit", db);
        return ok(checker->execute(check)->toJson());
    });

    // ---- Live demos of the C++ concepts
    server.route("GET", "/api/concepts", [&](const HttpRequest&) {
        return ok(ConceptDemo::toJson(ConceptDemo::runAll(db)));
    });

    std::cout << "API ready:  http://127.0.0.1:" << port << "/api/health   (press Ctrl+C to stop)\n";
    if (!server.run(error)) {
        std::cerr << "SERVER ERROR: " << error << "\n";
        return 1;
    }
    return 0;
}
