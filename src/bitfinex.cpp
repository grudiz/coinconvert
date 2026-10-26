#include "bitfinex.hpp"

#include <unordered_map>

#include "http.hpp"
#include "json.hpp"
#include "markets.hpp"

namespace coinconvert {
namespace {

// Trading symbols from Bitfinex's public pair list, checked on 2026-10-26.
// The multi-ticker row is [SYMBOL, BID, BID_SIZE, ASK, ASK_SIZE, DAILY_CHANGE,
// DAILY_CHANGE_RELATIVE, LAST_PRICE, ...]. Last price is index 7.
constexpr Market kMarkets[] = {
    {"ADA", "BTC", "tADABTC"}, {"ADA", "USD", "tADAUSD"}, {"APT", "USD", "tAPTUSD"},
    {"ARB", "USD", "tARBUSD"}, {"BTC", "EUR", "tBTCEUR"}, {"BTC", "GBP", "tBTCGBP"},
    {"BTC", "USD", "tBTCUSD"}, {"DOT", "USD", "tDOTUSD"}, {"ETC", "BTC", "tETCBTC"},
    {"ETC", "USD", "tETCUSD"}, {"ETH", "BTC", "tETHBTC"}, {"ETH", "EUR", "tETHEUR"},
    {"ETH", "GBP", "tETHGBP"}, {"ETH", "USD", "tETHUSD"}, {"FIL", "USD", "tFILUSD"},
    {"LTC", "BTC", "tLTCBTC"}, {"LTC", "USD", "tLTCUSD"}, {"SOL", "BTC", "tSOLBTC"},
    {"SOL", "USD", "tSOLUSD"}, {"SUI", "USD", "tSUIUSD"}, {"TRX", "BTC", "tTRXBTC"},
    {"TRX", "EUR", "tTRXEUR"}, {"TRX", "USD", "tTRXUSD"}, {"UNI", "USD", "tUNIUSD"},
    {"XLM", "USD", "tXLMUSD"}, {"XMR", "BTC", "tXMRBTC"}, {"XMR", "USD", "tXMRUSD"},
    {"XRP", "BTC", "tXRPBTC"}, {"XRP", "USD", "tXRPUSD"}, {"ZEC", "BTC", "tZECBTC"},
    {"ZEC", "USD", "tZECUSD"},
};

constexpr size_t kLastPrice = 7;

std::unordered_map<std::string, double> read_tickers(const Json& root) {
    if (!root.is_array()) {
        throw FailError(Fail::Runtime, "bitfinex returned no ticker");
    }
    if (root.size() >= 2 && root.at(0).is_string() && root.at(0).string() == "error") {
        std::string message = "bitfinex rejected the request";
        if (root.size() >= 3 && root.at(2).is_string()) {
            message = root.at(2).string();
        }
        throw FailError(Fail::Runtime, "bitfinex: " + message);
    }
    std::unordered_map<std::string, double> prices;
    for (size_t i = 0; i < root.size(); ++i) {
        const Json& row = root.at(i);
        if (!row.is_array() || row.size() <= kLastPrice || !row.at(0).is_string() || !row.at(kLastPrice).is_number()) {
            throw FailError(Fail::Runtime, "bitfinex returned no ticker");
        }
        prices.emplace(row.at(0).string(), row.at(kLastPrice).number());
    }
    return prices;
}

}  // namespace

double BitfinexProvider::rate(const Currency& from, const Currency& to, HttpClient& http) const {
    const auto plan = plan_conversion(kMarkets, sizeof(kMarkets) / sizeof(kMarkets[0]), from.code, to.code);
    if (!plan) {
        throw no_rate(id(), from, to);
    }
    const std::string url = "https://api-pub.bitfinex.com/v2/tickers?symbols=" + plan_symbols(*plan);
    const Json root = parse_json(id(), fetch(http, id(), url));
    return apply_rate(id(), from.code, to.code, *plan, read_tickers(root));
}

}  // namespace coinconvert
