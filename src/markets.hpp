#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace coinconvert {

struct Market {
    const char* base;
    const char* quote;
    const char* symbol;
};

struct Leg {
    std::string symbol;
    bool invert = false;
};

struct RatePlan {
    std::vector<Leg> legs;
};

// Direct book first. Otherwise one cross through USD, EUR, GBP, BTC, or ETH,
// in that order. Both legs always come from the same venue's own markets.
std::optional<RatePlan> plan_conversion(const Market* markets, size_t count,
                                        const std::string& from, const std::string& to);

double apply_rate(const std::string& provider, const std::string& from, const std::string& to,
                  const RatePlan& plan, const std::unordered_map<std::string, double>& prices);

std::string plan_symbols(const RatePlan& plan);

}
