#include "currency.hpp"

#include "text.hpp"

namespace coinconvert {
namespace {

struct Row {
    const char* code;
    const char* name;
    AssetKind kind;
    const char* coingecko;
    const char* paprika;
};

// CoinGecko ids were checked against /simple/price, and CoinPaprika ids against
// /price-converter, on 2026-10-26. An empty Paprika id means that provider
// does not list the currency. SNX still uses CoinGecko's historical id "havven".
constexpr Row kRows[] = {
    {"1INCH", "1inch", AssetKind::Crypto, "1inch", "1inch-1inch"},
    {"AAVE", "Aave", AssetKind::Crypto, "aave", "aave-new"},
    {"ADA", "Cardano", AssetKind::Crypto, "cardano", "ada-cardano"},
    {"ALGO", "Algorand", AssetKind::Crypto, "algorand", "algo-algorand"},
    {"ANKR", "Ankr", AssetKind::Crypto, "ankr", "ankr-ankr-network"},
    {"APE", "ApeCoin", AssetKind::Crypto, "apecoin", "ape-apecoin"},
    {"APT", "Aptos", AssetKind::Crypto, "aptos", "apt-aptos"},
    {"ARB", "Arbitrum", AssetKind::Crypto, "arbitrum", "arb-arbitrum"},
    {"ATOM", "Cosmos", AssetKind::Crypto, "cosmos", "atom-cosmos"},
    {"AVAX", "Avalanche", AssetKind::Crypto, "avalanche-2", "avax-avalanche"},
    {"AXS", "Axie Infinity", AssetKind::Crypto, "axie-infinity", "axs-axie-infinity"},
    {"BAT", "Basic Attention Token", AssetKind::Crypto, "basic-attention-token", "bat-basic-attention-token"},
    {"BCH", "Bitcoin Cash", AssetKind::Crypto, "bitcoin-cash", "bch-bitcoin-cash"},
    {"BNB", "BNB", AssetKind::Crypto, "binancecoin", "bnb-binance-coin"},
    {"BONK", "Bonk", AssetKind::Crypto, "bonk", "bonk-bonk"},
    {"BTC", "Bitcoin", AssetKind::Crypto, "bitcoin", "btc-bitcoin"},
    {"CAKE", "PancakeSwap", AssetKind::Crypto, "pancakeswap-token", "cake-pancakeswap"},
    {"CHZ", "Chiliz", AssetKind::Crypto, "chiliz", "chz-chiliz"},
    {"COMP", "Compound", AssetKind::Crypto, "compound-governance-token", "comp-compoundd"},
    {"CRO", "Cronos", AssetKind::Crypto, "crypto-com-chain", "cro-cryptocom-chain"},
    {"CRV", "Curve", AssetKind::Crypto, "curve-dao-token", "crv-curve-dao-token"},
    {"DAI", "Dai", AssetKind::Crypto, "dai", "dai-dai"},
    {"DASH", "Dash", AssetKind::Crypto, "dash", "dash-dash"},
    {"DOGE", "Dogecoin", AssetKind::Crypto, "dogecoin", "doge-dogecoin"},
    {"DOT", "Polkadot", AssetKind::Crypto, "polkadot", "dot-polkadot"},
    {"ENA", "Ethena", AssetKind::Crypto, "ethena", "ena-ethena"},
    {"ENJ", "Enjin", AssetKind::Crypto, "enjincoin", "enj-enjin-coin"},
    {"EOS", "EOS", AssetKind::Crypto, "eos", "eos-eos"},
    {"ETC", "Ethereum Classic", AssetKind::Crypto, "ethereum-classic", "etc-ethereum-classic"},
    {"ETH", "Ethereum", AssetKind::Crypto, "ethereum", "eth-ethereum"},
    {"FET", "Artificial Superintelligence Alliance", AssetKind::Crypto, "fetch-ai", "fetch-ai"},
    {"FIL", "Filecoin", AssetKind::Crypto, "filecoin", "fil-filecoin"},
    {"FLOKI", "Floki", AssetKind::Crypto, "floki", "floki-floki-inu"},
    {"FLOW", "Flow", AssetKind::Crypto, "flow", "flow-flow"},
    {"GALA", "Gala", AssetKind::Crypto, "gala", "gala-gala"},
    {"GRT", "The Graph", AssetKind::Crypto, "the-graph", "grt-the-graph"},
    {"HBAR", "Hedera", AssetKind::Crypto, "hedera-hashgraph", "hbar-hedera-hashgraph"},
    {"HYPE", "Hyperliquid", AssetKind::Crypto, "hyperliquid", "hype-hyperliquid"},
    {"ICP", "Internet Computer", AssetKind::Crypto, "internet-computer", "icp-internet-computer"},
    {"IMX", "Immutable", AssetKind::Crypto, "immutable-x", "imx-immutable-x"},
    {"INJ", "Injective", AssetKind::Crypto, "injective-protocol", "inj-injective-protocol"},
    {"JUP", "Jupiter", AssetKind::Crypto, "jupiter-exchange-solana", "jup-jupiter-exchange-token"},
    {"KAS", "Kaspa", AssetKind::Crypto, "kaspa", "kas-kaspa"},
    {"KAVA", "Kava", AssetKind::Crypto, "kava", "kava-kava"},
    {"LDO", "Lido DAO", AssetKind::Crypto, "lido-dao", "ldo-lido-dao"},
    {"LINK", "Chainlink", AssetKind::Crypto, "chainlink", "link-chainlink"},
    {"LTC", "Litecoin", AssetKind::Crypto, "litecoin", "ltc-litecoin"},
    {"MANA", "Decentraland", AssetKind::Crypto, "decentraland", "mana-decentraland"},
    {"MINA", "Mina", AssetKind::Crypto, "mina-protocol", "mina-mina-protocol"},
    {"MKR", "Maker", AssetKind::Crypto, "maker", "mkr-maker"},
    {"NEAR", "NEAR Protocol", AssetKind::Crypto, "near", "near-near-protocol"},
    {"OKB", "OKB", AssetKind::Crypto, "okb", "okb-okb"},
    {"ONDO", "Ondo", AssetKind::Crypto, "ondo-finance", "ondo-ondo"},
    {"OP", "Optimism", AssetKind::Crypto, "optimism", "op-optimism"},
    {"PEPE", "Pepe", AssetKind::Crypto, "pepe", "pepe-pepe"},
    {"POL", "Polygon", AssetKind::Crypto, "polygon-ecosystem-token", "pol-polygon-ecosystem-token"},
    {"QNT", "Quant", AssetKind::Crypto, "quant-network", "qnt-quant"},
    {"RENDER", "Render", AssetKind::Crypto, "render-token", "rndr-render-token"},
    {"RUNE", "THORChain", AssetKind::Crypto, "thorchain", "rune-thorchain"},
    {"SAND", "The Sandbox", AssetKind::Crypto, "the-sandbox", "sand-the-sandbox"},
    {"SEI", "Sei", AssetKind::Crypto, "sei-network", "sei-sei"},
    {"SHIB", "Shiba Inu", AssetKind::Crypto, "shiba-inu", "shib-shiba-inu"},
    {"SNX", "Synthetix", AssetKind::Crypto, "havven", "snx-synthetix-network-token"},
    {"SOL", "Solana", AssetKind::Crypto, "solana", "sol-solana"},
    {"STRK", "Starknet", AssetKind::Crypto, "starknet", "strk-starknet"},
    {"SUI", "Sui", AssetKind::Crypto, "sui", "sui-sui"},
    {"SUSHI", "Sushi", AssetKind::Crypto, "sushi", "sushi-sushi"},
    {"TAO", "Bittensor", AssetKind::Crypto, "bittensor", "tao-bittensor"},
    {"TIA", "Celestia", AssetKind::Crypto, "celestia", "tia-celestia"},
    {"TRX", "TRON", AssetKind::Crypto, "tron", "trx-tron"},
    {"UNI", "Uniswap", AssetKind::Crypto, "uniswap", "uni-uniswap"},
    {"USDC", "USD Coin", AssetKind::Crypto, "usd-coin", "usdc-usd-coin"},
    {"USDT", "Tether", AssetKind::Crypto, "tether", "usdt-tether"},
    {"VET", "VeChain", AssetKind::Crypto, "vechain", "vet-vechain"},
    {"WBTC", "Wrapped Bitcoin", AssetKind::Crypto, "wrapped-bitcoin", "wbtc-wrapped-bitcoin"},
    {"WIF", "dogwifhat", AssetKind::Crypto, "dogwifcoin", "wif-dogwifcoin"},
    {"WLD", "Worldcoin", AssetKind::Crypto, "worldcoin-wld", "wld-worldcoin"},
    {"XLM", "Stellar", AssetKind::Crypto, "stellar", "xlm-stellar"},
    {"XMR", "Monero", AssetKind::Crypto, "monero", "xmr-monero"},
    {"XRP", "XRP", AssetKind::Crypto, "ripple", "xrp-xrp"},
    {"XTZ", "Tezos", AssetKind::Crypto, "tezos", "xtz-tezos"},
    {"YFI", "yearn.finance", AssetKind::Crypto, "yearn-finance", "yfi-yearnfinance"},
    {"ZEC", "Zcash", AssetKind::Crypto, "zcash", "zec-zcash"},
    {"ZRX", "0x", AssetKind::Crypto, "0x", "zrx-0x"},
    {"AED", "UAE Dirham", AssetKind::Fiat, "", ""},
    {"ARS", "Argentine Peso", AssetKind::Fiat, "", "ars-argentine-peso"},
    {"AUD", "Australian Dollar", AssetKind::Fiat, "", "aud-australian-dollar"},
    {"BRL", "Brazilian Real", AssetKind::Fiat, "", "brl-brazil-real"},
    {"CAD", "Canadian Dollar", AssetKind::Fiat, "", "cad-canadian-dollar"},
    {"CHF", "Swiss Franc", AssetKind::Fiat, "", "chf-swiss-franc"},
    {"CLP", "Chilean Peso", AssetKind::Fiat, "", "clp-chilean-peso"},
    {"CNY", "Chinese Yuan", AssetKind::Fiat, "", "cny-yuan-renminbi"},
    {"CZK", "Czech Koruna", AssetKind::Fiat, "", "czk-czech-koruna"},
    {"DKK", "Danish Krone", AssetKind::Fiat, "", "dkk-danish-krone"},
    {"EUR", "Euro", AssetKind::Fiat, "", "eur-euro"},
    {"GBP", "British Pound", AssetKind::Fiat, "", "gbp-pound-sterling"},
    {"HKD", "Hong Kong Dollar", AssetKind::Fiat, "", "hkd-hong-kong-dollar"},
    {"HUF", "Hungarian Forint", AssetKind::Fiat, "", "huf-hungarian-forint"},
    {"IDR", "Indonesian Rupiah", AssetKind::Fiat, "", "idr-indonesian-rupiah"},
    {"ILS", "Israeli Shekel", AssetKind::Fiat, "", "ils-israeli-shekel"},
    {"INR", "Indian Rupee", AssetKind::Fiat, "", "inr-indian-rupee"},
    {"JPY", "Japanese Yen", AssetKind::Fiat, "", "jpy-japanese-yen"},
    {"KRW", "South Korean Won", AssetKind::Fiat, "", "krw-south-korea-won"},
    {"MXN", "Mexican Peso", AssetKind::Fiat, "", "mxn-mexican-peso"},
    {"MYR", "Malaysian Ringgit", AssetKind::Fiat, "", "myr-malaysian-ringgit"},
    {"NGN", "Nigerian Naira", AssetKind::Fiat, "", "ngn-nigerian-naira"},
    {"NOK", "Norwegian Krone", AssetKind::Fiat, "", "nok-norwegian-krone"},
    {"NZD", "New Zealand Dollar", AssetKind::Fiat, "", "nzd-new-zealand-dollar"},
    {"PHP", "Philippine Peso", AssetKind::Fiat, "", ""},
    {"PKR", "Pakistani Rupee", AssetKind::Fiat, "", "pkr-pakistani-rupee"},
    {"PLN", "Polish Zloty", AssetKind::Fiat, "", "pln-polish-zloty"},
    {"SAR", "Saudi Riyal", AssetKind::Fiat, "", ""},
    {"SEK", "Swedish Krona", AssetKind::Fiat, "", "sek-swedish-krona"},
    {"SGD", "Singapore Dollar", AssetKind::Fiat, "", "sgd-singapore-dollar"},
    {"THB", "Thai Baht", AssetKind::Fiat, "", "thb-thai-baht"},
    {"TRY", "Turkish Lira", AssetKind::Fiat, "", "try-turkish-lira"},
    {"TWD", "New Taiwan Dollar", AssetKind::Fiat, "", "twd-taiwan-new-dollar"},
    {"UAH", "Ukrainian Hryvnia", AssetKind::Fiat, "", "uah-ukrainian-hryvnia"},
    {"USD", "US Dollar", AssetKind::Fiat, "", "usd-us-dollars"},
    {"VND", "Vietnamese Dong", AssetKind::Fiat, "", "vnd-vietnamese-dong"},
    {"ZAR", "South African Rand", AssetKind::Fiat, "", "zar-south-african-rand"},
};

const std::vector<Currency>& table() {
    static const std::vector<Currency> currencies = [] {
        std::vector<Currency> out;
        out.reserve(sizeof(kRows) / sizeof(kRows[0]));
        for (const Row& row : kRows) {
            Currency currency;
            currency.code = row.code;
            currency.name = row.name;
            currency.kind = row.kind;
            currency.coingecko = row.coingecko;
            currency.paprika = row.paprika;
            out.push_back(std::move(currency));
        }
        return out;
    }();
    return currencies;
}

}  // namespace

const std::vector<Currency>& all_currencies() { return table(); }

const Currency* find_currency(std::string_view code) {
    const std::string key = ascii_upper(code);
    for (const Currency& currency : table()) {
        if (currency.code == key) {
            return &currency;
        }
    }
    return nullptr;
}

}  // namespace coinconvert
