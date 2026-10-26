#include "json.hpp"

#include <charconv>
#include <cmath>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "error.hpp"

namespace coinconvert {

class JsonParser {
public:
    explicit JsonParser(const std::string& text) : text_(text) {}

    Json parse() {
        skip();
        Json value = parse_value();
        skip();
        if (index_ != text_.size()) {
            throw std::runtime_error("trailing data");
        }
        return value;
    }

private:
    const std::string& text_;
    size_t index_ = 0;
    int depth_ = 0;

    [[noreturn]] void fail(const char* message) const {
        throw std::runtime_error(message);
    }

    void skip() {
        while (index_ < text_.size()) {
            const unsigned char c = static_cast<unsigned char>(text_[index_]);
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++index_;
            } else {
                break;
            }
        }
    }

    char peek() const {
        if (index_ >= text_.size()) {
            fail("unexpected end of JSON");
        }
        return text_[index_];
    }

    char get() {
        const char c = peek();
        ++index_;
        return c;
    }

    void expect(char wanted) {
        if (get() != wanted) {
            fail("unexpected character");
        }
    }

    Json parse_value() {
        if (++depth_ > 64) {
            fail("JSON nested too deeply");
        }
        skip();
        const char c = peek();
        Json value;
        if (c == '{') {
            value = parse_object();
        } else if (c == '[') {
            value = parse_array();
        } else if (c == '"') {
            value = parse_string();
        } else if (c == 't') {
            value = parse_literal("true", true);
        } else if (c == 'f') {
            value = parse_literal("false", false);
        } else if (c == 'n') {
            value = parse_null();
        } else if (c == '-' || (c >= '0' && c <= '9')) {
            value = parse_number();
        } else {
            fail("unexpected character");
        }
        --depth_;
        return value;
    }

    Json parse_object() {
        expect('{');
        Json value;
        value.type_ = Json::Type::Object;
        skip();
        if (peek() == '}') {
            get();
            return value;
        }
        while (true) {
            skip();
            Json key = parse_string();
            skip();
            expect(':');
            Json item = parse_value();
            bool replaced = false;
            for (auto& entry : value.object_) {
                if (entry.first == key.string_) {
                    entry.second = std::move(item);
                    replaced = true;
                    break;
                }
            }
            if (!replaced) {
                value.object_.emplace_back(std::move(key.string_), std::move(item));
            }
            skip();
            const char c = get();
            if (c == '}') {
                break;
            }
            if (c != ',') {
                fail("expected comma in object");
            }
        }
        return value;
    }

    Json parse_array() {
        expect('[');
        Json value;
        value.type_ = Json::Type::Array;
        skip();
        if (peek() == ']') {
            get();
            return value;
        }
        while (true) {
            value.array_.push_back(parse_value());
            skip();
            const char c = get();
            if (c == ']') {
                break;
            }
            if (c != ',') {
                fail("expected comma in array");
            }
        }
        return value;
    }

    Json parse_string() {
        expect('"');
        Json value;
        value.type_ = Json::Type::String;
        std::string out;
        while (true) {
            if (index_ >= text_.size()) {
                fail("unterminated string");
            }
            const unsigned char c = static_cast<unsigned char>(text_[index_++]);
            if (c == '"') {
                break;
            }
            if (c < 0x20) {
                fail("raw control character in string");
            }
            if (c != '\\') {
                out.push_back(static_cast<char>(c));
                continue;
            }
            if (index_ >= text_.size()) {
                fail("unterminated escape");
            }
            const char esc = text_[index_++];
            switch (esc) {
            case '"':
            case '\\':
            case '/':
                out.push_back(esc);
                break;
            case 'b':
                out.push_back('\b');
                break;
            case 'f':
                out.push_back('\f');
                break;
            case 'n':
                out.push_back('\n');
                break;
            case 'r':
                out.push_back('\r');
                break;
            case 't':
                out.push_back('\t');
                break;
            case 'u':
                append_utf8(out, read_hex_codepoint());
                break;
            default:
                fail("invalid escape");
            }
        }
        value.string_ = std::move(out);
        return value;
    }

    uint32_t read_hex_codepoint() {
        if (index_ + 4 > text_.size()) {
            fail("short unicode escape");
        }
        uint32_t code = 0;
        for (int i = 0; i < 4; ++i) {
            const unsigned char c = static_cast<unsigned char>(text_[index_++]);
            code <<= 4;
            if (c >= '0' && c <= '9') {
                code += c - '0';
            } else if (c >= 'a' && c <= 'f') {
                code += c - 'a' + 10;
            } else if (c >= 'A' && c <= 'F') {
                code += c - 'A' + 10;
            } else {
                fail("invalid unicode escape");
            }
        }
        if (code >= 0xD800 && code <= 0xDFFF) {
            fail("unsupported surrogate escape");
        }
        return code;
    }

    static void append_utf8(std::string& out, uint32_t code) {
        if (code <= 0x7F) {
            out.push_back(static_cast<char>(code));
        } else if (code <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (code >> 6)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xE0 | (code >> 12)));
            out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        }
    }

    Json parse_literal(const char* literal, bool boolean) {
        for (const char* p = literal; *p != '\0'; ++p) {
            if (get() != *p) {
                fail("invalid literal");
            }
        }
        Json value;
        value.type_ = Json::Type::Bool;
        value.bool_ = boolean;
        return value;
    }

    Json parse_null() {
        for (const char* p = "null"; *p != '\0'; ++p) {
            if (get() != *p) {
                fail("invalid literal");
            }
        }
        Json value;
        value.type_ = Json::Type::Null;
        return value;
    }

    Json parse_number() {
        const size_t start = index_;
        if (peek() == '-') {
            get();
        }
        if (index_ >= text_.size() || text_[index_] < '0' || text_[index_] > '9') {
            fail("invalid number");
        }
        if (text_[index_] == '0') {
            get();
            if (index_ < text_.size() && text_[index_] >= '0' && text_[index_] <= '9') {
                fail("invalid number");
            }
        } else {
            while (index_ < text_.size() && text_[index_] >= '0' && text_[index_] <= '9') {
                get();
            }
        }
        if (index_ < text_.size() && text_[index_] == '.') {
            get();
            if (index_ >= text_.size() || text_[index_] < '0' || text_[index_] > '9') {
                fail("invalid number");
            }
            while (index_ < text_.size() && text_[index_] >= '0' && text_[index_] <= '9') {
                get();
            }
        }
        if (index_ < text_.size() && (text_[index_] == 'e' || text_[index_] == 'E')) {
            get();
            if (index_ < text_.size() && (text_[index_] == '+' || text_[index_] == '-')) {
                get();
            }
            if (index_ >= text_.size() || text_[index_] < '0' || text_[index_] > '9') {
                fail("invalid number");
            }
            while (index_ < text_.size() && text_[index_] >= '0' && text_[index_] <= '9') {
                get();
            }
        }
        double parsed = 0;
        const char* begin = text_.data() + start;
        const char* end = text_.data() + index_;
        const auto result = std::from_chars(begin, end, parsed);
        if (result.ec != std::errc() || result.ptr != end || !std::isfinite(parsed)) {
            fail("invalid number");
        }
        Json value;
        value.type_ = Json::Type::Number;
        value.number_ = parsed;
        return value;
    }
};

namespace {

double parse_decimal_text(const std::string& text) {
    if (text.empty()) {
        throw std::runtime_error("empty decimal");
    }
    double parsed = 0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc() || result.ptr != end || !std::isfinite(parsed)) {
        throw std::runtime_error("invalid decimal");
    }
    return parsed;
}

}  // namespace

Json Json::parse(const std::string& text) {
    std::string_view view = text;
    if (view.size() >= 3 &&
        static_cast<unsigned char>(view[0]) == 0xEF &&
        static_cast<unsigned char>(view[1]) == 0xBB &&
        static_cast<unsigned char>(view[2]) == 0xBF) {
        return JsonParser(text.substr(3)).parse();
    }
    return JsonParser(text).parse();
}

bool Json::boolean() const {
    if (type_ != Type::Bool) {
        throw std::runtime_error("JSON value is not a boolean");
    }
    return bool_;
}

double Json::number() const {
    if (type_ != Type::Number) {
        throw std::runtime_error("JSON value is not a number");
    }
    return number_;
}

const std::string& Json::string() const {
    if (type_ != Type::String) {
        throw std::runtime_error("JSON value is not a string");
    }
    return string_;
}

double Json::decimal() const {
    if (type_ == Type::Number) {
        return number_;
    }
    if (type_ == Type::String) {
        return parse_decimal_text(string_);
    }
    throw std::runtime_error("JSON value is not a decimal");
}

size_t Json::size() const {
    if (type_ == Type::Array) {
        return array_.size();
    }
    if (type_ == Type::Object) {
        return object_.size();
    }
    throw std::runtime_error("JSON value has no size");
}

const Json& Json::at(size_t index) const {
    if (type_ != Type::Array || index >= array_.size()) {
        throw std::runtime_error("JSON array index out of range");
    }
    return array_[index];
}

const Json* Json::find(const std::string& key) const {
    if (type_ != Type::Object) {
        return nullptr;
    }
    const Json* found = nullptr;
    for (const auto& entry : object_) {
        if (entry.first == key) {
            found = &entry.second;
        }
    }
    return found;
}

const Json& Json::at(const std::string& key) const {
    const Json* value = find(key);
    if (value == nullptr) {
        throw std::runtime_error("JSON object is missing a key");
    }
    return *value;
}

const std::vector<std::pair<std::string, Json>>& Json::object() const {
    if (type_ != Type::Object) {
        throw std::runtime_error("JSON value is not an object");
    }
    return object_;
}

Json parse_json(const std::string& provider, const std::string& body) {
    try {
        return Json::parse(body);
    } catch (const std::exception&) {
        throw FailError(Fail::Runtime, provider + " returned data that was not valid JSON");
    }
}

}  // namespace coinconvert
