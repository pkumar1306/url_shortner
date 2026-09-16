#include "services/url_service.h"

#include "utils/url_utils.h"

#include <iostream>

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
}

UrlService::UrlService(std::string baseUrl)
    : baseUrl_(std::move(baseUrl))
{
}

void UrlService::createUrl(
    const std::string &longUrl,
    std::function<void(const drogon::HttpResponsePtr &)> callback)
{
    insertUrl(longUrl, generateShortCode(), std::move(callback));
}

void UrlService::insertUrl(
    const std::string &longUrl,
    std::string code,
    std::function<void(const drogon::HttpResponsePtr &)> callback)
{
    database()->execSqlAsync(
        "INSERT INTO urls (code, original_url) VALUES ($1, $2)",
        [this, code, callback](const drogon::orm::Result &) {
            Json::Value body;
            body["code"] = code;
            body["short_url"] = baseUrl_ + "/" + code;

            auto response = drogon::HttpResponse::newHttpJsonResponse(body);
            response->setStatusCode(drogon::k201Created);
            callback(response);
        },
        [this, longUrl, callback](const drogon::orm::DrogonDbException &error) {
            if (isUniqueViolation(error)) {
                insertUrl(longUrl, generateShortCode(), callback);
                return;
            }

            std::cerr << "Database error: " << error.base().what() << '\n';
            callback(jsonError(drogon::k500InternalServerError, "Database error"));
        },
        code,
        longUrl);
}

void UrlService::redirect(
    const std::string &code,
    const ClickMetadata &metadata,
    std::function<void(const drogon::HttpResponsePtr &)> callback)
{
    auto dbClient = database();
    dbClient->execSqlAsync(
        "SELECT original_url FROM urls WHERE code = $1",
        [this, dbClient, code, metadata, callback](const drogon::orm::Result &result) {
            if (result.empty()) {
                callback(jsonError(drogon::k404NotFound, "Short URL not found"));
                return;
            }

            const std::string longUrl = result[0]["original_url"].as<std::string>();
            dbClient->execSqlAsync(
                "INSERT INTO clicks (url_code, ip_address, user_agent, referrer) "
                "VALUES ($1, $2, $3, $4)",
                [dbClient, code, longUrl, callback](const drogon::orm::Result &) {
                    dbClient->execSqlAsync(
                        "UPDATE urls SET click_count = click_count + 1 WHERE code = $1",
                        [longUrl, callback](const drogon::orm::Result &) {
                            callback(drogon::HttpResponse::newRedirectionResponse(longUrl));
                        },
                        [callback](const drogon::orm::DrogonDbException &error) {
                            std::cerr << "Click counter error: " << error.base().what() << '\n';
                            callback(jsonError(drogon::k500InternalServerError, "Database error"));
                        },
                        code);
                },
                [callback](const drogon::orm::DrogonDbException &error) {
                    std::cerr << "Click event error: " << error.base().what() << '\n';
                    callback(jsonError(drogon::k500InternalServerError, "Database error"));
                },
                code,
                metadata.ipAddress,
                metadata.userAgent,
                metadata.referrer);
        },
        [callback](const drogon::orm::DrogonDbException &error) {
            std::cerr << "Database error: " << error.base().what() << '\n';
            callback(jsonError(drogon::k500InternalServerError, "Database error"));
        },
        code);
}

void UrlService::getStats(
    const std::string &code,
    std::function<void(const drogon::HttpResponsePtr &)> callback)
{
    auto dbClient = database();
    dbClient->execSqlAsync(
        "SELECT code, original_url, click_count FROM urls WHERE code = $1",
        [dbClient, code, callback](const drogon::orm::Result &result) {
            if (result.empty()) {
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
                [body, callback](const drogon::orm::Result &dailyResult) mutable {
                    for (const auto &row : dailyResult) {
                        Json::Value day;
                        day["day"] = row["day"].as<std::string>();
                        day["clicks"] = row["clicks"].as<int64_t>();
                        body["clicks_by_day"].append(day);
                    }
                    callback(drogon::HttpResponse::newHttpJsonResponse(body));
                },
                [callback](const drogon::orm::DrogonDbException &error) {
                    std::cerr << "Analytics error: " << error.base().what() << '\n';
                    callback(jsonError(drogon::k500InternalServerError, "Database error"));
                },
                code);
        },
        [callback](const drogon::orm::DrogonDbException &error) {
            std::cerr << "Analytics error: " << error.base().what() << '\n';
            callback(jsonError(drogon::k500InternalServerError, "Database error"));
        },
        code);
}
