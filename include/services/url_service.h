#pragma once

#include <drogon/drogon.h>

#include <cstdint>
#include <functional>
#include <string>

struct ClickMetadata {
    std::string ipAddress;
    std::string userAgent;
    std::string referrer;
};

class UrlService {
  public:
    explicit UrlService(std::string baseUrl);

    void createUrl(
        const std::string &longUrl,
        std::int64_t userId,
        std::function<void(const drogon::HttpResponsePtr &)> callback);

    void redirect(
        const std::string &code,
        const ClickMetadata &metadata,
        std::function<void(const drogon::HttpResponsePtr &)> callback);

    void getStats(
        const std::string &code,
        std::int64_t userId,
        std::function<void(const drogon::HttpResponsePtr &)> callback);

  private:
    std::string baseUrl_;

    void insertUrl(
        const std::string &longUrl,
        std::int64_t userId,
        std::string code,
        std::function<void(const drogon::HttpResponsePtr &)> callback);
};
