#pragma once

#include "provider.hpp"

namespace coinconvert {

class CoinGeckoProvider : public PriceProvider {
public:
    const char* id() const override { return "coingecko"; }
    const char* summary() const override { return "CoinGecko aggregated spot prices"; }
    double rate(const Currency& from, const Currency& to, HttpClient& http) const override;
};

}
