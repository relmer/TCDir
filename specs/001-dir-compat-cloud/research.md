# Research: CMD Dir Compatibility & Cloud File Visualization

**Feature**: 001-dir-compat-cloud  
**Date**: 2026-01-24

## Cloud File Attributes

### Decision: Use standard Windows file attributes for cloud status detection

**Rationale**: Windows Cloud Files API (cfapi) uses standard `FILE_ATTRIBUTE_*` flags that are returned by `FindFirstFile`/`FindNextFile`. No additional API calls needed for attribute-based filtering or status display.

**Alternatives considered**:
- Cloud Filter API (`CfGetPlaceholderStateFromFindData`) — more precise but requires cfapi.h and adds complexity
- Direct registry/config detection of sync providers — too fragile

### Attribute Mappings

| Cloud Status | Attributes | Display |
|--------------|------------|---------|
| Cloud-only (placeholder) | `RECALL_ON_DATA_ACCESS` or `RECALL_ON_OPEN` or `OFFLINE` | ☁ (blue) |
| Locally available | `UNPINNED` and no RECALL attributes | ✓ (green) |
| Pinned (always local) | `PINNED` | ● (dark green) |
| Not a cloud file | None of the above | (no symbol) |

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
