#pragma once

#include "utils/logger.h"

#include <drogon/drogon.h>
#ifdef ERROR
#undef ERROR
#endif

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>

class AuthService {
  public:
    using IssueCallback = std::function<void(
        const std::string &,
        std::int64_t)>;
    using AuthenticateCallback = std::function<void(
        std::optional<std::int64_t>)>;

    /// Construct with an injected logger (production use).
    explicit AuthService(std::shared_ptr<Logger> logger);

    /// Legacy default constructor (for tests / backwards compat).
    AuthService();

    void issueKey(IssueCallback callback);
    void authenticate(
        const std::string &authorization,
        AuthenticateCallback callback);

  private:
    std::shared_ptr<Logger> logger_;
};