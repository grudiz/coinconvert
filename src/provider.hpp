#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "currency.hpp"
#include "error.hpp"

namespace coinconvert {

class HttpClient;

class PriceProvider {
public:
    virtual ~PriceProvider() = default;
    virtual const char* id() const = 0;
    virtual const char* summary() const = 0;
    virtual double rate(const Currency& from, const Currency& to, HttpClient& http) const = 0;
};

const std::vector<const PriceProvider*>& providers();
const PriceProvider* find_provider(std::string_view id);
const PriceProvider& default_provider();

inline FailError no_rate(const std::string& provider, const Currency& from, const Currency& to) {
    return FailError(Fail::Runtime, provider + " has no rate for " + from.code + "/" + to.code);
}

}
