#include "config/app_config.h"
#include "database/database.h"
#include "handlers/url_handler.h"
#include "services/auth_service.h"
#include "services/url_service.h"
#include "utils/logger.h"

#include <drogon/drogon.h>
#ifdef ERROR
#undef ERROR
#endif

#include <filesystem>
#include <memory>

int main()
{
    // ---- Load configuration from environment variables --------------------
    const AppConfig config = AppConfig::fromEnvironment();

    // ---- Create the shared logger ----------------------------------------
    // Ensure the log directory exists before opening the file.
    if (!config.logFile.empty()) {
        const auto logDir = std::filesystem::path(config.logFile).parent_path();
        if (!logDir.empty()) {
            std::filesystem::create_directories(logDir);
        }
    }

    auto logger = std::make_shared<Logger>(
        parseLogLevel(config.logLevel),
        config.logFile);

    logger->info("=== URL Shortener starting ===");
    logger->info("Log level: " + config.logLevel);
    logger->info("Log file:  " + (config.logFile.empty() ? "(console only)" : config.logFile));
    logger->info("Port:      " + std::to_string(config.port));
    logger->info("Base URL:  " + config.baseUrl);
    logger->debug("DB host:   " + config.dbHost + ":" + std::to_string(config.dbPort));
    logger->debug("DB name:   " + config.dbName);

    // ---- Configure database -----------------------------------------------
    Database::configure(config, logger);

    // ---- Wire up services and handlers with the shared logger -------------
    auto service     = std::make_shared<UrlService>(config.baseUrl, logger);
    auto authService = std::make_shared<AuthService>(logger);
    UrlHandler handler(service, authService, logger);
    handler.registerRoutes();

    logger->info("Listening on 0.0.0.0:" + std::to_string(config.port));

    // ---- Start the event loop ---------------------------------------------
    drogon::app()
        .addListener("0.0.0.0", config.port)
        .setThreadNum(1)
        .run();

    // After run() returns (server shut down).
    logger->info("=== URL Shortener stopped ===");
}
