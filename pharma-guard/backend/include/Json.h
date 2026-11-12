// Json.h - a tiny JSON writer and reader (no third-party library needed).
//
//   JsonObject / JsonArray : build JSON text   {"a":1,"b":["x","y"]}
//   JsonValue              : parse the JSON text that the browser sends us
//
// CONCEPTS SHOWN HERE
//   * Method overloading: JsonObject::add has one version per value type.
//     Note the extra const char* overload - without it, add("key", "text")
//     would silently pick the bool version (a classic C++ trap!).
//   * "this" pointer: every add() returns *this so calls can be chained.
#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class Json {
public:
    // Wraps a string in quotes and escapes special characters.
    static std::string quote(const std::string& s);
};

class JsonArray {
public:
    JsonArray& add(const std::string& value);
    JsonArray& add(const char* value);
    JsonArray& add(int value);
    JsonArray& addRaw(const std::string& rawJson);  // value that is already JSON
    static JsonArray ofStrings(const std::vector<std::string>& values);
    std::string str() const;

private:
    std::vector<std::string> parts_;
};

class JsonObject {
public:
    JsonObject& add(const std::string& key, const std::string& value);
    JsonObject& add(const std::string& key, const char* value);
    JsonObject& add(const std::string& key, bool value);
    JsonObject& add(const std::string& key, int value);
    JsonObject& add(const std::string& key, std::size_t value);
    JsonObject& add(const std::string& key, double value);
    JsonObject& addRaw(const std::string& key, const std::string& rawJson);
    std::string str() const;

private:
    std::vector<std::pair<std::string, std::string>> fields_;  // key -> already-encoded value
};

// A parsed JSON document. Arrays use `items`; objects use parallel `keys`/`values`.
class JsonValue {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type type = Type::Null;
    bool boolean = false;
    double number = 0;
    std::string text;
    std::vector<JsonValue> items;
    std::vector<std::string> keys;
    std::vector<JsonValue> values;

    // For objects: returns the member called `key`, or nullptr if missing.
    const JsonValue* get(const std::string& key) const;

    // Parses text. Returns false and fills `error` when the JSON is invalid.
    static bool parse(const std::string& source, JsonValue& out, std::string& error);
};
