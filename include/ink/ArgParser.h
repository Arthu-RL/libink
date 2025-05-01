#ifndef ARGPARSER_H
#define ARGPARSER_H

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "ink/ink_base.hpp"
#include "EnhancedJson.h"

namespace ink {

class INK_API ArgParser
{
public:
    ArgParser(const std::string& description);
    ArgParser(const ArgParser&) = delete;
    ArgParser(ArgParser&&) = delete;
    ~ArgParser() = default;
    ArgParser& operator=(const ArgParser&) = delete;
    ArgParser& operator=(ArgParser&&) = delete;

    static std::string argsToString(int argc, char** argv);

    void add_argument(const std::string& short_id,
                      const std::string& long_id,
                      const std::string& desc,
                      const std::string& help,
                      const std::string& default_value,
                      const bool required);

    void add_argument(const std::string& long_id,
                      const std::string& desc,
                      const std::string& help,
                      const std::string& default_value,
                      const bool required);

    ink::EnhancedJson parse_args(const std::string& args);

    void show_help();

private:
    std::string _description;
    std::unordered_map<std::string, std::vector<std::string>> _added_args;

    std::string extract_value(const std::string& args, size_t pos);
};

}

#endif // ARGPARSER_H
