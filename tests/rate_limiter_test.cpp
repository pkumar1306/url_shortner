#include "utils/rate_limiter.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

int main()
{
    RateLimiter limiter(10.0, 10.0 / 60.0);

    const std::string ip = "127.0.0.1";

    // First 10 requests should succeed
    for (int i = 0; i < 10; ++i)
    {
        assert(limiter.allow(ip));
    }

    // 11th request should fail
    assert(!limiter.allow(ip));

    std::cout << "Rate limit reached successfully.\n";

    // Wait for approximately 6 seconds.
    // 10 tokens/minute = 1 token every 6 seconds.
    std::this_thread::sleep_for(std::chrono::seconds(6));

    // A token should have refilled.
    assert(limiter.allow(ip));

    std::cout << "Token refill works successfully.\n";
    std::cout << "All tests passed!\n";

    return 0;
}