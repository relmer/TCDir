# Research: Archive Browsing

**Feature**: 012-archive-browsing | **Date**: 2026-11-05

## R1. Which Windows-provided archive readers exist?

**Findings** (verified on Windows 11 build 26300, x64):

| Reader | Availability | Formats | Notes |
|--------|--------------|---------|-------|
| `archiveint.dll` (System32) | Windows 11 23H2 and later. Absent on Windows 10. | 7z, rar, rar5, tar (all variants), gzip/bzip2/xz/zstd/lz4/lzma filters, cab, cpio, xar, lha, ar, iso9660, zip | A Microsoft build of libarchive 3.8.x (`archive_version_string` returns `3.8.9 (WinBuild...)`). It exports the standard libarchive C API: `archive_read_new`, `archive_read_support_format_all`, `archive_read_support_filter_all`, `archive_read_open_filename_w`, `archive_read_next_header`, `archive_read_data_skip`, `archive_entry_pathname_w`, `archive_entry_size`, `archive_entry_mtime`, `archive_entry_filetype`, `archive_entry_is_encrypted`, `archive_error_string`, `archive_read_free`. No import library or header ships in the SDK, so the DLL is loaded at run time with `LoadLibraryExW (..., LOAD_LIBRARY_SEARCH_SYSTEM32)` and `GetProcAddress`. Explorer and `tar.exe` use the same DLL. |
| Shell zip folder (`zipfldr.dll`) via `IShellFolder` | All supported Windows versions | zip only | COM, slow on large archives (one shell item per entry), no access to raw attributes, requires COM initialization on every worker thread. |
| IMAPI2FS (`IFileSystemImage::ImportFileSystem`) | All versions | ISO9660, Joliet, UDF | Imports from an optical recorder or a multisession stream, not from an arbitrary .iso file path. Not suitable. |
| Virtual disk mount (`AttachVirtualDisk`) | All versions | iso, vhd | Mounts a volume. Violates FR-012 (no change to system state). Rejected. |

## R2. Reader for .zip

**Decision**: Parse the .zip central directory in tcdir (`CZipReader`), with no OS dependency.

**Rationale**:
- A listing needs only the central directory. No decompression codec is involved, so "use Windows' codecs" has no benefit for .zip.
- Works on Windows 10, where `archiveint.dll` is absent. .zip is the P1 format and must work everywhere.
- The parser operates on a byte-source interface, so unit tests feed synthetic in-memory archives (constitution II).
- Reading the central directory is a single seek plus one read: 10,000 entries in well under a second (SC-003).

**Format details used**:
- End of central directory (EOCD, signature `0x06054B50`) is located by a backward scan of the last 65,557 bytes (22-byte record plus a comment of up to 65,535 bytes).
- ZIP64: EOCD locator `0x07064B50` and ZIP64 EOCD `0x06064B50` supply 64-bit entry count, directory size and offset. The ZIP64 extra field (`0x0001`) supplies 64-bit sizes per entry.
- Central directory header `0x02014B50`. General-purpose flag bit 0 indicates encryption; bit 11 indicates UTF-8 names. Without bit 11, names are decoded with the Info-ZIP Unicode Path extra field (`0x7075`) when present, otherwise with the OEM code page (CP437 by convention).
- Times: the NTFS extra field (`0x000A`, tag `0x0001`) supplies a UTC FILETIME. Otherwise the DOS date/time is a local time and is converted to UTC with `TzSpecificLocalTimeToSystemTime`, which applies the DST rules in effect on that date.
- Attributes: when the "version made by" host is MS-DOS (0), NTFS (10) or VFAT (14), the low byte of the external attributes contains the DOS attributes. When the host is Unix (3), the high 16 bits contain the Unix mode; `S_IFDIR` indicates a directory, and the absence of owner write permission maps to read-only.
- A trailing `/` or `\` in the name indicates a directory entry.

**Alternatives considered**: `zipfldr` shell namespace (slow, COM, no attributes); `archiveint.dll` for .zip too (absent on Windows 10).

## R3. Reader for .iso

**Decision**: Parse ISO 9660 with the Joliet extension, and UDF, in tcdir (`CIsoReader`).

**Rationale**:
- No Windows API reads an .iso file without mounting it (R1).
- libarchive in `archiveint.dll` reads ISO 9660, Joliet and Rock Ridge but has no UDF support. Windows installation media and most modern disc images are UDF with a minimal ISO 9660 "bridge" tree that contains only a README. A libarchive-only reader would list the README instead of the real contents.
- Reading the file system metadata is a few sector reads per directory, so performance is not a concern.

**Format details used**:
- Sector size 2048. Volume descriptors start at sector 16: Primary (type 1), Supplementary (type 2, Joliet when escape sequence is `%/@`, `%/C` or `%/E`), terminator (type 255).
- UDF is detected by the Volume Recognition Sequence (`BEA01`, `NSR02` or `NSR03`, `TEA01`) at sectors 16 and later, then the Anchor Volume Descriptor Pointer (tag 2) at sector 256.
- UDF path: AVDP → Main Volume Descriptor Sequence → Partition Descriptor (tag 5) + Logical Volume Descriptor (tag 6) → File Set Descriptor (tag 256) → root ICB → File Entry (tag 261) or Extended File Entry (tag 266) → File Identifier Descriptors (tag 257). Names are OSTA compressed Unicode (compression ID 8 or 16).
- Preference order: UDF, then Joliet, then the primary ISO 9660 tree.
- UDF 2.50+ metadata partitions (Blu-ray) are out of scope for the first release; such an image falls back to the ISO 9660 tree, or reports an unsupported-format error when that tree is empty.
- ISO 9660 file names drop the `;1` version suffix and a trailing `.` .
- Hidden flag (file flags bit 0) maps to `FILE_ATTRIBUTE_HIDDEN`. All entries are read-only (`FILE_ATTRIBUTE_READONLY`), because a disc image is read-only media.

**Alternatives considered**: libarchive iso9660 (no UDF); mounting (violates FR-012); IMAPI2FS (does not accept a file path).

## R4. Reader for other formats

**Decision**: Load `archiveint.dll` dynamically (`CLibArchive`) and enumerate entries with `archive_read_next_header` plus `archive_read_data_skip`.

**Rationale**: This is the only Windows-provided reader for 7z, rar, tar and cab, and it is the same component that Explorer uses. It requires no third-party dependency (constitution "Windows SDK, STL only").

**Extensions mapped to libarchive** (case-insensitive, longest suffix wins): `.7z`, `.rar`, `.tar`, `.tar.gz`, `.tgz`, `.tar.bz2`, `.tbz`, `.tbz2`, `.tar.xz`, `.txz`, `.tar.zst`, `.tzst`, `.tar.lz4`, `.tar.lzma`, `.tlz`, `.cab`, `.cpio`, `.xar`, `.lzh`, `.lha`.

**Not mapped**: bare `.gz`, `.bz2`, `.xz`, `.zst` (single compressed streams, not containers); zip-based document formats such as `.docx`, `.jar`, `.nupkg` (treating every Office document as a folder would surprise users; out of scope).

**Absent DLL**: On Windows 10 or older Windows 11 builds, these extensions are not recognized as archives at all, so the files are listed as ordinary files. When the user targets such a file directly (`tcdir --archives foo.7z\`), tcdir reports "Browsing .7z archives requires Windows 11 23H2 or later." The DLL is loaded once, on first use, and only when the switch is on (no startup cost otherwise, SC-006).

**Performance**: For solid or compressed-stream formats (`.tar.gz`, solid `.7z`, solid `.rar`), libarchive must decompress the stream to find each header. Each archive is enumerated once and cached for the rest of the run (see R6), so recursion into an archive costs one pass.

**Thread safety**: libarchive `archive` objects are not shared between threads. Each enumeration creates and frees its own reader object, which is safe from any thread. The DLL load is guarded by a `std::once_flag`.

**Alternatives considered**: bundling libarchive or 7-Zip source (violates the no-third-party constraint, rejected per FR-011); spawning `tar.exe -tvf` (process creation per archive, text parsing of localized output; rejected).

## R5. Path resolution for paths inside an archive

**Decision**: When the switch is on and the target path (or a parent of it) does not exist as a real directory, walk the path components from the left. The first component that exists as a real file with a recognized archive extension splits the path into the archive file and an inner path. The final component of the inner path is a mask when it contains `*` or `?`, or when it does not resolve to a folder inside the archive.

**Rationale**: Matches how users type paths, and matches FR-004/FR-005. A real directory with an archive-like name (`foo.zip\` as a folder) always wins, because the split happens only on a real file (edge case in the spec).

## R6. Integration with the enumeration pipeline

**Decision**: Introduce an archive "virtual file system" layer: `CArchiveCache` opens each archive once, builds an in-memory tree of `ArchiveNode`s (implied folders included), and serves directory enumerations from it as `WIN32_FIND_DATAW`-compatible records, so the existing `FileInfo` and displayer code paths need no format-specific changes.

**Rationale**: The existing pipeline is built around `WIN32_FIND_DATAW`. Synthesizing those records keeps colors, icons, sorting, column layout and totals identical (FR-008) with no changes to sorting or displayers. Per-entry OS calls (owner, streams, cloud status, reparse targets) are skipped for entries whose `FileInfo` indicates an archive origin (FR-009).

## R7. Switch spelling

**Decision**: `--Archives` / `--Archives-` (help text capitalization follows `--Tree`, `--Icons`), parsed case-insensitively like other long switches. Config file and TCDIR key: `Archives`. Short alias: none (single-letter switches mirror `dir`).

## R8. Header text for archive locations

**Decision**: The header contains the full virtual path, for example `Directory of C:\tmp\release.zip\docs`, in the existing header layout. The volume line (drive label and serial) is the volume of the real archive file. The `.zip\` segment in the path makes the location clear (FR-010), so no extra archive marker is added.
