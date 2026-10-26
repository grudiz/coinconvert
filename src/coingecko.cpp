#include "coingecko.hpp"

#include <cmath>
#include <optional>

#include "http.hpp"
#include "json.hpp"
#include "text.hpp"

namespace coinconvert {
namespace {

constexpr const char* kVsCrypto[] = {"btc", "eth", "ltc", "bch", "xrp", "sol"};

bool is_vs(const Currency& currency) {
    if (currency.kind == AssetKind::Fiat) {
        return true;
    }
    const std::string code = ascii_lower(currency.code);
    for (const char* vs : kVsCrypto) {
        if (code == vs) {
            return true;
        }
    }
    return false;
}

std::string price_url(const std::string& ids, const std::string& vs) {
    return "https://api.coingecko.com/api/v3/simple/price?ids=" + ids + "&vs_currencies=" + vs;
}

void reject_status(const Json& root) {
    const Json* status = root.find("status");
    if (status == nullptr || !status->is_object()) {
        return;
    }
    const Json* code = status->find("error_code");
    if (code == nullptr) {
        return;
    }
    std::string detail = "request was rejected";
    if (const Json* message = status->find("error_message"); message != nullptr && message->is_string()) {
        detail = message->string();
    }
    throw FailError(Fail::Runtime, "coingecko: " + detail);
}

std::optional<double> quote(const Json& root, const std::string& id, const std::string& vs) {
    const Json* coin = root.find(id);
    if (coin == nullptr || !coin->is_object()) {
        return std::nullopt;
    }
    const Json* price = coin->find(vs);
    if (price == nullptr || !price->is_number()) {
        return std::nullopt;
    }
    const double value = price->number();
    if (!std::isfinite(value) || !(value > 0.0)) {
        return std::nullopt;
    }
    return value;
}

Json load(HttpClient& http, const std::string& url) {
    const Json root = parse_json("coingecko", fetch(http, "coingecko", url));
    reject_status(root);
    return root;
}

}  // namespace

double CoinGeckoProvider::rate(const Currency& from, const Currency& to, HttpClient& http) const {
    if (from.kind == AssetKind::Crypto && is_vs(to)) {
        const std::string vs = ascii_lower(to.code);
        const Json root = load(http, price_url(from.coingecko, vs));
        if (const auto price = quote(root, from.coingecko, vs)) {
            return *price;
        }
        if (to.kind == AssetKind::Fiat) {
            throw no_rate(id(), from, to);
        }
    } else if (to.kind == AssetKind::Crypto && is_vs(from)) {
        const std::string vs = ascii_lower(from.code);
        const Json root = load(http, price_url(to.coingecko, vs));
        if (const auto price = quote(root, to.coingecko, vs)) {
            return 1.0 / *price;
        }
        if (from.kind == AssetKind::Fiat) {
            throw no_rate(id(), from, to);
        }
    }

    if (from.kind == AssetKind::Crypto && to.kind == AssetKind::Crypto) {
        const Json root = load(http, price_url(from.coingecko + "," + to.coingecko, "usd"));
        const auto from_usd = quote(root, from.coingecko, "usd");
        const auto to_usd = quote(root, to.coingecko, "usd");
        if (!from_usd || !to_usd) {
            throw no_rate(id(), from, to);
        }
        return *from_usd / *to_usd;
    }

    if (from.kind == AssetKind::Fiat && to.kind == AssetKind::Fiat) {
        const std::string vs = ascii_lower(from.code) + "," + ascii_lower(to.code);
        const Json root = load(http, price_url("bitcoin", vs));
        const auto from_btc = quote(root, "bitcoin", ascii_lower(from.code));
        const auto to_btc = quote(root, "bitcoin", ascii_lower(to.code));
        if (!from_btc || !to_btc) {
            throw no_rate(id(), from, to);
        }
        return *to_btc / *from_btc;
    }

    throw no_rate(id(), from, to);
}

}  // namespace coinconvert
