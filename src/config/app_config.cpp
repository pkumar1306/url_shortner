#include "config/app_config.h"

#include <cstdlib>
#include <iostream>

namespace {
std::string environmentValue(const char *name, const std::string &fallback = {})
{
    const char *value = std::getenv(name);
    return value != nullptr ? std::string(value) : fallback;
}

unsigned short environmentPort(const char *name, unsigned short fallback)
{
    const auto value = environmentValue(name);
    if (value.empty()) {
        return fallback;
    }

    char *end = nullptr;
    const auto parsed = std::strtoul(value.c_str(), &end, 10);
    if (end != value.c_str() && *end == '\0' && parsed > 0 && parsed <= 65535) {
        return static_cast<unsigned short>(parsed);
    }
    return fallback;
}
}

AppConfig AppConfig::fromEnvironment()
{
    AppConfig config;
    config.port = environmentPort("PORT", config.port);
    config.baseUrl = environmentValue("BASE_URL");
    if (config.baseUrl.empty()) {
        config.baseUrl = "http://localhost:" + std::to_string(config.port);
    }

    config.dbHost = environmentValue("DB_HOST", config.dbHost);
    config.dbPort = environmentPort("DB_PORT", config.dbPort);
    config.dbName = environmentValue("DB_NAME", config.dbName);
    config.dbUser = environmentValue("DB_USER", config.dbUser);
    config.dbPassword = environmentValue("DB_PASSWORD");
    if (config.dbPassword.empty()) {
        std::cerr << "DB_PASSWORD is not set. Set it before starting the server.\n";
        std::exit(1);
    }
    return config;
}
