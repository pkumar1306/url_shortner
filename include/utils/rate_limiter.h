#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>

class RateLimiter
{
public:
    RateLimiter(double capacity, double refillRate);

    bool allow(const std::string& ip);

private:
    struct Bucket
    {
        double tokens;
        std::chrono::steady_clock::time_point lastRefill;
    };

    double capacity_;
    double refillRate_;

    std::unordered_map<std::string, Bucket> buckets_;

    std::mutex mutex_;
};