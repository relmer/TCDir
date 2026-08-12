# Phase 1 Data Model — Cross-Platform Support

Entities and relationships for the PAL boundary and the entry model. "Core" =
`TCDirCore` (platform-agnostic). "PAL" = platform backend.

## FileEntry (the enriched directory entry)

The unit produced in bulk by the PAL and consumed by the core for sort/display.

| Field | Owner | Notes |
|---|---|---|
| `name` | Core-readable | Wide name (UTF-16 Windows / UTF-32 POSIX). Native storage. |
| `size` | Core-readable | 64-bit byte size. Accessor; native source (`nFileSize*` / `st_size`). |
| `lastWriteNs`, `creationNs`, `lastAccessNs` | Core-readable | Portable times (int64 ns); native source (`FILETIME` / `timespec`). |
| `isDirectory` | PAL-set, core-readable | Structural; drives sort/recursion/layout/counts. |
| `isSymlink` | PAL-set, core-readable | Structural; drives link display + loop avoidance. |
| `linkTarget` | PAL-set, core-readable | Resolved in the enumeration pass for symlink entries (empty otherwise). |
| `attrToken` | **Opaque** (PAL) | Handed back to the PAL for column render / late queries. Core never introspects. |
| `themedMatchSet` | PAL-set, core-readable | Compact bitset over registered theme rule-handles (see ThemeRule). For local color/icon resolution — no per-entry PAL call. |
| `nativeBlob` | PAL-internal | `WIN32_FIND_DATAW` (Win) or `{name, struct stat, linkTag}` (POSIX). Backing store for accessors. |

**Validation/derivation rules**
- Times convert lazily in the accessor (no eager epoch math stored).
- `isDirectory`/`isSymlink` set from `dwFileAttributes`/`d_type`/`st_mode` during enumeration.
- POSIX "hidden" is *not* a FileEntry field — it is an attribute resolved by the PAL (dotfile or `UF_HIDDEN`).

## DirectoryListing

| Field | Notes |
|---|---|
| `dirPath` | The enumerated directory. |
| `entries` | `vector<FileEntry>` filled once by the PAL (maps to today's `m_vMatches`). |
| `status` | `HRESULT` — partial-failure tolerant (e.g. permission denied mid-scan). |

Relationship: one `DirectoryListing` per directory work-item; the core's
work-queue/threading produces many, recursing via each listing's directory entries.

## FilterContext (passed into bulk enumeration)

| Field | Notes |
|---|---|
| `mask` | Filename pattern to apply (when the core wants the PAL to match; quoted/recursive/dir-arg cases). |
| `attrFilter` | Opaque parsed `/A` filter spec (from PAL ParseAttributeFilter). |
| `themeRules` | The registered theme rule-handles, so the PAL can compute each entry's `themedMatchSet` in-pass. |
| `caseMatch` | Matching policy = case-sensitive (OS-native) per D8. |

## AttributeFilterSpec (opaque, PAL-owned)

- Produced by `ParseAttributeFilter(userArg)`; consumed by `Matches(entry, spec)`
  (typically pre-applied during enumeration). Core stores/passes it; never inspects.

## ThemeRule (the name-addressed theming bridge)

| Field | Owner | Notes |
|---|---|---|
| `attributeName` | Core (from config) | E.g. `"Hidden"`, `"Encrypted"`, `"Executable"`. |
| `handle` | PAL | Returned by `ValidateAttributeRule(name)`; invalid name → config error. |
| `color` / `icon` | Core (from config) | The theme payload the core owns. |
| `priority` | Core | Resolution order (first match wins), preserving existing icon priority. |

Relationship: Core builds a priority-ordered `ThemeRule[]` at config load; the
handles go into `FilterContext.themeRules`; per entry the core walks the list
against `FileEntry.themedMatchSet` locally.

## PlatformProfile

| Field | Notes |
|---|---|
| `availableColumns` | Which columns apply (e.g. cloud column hidden on Linux — FR-018). |
| `volumeHeaderMode` | Suppressed by default; opt-in POSIX header (mountpoint + fs) — FR-019. |
| `treeFooterMode` | Concise (dirs, files) by default; switch for full summary — FR-020. |
| `capabilities` | Streams? cloud? font-install? — gated features (D6). |

## VolumeInfo (per listing root)

| Field | Windows | POSIX |
|---|---|---|
| `freeSpace` | `GetDiskFreeSpaceEx` | `statvfs` |
| `label`/`serial` | `GetVolumeInformation` | n/a (omitted) |
| `mountpoint`/`fsType` | n/a | `statvfs`/mount table (opt-in header) |

## OwnerInfo

| Field | Windows | POSIX |
|---|---|---|
| `displayName` | SID → `LookupAccountSidW` | `st_uid`/`st_gid` → `getpwuid_r`/`getgrgid_r` |
