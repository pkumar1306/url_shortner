#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

class UrlStore {
  public:
    // Returns a newly allocated six-character code for a valid URL.
    std::string create(const std::string &url);
    std::optional<std::string> find(const std::string &code) const;

    static bool isPlausibleHttpUrl(const std::string &url);

  private:
    std::string generateCode();

    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> urls_;
};
