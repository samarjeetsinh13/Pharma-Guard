#include "Text.h"

#include <cctype>
#include <ctime>

std::string Text::trim(const std::string& s) {
    const char* spaces = " \t\r\n";
    std::string::size_type first = s.find_first_not_of(spaces);
    if (first == std::string::npos) return "";
    std::string::size_type last = s.find_last_not_of(spaces);
    return s.substr(first, last - first + 1);
}

std::string Text::toLower(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

std::string Text::normalize(const std::string& s) {
    std::string out;
    bool lastWasSpace = true;  // true at the start so leading spaces are dropped
    for (char raw : s) {
        unsigned char c = static_cast<unsigned char>(raw);
        if (std::isspace(c)) {
            if (!lastWasSpace) out += ' ';
            lastWasSpace = true;
        } else {
            out += static_cast<char>(std::tolower(c));
            lastWasSpace = false;
        }
    }
    if (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

std::string Text::normalizeCode(const std::string& s) {
    std::string out;
    for (char raw : s) {
        unsigned char c = static_cast<unsigned char>(raw);
        if (std::isalnum(c)) out += static_cast<char>(std::toupper(c));
    }
    return out;
}

namespace {
// A "dose token" is a word like 500mg, 650, 0.5g, tablet, capsule ...
bool isDoseToken(const std::string& t) {
    static const char* words[] = {"tablet", "tablets", "tab",  "tabs",   "capsule",
                                  "capsules", "cap",   "caps", "syrup",  "injection",
                                  "inhaler"};
    for (const char* w : words) {
        if (t == w) return true;
    }
    std::string::size_type i = 0;
    while (i < t.size() && (std::isdigit(static_cast<unsigned char>(t[i])) || t[i] == '.')) ++i;
    if (i == 0) return false;  // does not start with a number
    std::string unit = t.substr(i);
    return unit.empty() || unit == "mg" || unit == "g" || unit == "mcg" || unit == "ml" ||
           unit == "iu" || unit == "%";
}
}  // namespace

std::string Text::stripDose(const std::string& normalizedName) {
    std::string out;
    for (const std::string& token : split(normalizedName, ' ')) {
        if (token.empty() || isDoseToken(token)) continue;
        if (!out.empty()) out += ' ';
        out += token;
    }
    return out.empty() ? normalizedName : out;
}

std::vector<std::string> Text::split(const std::string& s, char separator) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : s) {
        if (c == separator) {
            parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    parts.push_back(current);
    return parts;
}

std::string Text::join(const std::vector<std::string>& parts) {
    return join(parts, ", ");  // one overload simply calls the other
}

std::string Text::join(const std::vector<std::string>& parts, const std::string& separator) {
    std::string out;
    for (std::vector<std::string>::size_type i = 0; i < parts.size(); ++i) {
        if (i > 0) out += separator;
        out += parts[i];
    }
    return out;
}

std::string Text::today() {
    std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char buffer[16];
    std::strftime(buffer, sizeof buffer, "%Y-%m-%d", &local);
    return buffer;
}

bool Text::isIsoDate(const std::string& s) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (int i : {0, 1, 2, 3, 5, 6, 8, 9}) {
        if (!std::isdigit(static_cast<unsigned char>(s[static_cast<std::string::size_type>(i)]))) return false;
    }
    int month = std::stoi(s.substr(5, 2));
    int day = std::stoi(s.substr(8, 2));
    return month >= 1 && month <= 12 && day >= 1 && day <= 31;
}
