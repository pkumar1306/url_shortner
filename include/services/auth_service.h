#pragma once

#include <drogon/drogon.h>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

class AuthService {
  public:
    using IssueCallback = std::function<void(
        const std::string &,
        std::int64_t)>;
    using AuthenticateCallback = std::function<void(
        std::optional<std::int64_t>)>;

    void issueKey(IssueCallback callback);
    void authenticate(
        const std::string &authorization,
        AuthenticateCallback callback);
};