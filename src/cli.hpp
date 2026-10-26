#pragma once

#include <iosfwd>

#include "http.hpp"

namespace coinconvert {

int cli_main(int argc, char** argv);
int cli_main(int argc, char** argv, HttpClient& http, std::ostream& out, std::ostream& err);

}
