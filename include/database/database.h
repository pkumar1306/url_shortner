#pragma once

#include "config/app_config.h"
#include "utils/logger.h"

#include <memory>

class Database {
  public:
    /// Configure the Drogon DB client and log connection status.
    static void configure(const AppConfig &config,
                          std::shared_ptr<Logger> logger = nullptr);
};
