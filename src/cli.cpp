#include "cli.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "currency.hpp"
#include "format.hpp"
#include "provider.hpp"
#include "text.hpp"
#include "version.hpp"

namespace coinconvert {
namespace {

const char* kHelp =
    "coinconvert — convert cryptocurrency and fiat amounts\n"
    "\n"
    "Usage:\n"
    "  coinconvert [--provider NAME] FROM AMOUNT TO\n"
    "  coinconvert --list-providers\n"
    "  coinconvert --list-currencies\n"
    "  coinconvert --help\n"
    "  coinconvert --version\n"
    "\n"
    "Examples:\n"
    "  coinconvert xmr 0.004 eur\n"
    "  coinconvert --provider kraken btc 0.01 usd\n"
    "  coinconvert eth 2 xmr\n"
    "  coinconvert eur 25 usd\n"
    "\n"
    "Options:\n"
    "  -p, --provider NAME   Price provider to query (default: coingecko)\n"
    "  --list-providers      Show providers and exit\n"
    "  --list-currencies     Show supported currency codes and exit\n"
    "  -h, --help            Show this help and exit\n"
    "  --version             Show the version and exit\n"
    "\n"
    "The result is rounded to two decimal places. The provider that supplied\n"
    "the rate is printed on the next line. A missing rate or a network failure\n"
    "is an error; coinconvert does not substitute a guessed price.\n"
    "Each run converts once and exits.\n";

struct Options {
    bool help = false;
    bool version = false;
    bool list_providers = false;
    bool list_currencies = false;
    std::string provider;
    bool provider_set = false;
    std::vector<std::string> positionals;
};

bool looks_like_number_token(const std::string& arg) {
    return arg.size() >= 2 && arg[0] == '-' &&
           ((arg[1] >= '0' && arg[1] <= '9') || arg[1] == '.');
}

Options parse_options(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i] == nullptr ? "" : argv[i];
        if (arg == "--help" || arg == "-h") {
            options.help = true;
        } else if (arg == "--version") {
            options.version = true;
        } else if (arg == "--list-providers") {
            options.list_providers = true;
        } else if (arg == "--list-currencies") {
            options.list_currencies = true;
        } else if (arg == "--provider" || arg == "-p") {
            if (i + 1 >= argc || argv[i + 1] == nullptr) {
                throw FailError(Fail::Usage, "missing provider name");
            }
            options.provider = argv[++i];
            options.provider_set = true;
        } else if (arg.rfind("--provider=", 0) == 0) {
            options.provider = arg.substr(std::string("--provider=").size());
            options.provider_set = true;
            if (options.provider.empty()) {
                throw FailError(Fail::Usage, "missing provider name");
            }
        } else if (!arg.empty() && arg[0] == '-' && !looks_like_number_token(arg)) {
            throw FailError(Fail::Usage, "unknown option '" + arg + "'");
        } else {
            options.positionals.push_back(arg);
        }
    }
    return options;
}

void print_providers(std::ostream& out) {
    for (const PriceProvider* provider : providers()) {
        out << std::left << std::setw(14) << provider->id() << provider->summary();
        if (ascii_iequals(provider->id(), default_provider().id())) {
            out << " (default)";
        }
        out << '\n';
    }
}

void print_currencies(std::ostream& out) {
    out << "crypto:\n";
    for (const Currency& currency : all_currencies()) {
        if (currency.kind == AssetKind::Crypto) {
            out << "  " << std::left << std::setw(8) << currency.code << currency.name << '\n';
        }
    }
    out << "fiat:\n";
    for (const Currency& currency : all_currencies()) {
        if (currency.kind == AssetKind::Fiat) {
            out << "  " << std::left << std::setw(8) << currency.code << currency.name << '\n';
        }
    }
}

std::string provider_names() {
    std::string names;
    for (const PriceProvider* provider : providers()) {
        if (!names.empty()) {
            names += ", ";
        }
        names += provider->id();
    }
    return names;
}

int convert(const Options& options, HttpClient& http, std::ostream& out) {
    if (options.positionals.size() != 3) {
        throw FailError(Fail::Usage, "usage: coinconvert [--provider NAME] FROM AMOUNT TO");
    }
    const Currency* from = find_currency(options.positionals[0]);
    if (from == nullptr) {
        throw FailError(Fail::Usage, "unknown currency '" + options.positionals[0] + "'");
    }
    double amount = 0;
    if (!parse_amount(options.positionals[1], amount)) {
        throw FailError(Fail::Usage, "amount must be a non-negative number");
    }
    const Currency* to = find_currency(options.positionals[2]);
    if (to == nullptr) {
        throw FailError(Fail::Usage, "unknown currency '" + options.positionals[2] + "'");
    }

    const PriceProvider* provider = &default_provider();
    std::string provider_id = provider->id();
    if (options.provider_set) {
        provider = find_provider(options.provider);
        if (provider == nullptr) {
            throw FailError(Fail::Usage, "unknown provider '" + options.provider +
                                             "'. Available providers: " + provider_names());
        }
        provider_id = provider->id();
    }

    double unit = 1;
    if (from->code != to->code) {
        unit = provider->rate(*from, *to, http);
        if (!std::isfinite(unit) || !(unit > 0.0)) {
            throw no_rate(provider_id, *from, *to);
        }
    } else {
        provider_id = "identity";
    }

    const double converted = amount * unit;
    if (!std::isfinite(converted)) {
        throw FailError(Fail::Runtime, "conversion result is not a finite number");
    }
    out << options.positionals[1] << ' ' << from->code << " = " << format_result(converted) << ' '
        << to->code << '\n';
    out << "provider: " << provider_id << '\n';
    return 0;
}

int execute(const Options& options, HttpClient& http, std::ostream& out) {
    if (options.help) {
        out << kHelp;
        return 0;
    }
    if (options.version) {
        out << "coinconvert " << kVersion << '\n';
        return 0;
    }
    const int modes = static_cast<int>(options.list_providers) + static_cast<int>(options.list_currencies);
    if (modes > 1 || (modes == 1 && !options.positionals.empty())) {
        throw FailError(Fail::Usage, "usage: coinconvert [--provider NAME] FROM AMOUNT TO");
    }
    if (options.list_providers) {
        print_providers(out);
        return 0;
    }
    if (options.list_currencies) {
        print_currencies(out);
        return 0;
    }
    return convert(options, http, out);
}

}  // namespace

int cli_main(int argc, char** argv) {
    CurlClient http;
    return cli_main(argc, argv, http, std::cout, std::cerr);
}

int cli_main(int argc, char** argv, HttpClient& http, std::ostream& out, std::ostream& err) {
    try {
        if (argc < 1) {
            throw FailError(Fail::Usage, "usage: coinconvert [--provider NAME] FROM AMOUNT TO");
        }
        return execute(parse_options(argc, argv), http, out);
    } catch (const FailError& error) {
        err << "coinconvert: " << error.what() << '\n';
        if (error.kind == Fail::Usage) {
            err << "try 'coinconvert --help'\n";
        }
        return static_cast<int>(error.kind);
    } catch (const std::exception& error) {
        err << "coinconvert: " << error.what() << '\n';
        return 2;
    }
}

}  // namespace coinconvert
