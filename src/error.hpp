#pragma once

#include <stdexcept>
#include <string>

namespace coinconvert {

enum class Fail {
    Usage = 1,
    Runtime = 2,
};

class FailError : public std::runtime_error {
public:
    Fail kind;

    FailError(Fail kind, const std::string& message)
        : std::runtime_error(message), kind(kind) {}
};

}
