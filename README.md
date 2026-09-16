# C++ URL Shortener (Phases 1–2)

A minimal URL-shortening HTTP API written in C++20 with [Drogon](https://github.com/drogonframework/drogon) and CMake. Data is intentionally stored only in memory for this phase, so links disappear when the server stops.

## Included

- `GET /` returns the API health message.
- `POST /api/v1/urls` validates an HTTP/HTTPS URL and creates a unique random six-character code.
- `GET /{code}` sends a `302 Found` redirect to the saved URL, or returns JSON `404` when absent.
- A small unit test for URL validation and storage.

Database persistence, accounts, analytics, rate limits, and click tracking are deliberately not part of this phase.

## Prerequisites

- CMake 3.20 or newer
- A C++20 compiler
- Drogon installed in a location CMake can discover (`find_package(Drogon CONFIG REQUIRED)`).

For example, on vcpkg:




```powershell
vcpkg install drogon
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

If Drogon is installed system-wide, omit the toolchain option:

```powershell
cmake -S . -B build
cmake --build build
```

On a Windows machine using the MSYS2/MinGW compiler, select that generator explicitly (the Visual Studio `NMake` generator is not usable unless Visual Studio build tools are installed):

```powershell
cmake -S . -B build -G "MinGW Makefiles" `
  -DCMAKE_MAKE_PROGRAM=C:/msys64/ucrt64/bin/mingw32-make.exe `
  -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe `
  -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

## Run

```powershell
./build/url_shortener
```

The server listens on `http://localhost:8080`. To have returned links use a deployed public address, set `BASE_URL` before starting it, for example: `$env:BASE_URL = "https://sho.rt"`.

## Try it

```powershell
curl http://localhost:8080/

curl -X POST http://localhost:8080/api/v1/urls `
  -H "Content-Type: application/json" `
  -d '{"url":"https://example.com/docs"}'

curl -i http://localhost:8080/<code-from-response>
curl -i http://localhost:8080/notfound
```

Expected creation response:

```json
{
  "code": "aB92xK",
  "short_url": "http://localhost:8080/aB92xK"
}
```

## Test

```powershell
ctest --test-dir build --output-on-failure
```



--- implementatition plan :


Absolutely. **C++ is a very good choice** for this, especially if your goal is to learn backend fundamentals, networking, databases, memory/concurrency, and system design rather than just learn a web framework.

For C++, I would slightly change the project so you're actually learning C++ backend engineering rather than trying to recreate FastAPI.

# C++ Project: URL Shortener + Analytics API

Build this:

```text
                  ┌──────────────────────┐
                  │      C++ API         │
                  │    HTTP Server       │
                  └──────────┬───────────┘
                             │
             ┌───────────────┼────────────────┐
             │               │                │
             ▼               ▼                ▼
        POST /shorten   GET /{code}     GET /{code}/stats
             │               │                │
             ▼               ▼                ▼
          Create          Redirect         Analytics
          short URL       + click log       queries
             │               │                │
             └───────────────┼────────────────┘
                             ▼
                       PostgreSQL
```

## The C++ stack

Keep it relatively simple:

**C++20 or C++23**

HTTP server:
**Drogon**

Database:
**PostgreSQL**

Database access:
**Drogon ORM / PostgreSQL client**

Build:
**CMake**

Testing:
**Catch2** or **GoogleTest**

Containerization later:
**Docker**

You don't need to learn all of these before starting.

---

# What you're going to implement

### 1. Create a short URL

```http
POST /api/v1/urls
```

Request:

```json
{
    "url": "https://www.youtube.com/watch?v=abc123"
}
```

Response:

```json
{
    "code": "aB92xK",
    "short_url": "http://localhost:8080/aB92xK"
}
```

---

### 2. Redirect

```http
GET /aB92xK
```

Your server:

```text
aB92xK
   ↓
database lookup
   ↓
original URL
   ↓
record click
   ↓
HTTP 302 redirect
```

---

### 3. Analytics

```http
GET /api/v1/urls/aB92xK/stats
```

Return:

```json
{
    "total_clicks": 143,
    "clicks_today": 27,
    "top_referrers": [
        {
            "referrer": "google.com",
            "clicks": 52
        },
        {
            "referrer": "instagram.com",
            "clicks": 31
        }
    ],
    "devices": {
        "mobile": 90,
        "desktop": 53
    }
}
```

---

# Your database

Start with just two tables.

### `urls`

```text
urls
--------------------------------
id
short_code
long_url
created_at
```

### `clicks`

```text
clicks
--------------------------------
id
url_id
timestamp
ip_address
user_agent
referrer
```

Later:

```text
users
--------------------------------
id
api_key
created_at
```

Don't add the users table initially.

---

# The most important part: how to learn while building

I wouldn't give you a huge C++ tutorial first.

Build the project in stages.

## Phase 1 — C++ web server

Your first goal is simply:

```http
GET /
```

Response:

```json
{
    "message": "URL Shortener API"
}
```

### Learn

You need to understand:

```text
C++ project structure
CMake
HTTP request
HTTP response
JSON
routing
```

At the end:

```text
Browser / curl
       ↓
C++ server
       ↓
JSON response
```

---

# Phase 2 — Create `/shorten`

Implement:

```http
POST /api/v1/urls
```

Initially **don't use PostgreSQL**.

Just keep URLs in memory:

```cpp
std::unordered_map<std::string, std::string> urls;
```

For example:

```text
"aB92xK" → "https://google.com"
"Z81LmP" → "https://youtube.com"
```

Now you're learning actual C++ data structures while building the API.

Your flow:

```text
POST /shorten
     ↓
parse JSON
     ↓
validate URL
     ↓
generate code
     ↓
unordered_map
     ↓
return JSON
```

---

# Phase 3 — Implement redirect

Create:

```http
GET /{code}
```

Example:

```text
GET /aB92xK
```

Your C++ program:

```cpp
auto it = urls.find(code);
```

Then:

```text
found
 ↓
302 redirect

not found
 ↓
404
```

At this point you have a functioning URL shortener.

---

# Phase 4 — Replace memory with PostgreSQL

This is a major milestone.

Currently:

```text
C++ process
   ↓
unordered_map
```

If you restart the program:

```text
💥 everything disappears
```

Now change it to:

```text
C++ API
   ↓
PostgreSQL
   ↓
persistent URLs
```

This is where you'll learn:

```text
SQL
database connections
CRUD
primary keys
foreign keys
unique constraints
transactions
indexes
```

---

# Phase 5 — Add analytics

Modify:

```text
GET /{code}
```

from:

```text
lookup
 ↓
redirect
```

to:

```text
lookup
 ↓
record click
 ↓
redirect
```

Capture:

```text
IP
User-Agent
Referer
Timestamp
```

Now create:

```http
GET /api/v1/urls/{code}/stats
```

Start with only:

```text
total clicks
```

Then add:

```text
clicks per day
top referrers
browser
device
```

---

# Phase 6 — Add concurrency

This is where doing it in C++ becomes particularly interesting.

Imagine:

```text
User A ──┐
User B ──┤
User C ──┼──→ C++ server
User D ──┤
User E ──┘
```

Multiple requests can happen simultaneously.

You should understand:

```text
threads
mutex
race conditions
thread safety
concurrent data structures
```

For example, your first in-memory implementation:

```cpp
std::unordered_map<std::string, std::string> urls;
```

can create problems if multiple threads access/write it simultaneously.

That gives you an actual reason to learn:

```cpp
std::mutex
std::lock_guard
```

rather than learning them theoretically.

---

# Phase 7 — Make short-code generation better

Start with:

```text
random 6-character string
```

Example:

```text
aB92xK
```

Then ask:

> What happens if that code already exists?

Implement:

```text
generate
   ↓
check database
   ↓
exists?
 ├── yes → generate again
 └── no  → save
```

Then, as an optional challenge, implement **Base62 encoding**.

For example:

```text
database ID: 125
       ↓
Base62
       ↓
cb
```

This gives you an opportunity to understand:

```text
number representation
encoding
collision probability
uniqueness
```

---

# Phase 8 — Add API keys

Only after everything else works.

Implement:

```http
POST /api/v1/urls
Authorization: Bearer abc123
```

Now users can own URLs.

Schema becomes:

```text
users
   │
   │ 1
   │
   │ many
   ▼
urls
   │
   │ 1
   │
   │ many
   ▼
clicks
```

---

# Phase 9 — Rate limiting

Add:

```text
100 requests / minute / API key
```

Now you get to think about:

```text
How do I track requests?
Where do I store counters?
What happens with concurrent requests?
What does "per minute" mean?
```

Later you can implement the rate limiter using:

```text
unordered_map
+
timestamps
+
mutex
```

Then, as an advanced version, replace it with Redis.

---

# Your project progression

I'd make your actual project roadmap:

```text
                    URL SHORTENER
                          │
        ┌─────────────────┴──────────────────┐
        │                                    │
   BASIC VERSION                         ADVANCED
        │                                    │
        ▼                                    ▼
 HTTP server                         API authentication
        │                            Rate limiting
 GET /                               Logging
        │                            Metrics
 POST /shorten                       Caching
        │                            Background jobs
 in-memory map                       Redis
        │                            Docker
 redirect                            Load testing
        │                            Deployment
        ▼
 PostgreSQL
        │
        ▼
 click tracking
        │
        ▼
 analytics API
```

---

# What I'd make your weekend target

Don't try to finish everything.

### Saturday

```text
[ ] Install C++ compiler
[ ] Install CMake
[ ] Create project
[ ] Get Drogon running
[ ] GET /
[ ] Return JSON
[ ] POST /api/v1/urls
[ ] Parse JSON
[ ] Generate short code
[ ] Store in unordered_map
[ ] GET /{code}
[ ] Redirect
```

**Saturday success condition:**

```bash
curl -X POST http://localhost:8080/api/v1/urls \
  -H "Content-Type: application/json" \
  -d '{"url":"https://youtube.com"}'
```

returns:

```json
{
    "code": "aB92xK"
}
```

and:

```bash
curl -I http://localhost:8080/aB92xK
```

returns a redirect.

That's your first major checkpoint.

---

### Sunday

```text
[ ] PostgreSQL
[ ] urls table
[ ] Database CRUD
[ ] Replace unordered_map
[ ] clicks table
[ ] Record click
[ ] GET /stats
[ ] COUNT clicks
[ ] GROUP BY day
[ ] Referrer analytics
[ ] Basic tests
```

Then stop.

Don't worry about deployment yet.

---

# What you'll learn from this one project

This is why I actually like the C++ version for you.

You'll touch:

```text
C++
├── classes
├── STL
├── unordered_map
├── strings
├── smart pointers
├── exceptions
├── RAII
├── concurrency
├── mutex
│
├── HTTP
├── REST APIs
├── JSON
│
├── SQL
├── PostgreSQL
├── indexes
├── transactions
│
├── CMake
├── unit testing
│
└── system design
```

And importantly, you'll be able to explain **why** each thing exists.

For example:

> "Why PostgreSQL?"

Because URL data needs persistence and relational queries for analytics.

> "Why an index on `short_code`?"

Because every redirect performs a lookup by short code.

> "Why a foreign key in clicks?"

Because each click belongs to a shortened URL.

> "Why concurrency matters?"

Because an HTTP server handles multiple requests concurrently.

That makes the project much stronger than simply saying **"I built a URL shortener."**

## One change I'd make from your original project

Don't begin with the full architecture.

Build:

```text
Version 1
C++ → memory → redirect
```

then:

```text
Version 2
C++ → PostgreSQL → redirect
```

then:

```text
Version 3
C++ → PostgreSQL → analytics
```

then:

```text
Version 4
C++ → auth + rate limiting + tests
```

That way every feature gives you a reason to learn a new concept.

**Your first task should be setting up the C++ HTTP server and getting `GET /` working.** After that, the project can be developed checkpoint-by-checkpoint rather than you trying to learn the entire stack at once.