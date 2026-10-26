#include "format.hpp"

#include <charconv>
#include <cmath>
#include <limits>

#include "error.hpp"

namespace coinconvert {
namespace {

bool append_exponent(std::string_view text, size_t& index, std::string& normalized) {
    if (index >= text.size() || (text[index] != 'e' && text[index] != 'E')) {
        return false;
    }
    normalized.push_back('e');
    ++index;
    if (index < text.size() && (text[index] == '+' || text[index] == '-')) {
        normalized.push_back(text[index]);
        ++index;
    }
    bool digit = false;
    while (index < text.size() && text[index] >= '0' && text[index] <= '9') {
        digit = true;
        normalized.push_back(text[index]);
        ++index;
    }
    return digit;
}

}  // namespace

bool parse_amount(std::string_view text, double& value) {
    if (text.empty()) {
        return false;
    }
    size_t index = 0;
    if (text[index] == '+') {
        ++index;
        if (index == text.size()) {
            return false;
        }
    }
    std::string normalized;
    normalized.reserve(text.size() + 2);
    bool digit = false;
    bool dot = false;
    for (; index < text.size(); ++index) {
        const char c = text[index];
        if (c >= '0' && c <= '9') {
            digit = true;
            normalized.push_back(c);
            continue;
        }
        if (c == '.' && !dot) {
            dot = true;
            if (normalized.empty()) {
                normalized.push_back('0');
            }
            normalized.push_back('.');
            continue;
        }
        if (c == 'e' || c == 'E') {
            if (!digit || !append_exponent(text, index, normalized) || index != text.size()) {
                return false;
            }
            break;
        }
        return false;
    }
    if (!digit) {
        return false;
    }
    if (!normalized.empty() && normalized.back() == '.') {
        normalized.push_back('0');
    }
    double parsed = 0;
    const char* begin = normalized.data();
    const char* end = normalized.data() + normalized.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc() || result.ptr != end || !std::isfinite(parsed) || parsed < 0) {
        return false;
    }
    value = parsed;
    return true;
}

std::string format_result(double value) {
    if (!std::isfinite(value)) {
        throw FailError(Fail::Runtime, "conversion result is not a finite number");
    }
    const long double limit = static_cast<long double>(std::numeric_limits<long long>::max());
    const long double scaled = static_cast<long double>(value) * 100.0L;
    if (scaled >= limit || scaled <= -limit) {
        throw FailError(Fail::Runtime, "conversion result is too large to display");
    }
    const long long cents = std::llround(scaled);
    const bool negative = cents < 0;
    unsigned long long magnitude = 0;
    if (negative) {
        magnitude = static_cast<unsigned long long>(-(cents + 1)) + 1ULL;
    } else {
        magnitude = static_cast<unsigned long long>(cents);
    }
    const unsigned long long whole = magnitude / 100ULL;
    const unsigned long long fraction = magnitude % 100ULL;
    std::string out = negative ? "-" : "";
    out += std::to_string(whole);
    out.push_back('.');
    out.push_back(static_cast<char>('0' + fraction / 10ULL));
    out.push_back(static_cast<char>('0' + fraction % 10ULL));
    return out;
}

}  // namespace coinconvert
