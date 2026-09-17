#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>

// ---------------------------------------------------------------------------
// LogLevel – five severity levels used throughout the application.
//
//   TRACE  – Very fine-grained diagnostic output (loop iterations, variable
//            dumps).  Typically disabled in production.
//   DEBUG  – Diagnostic detail useful during development (function entry/exit,
//            intermediate results).
//   INFO   – Normal operational events (startup, shutdown, request served).
//   WARN   – Unexpected but recoverable situations (rate-limit hit, fallback
//            used, deprecated call).
//   ERROR  – Failures that prevent an operation from completing (DB errors,
//            unhandled exceptions).
//
// The configured level acts as a *floor*: messages at or above the level are
// emitted; messages below are silently dropped.
// ---------------------------------------------------------------------------
// Undefine Windows GDI ERROR macro if present to avoid conflict with LogLevel::ERROR
#ifdef ERROR
#undef ERROR
#endif

enum class LogLevel
{
    TRACE = 0,
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4
};

/// Parse a case-insensitive string ("TRACE", "debug", …) into a LogLevel.
/// Returns LogLevel::INFO when the string is unrecognised.
LogLevel parseLogLevel(std::string_view text);

/// Convert a LogLevel back to its upper-case tag (e.g. "INFO").
const char* logLevelTag(LogLevel level);

// ---------------------------------------------------------------------------
// Logger – production-grade, thread-safe logger.
//
// Construction
//   Logger logger(LogLevel::INFO, "logs/app.log");   // console + file
//   Logger logger(LogLevel::DEBUG);                   // console only
//
// Usage
//   logger.info("Server started on port 8080");
//   logger.error("Database connection failed: " + err);
//   logger.trace("Token bucket state: " + std::to_string(tokens));
//
// Output format
//   2026-09-17T22:15:08.123Z [INFO ] [tid:14028] [req:a1b2c3d4] Server …
//
// Thread-local request ID
//   Call Logger::setRequestId("uuid") at the start of each HTTP request.
//   The ID is automatically included in every subsequent log line on the
//   same thread until cleared (setRequestId("")).
//
// Sensitive-data redaction
//   The logger automatically masks values for keys that match common
//   sensitive patterns (api_key, password, authorization, bearer tokens).
// ---------------------------------------------------------------------------
class Logger
{
public:
    /// Construct a logger that writes to the console (std::cerr).
    /// If `filePath` is non-empty, also open (append-mode) a log file.
    explicit Logger(LogLevel level = LogLevel::INFO,
                    const std::string& filePath = {});

    ~Logger();

    // -- Convenience log methods ------------------------------------------
    void trace(const std::string& message);
    void debug(const std::string& message);
    void info (const std::string& message);
    void warn (const std::string& message);
    void error(const std::string& message);

    // -- Generic log method -----------------------------------------------
    void log(LogLevel level, const std::string& message);

    // -- Configuration accessors ------------------------------------------
    LogLevel level() const { return level_; }
    void setLevel(LogLevel level) { level_ = level; }

    // -- Thread-local request ID ------------------------------------------
    static void        setRequestId(const std::string& id);
    static std::string getRequestId();

private:
    /// Build a fully formatted log line (timestamp, level, thread, reqId, msg).
    std::string formatLine(LogLevel level, const std::string& message) const;

    /// Redact sensitive values in `raw` before it reaches the output streams.
    static std::string sanitize(const std::string& raw);

    LogLevel      level_;
    std::ofstream fileStream_;
    std::mutex    mutex_;
};
