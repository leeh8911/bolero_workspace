#pragma once

#include <sstream>
#include <string>

#include <cxxopts.hpp>

namespace bolero {
class ArgParser {
   public:
    ArgParser(int argc, char* argv[]);

    template <typename T>
    void Add(const std::string& name, const std::string& desc = "", const T& default_value = T()) {
        std::ostringstream ss;
        ss << default_value;
        options.add_options()(name, desc, cxxopts::value<T>()->default_value(ss.str()));
    }

    cxxopts::ParseResult Parse();

   private:
    cxxopts::Options options;
    int argc_;
    char** argv_;
};
}  // namespace bolero