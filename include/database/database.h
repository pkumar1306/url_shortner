#pragma once

#include "config/app_config.h"

class Database {
  public:
    static void configure(const AppConfig &config);
};
