// Text.h - small string helpers.
//
// CONCEPTS SHOWN HERE
//   * Static member functions: Text has no data, only static functions. They
//     belong to the class itself, so you call them as Text::trim(...) without
//     creating a Text object.
//   * Method overloading: Text::join has two versions with different parameters.
#pragma once

#include <string>
#include <vector>

class Text {
public:
    // Removes spaces, tabs and line breaks from both ends.
    static std::string trim(const std::string& s);

    // "  PaRaCetamol " -> "paracetamol"
    static std::string toLower(const std::string& s);

    // trim + lower-case + collapse repeated inner spaces: "Dolo   650" -> "dolo 650"
    static std::string normalize(const std::string& s);

    // Keeps only letters and digits and upper-cases them: "pcm-2501 a12" -> "PCM2501A12".
    // Used to compare batch numbers and manufacturer names forgivingly.
    static std::string normalizeCode(const std::string& s);

    // Removes dose words from a medicine name: "paracetamol 500mg tablet" -> "paracetamol".
    static std::string stripDose(const std::string& normalizedName);

    // Splits on a separator and KEEPS empty pieces ("a||b" -> "a", "", "b").
    static std::vector<std::string> split(const std::string& s, char separator);

    // METHOD OVERLOADING: same name, different parameter lists.
    static std::string join(const std::vector<std::string>& parts);  // uses ", "
    static std::string join(const std::vector<std::string>& parts, const std::string& separator);

    // Today's local date as "YYYY-MM-DD".
    static std::string today();

    // True for strings shaped like 2026-11-30 (checks digits and ranges).
    static bool isIsoDate(const std::string& s);
};
