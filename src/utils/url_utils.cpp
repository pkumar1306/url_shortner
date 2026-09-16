#include "utils/url_utils.h"

#include <random>
#include <regex>

namespace {
const std::string characters =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";
}

std::string generateShortCode()
{
    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<std::size_t> distribution(0, characters.size() - 1);

    std::string code;
    code.reserve(6);
    for (int i = 0; i < 6; ++i) {
        code += characters[distribution(generator)];
    }
    return code;
}

bool isValidUrl(const std::string &url)
{
    static const std::regex pattern(
        R"(^(https?://)([^\s/$.?#].[^\s]*)$)", std::regex::icase);
    return std::regex_match(url, pattern);
}
