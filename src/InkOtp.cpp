#include "../include/ink/InkOtp.h"

#include <random>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

namespace ink {

namespace crypt {

std::string OTP::build_key(const std::size_t& text_length,
                              const std::size_t& seed_for_key_gen,
                              const std::size_t& limit_randint_gen)
{
    std::random_device rd;
    std::seed_seq seed{rd(), rd(), rd(), rd(),
                        static_cast<unsigned>(seed_for_key_gen)};
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> dist(1, static_cast<int>(limit_randint_gen));

    std::string key(text_length, '\0');

    for (std::size_t i = 0; i < text_length; ++i)
    {
        key[i] = static_cast<char>(dist(gen)*seed_for_key_gen%256);
    }

    return key;
}

std::string OTP::encrypt(const std::string& text, const std::string& key)
{
    if (key.size() < text.size()) {
        throw std::invalid_argument("OTP::encrypt: key must be at least as long as text");
    }

    std::string encrypted_text = "";
    encrypted_text.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i)
    {
        encrypted_text += char(text[i] ^ key[i]);
    }
    return encrypted_text;
}

std::string OTP::decrypt(const std::string& encrypted_text, const std::string& key)
{
    if (key.size() < encrypted_text.size()) {
        throw std::invalid_argument("OTP::decrypt: key must be at least as long as encrypted_text");
    }

    std::string decrypted_text = "";
    decrypted_text.reserve(encrypted_text.size());
    for (std::size_t i = 0; i < encrypted_text.size(); ++i)
    {
        decrypted_text += char(encrypted_text[i] ^ key[i]);
    }
    return decrypted_text;
}

std::string OTP::read_from_file(const std::string& filename)
{
    // Binary mode: this reads OTP-encrypted content, which is arbitrary
    // bytes, not text -- text mode would let CRLF translation (on
    // platforms that do it) corrupt it.
    std::ifstream infile(filename, std::ios::in | std::ios::binary);
    if (!infile.is_open())
        return "";

    std::ostringstream oss;
    oss << infile.rdbuf();
    infile.close();

    return oss.str();
}

bool OTP::write_to_file(const std::string& filename, const std::string& content)
{
    std::ofstream outfile(filename, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!outfile.is_open())
        return false;

    outfile << content;
    outfile.close();

    return true;
}


}

}
