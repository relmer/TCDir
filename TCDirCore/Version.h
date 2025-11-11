#pragma once

// Version information for TCDir
// The build number and year are automatically updated by the pre-build script

#define VERSION_MAJOR 4
#define VERSION_MINOR 0
#define VERSION_BUILD 1000
#define VERSION_YEAR 2025

// Helper macros for stringification
#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)

// Full version string as wide string (e.g., L"3.2.1000")
#define VERSION_WSTRING WIDEN(STRINGIFY(VERSION_MAJOR)) L"." WIDEN(STRINGIFY(VERSION_MINOR)) L"." WIDEN(STRINGIFY(VERSION_BUILD))

// Current year as wide string (e.g., L"2025")
#define VERSION_YEAR_WSTRING WIDEN(STRINGIFY(VERSION_YEAR))
