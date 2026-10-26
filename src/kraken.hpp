#pragma once

#include "provider.hpp"

namespace coinconvert {

class KrakenProvider : public PriceProvider {
public:
    const char* id() const override { return "kraken"; }
    const char* summary() const override { return "Kraken public last trade"; }
    double rate(const Currency& from, const Currency& to, HttpClient& http) const override;
};

}
