#include "coinpaprika.hpp"

#include <cmath>

#include "http.hpp"
#include "json.hpp"

namespace coinconvert {

double CoinPaprikaProvider::rate(const Currency& from, const Currency& to, HttpClient& http) const {
    if (from.paprika.empty() || to.paprika.empty()) {
        throw no_rate(id(), from, to);
    }
    const std::string url =
        "https://api.coinpaprika.com/v1/price-converter?base_currency_id=" + from.paprika +
        "&quote_currency_id=" + to.paprika + "&amount=1";
    const Json root = parse_json(id(), fetch(http, id(), url));
    const Json* price = root.find("price");
    if (price == nullptr || !price->is_number()) {
        throw no_rate(id(), from, to);
    }
    const double value = price->number();
    if (!std::isfinite(value) || !(value > 0.0)) {
        throw no_rate(id(), from, to);
    }
    return value;
}

}  // namespace coinconvert
