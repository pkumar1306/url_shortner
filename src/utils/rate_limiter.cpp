#include "utils/rate_limiter.h"
#include <algorithm>


RateLimiter::RateLimiter(double capacity, double refillRate)
    : capacity_(capacity),
      refillRate_(refillRate)
{
}

bool RateLimiter::allow(const std::string& ip)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::steady_clock::now();

    auto it = buckets_.find(ip);

    // First request from this IP
    if (it == buckets_.end())
    {
        buckets_[ip] = {
            capacity_ - 1,
            now
        };

        return true;
    }

    Bucket& bucket = it->second;

    // Calculate elapsed time
    std::chrono::duration<double> elapsed =
        now - bucket.lastRefill;

    // Refill tokens
    bucket.tokens += elapsed.count() * refillRate_;

    // Don't exceed capacity
    bucket.tokens = std::min(bucket.tokens, capacity_);

    bucket.lastRefill = now;

    // No token available
    if (bucket.tokens < 1.0)
    {
        return false;
    }

    // Consume one token
    bucket.tokens -= 1.0;

    return true;
}