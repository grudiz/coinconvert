#include "provider.hpp"

#include "bitfinex.hpp"
#include "coingecko.hpp"
#include "coinpaprika.hpp"
#include "kraken.hpp"
#include "text.hpp"

namespace coinconvert {

const std::vector<const PriceProvider*>& providers() {
    static const CoinGeckoProvider coingecko;
    static const CoinPaprikaProvider coinpaprika;
    static const KrakenProvider kraken;
    static const BitfinexProvider bitfinex;
    static const std::vector<const PriceProvider*> all{
        &coingecko,
        &coinpaprika,
        &kraken,
        &bitfinex,
    };
    return all;
}

const PriceProvider* find_provider(std::string_view id) {
    for (const PriceProvider* provider : providers()) {
        if (ascii_iequals(provider->id(), id)) {
            return provider;
        }
    }
    return nullptr;
}

const PriceProvider& default_provider() { return *providers().front(); }

}  // namespace coinconvert
