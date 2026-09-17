#include "utils/request_id.h"

#include <iomanip>
#include <random>
#include <sstream>

namespace RequestId {

std::string generate()
{
    // Thread-local Mersenne Twister seeded once per thread.
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<uint32_t> dist(0, 0xFFFFFFFFu);

    // UUID-v4 layout: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    // "4" marks version 4; y is one of {8, 9, a, b} (variant 1).
    const uint32_t a = dist(gen);
    const uint32_t b = dist(gen);
    const uint32_t c = dist(gen);
    const uint32_t d = dist(gen);

    // Inject version nibble (0100) into bits [15:12] of b.
    const uint32_t bVersioned = (b & 0xFFFF0FFFu) | 0x00004000u;

    // Inject variant bits (10xx) into bits [15:14] of c.
    const uint32_t cVariant = (c & 0x3FFFFFFFu) | 0x80000000u;

    std::ostringstream out;
    out << std::hex << std::setfill('0');
    out << std::setw(8) << a << '-';
    out << std::setw(4) << (bVersioned >> 16) << '-';
    out << std::setw(4) << (bVersioned & 0xFFFF) << '-';
    out << std::setw(4) << (cVariant >> 16) << '-';
    out << std::setw(4) << (cVariant & 0xFFFF);
    out << std::setw(8) << d;
    return out.str();
}

}

