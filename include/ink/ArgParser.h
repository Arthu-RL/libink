#ifndef ARGPARSER_H
#define ARGPARSER_H

#pragma once

#include <string>
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
    std::string extract_value(const std::string& args, size_t pos);

private:

    std::string _description;

    struct Arg {
        std::string short_id;
        std::string long_id;
        std::string help;
        std::string default_value;
        bool required;

        Arg(const std::string& _short_id, const std::string& _long_id, const std::string& _help, const std::string& _default_value, bool _required) :
            short_id(_short_id), long_id(_long_id), help(_help), default_value(_default_value), required(_required) {}

        Arg() = default;
        Arg(const Arg& arg) = default;
        Arg(Arg&& arg) noexcept = default;
        Arg& operator=(const Arg& arg) = default;
        Arg& operator=(Arg&& arg) noexcept = default;
    };

    // desc -> Arg
    std::unordered_map<std::string, Arg> _added_args;
};

}

#endif // ARGPARSER_H
