#pragma once

#include <string>
#include <string_view>

namespace coinconvert {

bool parse_amount(std::string_view text, double& value);
std::string format_result(double value);

}
