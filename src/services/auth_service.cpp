#include "services/auth_service.h"

#include "utils/auth_utils.h"

#include <iostream>

namespace {
drogon::orm::DbClientPtr database()
{
    return drogon::app().getDbClient("default");
}

}

void AuthService::issueKey(IssueCallback callback)
{
    const auto key = AuthUtils::generateKey();
    if (key.empty()) {
        callback({}, 0);
        return;
    }

    const auto keyValue = key;
    database()->execSqlAsync(
        "INSERT INTO users (api_key_hash) VALUES ($1) RETURNING id",
        [keyValue, callback](const drogon::orm::Result &result) {
            callback(keyValue, result[0]["id"].as<std::int64_t>());
        },
        [callback](const drogon::orm::DrogonDbException &error) {
            std::cerr << "API key database error: " << error.base().what() << '\n';
            callback({}, 0);
        },
        AuthUtils::hashKey(keyValue));
}

void AuthService::authenticate(
    const std::string &authorization,
    AuthenticateCallback callback)
{
    const auto token = AuthUtils::bearerToken(authorization);
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
        AuthUtils::hashKey(*token));
}