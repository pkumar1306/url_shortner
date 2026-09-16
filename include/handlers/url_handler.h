#pragma once

#include "services/auth_service.h"
#include "services/url_service.h"

#include "utils/rate_limiter.h" 
#include <memory>

class UrlHandler {
  
  public:
    UrlHandler(
      std::shared_ptr<UrlService> service,
      std::shared_ptr<AuthService> authService);
    void registerRoutes();
    

  private:
    std::shared_ptr<UrlService> service_;
    std::shared_ptr<AuthService> authService_;
    RateLimiter rateLimiter_;
};
