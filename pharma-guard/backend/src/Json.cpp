#include "Json.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <sstream>

// ---------------------------------------------------------------- writer

std::string Json::quote(const std::string& s) {
    std::string out = "\"";
    for (char raw : s) {
        unsigned char c = static_cast<unsigned char>(raw);
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof buf, "\\u%04x", c);
                    out += buf;
                } else {
                    out += raw;  // UTF-8 bytes pass through unchanged
                }
        }
    }
    out += '"';
    return out;
}

JsonArray& JsonArray::add(const std::string& value) {
    parts_.push_back(Json::quote(value));
    return *this;
}
JsonArray& JsonArray::add(const char* value) { return add(std::string(value)); }
JsonArray& JsonArray::add(int value) {
    parts_.push_back(std::to_string(value));
    return *this;
}
JsonArray& JsonArray::addRaw(const std::string& rawJson) {
    parts_.push_back(rawJson);
    return *this;
}
JsonArray JsonArray::ofStrings(const std::vector<std::string>& values) {
    JsonArray array;
    for (const std::string& v : values) array.add(v);
    return array;
}
std::string JsonArray::str() const {
    std::string out = "[";
    for (std::vector<std::string>::size_type i = 0; i < parts_.size(); ++i) {
        if (i > 0) out += ',';
        out += parts_[i];
    }
    return out + "]";
}

JsonObject& JsonObject::add(const std::string& key, const std::string& value) {
    fields_.emplace_back(key, Json::quote(value));
    return *this;
}
JsonObject& JsonObject::add(const std::string& key, const char* value) {
    return add(key, std::string(value));
}
JsonObject& JsonObject::add(const std::string& key, bool value) {
    fields_.emplace_back(key, value ? "true" : "false");
    return *this;
}
JsonObject& JsonObject::add(const std::string& key, int value) {
    fields_.emplace_back(key, std::to_string(value));
    return *this;
}
JsonObject& JsonObject::add(const std::string& key, std::size_t value) {
    fields_.emplace_back(key, std::to_string(value));
    return *this;
}
JsonObject& JsonObject::add(const std::string& key, double value) {
    std::ostringstream os;
    os << value;
    fields_.emplace_back(key, os.str());
    return *this;
}
JsonObject& JsonObject::addRaw(const std::string& key, const std::string& rawJson) {
    fields_.emplace_back(key, rawJson);
    return *this;
}
std::string JsonObject::str() const {
    std::string out = "{";
    for (std::vector<std::pair<std::string, std::string>>::size_type i = 0; i < fields_.size(); ++i) {
        if (i > 0) out += ',';
        out += Json::quote(fields_[i].first) + ":" + fields_[i].second;
    }
    return out + "}";
}

// ---------------------------------------------------------------- reader

const JsonValue* JsonValue::get(const std::string& key) const {
    if (type != Type::Object) return nullptr;
    for (std::vector<std::string>::size_type i = 0; i < keys.size(); ++i) {
        if (keys[i] == key) return &values[i];
    }
    return nullptr;
}

namespace {

// A small recursive-descent parser: one function per JSON grammar rule.
class Parser {
public:
    explicit Parser(const std::string& source) : s_(source) {}

    bool parseDocument(JsonValue& out) {
        if (!parseValue(out, 0)) return false;
        skipSpaces();
        if (pos_ != s_.size()) return fail("Unexpected text after the JSON value");
        return true;
    }

    const std::string& error() const { return error_; }

private:
    const std::string& s_;
    std::string::size_type pos_ = 0;
    std::string error_;

    bool fail(const std::string& message) {
        error_ = message + " (position " + std::to_string(pos_) + ")";
        return false;
    }

    void skipSpaces() {
        while (pos_ < s_.size() && std::isspace(static_cast<unsigned char>(s_[pos_]))) ++pos_;
    }

    bool parseValue(JsonValue& out, int depth) {
        if (depth > 32) return fail("JSON is nested too deeply");
        skipSpaces();
        if (pos_ >= s_.size()) return fail("Unexpected end of JSON");
        char c = s_[pos_];
        if (c == '{') return parseObject(out, depth);
        if (c == '[') return parseArray(out, depth);
        if (c == '"') {
            out.type = JsonValue::Type::String;
            return parseString(out.text);
        }
        if (c == 't' || c == 'f' || c == 'n') return parseLiteral(out);
        return parseNumber(out);
    }

    bool parseObject(JsonValue& out, int depth) {
        out.type = JsonValue::Type::Object;
        ++pos_;  // skip {
        skipSpaces();
        if (pos_ < s_.size() && s_[pos_] == '}') {
            ++pos_;
            return true;
        }
        for (;;) {
            skipSpaces();
            if (pos_ >= s_.size() || s_[pos_] != '"') return fail("Expected a string key");
            std::string key;
            if (!parseString(key)) return false;
            skipSpaces();
            if (pos_ >= s_.size() || s_[pos_] != ':') return fail("Expected ':' after key");
            ++pos_;
            JsonValue value;
            if (!parseValue(value, depth + 1)) return false;
            out.keys.push_back(key);
            out.values.push_back(value);
            skipSpaces();
            if (pos_ < s_.size() && s_[pos_] == ',') {
                ++pos_;
                continue;
            }
            if (pos_ < s_.size() && s_[pos_] == '}') {
                ++pos_;
                return true;
            }
            return fail("Expected ',' or '}' in object");
        }
    }

    bool parseArray(JsonValue& out, int depth) {
        out.type = JsonValue::Type::Array;
        ++pos_;  // skip [
        skipSpaces();
        if (pos_ < s_.size() && s_[pos_] == ']') {
            ++pos_;
            return true;
        }
        for (;;) {
            JsonValue item;
            if (!parseValue(item, depth + 1)) return false;
            out.items.push_back(item);
            skipSpaces();
            if (pos_ < s_.size() && s_[pos_] == ',') {
                ++pos_;
                continue;
            }
            if (pos_ < s_.size() && s_[pos_] == ']') {
                ++pos_;
                return true;
            }
            return fail("Expected ',' or ']' in array");
        }
    }

    static void appendUtf8(std::string& out, unsigned int cp) {
        if (cp < 0x80) {
            out += static_cast<char>(cp);
        } else if (cp < 0x800) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }

    bool parseString(std::string& out) {
        ++pos_;  // skip opening quote
        out.clear();
        while (pos_ < s_.size()) {
            char c = s_[pos_++];
            if (c == '"') return true;
            if (c != '\\') {
                out += c;
                continue;
            }
            if (pos_ >= s_.size()) break;
            char e = s_[pos_++];
            switch (e) {
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'u': {
                    if (pos_ + 4 > s_.size()) return fail("Bad \\u escape");
                    unsigned int cp = 0;
                    for (int i = 0; i < 4; ++i) {
                        char h = s_[pos_++];
                        cp <<= 4;
                        if (h >= '0' && h <= '9') cp |= static_cast<unsigned int>(h - '0');
                        else if (h >= 'a' && h <= 'f') cp |= static_cast<unsigned int>(h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') cp |= static_cast<unsigned int>(h - 'A' + 10);
                        else return fail("Bad \\u escape");
                    }
                    appendUtf8(out, cp);
                    break;
                }
                default: return fail("Unknown escape sequence");
            }
        }
        return fail("Unterminated string");
    }

    bool parseLiteral(JsonValue& out) {
        if (s_.compare(pos_, 4, "true") == 0) {
            out.type = JsonValue::Type::Bool;
            out.boolean = true;
            pos_ += 4;
            return true;
        }
        if (s_.compare(pos_, 5, "false") == 0) {
            out.type = JsonValue::Type::Bool;
            out.boolean = false;
            pos_ += 5;
            return true;
        }
        if (s_.compare(pos_, 4, "null") == 0) {
            out.type = JsonValue::Type::Null;
            pos_ += 4;
            return true;
        }
        return fail("Unknown literal");
    }

    bool parseNumber(JsonValue& out) {
        std::string::size_type start = pos_;
        while (pos_ < s_.size() && std::string("+-0123456789.eE").find(s_[pos_]) != std::string::npos) ++pos_;
        if (start == pos_) return fail("Unexpected character");
        out.type = JsonValue::Type::Number;
        out.number = std::strtod(s_.substr(start, pos_ - start).c_str(), nullptr);
        return true;
    }
};

}  // namespace

bool JsonValue::parse(const std::string& source, JsonValue& out, std::string& error) {
    Parser parser(source);
    JsonValue result;
    if (!parser.parseDocument(result)) {
        error = parser.error();
        return false;
    }
    out = result;
    return true;
}
