#include "services/auth_service.h"

#include <openssl/rand.h>
#include <openssl/sha.h>

#include <iomanip>
#include <sstream>
#include <string_view>

namespace {
drogon::orm::DbClientPtr database()
{
    return drogon::app().getDbClient("default");
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

std::optional<std::string> generateKey()
{
    unsigned char bytes[32];
    if (RAND_bytes(bytes, sizeof(bytes)) != 1) {
        return std::nullopt;
    }

    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (const auto byte : bytes) {
        result << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return result.str();
}
}

void AuthService::issueKey(IssueCallback callback)
{
    const auto key = generateKey();
    if (!key) {
        callback({}, 0);
        return;
    }

    const auto keyValue = *key;
    database()->execSqlAsync(
        "INSERT INTO users (api_key_hash) VALUES ($1) RETURNING id",
        [keyValue, callback](const drogon::orm::Result &result) {
            callback(keyValue, result[0]["id"].as<std::int64_t>());
        },
        [callback](const drogon::orm::DrogonDbException &) {
            callback({}, 0);
        },
        hashKey(keyValue));
}

void AuthService::authenticate(
    const std::string &authorization,
    AuthenticateCallback callback)
{
    const auto token = bearerToken(authorization);
    if (!token) {
        callback(std::nullopt);
        return;
    }

    database()->execSqlAsync(
        "SELECT id FROM users WHERE api_key_hash = $1",
        [callback](const drogon::orm::Result &result) {
            if (result.empty()) {
                callback(std::nullopt);
                return;
            }
            callback(result[0]["id"].as<std::int64_t>());
        },
        [callback](const drogon::orm::DrogonDbException &) {
            callback(std::nullopt);
        },
        hashKey(*token));
}