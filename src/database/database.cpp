#include "database/database.h"

#include <drogon/drogon.h>
#ifdef ERROR
#undef ERROR
#endif

void Database::configure(const AppConfig &config,
                         std::shared_ptr<Logger> logger)
{
    drogon::orm::PostgresConfig dbConfig{};
    dbConfig.host = config.dbHost;
    dbConfig.port = config.dbPort;
    dbConfig.databaseName = config.dbName;
    dbConfig.username = config.dbUser;
    dbConfig.password = config.dbPassword;
    dbConfig.connectionNumber = 1;
    dbConfig.name = "default";
    dbConfig.timeout = 5.0;
    drogon::app().addDbClient(dbConfig);

    if (logger) {
        logger->info("Database client configured: host=" + config.dbHost +
                     " port=" + std::to_string(config.dbPort) +
                     " db=" + config.dbName +
                     " user=" + config.dbUser);
    }

    drogon::app().registerBeginningAdvice([logger] {
        auto db = drogon::app().getDbClient("default");
        db->execSqlAsync(
            "SELECT 1",
            [logger](const drogon::orm::Result &) {
                if (logger) {
                    logger->info("PostgreSQL connection OK");
                }
            },
            [logger](const drogon::orm::DrogonDbException &error) {
                if (logger) {
                    logger->error(std::string("PostgreSQL connection failed: ") +
                                  error.base().what());
                }
            });
    });
}
