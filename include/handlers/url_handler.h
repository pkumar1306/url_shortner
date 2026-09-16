#pragma once

#include "services/url_service.h"

#include "utils/rate_limiter.h" 
#include <memory>

class UrlHandler {
  
  public:
    explicit UrlHandler(std::shared_ptr<UrlService> service);
    void registerRoutes();
    

  private:
    std::shared_ptr<UrlService> service_;
    RateLimiter rateLimiter_;
};
