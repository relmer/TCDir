# Developer Quickstart: CMD Dir Compatibility & Cloud File Visualization

**Feature**: 001-dir-compat-cloud  
**Date**: 2026-01-24

## Prerequisites

- Visual Studio 2026 (MSVC v145+)
- Windows SDK 10.0.22621.0 or later (for cloud attribute constants)
- ARM64 or x64 build environment

## Build & Test

```powershell
# Build solution
msbuild TCDir.sln /p:Configuration=Debug /p:Platform=ARM64

# Run unit tests
.\scripts\RunTests.ps1 -Platform ARM64 -Configuration Debug
```

---

## Implementation Guide

### Phase 1: Cloud File Visualization (US-001, US-002)

#### Step 1: Add Cloud Status Display

**File**: `TCDirCore/ResultsDisplayerNormal.cpp`

1. Add helper function to compute cloud status:
   ```cpp
   static ECloudStatus GetCloudStatus(DWORD dwFileAttributes);
   ```

2. Add helper to display cloud symbol with color:
   ```cpp
   static void DisplayCloudStatusSymbol(CConsole* pConsole, CConfig* pConfig, ECloudStatus status);
   ```

3. Modify `DisplayResultsNormalFileSize` to insert cloud column after size, before filename.

**Testing**: Create OneDrive test files with Files On-Demand enabled. Verify ☁/✓/● symbols appear correctly.

#### Step 2: Add Cloud Attribute Filtering

**File**: `TCDirCore/CommandLine.cpp`

1. Extend `s_kszAttributes[]` to include: `o`, `f`, `u`
2. Extend `s_kdwAttributes[]` with corresponding values
3. Add special handling for `/A:O` (composite attribute)

**Testing**: Unit tests in `CommandLineTests.cpp` for new attribute parsing.

---

### Phase 2: Extended Attributes (US-004)

#### Step 1: Add New Attribute Letters

**File**: `TCDirCore/CommandLine.cpp`

1. Add `x`, `i`, `b` to `s_kszAttributes[]`
2. Add corresponding `FILE_ATTRIBUTE_*` values to `s_kdwAttributes[]`

#### Step 2: Add Filesystem Validation

**File**: `TCDirCore/CommandLine.cpp`

1. Check if `/A:I` or `/A:B` is used
2. Call `CDriveInfo::IsReFS()` for target path
3. Display warning if not ReFS (but continue execution)

**Testing**: Run on NTFS volume with `/A:I`, verify warning appears.

---

### Phase 3: Time Field Selection (US-003)

#### Step 1: Add /T: Switch Parsing

**File**: `TCDirCore/CommandLine.cpp`

1. Add `m_timeField` member to `CCommandLine`
2. Parse `/T:C`, `/T:A`, `/T:W` in ParseArguments

#### Step 2: Modify Date Display

**File**: `TCDirCore/ResultsDisplayerNormal.cpp`

1. Modify `DisplayResultsNormalDateAndTime` to accept `ETimeField` parameter
2. Select appropriate `ftXxxTime` from `WIN32_FIND_DATA`

#### Step 3: Modify Date Sorting

**File**: `TCDirCore/FileComparator.cpp`

1. Modify `SO_DATE` handling to use selected time field

**Testing**: Unit tests comparing date output with different `/T:` values.

---

### Phase 4: Ownership (US-005)

#### Step 1: Add --owner Switch

**File**: `TCDirCore/CommandLine.cpp`

1. Add `m_fShowOwner` member
2. Parse `--owner` and `-owner` variants

#### Step 2: Add Owner Lookup

**File**: `TCDirCore/ResultsDisplayerNormal.cpp`

1. Add `GetFileOwner(path)` helper using:
   - `GetNamedSecurityInfoW` with `OWNER_SECURITY_INFORMATION`
   - `LookupAccountSidW` to resolve SID to name
2. Display owner between date/time and filename

**Testing**: Compare output to `dir /Q` for same directory.

---

### Phase 5: Alternate Data Streams (US-006)

#### Step 1: Add --streams Switch

**File**: `TCDirCore/CommandLine.cpp`

1. Add `m_fShowStreams` member
2. Parse `--streams` and `-streams` variants

#### Step 2: Add NTFS Validation

**File**: `TCDirCore/TCDir.cpp` (or appropriate validation point)

1. Check if `--streams` is enabled
2. Call `CDriveInfo::IsNTFS()` for target path
3. Display warning if not NTFS (but continue)

#### Step 3: Add Stream Enumeration

**File**: `TCDirCore/ResultsDisplayerNormal.cpp`

1. Add `EnumerateStreams(path)` using `FindFirstStreamW`/`FindNextStreamW`
2. Display indented lines below main file entry for each alternate stream

**Testing**: 
- Download file from internet (should have Zone.Identifier stream)
- Run with `--streams` flag
- Verify ADS appears indented below main file

---

## Key Patterns to Follow

### Error Handling

Use EHM macros consistently:
```cpp
HRESULT hr = S_OK;
// ... code using CHR, CBR, etc. ...
Error:
    return hr;
```

### Precompiled Headers

- Include `"pch.h"` as FIRST include in every .cpp file
- Add any new system headers to `pch.h`, not individual files
- Use quoted includes for project headers: `"Config.h"`

### Switch Parsing

Follow existing pattern in `CommandLine.cpp`:
- `/` and `-` are interchangeable for single-letter switches
- `--` prefix for long-form switches
- Use `_wcsnicmp` for case-insensitive matching

### Console Output

Use `CConsole` class methods:
- `Printf` for formatted output
- `SetAttributes` to change colors
- Follow existing patterns in `ResultsDisplayerNormal.cpp`

---

## Testing Strategy

| Component | Test File | Test Type |
|-----------|-----------|-----------|
| Switch parsing | `CommandLineTests.cpp` | Unit |
| Attribute filtering | `CommandLineTests.cpp` | Unit |
| Date field selection | `FileComparatorTests.cpp` | Unit |
| Cloud status display | `ResultsDisplayerTests.cpp` | Unit |
| Owner display | `ResultsDisplayerTests.cpp` | Unit |
| Stream enumeration | `ResultsDisplayerTests.cpp` | Unit |
| Filesystem validation | Integration | Manual |

---

## Common Issues

### "Undefined symbol FILE_ATTRIBUTE_PINNED"
**Solution**: Update Windows SDK to 10.0.22621.0 or define constant manually:
```cpp
#ifndef FILE_ATTRIBUTE_PINNED
#define FILE_ATTRIBUTE_PINNED 0x80000
#endif
```

### Cloud symbols not displaying
**Solution**: Ensure console output is UTF-8. Check `CConsole::SetMode` is called with appropriate flags.

### Owner lookup returns "Unknown"
**Solution**: This is expected for files where access is denied. Not an error condition.

### Streams not found on downloaded files
**Solution**: Ensure file is on NTFS volume. Some antivirus software strips Zone.Identifier stream.
