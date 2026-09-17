#include "utils/logger.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

// ---------------------------------------------------------------------------
// Helper: capture stderr output produced by a callable.
// ---------------------------------------------------------------------------
template <typename Fn>
std::string captureStderr(Fn fn)
{
    // Redirect std::cerr to a stringstream.
    std::ostringstream capture;
    auto *original = std::cerr.rdbuf(capture.rdbuf());
    fn();
    std::cerr.rdbuf(original);
    return capture.str();
}

// ---------------------------------------------------------------------------
// Test 1: Log-level filtering – messages below the threshold are dropped.
// ---------------------------------------------------------------------------
void testLevelFiltering()
{
    auto output = captureStderr([&] {
        Logger logger(LogLevel::WARN);
        logger.trace("should not appear");
        logger.debug("should not appear");
        logger.info("should not appear");
        logger.warn("this WARN should appear");
        logger.error("this ERROR should appear");
    });

    assert(output.find("should not appear") == std::string::npos);
    assert(output.find("[WARN ]") != std::string::npos);
    assert(output.find("this WARN should appear") != std::string::npos);
    assert(output.find("[ERROR]") != std::string::npos);
    assert(output.find("this ERROR should appear") != std::string::npos);
    std::cout << "PASS: testLevelFiltering\n";
}

// ---------------------------------------------------------------------------
// Test 2: All five levels emit when threshold is TRACE.
// ---------------------------------------------------------------------------
void testAllLevelsEmit()
{
    auto output = captureStderr([&] {
        Logger logger(LogLevel::TRACE);
        logger.trace("t");
        logger.debug("d");
        logger.info("i");
        logger.warn("w");
        logger.error("e");
    });

    assert(output.find("[TRACE]") != std::string::npos);
    assert(output.find("[DEBUG]") != std::string::npos);
    assert(output.find("[INFO ]") != std::string::npos);
    assert(output.find("[WARN ]") != std::string::npos);
    assert(output.find("[ERROR]") != std::string::npos);
    std::cout << "PASS: testAllLevelsEmit\n";
}

// ---------------------------------------------------------------------------
// Test 3: Request-ID appears in log output.
// ---------------------------------------------------------------------------
void testRequestId()
{
    auto output = captureStderr([&] {
        Logger logger(LogLevel::INFO);
        Logger::setRequestId("test-req-12345");
        logger.info("with request id");
        Logger::setRequestId("");
    });

    assert(output.find("[req:test-req-12345]") != std::string::npos);
    std::cout << "PASS: testRequestId\n";
}

// ---------------------------------------------------------------------------
// Test 4: Request-ID is absent when not set.
// ---------------------------------------------------------------------------
void testNoRequestId()
{
    auto output = captureStderr([&] {
        Logger logger(LogLevel::INFO);
        Logger::setRequestId("");
        logger.info("no request id");
    });

    assert(output.find("[req:") == std::string::npos);
    std::cout << "PASS: testNoRequestId\n";
}

// ---------------------------------------------------------------------------
// Test 5: Sensitive data redaction.
// ---------------------------------------------------------------------------
void testSanitization()
{
    auto output = captureStderr([&] {
        Logger logger(LogLevel::INFO);
        logger.info("api_key=secret123abc password=hunter2 authorization=Bearer tok123");
    });

    // The actual secret values must NOT appear in the output.
    assert(output.find("secret123abc") == std::string::npos);
    assert(output.find("hunter2") == std::string::npos);
    assert(output.find("tok123") == std::string::npos);
    // The key names should still be present (redaction replaces values only).
    assert(output.find("REDACTED") != std::string::npos);
    std::cout << "PASS: testSanitization\n";
}

// ---------------------------------------------------------------------------
// Test 6: File output – logger writes to a file in addition to console.
// ---------------------------------------------------------------------------
void testFileOutput()
{
    const std::string testFile = "test_logger_output.log";

    // Remove leftover file from a previous run.
    std::remove(testFile.c_str());

    {
        auto output = captureStderr([&] {
            Logger logger(LogLevel::INFO, testFile);
            logger.info("file-test-line");
        });
        // Should also appear on stderr.
        assert(output.find("file-test-line") != std::string::npos);
    }

    // Read the log file and verify.
    std::ifstream in(testFile);
    assert(in.is_open());
    std::string content((std::istreambuf_iterator<char>(in)),
                         std::istreambuf_iterator<char>());
    in.close();
    assert(content.find("file-test-line") != std::string::npos);
    assert(content.find("[INFO ]") != std::string::npos);

    std::remove(testFile.c_str());
    std::cout << "PASS: testFileOutput\n";
}

// ---------------------------------------------------------------------------
// Test 7: Log line format contains expected components.
// ---------------------------------------------------------------------------
void testLogLineFormat()
{
    auto output = captureStderr([&] {
        Logger logger(LogLevel::TRACE);
        Logger::setRequestId("fmt-check");
        logger.info("format test message");
        Logger::setRequestId("");
    });

    // Should contain ISO timestamp-like pattern.
    assert(output.find("T") != std::string::npos);
    assert(output.find("Z") != std::string::npos);
    // Level tag.
    assert(output.find("[INFO ]") != std::string::npos);
    // Thread ID.
    assert(output.find("[tid:") != std::string::npos);
    // Request ID.
    assert(output.find("[req:fmt-check]") != std::string::npos);
    // Message.
    assert(output.find("format test message") != std::string::npos);
    std::cout << "PASS: testLogLineFormat\n";
}

// ---------------------------------------------------------------------------
// Test 8: parseLogLevel helper.
// ---------------------------------------------------------------------------
void testParseLogLevel()
{
    assert(parseLogLevel("TRACE") == LogLevel::TRACE);
    assert(parseLogLevel("trace") == LogLevel::TRACE);
    assert(parseLogLevel("DEBUG") == LogLevel::DEBUG);
    assert(parseLogLevel("info")  == LogLevel::INFO);
    assert(parseLogLevel("WARN")  == LogLevel::WARN);
    assert(parseLogLevel("ERROR") == LogLevel::ERROR);
    assert(parseLogLevel("bogus") == LogLevel::INFO); // fallback
    assert(parseLogLevel("")      == LogLevel::INFO);
    std::cout << "PASS: testParseLogLevel\n";
}

// ---------------------------------------------------------------------------
// Test 9: Thread-local request-ID isolation between threads.
// ---------------------------------------------------------------------------
void testRequestIdThreadIsolation()
{
    Logger::setRequestId("main-thread-id");

    std::string childId;
    std::thread t([&childId] {
        // Child thread should start with an empty request ID.
        childId = Logger::getRequestId();
    });
    t.join();

    assert(childId.empty());
    assert(Logger::getRequestId() == "main-thread-id");
    Logger::setRequestId("");
    std::cout << "PASS: testRequestIdThreadIsolation\n";
}

// ---------------------------------------------------------------------------
int main()
{
    testLevelFiltering();
    testAllLevelsEmit();
    testRequestId();
    testNoRequestId();
    testSanitization();
    testFileOutput();
    testLogLineFormat();
    testParseLogLevel();
    testRequestIdThreadIsolation();

    std::cout << "\nAll logger tests passed!\n";
    return 0;
}

