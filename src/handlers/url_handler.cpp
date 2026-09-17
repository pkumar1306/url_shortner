#include "handlers/url_handler.h"

#include "utils/request_id.h"
#include "utils/url_utils.h"

#include <drogon/drogon.h>
#ifdef ERROR
#undef ERROR
#endif

#include <string_view>

namespace {
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

std::string statsCode(const std::string &path)
{
    constexpr std::string_view prefix = "/api/v1/urls/";
    constexpr std::string_view suffix = "/stats";
    if (path.size() <= prefix.size() + suffix.size() ||
        !path.starts_with(prefix) || !path.ends_with(suffix)) {
        return {};
    }
    return path.substr(prefix.size(), path.size() - prefix.size() - suffix.size());
}
}

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
UrlHandler::UrlHandler(
    std::shared_ptr<UrlService> service,
    std::shared_ptr<AuthService> authService,
    std::shared_ptr<Logger> logger)
    : service_(std::move(service)),
    authService_(std::move(authService)),
    logger_(std::move(logger)),
    rateLimiter_(10.0, 10.0 / 60.0)
{
}

// ---------------------------------------------------------------------------
// Route registration – each handler generates a request-ID, logs an access
// line, and includes the ID in the response headers.
// ---------------------------------------------------------------------------
void UrlHandler::registerRoutes()
{
    auto service = service_;
    auto authService = authService_;
    auto logger = logger_;

    // -- Health / root endpoint -------------------------------------------
    drogon::app().registerHandler(
        "/",
        [logger](const drogon::HttpRequestPtr &request,
           std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            const auto reqId = RequestId::generate();
            Logger::setRequestId(reqId);
            logger->debug("GET / (health check)");

            Json::Value body;
            body["message"] = "URL Shortener API";
            auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
            resp->addHeader("X-Request-Id", reqId);
            callback(resp);
        });

    // -- Issue API key ----------------------------------------------------
    drogon::app().registerHandler(
        "/api/v1/keys",
        [logger, authService](const drogon::HttpRequestPtr &request,
                      std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            const auto reqId = RequestId::generate();
            Logger::setRequestId(reqId);
            logger->info("POST /api/v1/keys – API key issuance request");

            authService->issueKey(
                [logger, reqId, callback](const std::string &key, std::int64_t userId) {
                    Logger::setRequestId(reqId);
                    if (key.empty()) {
                        logger->error("API key issuance failed");
                        auto resp = jsonError(
                            drogon::k500InternalServerError,
                            "Unable to issue API key");
                        resp->addHeader("X-Request-Id", reqId);
                        callback(resp);
                        return;
                    }

                    logger->info("API key issued for user_id=" +
                                 std::to_string(userId));
                    Json::Value body;
                    body["api_key"] = key;
                    auto response = drogon::HttpResponse::newHttpJsonResponse(body);
                    response->setStatusCode(drogon::k201Created);
                    response->addHeader("X-Request-Id", reqId);
                    callback(response);
                });
        },
        {drogon::Post});

    // -- Shorten URL ------------------------------------------------------
    drogon::app().registerHandler(
        "/api/v1/urls",
        [this, service, authService, logger](const drogon::HttpRequestPtr &request,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            const auto reqId = RequestId::generate();
            Logger::setRequestId(reqId);

            auto json = request->getJsonObject();
            std::string ip = request->getPeerAddr().toIp();

            logger->info("POST /api/v1/urls from ip=" + ip);

            if (!json || !json->isMember("url")) {
                logger->warn("Bad request: missing 'url' field");
                auto resp = jsonError(drogon::k400BadRequest, "Missing 'url'");
                resp->addHeader("X-Request-Id", reqId);
                callback(resp);
                return;
            }

            const std::string longUrl = (*json)["url"].asString();
            if (!isValidUrl(longUrl)) {
                logger->warn("Bad request: invalid URL format");
                auto resp = jsonError(drogon::k400BadRequest, "Invalid URL");
                resp->addHeader("X-Request-Id", reqId);
                callback(resp);
                return;
            }

            logger->debug("URL validation passed, authenticating…");

            authService->authenticate(
                request->getHeader("Authorization"),
                [this, service, logger, longUrl, ip, reqId, callback](std::optional<std::int64_t> userId) mutable {
                    Logger::setRequestId(reqId);
                    if (!userId) {
                        logger->warn("Authentication failed for POST /api/v1/urls from ip=" + ip);
                        auto resp = jsonError(
                            drogon::k401Unauthorized,
                            "Valid Bearer token required");
                        resp->addHeader("X-Request-Id", reqId);
                        callback(resp);
                        return;
                    }

                    logger->debug("Authenticated user_id=" +
                                  std::to_string(*userId) +
                                  ", checking rate limit");

                    if (!rateLimiter_.allow(ip)) {
                        logger->warn("Rate limit exceeded for ip=" + ip +
                                     " user_id=" + std::to_string(*userId));
                        auto resp = jsonError(
                            drogon::k429TooManyRequests,
                            "Too Many Requests");
                        resp->addHeader("X-Request-Id", reqId);
                        callback(resp);
                        return;
                    }

                    service->createUrl(longUrl, *userId, [reqId, callback](const drogon::HttpResponsePtr &resp) {
                        resp->addHeader("X-Request-Id", reqId);
                        callback(resp);
                    });
                });
        },
        {drogon::Post});

    // -- URL statistics ---------------------------------------------------
    drogon::app().registerHandler(
        "/api/v1/urls/{code}/stats",
        [service, authService, logger](const drogon::HttpRequestPtr &request,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            const auto reqId = RequestId::generate();
            Logger::setRequestId(reqId);

            const std::string code = statsCode(request->getPath());
            if (code.empty()) {
                logger->warn("Bad request: invalid URL code in stats path");
                auto resp = jsonError(drogon::k400BadRequest, "Invalid URL code");
                resp->addHeader("X-Request-Id", reqId);
                callback(resp);
                return;
            }

            logger->info("GET /api/v1/urls/" + code + "/stats");

            authService->authenticate(
                request->getHeader("Authorization"),
                [service, logger, code, reqId, callback](std::optional<std::int64_t> userId) mutable {
                    Logger::setRequestId(reqId);
                    if (!userId) {
                        logger->warn("Authentication failed for stats request code=" + code);
                        auto resp = jsonError(
                            drogon::k401Unauthorized,
                            "Valid Bearer token required");
                        resp->addHeader("X-Request-Id", reqId);
                        callback(resp);
                        return;
                    }
                    service->getStats(code, *userId, [reqId, callback](const drogon::HttpResponsePtr &resp) {
                        resp->addHeader("X-Request-Id", reqId);
                        callback(resp);
                    });
                });
        },
        {drogon::Get});

    // -- Public redirect --------------------------------------------------
    drogon::app().registerHandler(
        "/{code}",
        [service, logger](const drogon::HttpRequestPtr &request,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            const auto reqId = RequestId::generate();
            Logger::setRequestId(reqId);

            const std::string code = request->getPath().substr(1);
            const std::string ip = request->getPeerAddr().toIp();

            logger->info("GET /" + code + " (redirect) from ip=" + ip +
                         " ua=" + request->getHeader("User-Agent"));

            ClickMetadata metadata{
                ip,
                request->getHeader("User-Agent"),
                request->getHeader("Referer")};

            service->redirect(code, metadata, [reqId, callback](const drogon::HttpResponsePtr &resp) {
                resp->addHeader("X-Request-Id", reqId);
                callback(resp);
            });
        },
        {drogon::Get});

    logger_->info("All routes registered successfully");
}
