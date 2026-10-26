#include "bitfinex.hpp"
#include "cli.hpp"
#include "coingecko.hpp"
#include "coinpaprika.hpp"
#include "currency.hpp"
#include "error.hpp"
#include "format.hpp"
#include "http.hpp"
#include "json.hpp"
#include "kraken.hpp"

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check(bool condition, int line, const char* text) {
    if (!condition) {
        std::cerr << "FAIL " << line << ": " << text << '\n';
        ++g_failures;
    }
}

void check_eq(const std::string& got, const std::string& expected, int line) {
    if (got != expected) {
        std::cerr << "FAIL " << line << "\n  got: [" << got << "]\n  expected: [" << expected << "]\n";
        ++g_failures;
    }
}

bool close_to(double got, double expected) {
    const double scale = std::fabs(expected) > 1.0 ? std::fabs(expected) : 1.0;
    return std::fabs(got - expected) <= 1e-6 * scale;
}

class ScriptedHttp : public coinconvert::HttpClient {
public:
    std::vector<std::string> bodies;
    std::vector<std::string> urls;
    bool fail = false;

    std::string get(const std::string& url) override {
        urls.push_back(url);
        if (fail) {
            throw coinconvert::FailError(coinconvert::Fail::Runtime, "network request failed: timeout");
        }
        if (bodies.empty()) {
            throw coinconvert::FailError(coinconvert::Fail::Runtime, "unexpected request " + url);
        }
        std::string body = bodies.front();
        bodies.erase(bodies.begin());
        return body;
    }
};

const coinconvert::Currency& currency(const char* code) {
    const coinconvert::Currency* found = coinconvert::find_currency(code);
    if (found == nullptr) {
        throw std::runtime_error(std::string("missing test currency ") + code);
    }
    return *found;
}

struct Run {
    int code = 0;
    std::string out;
    std::string err;
};

Run run(std::initializer_list<std::string> args, ScriptedHttp& http) {
    std::vector<std::string> storage(args);
    std::vector<char*> argv;
    argv.reserve(storage.size());
    for (std::string& arg : storage) {
        argv.push_back(arg.data());
    }
    std::ostringstream out;
    std::ostringstream err;
    Run result;
    result.code = coinconvert::cli_main(static_cast<int>(argv.size()), argv.data(), http, out, err);
    result.out = out.str();
    result.err = err.str();
    return result;
}

void test_json() {
    const coinconvert::Json value = coinconvert::Json::parse(
        " { \"a\" : [1, \"x\\ny\", true, null, 1.5e-2], \"b\": \"\\u0041\" } ");
    check(value.is_object(), __LINE__, "object");
    check(value.at("a").at(0).number() == 1.0, __LINE__, "int");
    check(value.at("a").at(1).string() == "x\ny", __LINE__, "escape");
    check(value.at("a").at(2).boolean(), __LINE__, "bool");
    check(value.at("a").at(3).is_null(), __LINE__, "null");
    check(close_to(value.at("a").at(4).number(), 0.015), __LINE__, "exponent");
    check(value.at("b").string() == "A", __LINE__, "unicode");
    check(coinconvert::Json::parse("[\"521.42\"]").at(0).decimal() == 521.42, __LINE__, "decimal string");
    bool threw = false;
    try {
        coinconvert::Json::parse("{\"a\":1} trailing");
    } catch (const std::exception&) {
        threw = true;
    }
    check(threw, __LINE__, "trailing junk");
}

void test_format_and_amount() {
    check_eq(coinconvert::format_result(0), "0.00", __LINE__);
    check_eq(coinconvert::format_result(1), "1.00", __LINE__);
    check_eq(coinconvert::format_result(1.2), "1.20", __LINE__);
    check_eq(coinconvert::format_result(1.234), "1.23", __LINE__);
    check_eq(coinconvert::format_result(2.08568), "2.09", __LINE__);
    check_eq(coinconvert::format_result(0.004), "0.00", __LINE__);
    check_eq(coinconvert::format_result(123456789.126), "123456789.13", __LINE__);
    check_eq(coinconvert::format_result(-1.2), "-1.20", __LINE__);

    double amount = -1;
    check(coinconvert::parse_amount("0.004", amount) && amount == 0.004, __LINE__, "0.004");
    check(coinconvert::parse_amount("+2", amount) && amount == 2, __LINE__, "+2");
    check(coinconvert::parse_amount(".5", amount) && amount == 0.5, __LINE__, ".5");
    check(coinconvert::parse_amount("5.", amount) && amount == 5, __LINE__, "5.");
    check(coinconvert::parse_amount("1e-3", amount) && close_to(amount, 0.001), __LINE__, "1e-3");
    check(coinconvert::parse_amount("0", amount) && amount == 0, __LINE__, "zero");
    check(!coinconvert::parse_amount("-1", amount), __LINE__, "negative");
    check(!coinconvert::parse_amount("1.2.3", amount), __LINE__, "two dots");
    check(!coinconvert::parse_amount("", amount), __LINE__, "empty");
    check(!coinconvert::parse_amount("inf", amount), __LINE__, "inf");
    check(!coinconvert::parse_amount("1e", amount), __LINE__, "bare exponent");
}

void test_currencies() {
    check(coinconvert::find_currency("xmr") != nullptr, __LINE__, "xmr");
    check(coinconvert::find_currency("eur")->kind == coinconvert::AssetKind::Fiat, __LINE__, "eur fiat");
    check(coinconvert::find_currency("XMR")->coingecko == "monero", __LINE__, "monero id");
    check(coinconvert::find_currency("nope") == nullptr, __LINE__, "unknown");
    check(coinconvert::find_currency("php")->paprika.empty(), __LINE__, "php unlisted on paprika");
    check(!coinconvert::find_currency("eur")->paprika.empty(), __LINE__, "eur listed");
}

void test_coingecko() {
    coinconvert::CoinGeckoProvider provider;
    ScriptedHttp http;
    http.bodies.push_back(R"({"monero":{"eur":521.42}})");
    const double rate = provider.rate(currency("xmr"), currency("eur"), http);
    check(close_to(rate, 521.42), __LINE__, "xmr eur");
    check(http.urls.size() == 1, __LINE__, "one request");
    check(http.urls[0].find("ids=monero") != std::string::npos, __LINE__, "monero id");
    check(http.urls[0].find("vs_currencies=eur") != std::string::npos, __LINE__, "eur quote");

    ScriptedHttp fiat;
    fiat.bodies.push_back(R"({"bitcoin":{"usd":80000,"eur":71200}})");
    const double eur_per_usd = provider.rate(currency("usd"), currency("eur"), fiat);
    check(close_to(eur_per_usd, 71200.0 / 80000.0), __LINE__, "fiat cross");
    check(fiat.urls[0].find("ids=bitcoin") != std::string::npos, __LINE__, "btc bridge");

    ScriptedHttp inverse;
    inverse.bodies.push_back(R"({"monero":{"btc":0.0073}})");
    const double xmr_per_btc = provider.rate(currency("btc"), currency("xmr"), inverse);
    check(close_to(xmr_per_btc, 1.0 / 0.0073), __LINE__, "inverse");

    ScriptedHttp cross;
    cross.bodies.push_back(R"({"dogecoin":{"usd":0.2},"shiba-inu":{"usd":0.00002}})");
    const double shib_per_doge = provider.rate(currency("doge"), currency("shib"), cross);
    check(close_to(shib_per_doge, 0.2 / 0.00002), __LINE__, "usd cross");
    check(cross.urls.size() == 1, __LINE__, "single usd request");

    ScriptedHttp fallback;
    fallback.bodies.push_back(R"({"monero":{}})");
    fallback.bodies.push_back(R"({"solana":{"usd":100},"monero":{"usd":50}})");
    const double xmr_per_sol = provider.rate(currency("sol"), currency("xmr"), fallback);
    check(close_to(xmr_per_sol, 2.0), __LINE__, "fallback cross");
    check(fallback.urls.size() == 2, __LINE__, "fallback requests");

    ScriptedHttp missing;
    missing.bodies.push_back(R"({"monero":{}})");
    bool threw = false;
    try {
        provider.rate(currency("xmr"), currency("eur"), missing);
    } catch (const coinconvert::FailError& error) {
        threw = error.what() == std::string("coingecko has no rate for XMR/EUR");
    }
    check(threw, __LINE__, "missing fiat quote");
}

void test_coinpaprika() {
    coinconvert::CoinPaprikaProvider provider;
    ScriptedHttp http;
    http.bodies.push_back(R"({"price":521.42,"amount":1})");
    const double rate = provider.rate(currency("xmr"), currency("eur"), http);
    check(close_to(rate, 521.42), __LINE__, "paprika rate");
    check(http.urls[0].find("base_currency_id=xmr-monero") != std::string::npos, __LINE__, "base");
    check(http.urls[0].find("quote_currency_id=eur-euro") != std::string::npos, __LINE__, "quote");

    ScriptedHttp blocked;
    blocked.fail = true;
    bool threw = false;
    try {
        provider.rate(currency("php"), currency("eur"), blocked);
    } catch (const coinconvert::FailError& error) {
        threw = std::string(error.what()).find("no rate") != std::string::npos;
    }
    check(threw && blocked.urls.empty(), __LINE__, "unlisted currency skips the network");
}

void test_kraken() {
    coinconvert::KrakenProvider provider;
    ScriptedHttp direct;
    direct.bodies.push_back(R"({"error":[],"result":{"XXMRZEUR":{"c":["521.42","0.1"]}}})");
    const double eur = provider.rate(currency("xmr"), currency("eur"), direct);
    check(close_to(eur, 521.42), __LINE__, "kraken direct");
    check(direct.urls[0].find("pair=XXMRZEUR") != std::string::npos, __LINE__, "direct pair");
    check(direct.urls[0].find("XXBT") == std::string::npos, __LINE__, "no bitcoin cross");

    ScriptedHttp cross;
    cross.bodies.push_back(
        R"({"error":[],"result":{"XXMRZUSD":{"c":["584.30","1"]},"ZGBPZUSD":{"c":["1.335","1"]}}})");
    const double gbp = provider.rate(currency("xmr"), currency("gbp"), cross);
    check(close_to(gbp, 584.30 / 1.335), __LINE__, "kraken gbp cross");
    check(cross.urls[0].find("XXMRZUSD") != std::string::npos, __LINE__, "usd leg");
    check(cross.urls[0].find("ZGBPZUSD") != std::string::npos, __LINE__, "gbp leg");

    ScriptedHttp broken;
    broken.bodies.push_back(
        R"({"error":["EQuery:Unknown asset pair"],"result":{"XXMRZEUR":{"c":["521.42","1"]}}})");
    bool threw = false;
    try {
        provider.rate(currency("xmr"), currency("eur"), broken);
    } catch (const coinconvert::FailError& error) {
        threw = std::string(error.what()).find("EQuery:Unknown asset pair") != std::string::npos;
    }
    check(threw, __LINE__, "kraken error is not a price");

    ScriptedHttp offline;
    offline.fail = true;
    threw = false;
    try {
        provider.rate(currency("hype"), currency("eur"), offline);
    } catch (const coinconvert::FailError& error) {
        threw = std::string(error.what()).find("no rate") != std::string::npos;
    }
    check(threw && offline.urls.empty(), __LINE__, "kraken does not invent HYPE");
}

void test_bitfinex() {
    coinconvert::BitfinexProvider provider;
    ScriptedHttp direct;
    direct.bodies.push_back(
        R"([["tXMRUSD",1,1,1,1,1,111.0,584.25,1,1,1]])");
    const double usd = provider.rate(currency("xmr"), currency("usd"), direct);
    check(close_to(usd, 584.25), __LINE__, "last price index");

    ScriptedHttp gbp;
    gbp.bodies.push_back(
        R"([["tXMRBTC",0,0,0,0,0,0,0.00733,0,0,0],["tBTCGBP",0,0,0,0,0,0,59700,0,0,0]])");
    const double pounds = provider.rate(currency("xmr"), currency("gbp"), gbp);
    check(close_to(pounds, 0.00733 * 59700.0), __LINE__, "xmr gbp");

    ScriptedHttp fx;
    fx.bodies.push_back(
        R"([["tBTCEUR",0,0,0,0,0,0,70000,0,0,0],["tBTCUSD",0,0,0,0,0,0,80000,0,0,0]])");
    const double usd_per_eur = provider.rate(currency("eur"), currency("usd"), fx);
    check(close_to(usd_per_eur, 80000.0 / 70000.0), __LINE__, "eur usd");

    ScriptedHttp offline;
    offline.fail = true;
    bool threw = false;
    try {
        provider.rate(currency("hype"), currency("usd"), offline);
    } catch (const coinconvert::FailError&) {
        threw = true;
    }
    check(threw && offline.urls.empty(), __LINE__, "bitfinex missing market");
}

void test_cli() {
    ScriptedHttp http;
    http.bodies.push_back(R"({"monero":{"eur":521.42}})");
    const Run converted = run({"coinconvert", "xmr", "0.004", "eur"}, http);
    check(converted.code == 0, __LINE__, "exit");
    check_eq(converted.out, "0.004 XMR = 2.09 EUR\nprovider: coingecko\n", __LINE__);
    check(converted.err.empty(), __LINE__, "no stderr");

    ScriptedHttp kraken;
    kraken.bodies.push_back(R"({"error":[],"result":{"XXMRZEUR":{"c":["500","1"]}}})");
    const Run selected = run({"coinconvert", "xmr", "0.004", "eur", "--provider", "Kraken"}, kraken);
    check(selected.code == 0, __LINE__, "kraken exit");
    check_eq(selected.out, "0.004 XMR = 2.00 EUR\nprovider: kraken\n", __LINE__);
    check(kraken.urls[0].find("api.kraken.com") != std::string::npos, __LINE__, "kraken host");

    ScriptedHttp idle;
    idle.fail = true;
    const Run providers = run({"coinconvert", "--list-providers"}, idle);
    check(providers.code == 0, __LINE__, "list providers");
    check(providers.out.find("coingecko") != std::string::npos, __LINE__, "lists coingecko");
    check(providers.out.find("coinpaprika") != std::string::npos, __LINE__, "lists paprika");
    check(providers.out.find("kraken") != std::string::npos, __LINE__, "lists kraken");
    check(providers.out.find("bitfinex") != std::string::npos, __LINE__, "lists bitfinex");
    check(providers.out.find("default") != std::string::npos, __LINE__, "marks default");
    check(idle.urls.empty(), __LINE__, "list is offline");

    const Run same = run({"coinconvert", "xmr", "0.004", "xmr"}, idle);
    check(same.code == 0, __LINE__, "identity exit");
    check_eq(same.out, "0.004 XMR = 0.00 XMR\nprovider: identity\n", __LINE__);

    const Run unknown = run({"coinconvert", "foo", "1", "eur"}, idle);
    check(unknown.code == 1, __LINE__, "unknown currency");
    check(unknown.err.find("unknown currency 'foo'") != std::string::npos, __LINE__, "currency message");

    const Run bad_provider = run({"coinconvert", "--provider", "coinbase", "xmr", "1", "eur"}, idle);
    check(bad_provider.code == 1, __LINE__, "unknown provider");
    check(bad_provider.err.find("unknown provider") != std::string::npos, __LINE__, "provider message");

    ScriptedHttp down;
    down.fail = true;
    const Run failure = run({"coinconvert", "btc", "1", "eur"}, down);
    check(failure.code == 2, __LINE__, "network exit");
    check(failure.err.find("network request failed") != std::string::npos, __LINE__, "network message");
    check(failure.out.empty(), __LINE__, "no partial result");

    const Run help = run({"coinconvert", "--help"}, idle);
    check(help.code == 0 && help.out.find("coinconvert xmr 0.004 eur") != std::string::npos, __LINE__, "help");

    const Run version = run({"coinconvert", "--version"}, idle);
    check_eq(version.out, "coinconvert 1.0.0\n", __LINE__);
}

}  // namespace

int main() {
    try {
        test_json();
        test_format_and_amount();
        test_currencies();
        test_coingecko();
        test_coinpaprika();
        test_kraken();
        test_bitfinex();
        test_cli();
    } catch (const std::exception& error) {
        std::cerr << "exception: " << error.what() << '\n';
        return 1;
    }
    if (g_failures != 0) {
        std::cerr << g_failures << " failure(s)\n";
        return 1;
    }
    std::cout << "ok\n";
    return 0;
}
