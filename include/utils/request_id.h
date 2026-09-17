#pragma once

#include <string>

// ---------------------------------------------------------------------------
// RequestId – lightweight UUID-v4 generator for HTTP request correlation.
//
// Each incoming request gets a unique ID that is threaded through every log
// line and can be returned to the caller in a response header for debugging.
//
// Implementation uses <random> (no Boost dependency) and produces a standard
// 8-4-4-4-12 hex string, e.g. "3f2504e0-4f89-11d3-9a0c-0305e82c3301".
// ---------------------------------------------------------------------------
namespace RequestId {

/// Generate a new random UUID-v4 string.
std::string generate();

}

