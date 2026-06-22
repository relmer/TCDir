# Version System

## Overview

TCDir uses manual semantic versioning. The version is bumped by hand when cutting a release; there is no auto-incrementing build counter.

## Files

- **`TCDirCore/Version.h`** - Contains the version macros (`MAJOR.MINOR.PATCH`) and the copyright year.

## Version Format

The version follows semantic versioning: `MAJOR.MINOR.PATCH`

Example: `5.6.0`

- **MAJOR** - incompatible / milestone changes
- **MINOR** - backward-compatible feature additions
- **PATCH** - backward-compatible fixes

`VERSION_BUILD_TIMESTAMP` (the compiler's `__DATE__ " " __TIME__`) identifies an individual compile when that granularity is needed.

## Bumping the Version

1. Open `TCDirCore/Version.h`
2. Edit the relevant macro(s):
   ```cpp
   #define VERSION_MAJOR 5
   #define VERSION_MINOR 6
   #define VERSION_PATCH 0
   #define VERSION_YEAR  2026
   ```
3. Save and rebuild.

The version flows into the application (via `Version.h`) and the resource file (`TCDir/TCDir.rc`), and is shown in the `/?` usage/help text. The release workflow (`.github/workflows/release.yml`) validates that the git tag matches `Version.h`.

## Usage in Code

```cpp
#include "Version.h"

// Use VERSION_WSTRING for wide string contexts
console.ColorPuts (VERSION_WSTRING);  // Expands to L"5.6.0"
```