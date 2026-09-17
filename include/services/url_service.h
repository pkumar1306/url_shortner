#pragma once

#include "utils/logger.h"

#include <drogon/drogon.h>
#ifdef ERROR
#undef ERROR
#endif

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

struct ClickMetadata {
    std::string ipAddress;
    std::string userAgent;
    std::string referrer;
};

class UrlService {
  public:
    /// Construct with a base URL and an injected logger.
    UrlService(std::string baseUrl, std::shared_ptr<Logger> logger);

    /// Legacy constructor (no logger – for backward compat / tests).
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
    std::shared_ptr<Logger> logger_;

    void insertUrl(
        const std::string &longUrl,
        std::int64_t userId,
        std::string code,
        std::function<void(const drogon::HttpResponsePtr &)> callback);
};
