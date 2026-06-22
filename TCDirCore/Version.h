#pragma once

// Version information for TCDir
//
// Semantic versioning: MAJOR.MINOR.PATCH. All three are bumped
// manually. VERSION_YEAR tracks the copyright year. There is no
// auto-incrementing build counter -- VERSION_BUILD_TIMESTAMP below
// identifies an individual compile when that granularity is needed.

#define VERSION_MAJOR 5
#define VERSION_MINOR 6
#define VERSION_PATCH 0
#define VERSION_YEAR 2026

// Helper macros for stringification
#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)

// Full version string as wide string (e.g., L"3.2.1")
#define VERSION_WSTRING WIDEN(STRINGIFY(VERSION_MAJOR)) L"." WIDEN(STRINGIFY(VERSION_MINOR)) L"." WIDEN(STRINGIFY(VERSION_PATCH))

// Build timestamp as wide string (uses compiler's __DATE__ and __TIME__)
#define VERSION_BUILD_TIMESTAMP WIDEN(__DATE__) L" " WIDEN(__TIME__)

// Current year as wide string (e.g., L"2025")
#define VERSION_YEAR_WSTRING WIDEN(STRINGIFY(VERSION_YEAR))
