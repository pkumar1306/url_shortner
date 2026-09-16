#include "url_store.h"

#include <random>
#include <regex>

namespace {
const std::string characters =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";
}

std::string UrlStore::create(const std::string &url)
{
    std::lock_guard lock(mutex_);
    std::string code;
    do {
        code = generateCode();
    } while (urls_.contains(code));
    urls_[code] = url;
    return code;
}

std::optional<std::string> UrlStore::find(const std::string &code) const
{
    std::lock_guard lock(mutex_);
    const auto iterator = urls_.find(code);
    if (iterator == urls_.end()) {
        return std::nullopt;
    }
    return iterator->second;
}

bool UrlStore::isPlausibleHttpUrl(const std::string &url)
{
    static const std::regex pattern(
        R"(^(https?://)([^\s/$.?#].[^\s]*)$)", std::regex::icase);
    return std::regex_match(url, pattern);
}

std::string UrlStore::generateCode()
{
    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<std::size_t> distribution(0, characters.size() - 1);

    std::string code;
    code.reserve(6);
    for (int index = 0; index < 6; ++index) {
        code += characters[distribution(generator)];
    }
    return code;
}