#include "database/database.h"

#include <drogon/drogon.h>

#include <iostream>

void Database::configure(const AppConfig &config)
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

    drogon::app().registerBeginningAdvice([] {
        auto db = drogon::app().getDbClient("default");
        db->execSqlAsync(
            "SELECT 1",
            [](const drogon::orm::Result &) {
                std::cout << "PostgreSQL connection OK\n";
            },
            [](const drogon::orm::DrogonDbException &error) {
                std::cerr << "PostgreSQL error: " << error.base().what() << '\n';
            });
    });
}
