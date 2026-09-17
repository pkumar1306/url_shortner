#include "services/url_service.h"

#include "utils/url_utils.h"

namespace {
drogon::orm::DbClientPtr database()
{
    return drogon::app().getDbClient("default");
}

drogon::HttpResponsePtr jsonError(
    drogon::HttpStatusCode status,
    const std::string &message)
{
    Json::Value body;
    body["error"] = message;
    auto response = drogon::HttpResponse::newHttpJsonResponse(body);
    response->setStatusCode(status);
    return response;
}

bool isUniqueViolation(const drogon::orm::DrogonDbException &error)
{
    const auto *sqlError =
        dynamic_cast<const drogon::orm::SqlError *>(&error.base());
    return sqlError != nullptr && sqlError->sqlState() == "23505";
}

// Fallback no-op logger used when no logger is injected.
std::shared_ptr<Logger> nullLogger()
{
    static auto instance = std::make_shared<Logger>(LogLevel::ERROR);
    return instance;
}
}

// ---------------------------------------------------------------------------
// Constructors
// ---------------------------------------------------------------------------
UrlService::UrlService(std::string baseUrl, std::shared_ptr<Logger> logger)
    : baseUrl_(std::move(baseUrl)),
      logger_(std::move(logger))
{
}

UrlService::UrlService(std::string baseUrl)
    : baseUrl_(std::move(baseUrl)),
      logger_(nullLogger())
{
}

// ---------------------------------------------------------------------------
// createUrl – generate a short code and persist the mapping.
// ---------------------------------------------------------------------------
void UrlService::createUrl(
    const std::string &longUrl,
    std::int64_t userId,
    std::function<void(const drogon::HttpResponsePtr &)> callback)
{
    const auto code = generateShortCode();
    logger_->debug("Creating short URL: code=" + code +
                   " user_id=" + std::to_string(userId));
    insertUrl(longUrl, userId, code, std::move(callback));
}

// ---------------------------------------------------------------------------
// insertUrl – attempt to INSERT; retry on unique-violation (code collision).
// ---------------------------------------------------------------------------
void UrlService::insertUrl(
    const std::string &longUrl,
    std::int64_t userId,
    std::string code,
    std::function<void(const drogon::HttpResponsePtr &)> callback)
{
    database()->execSqlAsync(
        "INSERT INTO urls (code, original_url, user_id) VALUES ($1, $2, $3)",
        [this, code, callback](const drogon::orm::Result &) {
            logger_->info("Short URL created: code=" + code +
                          " short_url=" + baseUrl_ + "/" + code);

            Json::Value body;
            body["code"] = code;
            body["short_url"] = baseUrl_ + "/" + code;

            auto response = drogon::HttpResponse::newHttpJsonResponse(body);
            response->setStatusCode(drogon::k201Created);
            callback(response);
        },
        [this, longUrl, userId, callback](const drogon::orm::DrogonDbException &error) {
            if (isUniqueViolation(error)) {
                logger_->warn("Short code collision detected, retrying with new code");
                insertUrl(longUrl, userId, generateShortCode(), callback);
                return;
            }

            logger_->error(std::string("URL insert DB error: ") +
                           error.base().what());
            callback(jsonError(drogon::k500InternalServerError, "Database error"));
        },
        code,
        longUrl,
        userId);
}

// ---------------------------------------------------------------------------
// redirect – look up the original URL and record a click event.
// ---------------------------------------------------------------------------
void UrlService::redirect(
    const std::string &code,
    const ClickMetadata &metadata,
    std::function<void(const drogon::HttpResponsePtr &)> callback)
{
    logger_->debug("Redirect lookup: code=" + code +
                   " ip=" + metadata.ipAddress);

    auto dbClient = database();
    dbClient->execSqlAsync(
        "SELECT original_url FROM urls WHERE code = $1",
        [this, dbClient, code, metadata, callback](const drogon::orm::Result &result) {
            if (result.empty()) {
                logger_->warn("Redirect failed: code=" + code + " not found");
                callback(jsonError(drogon::k404NotFound, "Short URL not found"));
                return;
            }

            const std::string longUrl = result[0]["original_url"].as<std::string>();
            logger_->trace("Code " + code + " resolves to " + longUrl);

            dbClient->execSqlAsync(
                "INSERT INTO clicks (url_code, ip_address, user_agent, referrer) "
                "VALUES ($1, $2, $3, $4)",
                [this, dbClient, code, longUrl, callback](const drogon::orm::Result &) {
                    dbClient->execSqlAsync(
                        "UPDATE urls SET click_count = click_count + 1 WHERE code = $1",
                        [this, code, longUrl, callback](const drogon::orm::Result &) {
                            logger_->info("Redirect served: code=" + code);
                            callback(drogon::HttpResponse::newRedirectionResponse(longUrl));
                        },
                        [this, callback](const drogon::orm::DrogonDbException &error) {
                            logger_->error(std::string("Click counter update error: ") +
                                           error.base().what());
                            callback(jsonError(drogon::k500InternalServerError, "Database error"));
                        },
                        code);
                },
                [this, callback](const drogon::orm::DrogonDbException &error) {
                    logger_->error(std::string("Click event insert error: ") +
                                   error.base().what());
                    callback(jsonError(drogon::k500InternalServerError, "Database error"));
                },
                code,
                metadata.ipAddress,
                metadata.userAgent,
                metadata.referrer);
        },
        [this, callback](const drogon::orm::DrogonDbException &error) {
            logger_->error(std::string("URL lookup DB error: ") +
                           error.base().what());
            callback(jsonError(drogon::k500InternalServerError, "Database error"));
        },
        code);
}

// ---------------------------------------------------------------------------
// getStats – return click statistics for a URL owned by the given user.
// ---------------------------------------------------------------------------
void UrlService::getStats(
    const std::string &code,
    std::int64_t userId,
    std::function<void(const drogon::HttpResponsePtr &)> callback)
{
    logger_->debug("Stats request: code=" + code +
                   " user_id=" + std::to_string(userId));

    auto dbClient = database();
    dbClient->execSqlAsync(
        "SELECT code, original_url, click_count FROM urls "
        "WHERE code = $1 AND user_id = $2",
        [this, dbClient, code, callback](const drogon::orm::Result &result) {
            if (result.empty()) {
                logger_->warn("Stats not found: code=" + code +
                              " (wrong owner or missing URL)");
                callback(jsonError(drogon::k404NotFound, "Short URL not found"));
                return;
            }

            Json::Value body;
            body["code"] = result[0]["code"].as<std::string>();
            body["original_url"] = result[0]["original_url"].as<std::string>();
            body["total_clicks"] = result[0]["click_count"].as<int64_t>();
            body["clicks_by_day"] = Json::Value(Json::arrayValue);

            dbClient->execSqlAsync(
                "SELECT to_char(clicked_at, 'YYYY-MM-DD') AS day, "
                "COUNT(*) AS clicks FROM clicks WHERE url_code = $1 "
                "GROUP BY day ORDER BY day",
                [this, code, body, callback](const drogon::orm::Result &dailyResult) mutable {
                    for (const auto &row : dailyResult) {
                        Json::Value day;
                        day["day"] = row["day"].as<std::string>();
                        day["clicks"] = row["clicks"].as<int64_t>();
                        body["clicks_by_day"].append(day);
                    }
                    logger_->info("Stats served: code=" + code +
                                  " total_clicks=" +
                                  std::to_string(body["total_clicks"].asInt64()));
                    callback(drogon::HttpResponse::newHttpJsonResponse(body));
                },
                [this, callback](const drogon::orm::DrogonDbException &error) {
                    logger_->error(std::string("Daily analytics query error: ") +
                                   error.base().what());
                    callback(jsonError(drogon::k500InternalServerError, "Database error"));
                },
                code);
        },
        [this, callback](const drogon::orm::DrogonDbException &error) {
            logger_->error(std::string("Stats query DB error: ") +
                           error.base().what());
            callback(jsonError(drogon::k500InternalServerError, "Database error"));
        },
        code,
        userId);
}
