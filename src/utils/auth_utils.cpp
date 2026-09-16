#include "utils/auth_utils.h"

#include <openssl/rand.h>
#include <openssl/sha.h>

#include <iomanip>
#include <sstream>
#include <string_view>

namespace AuthUtils {
std::string generateKey()
{
    unsigned char bytes[32];
    if (RAND_bytes(bytes, sizeof(bytes)) != 1) {
        return {};
    }

    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (const auto byte : bytes) {
        result << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return result.str();
}

std::string hashKey(const std::string &key)
{
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char *>(key.data()), key.size(), digest);

    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (const auto byte : digest) {
        result << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return result.str();
}

std::optional<std::string> bearerToken(const std::string &authorization)
{
    constexpr std::string_view prefix = "Bearer ";
    if (!authorization.starts_with(prefix) || authorization.size() == prefix.size()) {
        return std::nullopt;
    }
    return authorization.substr(prefix.size());
}
}