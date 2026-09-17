#include "services/auth_service.h"

#include "utils/auth_utils.h"

namespace {
drogon::orm::DbClientPtr database()
{
    return drogon::app().getDbClient("default");
}

// Fallback no-op logger used when no logger is injected (e.g. in tests).
std::shared_ptr<Logger> nullLogger()
{
    static auto instance = std::make_shared<Logger>(LogLevel::ERROR);
    return instance;
}
}

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------
AuthService::AuthService(std::shared_ptr<Logger> logger)
    : logger_(std::move(logger))
{
}

AuthService::AuthService()
    : logger_(nullLogger())
{
}

// ---------------------------------------------------------------------------
// issueKey – generate a random API key, hash it, and store in the DB.
// ---------------------------------------------------------------------------
void AuthService::issueKey(IssueCallback callback)
{
    const auto key = AuthUtils::generateKey();
    if (key.empty()) {
        logger_->error("Failed to generate API key (RAND_bytes failed)");
        callback({}, 0);
        return;
    }

    logger_->debug("Issuing new API key (hash will be stored in DB)");

    const auto keyValue = key;
    database()->execSqlAsync(
        "INSERT INTO users (api_key_hash) VALUES ($1) RETURNING id",
        [this, keyValue, callback](const drogon::orm::Result &result) {
            const auto userId = result[0]["id"].as<std::int64_t>();
            logger_->info("API key issued successfully for user_id=" +
                          std::to_string(userId));
            callback(keyValue, userId);
        },
        [this, callback](const drogon::orm::DrogonDbException &error) {
            logger_->error(std::string("API key DB insert failed: ") +
                           error.base().what());
            callback({}, 0);
        },
        AuthUtils::hashKey(keyValue));
}

// ---------------------------------------------------------------------------
// authenticate – validate the Bearer token against stored key hashes.
// ---------------------------------------------------------------------------
void AuthService::authenticate(
    const std::string &authorization,
    AuthenticateCallback callback)
{
    const auto token = AuthUtils::bearerToken(authorization);
    if (!token) {
        logger_->debug("Authentication failed: missing or malformed Bearer token");
        callback(std::nullopt);
        return;
    }

    logger_->trace("Authenticating bearer token (hash lookup)");

    database()->execSqlAsync(
        "SELECT id FROM users WHERE api_key_hash = $1",
        [this, callback](const drogon::orm::Result &result) {
            if (result.empty()) {
                logger_->warn("Authentication failed: no user found for token hash");
                callback(std::nullopt);
                return;
            }
            const auto userId = result[0]["id"].as<std::int64_t>();
            logger_->debug("Authenticated user_id=" + std::to_string(userId));
            callback(userId);
        },
        [this, callback](const drogon::orm::DrogonDbException &error) {
            logger_->error(std::string("Authentication DB query failed: ") +
                           error.base().what());
            callback(std::nullopt);
        },
        AuthUtils::hashKey(*token));
}