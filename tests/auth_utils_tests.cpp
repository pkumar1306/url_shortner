#include "utils/auth_utils.h"

#include <cassert>
#include <cctype>

int main()
{
    const std::string firstKey = AuthUtils::generateKey();
    const std::string secondKey = AuthUtils::generateKey();

    assert(firstKey.size() == 64);
    assert(secondKey.size() == 64);
    assert(firstKey != secondKey);
    for (const unsigned char character : firstKey) {
        assert(std::isxdigit(character));
    }

    const auto firstHash = AuthUtils::hashKey(firstKey);
    assert(firstHash.size() == 64);
    assert(firstHash == AuthUtils::hashKey(firstKey));
    assert(firstHash != AuthUtils::hashKey(secondKey));
    assert(firstHash != firstKey);

    assert(!AuthUtils::bearerToken(""));
    assert(!AuthUtils::bearerToken("Basic secret"));
    assert(!AuthUtils::bearerToken("Bearer "));
    assert(!AuthUtils::bearerToken("bearer secret"));
    assert(AuthUtils::bearerToken("Bearer secret") == "secret");
    return 0;
}