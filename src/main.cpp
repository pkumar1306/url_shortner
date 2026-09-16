#include "config/app_config.h"
#include "database/database.h"
#include "handlers/url_handler.h"
#include "services/url_service.h"

#include <drogon/drogon.h>

#include <memory>

int main()
{
    const AppConfig config = AppConfig::fromEnvironment();
    Database::configure(config);

    auto service = std::make_shared<UrlService>(config.baseUrl);
    auto authService = std::make_shared<AuthService>();
    UrlHandler handler(service, authService);
    handler.registerRoutes();

    drogon::app()
        .addListener("0.0.0.0", config.port)
        .setThreadNum(1)
        .run();
}
