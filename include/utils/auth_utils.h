#pragma once

#include <optional>
#include <string>

namespace AuthUtils {
std::string generateKey();
std::string hashKey(const std::string &key);
std::optional<std::string> bearerToken(const std::string &authorization);
}