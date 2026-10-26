#pragma once

#include <string>
#include <utility>
#include <vector>

namespace coinconvert {

class JsonParser;

class Json {
    friend class JsonParser;

public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    static Json parse(const std::string& text);

    Type type() const { return type_; }
    bool is_null() const { return type_ == Type::Null; }
    bool is_bool() const { return type_ == Type::Bool; }
    bool is_number() const { return type_ == Type::Number; }
    bool is_string() const { return type_ == Type::String; }
    bool is_array() const { return type_ == Type::Array; }
    bool is_object() const { return type_ == Type::Object; }

    bool boolean() const;
    double number() const;
    const std::string& string() const;
    double decimal() const;

    size_t size() const;
    const Json& at(size_t index) const;
    const Json* find(const std::string& key) const;
    const Json& at(const std::string& key) const;
    const std::vector<std::pair<std::string, Json>>& object() const;

private:
    Type type_ = Type::Null;
    bool bool_ = false;
    double number_ = 0;
    std::string string_;
    std::vector<Json> array_;
    std::vector<std::pair<std::string, Json>> object_;
};

Json parse_json(const std::string& provider, const std::string& body);

}
