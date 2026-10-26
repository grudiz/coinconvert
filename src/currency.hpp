#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace coinconvert {

enum class AssetKind { Fiat, Crypto };

struct Currency {
    std::string code;
    std::string name;
    AssetKind kind = AssetKind::Fiat;
    std::string coingecko;
    std::string paprika;
};

const std::vector<Currency>& all_currencies();
const Currency* find_currency(std::string_view code);

}
