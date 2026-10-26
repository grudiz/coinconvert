#pragma once

#include "provider.hpp"

namespace coinconvert {

class BitfinexProvider : public PriceProvider {
public:
    const char* id() const override { return "bitfinex"; }
    const char* summary() const override { return "Bitfinex public last trade"; }
    double rate(const Currency& from, const Currency& to, HttpClient& http) const override;
};

}
