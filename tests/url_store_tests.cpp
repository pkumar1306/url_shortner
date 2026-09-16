#include "url_store.h"

#include <cassert>
#include <cctype>
#include <string>

int main() {
    assert(UrlStore::isPlausibleHttpUrl("https://example.com/a?b=c"));
    assert(UrlStore::isPlausibleHttpUrl("http://localhost:3000/path"));
    assert(!UrlStore::isPlausibleHttpUrl("ftp://example.com"));
    assert(!UrlStore::isPlausibleHttpUrl("https://"));
    assert(!UrlStore::isPlausibleHttpUrl("not a url"));

    UrlStore store;
    const std::string code = store.create("https://example.com");
    assert(code.size() == 6);
    for (const unsigned char character : code) {
        assert(std::isalnum(character));
    }
    assert(store.find(code) == "https://example.com");
    assert(!store.find("missing"));
}
