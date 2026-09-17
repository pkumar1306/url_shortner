#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <regex>
#include <sstream>
#include <thread>

// ---------------------------------------------------------------------------
// Thread-local request ID – set once per HTTP request, automatically included
// in every log line produced on that thread.
// ---------------------------------------------------------------------------
static thread_local std::string tl_requestId;

void Logger::setRequestId(const std::string& id) { tl_requestId = id; }
std::string Logger::getRequestId()               { return tl_requestId; }

// ---------------------------------------------------------------------------
// LogLevel helpers
// ---------------------------------------------------------------------------
LogLevel parseLogLevel(std::string_view text)
{
    // Build an upper-cased copy for case-insensitive comparison.
    std::string upper(text);
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    if (upper == "TRACE") return LogLevel::TRACE;
    if (upper == "DEBUG") return LogLevel::DEBUG;
    if (upper == "INFO")  return LogLevel::INFO;
    if (upper == "WARN")  return LogLevel::WARN;
    if (upper == "ERROR") return LogLevel::ERROR;
    return LogLevel::INFO;   // safe default
}

const char* logLevelTag(LogLevel level)
{
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
    }
    return "?????";
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------
Logger::Logger(LogLevel level, const std::string& filePath)
    : level_(level)
{
    if (!filePath.empty()) {
        fileStream_.open(filePath, std::ios::app);
        if (!fileStream_.is_open()) {
            std::cerr << "[WARN ] Could not open log file: " << filePath
                      << " – falling back to console only.\n";
        }
    }
}

Logger::~Logger()
{
    if (fileStream_.is_open()) {
        fileStream_.flush();
        fileStream_.close();
    }
}

// ---------------------------------------------------------------------------
// Convenience methods
// ---------------------------------------------------------------------------
void Logger::trace(const std::string& message) { log(LogLevel::TRACE, message); }
void Logger::debug(const std::string& message) { log(LogLevel::DEBUG, message); }
void Logger::info (const std::string& message) { log(LogLevel::INFO,  message); }
void Logger::warn (const std::string& message) { log(LogLevel::WARN,  message); }
void Logger::error(const std::string& message) { log(LogLevel::ERROR, message); }

// ---------------------------------------------------------------------------
// Core log method
// ---------------------------------------------------------------------------
void Logger::log(LogLevel level, const std::string& message)
{
    // ---- Level gate – drop messages below the configured threshold. -------
    if (level < level_) {
        return;
    }

    const std::string line = formatLine(level, message);

    // ---- Thread-safe write to all sinks. ----------------------------------
    std::lock_guard<std::mutex> lock(mutex_);
    std::cerr << line;                       // console sink (stderr)
    if (fileStream_.is_open()) {
        fileStream_ << line;
        fileStream_.flush();                 // ensure durability
    }
}

// ---------------------------------------------------------------------------
// Sensitive-data redaction
//
// Patterns matched (case-insensitive key, value after '=' or ':' or '"'):
//   api_key, password, authorization, bearer, token, secret
//
// Values are replaced with "***REDACTED***".
// ---------------------------------------------------------------------------
std::string Logger::sanitize(const std::string& raw)
{
    static const std::regex bearerPattern(
        R"rx((bearer)\s+([^\s",}]+))rx",
        std::regex::icase);
    static const std::regex kvPattern(
        R"rx((api[_-]?key|password|authorization|token|secret|db[_-]?password)\s*[:=]\s*"?([^\s",}]+)"?)rx",
        std::regex::icase);

    std::string clean = std::regex_replace(raw, bearerPattern, "$1 ***REDACTED***");
    clean = std::regex_replace(clean, kvPattern, "$1=***REDACTED***");
    return clean;
}

// ---------------------------------------------------------------------------
// Line formatting
//
// Output:  2026-09-17T22:15:08.123Z [INFO ] [tid:14028] [req:abc123] message
// ---------------------------------------------------------------------------
std::string Logger::formatLine(LogLevel level, const std::string& message) const
{
    // ---- ISO-8601 timestamp with millisecond precision --------------------
    const auto now    = std::chrono::system_clock::now();
    const auto timeT  = std::chrono::system_clock::to_time_t(now);
    const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                            now.time_since_epoch()) % 1000;

    std::tm utcTm{};
#if defined(_WIN32)
    gmtime_s(&utcTm, &timeT);
#else
    gmtime_r(&timeT, &utcTm);
#endif

    std::ostringstream out;
    out << std::put_time(&utcTm, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << millis.count() << 'Z';

    // ---- Level tag --------------------------------------------------------
    out << " [" << logLevelTag(level) << ']';

    // ---- Thread ID --------------------------------------------------------
    out << " [tid:" << std::this_thread::get_id() << ']';

    // ---- Request ID (if set) ----------------------------------------------
    const auto& reqId = tl_requestId;
    if (!reqId.empty()) {
        out << " [req:" << reqId << ']';
    }

    // ---- Sanitised message ------------------------------------------------
    out << ' ' << sanitize(message) << '\n';

    return out.str();
}
