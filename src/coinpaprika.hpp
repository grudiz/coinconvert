#pragma once

#include "provider.hpp"

namespace coinconvert {

class CoinPaprikaProvider : public PriceProvider {
public:
    const char* id() const override { return "coinpaprika"; }
    const char* summary() const override { return "CoinPaprika price converter"; }
    double rate(const Currency& from, const Currency& to, HttpClient& http) const override;
};

}
