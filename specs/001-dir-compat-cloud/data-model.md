# Data Model: CMD Dir Compatibility & Cloud File Visualization

**Feature**: 001-dir-compat-cloud  
**Date**: 2026-01-24

## New Enumerations

### ETimeField (CommandLine.h)

```cpp
enum class ETimeField
{
    TF_WRITTEN,     // W - ftLastWriteTime (default)
    TF_CREATION,    // C - ftCreationTime
    TF_ACCESS       // A - ftLastAccessTime
};
```

**Usage**: Stored in `CCommandLine::m_timeField`. Affects:
- Date/time display in ResultsDisplayer classes
- Date sorting in FileComparator when `/O:D` is used

### ECloudStatus (derived, not stored)

```cpp
enum class ECloudStatus
{
    CS_NONE,        // Not a cloud file
    CS_CLOUD_ONLY,  // Placeholder, not locally available
    CS_LOCAL,       // Available locally, can be dehydrated
    CS_PINNED       // Pinned, always available locally
};
```

**Usage**: Computed from file attributes at display time. Not stored in CCommandLine.

**Computation logic**:
```cpp
ECloudStatus GetCloudStatus(DWORD dwFileAttributes)
{
    if (dwFileAttributes & FILE_ATTRIBUTE_PINNED)
        return CS_PINNED;
    if (dwFileAttributes & (FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS | 
                            FILE_ATTRIBUTE_RECALL_ON_OPEN | 
                            FILE_ATTRIBUTE_OFFLINE))
        return CS_CLOUD_ONLY;
    if (dwFileAttributes & FILE_ATTRIBUTE_UNPINNED)
        return CS_LOCAL;
    return CS_NONE;
}
```

---

## Extended Attribute Arrays (CommandLine.cpp)

### Current State

```cpp
static constexpr WCHAR s_kszAttributes[] = L"dhsratecp0";
static constexpr DWORD s_kdwAttributes[] = { 
    FILE_ATTRIBUTE_DIRECTORY,       // d
    FILE_ATTRIBUTE_HIDDEN,          // h
    FILE_ATTRIBUTE_SYSTEM,          // s
    FILE_ATTRIBUTE_READONLY,        // r
    FILE_ATTRIBUTE_ARCHIVE,         // a
    FILE_ATTRIBUTE_TEMPORARY,       // t
    FILE_ATTRIBUTE_ENCRYPTED,       // e
    FILE_ATTRIBUTE_COMPRESSED,      // c
    FILE_ATTRIBUTE_REPARSE_POINT,   // p
    FILE_ATTRIBUTE_SPARSE_FILE      // 0
};
```

### New State

```cpp
static constexpr WCHAR s_kszAttributes[] = L"dhsratecp0xoibfu";
static constexpr DWORD s_kdwAttributes[] = { 
    FILE_ATTRIBUTE_DIRECTORY,            // d
    FILE_ATTRIBUTE_HIDDEN,               // h
    FILE_ATTRIBUTE_SYSTEM,               // s
    FILE_ATTRIBUTE_READONLY,             // r
    FILE_ATTRIBUTE_ARCHIVE,              // a
    FILE_ATTRIBUTE_TEMPORARY,            // t
    FILE_ATTRIBUTE_ENCRYPTED,            // e
    FILE_ATTRIBUTE_COMPRESSED,           // c
    FILE_ATTRIBUTE_REPARSE_POINT,        // p
    FILE_ATTRIBUTE_SPARSE_FILE,          // 0
    FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,  // x
    0,                                   // o (composite - special handling)
    FILE_ATTRIBUTE_INTEGRITY_STREAM,     // i
    FILE_ATTRIBUTE_NO_SCRUB_DATA,        // b
    FILE_ATTRIBUTE_PINNED,               // f
    FILE_ATTRIBUTE_UNPINNED              // u
};
```

**Note**: `/A:O` requires special handling because it's a composite of three attributes. The AttributeHandler will need modification to OR together OFFLINE | RECALL_ON_OPEN | RECALL_ON_DATA_ACCESS when 'o' is encountered.

---

## New CCommandLine Members

```cpp
// In CommandLine.h, add to public members section:

ETimeField         m_timeField     = ETimeField::TF_WRITTEN;  // /T: switch
bool               m_fShowOwner    = false;                    // --owner switch
bool               m_fShowStreams  = false;                    // --streams switch
```

---

## New CConfig::EAttribute Entries

```cpp
// In Config.h, add to EAttribute enum:

enum EAttribute
{
    // ... existing entries ...
    
    CloudStatusCloudOnly,    // ☁ symbol color (configurable, default: bright blue)
    CloudStatusLocal,        // ✓ symbol color (configurable, default: bright green)
    CloudStatusPinned,       // ● symbol color (configurable, default: green)
    
    __count
};
```

**Default colors** (user-configurable via TCDIR env var):
- `CloudStatusCloudOnly`: `FOREGROUND_BLUE | FOREGROUND_INTENSITY` (bright blue)
- `CloudStatusLocal`: `FOREGROUND_GREEN | FOREGROUND_INTENSITY` (bright green)
- `CloudStatusPinned`: `FOREGROUND_GREEN` (green, non-bright)

**TCDIR env var format** (follows existing `<Item>=<Fore> [on <Back>]` pattern):
```
$env:TCDIR = "CloudOnly=LightBlue;Local=LightGreen;Pinned=Green"
```
Or in cmd:
```
set TCDIR=CloudOnly=LightBlue;Local=LightGreen;Pinned=Green
```

These new items will be added to the `<Item>` list in `--Env` help output.

---

## CDriveInfo Helper Methods

```cpp
// In DriveInfo.h, add to public methods:

bool IsNTFS() const { return _wcsicmp(m_szFileSystemName, L"NTFS") == 0; }
bool IsReFS() const { return _wcsicmp(m_szFileSystemName, L"ReFS") == 0; }
```

---

## Alternate Data Stream Structure

```cpp
// For --streams display, parsed from FindFirstStreamW/FindNextStreamW

struct AltDataStream
{
    wstring     name;   // Stream name (e.g., "Zone.Identifier")
    LONGLONG    size;   // Stream size in bytes
};
```

**Note**: This is a local structure used only during display. Not persisted or stored in CDirectoryInfo.

---

## Validation Rules

| Field | Validation | Error Handling |
|-------|------------|----------------|
| `/T:` value | Must be C, A, or W | Return E_INVALIDARG |
| `/A:I`, `/A:B` | Target volume must be ReFS | Display warning, continue |
| `--streams` | Target volume must be NTFS | Display warning, continue |
| `--owner` lookup | May fail with access denied | Display "Unknown" for that file |
