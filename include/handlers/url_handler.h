#pragma once

#include "services/auth_service.h"
#include "services/url_service.h"
#include "utils/logger.h"
#include "utils/rate_limiter.h"

#include <memory>

class UrlHandler {
  
  public:
    UrlHandler(
      std::shared_ptr<UrlService> service,
      std::shared_ptr<AuthService> authService,
      std::shared_ptr<Logger> logger);

    void registerRoutes();
    

  private:
    std::shared_ptr<UrlService> service_;
    std::shared_ptr<AuthService> authService_;
    std::shared_ptr<Logger> logger_;
    RateLimiter rateLimiter_;
};
