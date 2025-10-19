# Automatic Build Version System

## Overview

TCDir uses an automatic build number incrementer that updates the version with each build.

## Files

- **`TCDirCore/Version.h`** - Contains version macros (MAJOR.MINOR.BUILD)
- **`IncrementVersion.ps1`** - PowerShell script that increments the build number

## Version Format

The version follows semantic versioning: `MAJOR.MINOR.BUILD`

Example: `1.0.123`

## How It Works

1. Before each build, the pre-build event runs `IncrementVersion.ps1`
2. The script reads `Version.h` and increments `VERSION_BUILD`
3. The updated version is compiled into the application
4. The version is displayed in the usage/help text

## Setting Up Pre-Build Event

**Note:** This should already be configured in the project, but if you need to add it manually:

1. Right-click the **TCDir** or **TCDirCore** project → **Properties**
2. Navigate to **Build Events** → **Pre-Build Event**
3. Add the following command:

```cmd
powershell -ExecutionPolicy Bypass -File "$(SolutionDir)IncrementVersion.ps1"
```

## Manual Version Control

To manually set the version:

1. Open `TCDirCore/Version.h`
2. Edit the version numbers:
   ```cpp
   #define VERSION_MAJOR 1
   #define VERSION_MINOR 0
   #define VERSION_BUILD 0
   ```
3. Save and rebuild

## Usage in Code

```cpp
#include "Version.h"

// Use VERSION_WSTRING for wide string contexts
static LPCWSTR s_usageLines[] = {
    L"My Application",
    VERSION_WSTRING,  // Expands to L"1.0.123"
    // ...
};
```

## Troubleshooting

### Script Execution Error
If you see PowerShell execution policy errors:
```powershell
Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
```

### Version Not Incrementing
1. Check that the pre-build event is configured
2. Verify `IncrementVersion.ps1` exists in the solution directory
3. Check the Build Output window for script errors

### Build Fails After Adding Version
1. Ensure `#include "Version.h"` is added to files that use version macros
2. Rebuild the entire solution (not just Build)
