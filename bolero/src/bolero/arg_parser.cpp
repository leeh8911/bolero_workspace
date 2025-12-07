#include "bolero/arg_parser.hpp"

namespace bolero {
ArgParser::ArgParser(int argc, char* argv[]) : options("Bolero Options", "1.0"), argc_(argc), argv_(argv) {
}

cxxopts::ParseResult ArgParser::Parse() {
    return options.parse(argc_, argv_);
}
}  // namespace bolero