#include "kraken.hpp"

#include <exception>
#include <unordered_map>

#include "http.hpp"
#include "json.hpp"
#include "markets.hpp"

namespace coinconvert {
namespace {

// Symbols are Kraken's AssetPairs keys, checked on 2026-10-26.
// The ticker field "c"[0] is the last trade price.
constexpr Market kMarkets[] = {
    {"XMR", "EUR", "XXMRZEUR"},  {"XMR", "USD", "XXMRZUSD"},  {"XMR", "BTC", "XXMRXXBT"},
    {"ETH", "EUR", "XETHZEUR"},  {"ETH", "USD", "XETHZUSD"},  {"ETH", "GBP", "XETHZGBP"},
    {"ETH", "JPY", "XETHZJPY"},  {"ETH", "CAD", "XETHZCAD"},  {"ETH", "AUD", "ETHAUD"},
    {"ETH", "CHF", "ETHCHF"},    {"ETH", "BTC", "XETHXXBT"},  {"BTC", "EUR", "XXBTZEUR"},
    {"BTC", "USD", "XXBTZUSD"},  {"BTC", "GBP", "XXBTZGBP"},  {"BTC", "JPY", "XXBTZJPY"},
    {"BTC", "CAD", "XXBTZCAD"},  {"BTC", "AUD", "XBTAUD"},    {"BTC", "CHF", "XBTCHF"},
    {"EUR", "USD", "ZEURZUSD"},  {"GBP", "USD", "ZGBPZUSD"},  {"EUR", "GBP", "EURGBP"},
    {"EUR", "JPY", "EURJPY"},    {"EUR", "CHF", "EURCHF"},    {"EUR", "CAD", "EURCAD"},
    {"EUR", "AUD", "EURAUD"},    {"USD", "CHF", "USDCHF"},    {"USD", "CAD", "ZUSDZCAD"},
    {"USD", "JPY", "ZUSDZJPY"},  {"AUD", "USD", "AUDUSD"},    {"LTC", "EUR", "XLTCZEUR"},
    {"LTC", "USD", "XLTCZUSD"},  {"LTC", "BTC", "XLTCXXBT"},  {"LTC", "GBP", "LTCGBP"},
    {"XRP", "EUR", "XXRPZEUR"},  {"XRP", "USD", "XXRPZUSD"},  {"XRP", "BTC", "XXRPXXBT"},
    {"XRP", "GBP", "XRPGBP"},    {"ADA", "EUR", "ADAEUR"},    {"ADA", "USD", "ADAUSD"},
    {"ADA", "BTC", "ADAXBT"},    {"ADA", "GBP", "ADAGBP"},    {"SOL", "EUR", "SOLEUR"},
    {"SOL", "USD", "SOLUSD"},    {"SOL", "BTC", "SOLXBT"},    {"SOL", "GBP", "SOLGBP"},
    {"XLM", "EUR", "XXLMZEUR"},  {"XLM", "USD", "XXLMZUSD"},  {"XLM", "BTC", "XXLMXXBT"},
    {"XLM", "GBP", "XXLMZGBP"},  {"DOGE", "EUR", "XDGEUR"},   {"DOGE", "USD", "XDGUSD"},
    {"DOGE", "BTC", "XXDGXXBT"}, {"DOGE", "GBP", "XDGGBP"},   {"DOT", "EUR", "DOTEUR"},
    {"DOT", "USD", "DOTUSD"},    {"DOT", "BTC", "DOTXBT"},    {"DOT", "GBP", "DOTGBP"},
    {"LINK", "EUR", "LINKEUR"},  {"LINK", "USD", "LINKUSD"},  {"LINK", "BTC", "LINKXBT"},
    {"LINK", "GBP", "LINKGBP"},  {"ATOM", "EUR", "ATOMEUR"},  {"ATOM", "USD", "ATOMUSD"},
    {"ATOM", "BTC", "ATOMXBT"},  {"ATOM", "GBP", "ATOMGBP"},  {"ETC", "EUR", "XETCZEUR"},
    {"ETC", "USD", "XETCZUSD"},  {"ETC", "BTC", "XETCXXBT"},  {"UNI", "EUR", "UNIEUR"},
    {"UNI", "USD", "UNIUSD"},    {"UNI", "BTC", "UNIXBT"},    {"AAVE", "EUR", "AAVEEUR"},
    {"AAVE", "USD", "AAVEUSD"},  {"AAVE", "BTC", "AAVEXBT"},  {"AAVE", "GBP", "AAVEGBP"},
    {"ALGO", "EUR", "ALGOEUR"},  {"ALGO", "USD", "ALGOUSD"},  {"ALGO", "BTC", "ALGOXBT"},
    {"FIL", "EUR", "FILEUR"},    {"FIL", "USD", "FILUSD"},    {"FIL", "BTC", "FILXBT"},
    {"FIL", "GBP", "FILGBP"},    {"BCH", "EUR", "BCHEUR"},    {"BCH", "USD", "BCHUSD"},
    {"BCH", "BTC", "BCHXBT"},    {"BCH", "GBP", "BCHGBP"},    {"TRX", "EUR", "TRXEUR"},
    {"TRX", "USD", "TRXUSD"},    {"TRX", "BTC", "TRXXBT"},    {"ZEC", "EUR", "XZECZEUR"},
    {"ZEC", "USD", "XZECZUSD"},  {"DASH", "EUR", "DASHEUR"},  {"DASH", "USD", "DASHUSD"},
    {"XTZ", "EUR", "XTZEUR"},    {"XTZ", "USD", "XTZUSD"},    {"NEAR", "EUR", "NEAREUR"},
    {"NEAR", "USD", "NEARUSD"},  {"APT", "EUR", "APTEUR"},    {"APT", "USD", "APTUSD"},
    {"SUI", "EUR", "SUIEUR"},    {"SUI", "USD", "SUIUSD"},    {"SUI", "GBP", "SUIGBP"},
    {"INJ", "EUR", "INJEUR"},    {"INJ", "USD", "INJUSD"},    {"PEPE", "EUR", "PEPEEUR"},
    {"PEPE", "USD", "PEPEUSD"},  {"PEPE", "GBP", "PEPEGBP"},  {"SHIB", "EUR", "SHIBEUR"},
    {"SHIB", "USD", "SHIBUSD"},  {"GRT", "EUR", "GRTEUR"},    {"GRT", "USD", "GRTUSD"},
    {"CRV", "USD", "CRVUSD"},    {"LDO", "USD", "LDOUSD"},    {"COMP", "EUR", "COMPEUR"},
    {"COMP", "USD", "COMPUSD"},  {"COMP", "BTC", "COMPXBT"},  {"MINA", "USD", "MINAUSD"},
    {"AXS", "USD", "AXSUSD"},    {"APE", "EUR", "APEEUR"},    {"APE", "USD", "APEUSD"},
    {"CHZ", "USD", "CHZUSD"},    {"BAT", "USD", "BATUSD"},    {"ZRX", "USD", "ZRXUSD"},
    {"1INCH", "USD", "1INCHUSD"},{"SAND", "USD", "SANDUSD"},  {"MANA", "USD", "MANAUSD"},
    {"YFI", "USD", "YFIUSD"},    {"SUSHI", "USD", "SUSHIUSD"},{"ANKR", "USD", "ANKRUSD"},
    {"ENJ", "USD", "ENJUSD"},    {"FLOW", "USD", "FLOWUSD"},  {"KAVA", "USD", "KAVAUSD"},
    {"SNX", "EUR", "SNXEUR"},    {"SNX", "USD", "SNXUSD"},
};

std::unordered_map<std::string, double> read_tickers(const Json& root, const RatePlan& plan) {
    if (const Json* error = root.find("error"); error != nullptr && error->is_array() && error->size() > 0) {
        std::string message = "kraken rejected the request";
        if (error->at(0).is_string() && !error->at(0).string().empty()) {
            message = error->at(0).string();
        }
        throw FailError(Fail::Runtime, "kraken: " + message);
    }
    const Json* result = root.find("result");
    if (result == nullptr || !result->is_object()) {
        throw FailError(Fail::Runtime, "kraken returned no ticker");
    }
    std::unordered_map<std::string, double> prices;
    for (const Leg& leg : plan.legs) {
        if (prices.count(leg.symbol) != 0) {
            continue;
        }
        const Json* ticker = result->find(leg.symbol);
        if (ticker == nullptr || !ticker->is_object()) {
            continue;
        }
        const Json* last = ticker->find("c");
        if (last == nullptr || !last->is_array() || last->size() == 0) {
            continue;
        }
        try {
            prices.emplace(leg.symbol, last->at(0).decimal());
        } catch (const std::exception&) {
            continue;
        }
    }
    return prices;
}

}  // namespace

double KrakenProvider::rate(const Currency& from, const Currency& to, HttpClient& http) const {
    const auto plan = plan_conversion(kMarkets, sizeof(kMarkets) / sizeof(kMarkets[0]), from.code, to.code);
    if (!plan) {
        throw no_rate(id(), from, to);
    }
    const std::string symbols = plan_symbols(*plan);
    const std::string url = "https://api.kraken.com/0/public/Ticker?pair=" + symbols;
    const Json root = parse_json(id(), fetch(http, id(), url));
    return apply_rate(id(), from.code, to.code, *plan, read_tickers(root, *plan));
}

}  // namespace coinconvert
