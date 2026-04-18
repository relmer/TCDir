# Data Model: Symlink & Junction Target Display

**Date**: 2026-04-18
**Feature**: 007-symlink-junction-targets

## Entities

### FileInfo (modified)

Existing struct in `DirectoryInfo.h`. Inherits from `WIN32_FIND_DATA`.

| Field | Type | Source | New? |
|-------|------|--------|------|
| `cFileName` | `WCHAR[MAX_PATH]` | `WIN32_FIND_DATA` | No |
| `dwFileAttributes` | `DWORD` | `WIN32_FIND_DATA` | No |
| `dwReserved0` | `DWORD` | `WIN32_FIND_DATA` (reparse tag) | No |
| `m_vStreams` | `vector<SStreamInfo>` | Stream enumeration | No |
| `m_strLowerName` | `wstring` | Constructed from `cFileName` | No |
| **`m_strReparseTarget`** | **`wstring`** | **Reparse buffer parsing** | **Yes** |

**Rules:**
- `m_strReparseTarget` is empty for non-reparse files (zero overhead)
- Populated only when `dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT` is set AND `dwReserved0` is one of: `IO_REPARSE_TAG_MOUNT_POINT`, `IO_REPARSE_TAG_SYMLINK`, `IO_REPARSE_TAG_APPEXECLINK`
- Contains the display-ready target path (PrintName for junctions/symlinks, target exe for AppExecLinks)
- `\??\` prefix already stripped before storage

### SReparseDataBuffer (new, internal)

Manual definition of the Windows kernel `REPARSE_DATA_BUFFER` struct for user-mode use.

| Field | Type | Notes |
|-------|------|-------|
| `ReparseTag` | `ULONG` | `IO_REPARSE_TAG_*` value |
| `ReparseDataLength` | `USHORT` | Byte count of data after header |
| `Reserved` | `USHORT` | Must be 0 |
| `SymbolicLinkReparseBuffer` | struct | For `IO_REPARSE_TAG_SYMLINK` |
| `MountPointReparseBuffer` | struct | For `IO_REPARSE_TAG_MOUNT_POINT` |
| `GenericReparseBuffer` | struct | For AppExecLink and others |

**Not exposed outside `ReparsePointResolver.cpp`** — implementation detail only.

## State Transitions

None. `m_strReparseTarget` is write-once (populated during enumeration, read during display, never modified).

## Relationships

```
CDirectoryInfo (1) ──contains──► FileInfo (many)
    │                               │
    │ m_dirPath                     │ m_strReparseTarget (new)
    │                               │ dwReserved0 (reparse tag)
    │                               │ dwFileAttributes (REPARSE_POINT flag)
    ▼                               │
AddMatchToList() ──calls──► ResolveReparseTarget(dirPath, fileInfo)
                                    │
                                    ▼
                            CreateFileW + DeviceIoControl
                                    │
                                    ▼
                            ParseSymlinkBuffer() / ParseJunctionBuffer() / ParseAppExecLinkBuffer()
```

## Validation Rules

- Reparse tag must be one of the three supported values; all others → empty target
- `PrintName` length must be > 0 for junctions/symlinks; if empty, fall back to SubstituteName with `\??\` stripped
- AppExecLink version must be 3; all string reads must stay within buffer bounds
- `CreateFileW` and `DeviceIoControl` failures → empty target (silent)
