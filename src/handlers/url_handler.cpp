#include "handlers/url_handler.h"

#include "utils/url_utils.h"

#include <drogon/drogon.h>

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

UrlHandler::UrlHandler(std::shared_ptr<UrlService> service)
    : service_(std::move(service)),
    rateLimiter_(10.0, 10.0 / 60.0)
{
}


void UrlHandler::registerRoutes()
{
    auto service = service_;

    

    drogon::app().registerHandler(
        "/",
        [](const drogon::HttpRequestPtr &,
           std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            Json::Value body;
            body["message"] = "URL Shortener API";
            callback(drogon::HttpResponse::newHttpJsonResponse(body));
        });

    drogon::app().registerHandler(
        "/api/v1/urls",
        [this,service](const drogon::HttpRequestPtr &request,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            auto json = request->getJsonObject();
            std::string ip = request->getPeerAddr().toIp();

            if (!json || !json->isMember("url")) {
                callback(jsonError(drogon::k400BadRequest, "Missing 'url'"));
                return;
            }

            const std::string longUrl = (*json)["url"].asString();
            if (!isValidUrl(longUrl)) {
                callback(jsonError(drogon::k400BadRequest, "Invalid URL"));
                return;
            }


            if (!rateLimiter_.allow(ip))
            {
                callback(jsonError(
                                drogon::k429TooManyRequests,
                                "Too Many Requests"
                            ));
                            return;
            }

            service->createUrl(longUrl, std::move(callback));
        },
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/v1/urls/{code}/stats",
        [service](const drogon::HttpRequestPtr &request,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            const std::string code = statsCode(request->getPath());
            if (code.empty()) {
                callback(jsonError(drogon::k400BadRequest, "Invalid URL code"));
                return;
            }
            service->getStats(code, std::move(callback));
        },
        {drogon::Get});

    drogon::app().registerHandler(
        "/{code}",
        [service](const drogon::HttpRequestPtr &request,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            const std::string code = request->getPath().substr(1);
            ClickMetadata metadata{
                request->getPeerAddr().toIp(),
                request->getHeader("User-Agent"),
                request->getHeader("Referer")};
            service->redirect(code, metadata, std::move(callback));
        },
        {drogon::Get});
}
