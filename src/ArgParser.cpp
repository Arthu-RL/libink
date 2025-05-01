#include "../include/ink/ArgParser.h"

#include "../include/ink/Inkogger.h"
#include "../include/ink/InkException.h"

namespace ink {

ArgParser::ArgParser(const std::string& description) :
    _description(description),
    _added_args()
{
    // Empty
}

std::string ArgParser::argsToString(int argc, char** argv)
{
    std::string all_args;
    for (int i = 1; i < argc; i++) {
        all_args += std::string(argv[i]) + " ";
    }
    return all_args;
}

void ArgParser::add_argument(const std::string& short_id,
                             const std::string& long_id,
                             const std::string& desc,
                             const std::string& help,
                             const std::string& default_value,
                             const bool required)
{
    INK_THROW_IF(long_id.find(desc) == std::string::npos, "Invalid argument added!");
    INK_THROW_IF(_added_args.find(desc) != _added_args.end(), "Cannot add same argument "+desc+".");
    _added_args[desc] = {short_id, long_id, desc, help, default_value, required ? "1" : "0"};
}

void ArgParser::add_argument(const std::string& long_id,
                             const std::string& desc,
                             const std::string& help,
                             const std::string& default_value,
                             const bool required)
{
    INK_THROW_IF(long_id.find(desc) == std::string::npos, "Invalid argument added!");
    INK_THROW_IF(_added_args.find(desc) != _added_args.end(), "Cannot add same argument "+desc+".");
    _added_args[desc] = {"", long_id, desc, help, default_value, required ? "1" : "0"};
}

std::string ArgParser::extract_value(const std::string& args, size_t pos)
{
    while (pos < args.length() && args[pos] == ' ')
        pos++;

    if (pos >= args.length())
        return "";

    if (args[pos] == '=')
    {
        pos++;
        while (pos < args.length() && args[pos] == ' ')
            pos++;
    }

    if (pos >= args.length())
        return "";

    if (args[pos] == '"' || args[pos] == '\'')
    {
        char quote = args[pos];
        size_t start = pos + 1;
        size_t end = args.find(quote, start);

        if (end == std::string::npos)
            return args.substr(start);

        return args.substr(start, end - start);
    }

    size_t start = pos;
    size_t end = args.find_first_of(" \t\n", start);

    if (end == std::string::npos)
        return args.substr(start);

    return args.substr(start, end - start);
}

ink::EnhancedJson ArgParser::parse_args(const std::string& args)
{
    ink::EnhancedJson parsed_args;
    std::vector<std::string> lacking;

    // process all arguments and collect missing required ones
    for (auto it = _added_args.begin(); it != _added_args.end(); ++it)
    {
        const auto& arg = it->second;
        int col = 0;
        std::string short_id = arg[col++];
        std::string long_id = arg[col++];
        std::string desc = arg[col++];
        std::string help = arg[col++];
        std::string default_value = arg[col++];
        bool required = static_cast<bool>(std::stoi(arg[col++]));

        size_t long_id_find = args.find(long_id);
        size_t short_id_find = short_id.empty() ? std::string::npos : args.find(short_id);

        std::string arg_value = "";

        if (long_id_find != std::string::npos)
        {
            size_t pos = long_id_find + long_id.length();
            arg_value = extract_value(args, pos);
        }
        else if (short_id_find != std::string::npos)
        {
            size_t pos = short_id_find + short_id.length();
            arg_value = extract_value(args, pos);
        }

        if (!arg_value.empty())
            parsed_args[desc] = ink::EnhancedJson(arg_value);
        else if (!required && !default_value.empty())
            parsed_args[desc] = ink::EnhancedJson(default_value);
        else if (required)
            lacking.push_back(long_id);
    }

    // handle missing required args
    if (!lacking.empty())
    {
        std::string error_msg = "Missing required arguments: ";
        for (size_t i = 0; i < lacking.size(); ++i) {
            if (i > 0) {
                error_msg += ", ";
            }
            error_msg += lacking[i];
        }

        show_help();
        throw INKException(error_msg);
    }

    return parsed_args;
}

void ArgParser::show_help()
{
    INK_LOG << _description;
    INK_LOG << "Available arguments:";

    for (auto it = _added_args.begin(); it != _added_args.end(); ++it)
    {
        const auto& arg = it->second;
        int col = 0;
        std::string short_id = arg[col++];
        std::string long_id = arg[col++];
        std::string desc = arg[col++];
        std::string help = arg[col++];
        std::string default_value = arg[col++];
        bool required = static_cast<bool>(std::stoi(arg[col++]));

        std::string arg_line = "  " + short_id + ", " + long_id;
        std::string status = required ? "Required" : "Optional";

        if (!default_value.empty() && !required)
            status += " (Default: " + default_value + ")";

        INK_LOG << arg_line;
        INK_LOG << "    " << status << " " << help;
    }
}

}
