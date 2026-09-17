#pragma once

#include <cstdint>
#include <string>

struct AppConfig {
    unsigned short port{8080};
    std::string baseUrl;
    std::string dbHost{"127.0.0.1"};
    unsigned short dbPort{5432};
    std::string dbName{"url_shortener"};
    std::string dbUser{"postgres"};
    std::string dbPassword;

    // Logging configuration – read from LOG_LEVEL and LOG_FILE env vars.
    std::string logLevel{"INFO"};        // TRACE | DEBUG | INFO | WARN | ERROR
    std::string logFile{"logs/app.log"}; // empty = console only

    static AppConfig fromEnvironment();
};
