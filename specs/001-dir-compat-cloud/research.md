# Research: CMD Dir Compatibility & Cloud File Visualization

**Feature**: 001-dir-compat-cloud  
**Date**: 2026-01-24  
**Updated**: 2026-01-26

## Cloud File Attributes

### Decision: Use Hybrid Approach - Attributes + Sync Root Detection

**Rationale**: OneDrive removes all placeholder metadata from fully-hydrated files, making them indistinguishable from regular local files via attributes or `CfGetPlaceholderStateFromFindData`. The solution is to combine:
1. File attribute checks for explicit cloud states (PINNED, UNPINNED, RECALL_*, OFFLINE)
2. Sync root detection via `CfGetSyncRootInfoByPath` (called once per directory)

**Key Discovery**: Files marked "Available on this device" in Explorer have only `FILE_ATTRIBUTE_ARCHIVE` (0x20) - no cloud bits whatsoever. `CfGetPlaceholderStateFromFindData` returns `CF_PLACEHOLDER_STATE_NO_STATES` for these files because OneDrive fully strips placeholder reparse data.

**Implementation**:
1. Call `CfGetSyncRootInfoByPath(dirPath, ...)` once per directory
2. If it succeeds → directory is under a cloud sync root (OneDrive, etc.)
3. For each file in that directory:
   - `PINNED` attribute → Pinned (●)
   - `RECALL_ON_*` or `OFFLINE` → Cloud-only (○)
   - `UNPINNED` attribute → Local (◐)
   - No cloud bits BUT in sync root → Local (◐) ← **This is the key fix**
   - No cloud bits AND not in sync root → Not a cloud file (blank)

**Requirements**: Windows 10 1709+, `#include <cfapi.h>`, `#pragma comment(lib, "cldapi.lib")`

### Attribute Mappings (Final)

| Cloud Status | Detection Method | Display |
|--------------|------------------|---------|
| Pinned (always local) | `FILE_ATTRIBUTE_PINNED` | ● (green) |
| Cloud-only (placeholder) | `RECALL_ON_*` or `OFFLINE` attributes | ○ (blue) |
| Locally synced | `UNPINNED` attr OR (no attrs + in sync root) | ◐ (green) |
| Not a cloud file | No cloud attrs + not in sync root | (no symbol) |

### APIs Used

```cpp
// Once per directory - check if under a cloud sync root
CF_SYNC_ROOT_BASIC_INFO info = {};
HRESULT hr = CfGetSyncRootInfoByPath(pszPath, CF_SYNC_ROOT_INFO_BASIC, &info, sizeof(info), nullptr);
bool fInSyncRoot = SUCCEEDED(hr);

// Per-file - check explicit cloud attributes
if (dwAttr & FILE_ATTRIBUTE_PINNED)           → CS_PINNED
else if (dwAttr & (RECALL_ON_* | OFFLINE))    → CS_CLOUD_ONLY  
else if (dwAttr & FILE_ATTRIBUTE_UNPINNED)    → CS_LOCAL
else if (fInSyncRoot)                         → CS_LOCAL  // Key: hydrated files in sync root
else                                          → CS_NONE
```

### Attribute Constants (from Windows SDK)

```cpp
#define FILE_ATTRIBUTE_OFFLINE                  0x00001000  // O
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED      0x00002000  // X
#define FILE_ATTRIBUTE_INTEGRITY_STREAM         0x00008000  // I (ReFS)
#define FILE_ATTRIBUTE_NO_SCRUB_DATA            0x00020000  // B (ReFS)
#define FILE_ATTRIBUTE_PINNED                   0x00080000  // F
#define FILE_ATTRIBUTE_UNPINNED                 0x00100000  // U
#define FILE_ATTRIBUTE_RECALL_ON_OPEN           0x00040000  // (part of O composite)
#define FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS    0x00400000  // (part of O composite)
```

**Note**: `FILE_ATTRIBUTE_RECALL_ON_OPEN` and `FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS` are combined into `/A:O` for user simplicity. The `/A:O` filter matches files with ANY of: OFFLINE, RECALL_ON_OPEN, or RECALL_ON_DATA_ACCESS.

---

## Time Field Selection

### Decision: Add `ETimeField` enum to CCommandLine, use in FileComparator and ResultsDisplayer

**Rationale**: `WIN32_FIND_DATA` already contains `ftCreationTime`, `ftLastAccessTime`, and `ftLastWriteTime`. No additional API calls needed.

**Implementation**:
```cpp
enum class ETimeField
{
    TF_CREATION,    // C - ftCreationTime
    TF_ACCESS,      // A - ftLastAccessTime
    TF_WRITTEN      // W - ftLastWriteTime (default)
};
```

**Alternatives considered**:
- Separate flags for display vs sort — adds complexity; CMD `dir` uses single `/T:` for both

---

## File Ownership (`--owner`)

### Decision: Use `GetNamedSecurityInfoW` + `LookupAccountSidW`

**Rationale**: Standard Windows Security API. Returns owner in DOMAIN\User format.

**API sequence**:
1. `GetNamedSecurityInfoW(path, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &pSidOwner, ...)`
2. `LookupAccountSidW(NULL, pSidOwner, szAccountName, &cchAccountName, szDomainName, &cchDomainName, &sidType)`
3. Format as `DOMAIN\AccountName`

**Performance**: ~1ms per file on local NTFS. Opt-in only to avoid impacting normal listings.

**Error handling**: If lookup fails (access denied, orphaned SID), display "Unknown" or the raw SID string.

**Alternatives considered**:
- `icacls` subprocess — too slow, parsing overhead
- WMI queries — heavyweight, unnecessary

---

## Alternate Data Streams (`--streams`)

### Decision: Use `FindFirstStreamW` / `FindNextStreamW`

**Rationale**: Native NTFS stream enumeration API. Returns stream name and size.

**API**:
```cpp
HANDLE hFind = FindFirstStreamW(szFilePath, FindStreamInfoStandard, &streamData, 0);
// streamData.cStreamName contains L":streamname:$DATA"
// streamData.StreamSize contains the stream size
```

**Display format**: Show each non-default stream indented below the file:
```
  document.docx               125 KB   2026-01-15
                    :Zone.Identifier:$DATA    26 bytes
```

**NTFS-only**: Must check filesystem type via `CDriveInfo::GetFileSystemName()`. Display warning and skip if not "NTFS".

**Alternatives considered**:
- BackupRead API — more complex, designed for backup scenarios
- Direct NTFS parsing — way too complex

---

## Filesystem Detection

### Decision: Use existing `CDriveInfo::GetFileSystemName()` 

**Rationale**: Already implemented. Returns "NTFS", "ReFS", "FAT32", etc.

**Usage**:
- `--streams`: Warn and skip if not "NTFS"
- `/A:I`, `/A:B`: Warn and skip if not "ReFS"

**Implementation**: Add helper methods to CDriveInfo:
```cpp
bool IsNTFS() const { return wcscmp(m_szFileSystemName, L"NTFS") == 0; }
bool IsReFS() const { return wcscmp(m_szFileSystemName, L"ReFS") == 0; }
```

---

## Unicode Symbols

### Decision: Use Unicode symbols only (no ASCII fallback)

**Symbols**:
- ☁ (U+2601) — Cloud-only file
- ✓ (U+2713) — Locally available  
- ● (U+25CF) — Pinned (always local)

---

## Switch Prefix Handling

### Decision: Follow existing patterns for `-`/`/` fungibility

**Existing pattern** (from CommandLine.cpp):
- Single-char switches: `-X` or `/X` are equivalent
- Multi-char switches: `--name` (double dash) or `/name` (single slash) are equivalent
- Disable suffix: `-X-` disables the flag

**New switches**:
- `/T:C`, `/T:A`, `/T:W` — single char with value, follows `/O:` pattern
- `--owner`, `--streams` — multi-char, follows `--env`, `--config` pattern
- `/A:X`, `/A:O`, etc. — single char attributes, extends existing `/A:` handler
