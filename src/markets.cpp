#include "markets.hpp"

#include <cmath>
#include <vector>

#include "error.hpp"

namespace coinconvert {
namespace {

constexpr const char* kHubs[] = {"USD", "EUR", "GBP", "BTC", "ETH"};

struct Edge {
    const char* symbol = nullptr;
    bool invert = false;
};

std::optional<Edge> find_edge(const Market* markets, size_t count, const std::string& from,
                              const std::string& to) {
    for (size_t i = 0; i < count; ++i) {
        if (from == markets[i].base && to == markets[i].quote) {
            return Edge{markets[i].symbol, false};
        }
    }
    for (size_t i = 0; i < count; ++i) {
        if (from == markets[i].quote && to == markets[i].base) {
            return Edge{markets[i].symbol, true};
        }
    }
    return std::nullopt;
}

}  // namespace

std::optional<RatePlan> plan_conversion(const Market* markets, size_t count, const std::string& from,
                                        const std::string& to) {
    if (auto direct = find_edge(markets, count, from, to)) {
        RatePlan plan;
        plan.legs.push_back(Leg{direct->symbol, direct->invert});
        return plan;
    }
    for (const char* hub : kHubs) {
        if (from == hub || to == hub) {
            continue;
        }
        auto left = find_edge(markets, count, from, std::string(hub));
        auto right = find_edge(markets, count, std::string(hub), to);
        if (left && right) {
            RatePlan plan;
            plan.legs.push_back(Leg{left->symbol, left->invert});
            plan.legs.push_back(Leg{right->symbol, right->invert});
            return plan;
        }
    }
    return std::nullopt;
}

double apply_rate(const std::string& provider, const std::string& from, const std::string& to,
                  const RatePlan& plan, const std::unordered_map<std::string, double>& prices) {
    const std::string pair = from + "/" + to;
    double rate = 1.0;
    for (const Leg& leg : plan.legs) {
        const auto it = prices.find(leg.symbol);
        if (it == prices.end() || !std::isfinite(it->second) || !(it->second > 0.0)) {
            throw FailError(Fail::Runtime, provider + " returned no price for " + pair);
        }
        rate *= leg.invert ? (1.0 / it->second) : it->second;
    }
    if (!std::isfinite(rate) || !(rate > 0.0)) {
        throw FailError(Fail::Runtime, provider + " returned no price for " + pair);
    }
    return rate;
}

std::string plan_symbols(const RatePlan& plan) {
    std::vector<std::string> seen;
    std::string joined;
    for (const Leg& leg : plan.legs) {
        bool already = false;
        for (const std::string& symbol : seen) {
            if (symbol == leg.symbol) {
                already = true;
                break;
            }
        }
        if (already) {
            continue;
        }
        seen.push_back(leg.symbol);
        if (!joined.empty()) {
            joined.push_back(',');
        }
        joined += leg.symbol;
    }
    return joined;
}

}  // namespace coinconvert
