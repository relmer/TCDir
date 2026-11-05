# Implementation Plan: Archive Browsing

**Branch**: `012-archive-browsing` | **Date**: 2026-11-05 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/012-archive-browsing/spec.md`

## Summary

Add an opt-in `--Archives` switch. With it, tcdir treats every file with a recognized archive extension as a directory: as a target, inside a target path (`x.zip\docs\*.md`), in listings, and in `-S` and `--Tree` recursion. Without it, all behavior is identical to today.

Archive contents come from three readers behind one interface:

- `CZipReader`: native .zip central-directory parser (all Windows versions, no decompression).
- `CIsoReader`: native ISO 9660 + Joliet + UDF parser (no mounting).
- `CLibArchiveReader`: run-time binding to the Windows 11 `archiveint.dll` (libarchive) for 7z, rar, tar family, cab, cpio, xar, lha.

A `CArchiveFileSystem` layer opens each archive once, builds an in-memory tree with implied folders, and serves enumerations as `WIN32_FIND_DATAW` records through a new `CFileEnumerator`. That class replaces the four `FindFirstFile` loops in `CDirectoryLister` and `CMultiThreadedLister`, so sorting, colors, icons, columns and totals apply unchanged to archive entries. See [research.md](research.md) for the reader decisions.

## Technical Context

**Language/Version**: C++ (stdcpplatest, MSVC v145+)

**Primary Dependencies**: Windows SDK and STL only. `archiveint.dll` is loaded at run time from System32 when present (no import library, no header, no bundled code).

**Storage**: N/A (read-only access to archive files)

**Testing**: Microsoft C++ Unit Test Framework. Readers take an `IByteSource`, so tests use synthetic in-memory archive bytes. Lister integration tests use the existing `ScopedFileSystemMock` plus a mock `IArchiveFileAccess`.

**Target Platform**: Windows 10/11, x64 and ARM64. 7z/rar/tar/cab support requires Windows 11 23H2 or later. .zip and .iso work on every supported version.

**Project Type**: CLI tool (desktop)

**Performance Goals**: Root listing of a 10,000-entry .zip in under 1 s (SC-003). No measurable regression without the switch (SC-006): the only cost without the switch is one `bool` test per enumerated entry.

**Constraints**: No extraction, no temp files, no mounting (FR-012, FR-015). Each archive is parsed at most once per run. Thread-safe access from the multi-threaded lister workers.

**Scale/Scope**: Archives up to 100,000+ entries and over 4 GB (ZIP64, 64-bit ISO extents).

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Code Quality | PASS | EHM in every failable function. Readers return HRESULT with a single `Error:` exit. `unique_ptr`/`shared_ptr` for reader and tree ownership. |
| II. Testing Discipline | PASS | Every parser accepts `IByteSource`; tests build archives in memory. `CArchiveFileSystem` accepts `IArchiveFileAccess`, so tests never open real files. `CLibArchiveReader` entry conversion is a pure function tested with synthetic values; the DLL binding itself is covered only by the manual quickstart (no real-system calls in unit tests). |
| III. UX Consistency | PASS | `--Archives`/`--Archives-` follows the `--Tree`/`--Ellipsize` pattern, with TCDIR, config file, `--Env`, `--Config`, `--Settings` and `-?` support. Errors use the existing `{Error}` style on the console error path. |
| IV. Performance | PASS | Central-directory or metadata reads only. Lazy DLL load. Per-archive cache. Switch-off path adds no I/O. |
| V. Simplicity | PASS with note | New code is split into focused units (one reader per format, tree, path splitter, matcher, enumerator). No third-party dependency. The UDF parser is the largest single unit; it is required because Windows provides no non-mounting ISO reader and most real-world .iso files are UDF (research R3). |

**Gate result: PASS.** Post-design re-check: PASS (no new violations from the data model or contracts).

## Project Structure

### Documentation (this feature)

```text
specs/012-archive-browsing/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/
│   └── cli.md           # Command-line, env var and config contract
└── tasks.md             # Phase 2 output (/speckit.tasks)
```

### Source Code (files affected)

```text
TCDirCore/
├── ArchiveEntry.h               # New: SArchiveEntry, EArchiveFormat
├── ByteSource.h/.cpp            # New: IByteSource, CFileByteSource, CMemoryByteSource
├── ArchiveFormat.h/.cpp         # New: CArchiveFormat (extension → format, libarchive availability)
├── WildcardMatcher.h/.cpp       # New: CWildcardMatcher (FindFirstFile-compatible * ? matching)
├── ZipReader.h/.cpp             # New: CZipReader
├── DiscImage.h/.cpp             # New: CDiscImage (sector reads shared by the ISO and UDF readers)
├── IsoReader.h/.cpp             # New: CIsoReader (ISO 9660 + Joliet)
├── UdfReader.h/.cpp             # New: CUdfReader (UDF 1.02-2.01)
├── LibArchiveReader.h/.cpp      # New: CLibArchiveReader (archiveint.dll binding)
├── ArchiveTree.h/.cpp           # New: CArchiveTree (nodes, implied folders, lookup, find-data synthesis)
├── ArchiveFileAccess.h/.cpp     # New: IArchiveFileAccess + CArchiveFileAccess (real file probes, readers)
├── ArchiveFileSystem.h/.cpp     # New: CArchiveFileSystem (path split, cache, virtual enumeration)
├── FileEnumerator.h/.cpp        # New: CFileEnumerator (real or archive enumeration behind one call)
├── DirectoryInfo.h              # CDirectoryInfo::m_fInArchive and archive error fields
├── DirectoryLister.h/.cpp       # Use CFileEnumerator; archive-aware existence check; skip OS calls for archive entries
├── MultiThreadedLister.h/.cpp   # Use CFileEnumerator; propagate archive errors to the main thread
├── MaskGrouper.h/.cpp           # Archive-aware directory test when splitting masks
├── ResultsDisplayer*.cpp        # Skip owner, streams, sync-root and free-space calls for archive locations
├── CommandLine.h/.cpp           # --Archives / --Archives-
├── Config.h/.cpp                # Archives switch mapping and source tracking
├── Usage.cpp                    # Help row, --Env/--Config/--Settings rows
├── TCDir.cpp                    # Create CArchiveFileSystem when the switch is on
├── pch.h                        # Any new SDK headers
└── TCDirCore.vcxproj(.filters)  # New files

UnitTest/
├── Mocks/ZipBuilder.h/.cpp      # New: in-memory zip builder for tests
├── Mocks/IsoImageBuilder.h/.cpp # New: in-memory ISO 9660 and UDF image builder for tests
├── Mocks/MockArchiveFileAccess.h# New
├── WildcardMatcherTests.cpp     # New
├── ArchiveFormatTests.cpp       # New
├── ZipReaderTests.cpp           # New
├── IsoReaderTests.cpp           # New (ISO 9660, Joliet, UDF)
├── LibArchiveReaderTests.cpp    # New (pure conversion only)
├── ArchiveTreeTests.cpp         # New
├── ArchiveFileSystemTests.cpp   # New (path split, cache, errors)
├── ArchiveListingScenarioTests.cpp # New (listers + displayers end to end with mocks)
├── CommandLineTests.cpp, ConfigTests.cpp, ConfigFileTests.cpp # Switch tests
└── UnitTest.vcxproj(.filters)

README.md, CHANGELOG.md          # User documentation
```

**Structure Decision**: Each archive format has its own reader class that produces a flat `vector<SArchiveEntry>`. `CArchiveTree` is the only code that understands folder structure, and `CArchiveFileSystem` is the only code that the listers call. The listers see `WIN32_FIND_DATAW` records through `CFileEnumerator`, so displayers and comparators need only "is this an archive location?" checks to skip OS calls.

## Key Design Points

1. **Archive files become directories.** With the switch, `CFileEnumerator` sets `FILE_ATTRIBUTE_DIRECTORY` on each real file whose extension is recognized (and that is not an offline or recall-on-access cloud placeholder), and registers the path with `CArchiveFileSystem`. Recursion, tree drawing, attribute filters, directory counts and icons then apply with no further changes.
2. **Virtual path lookup.** `CArchiveFileSystem::Resolve (dirPath)` walks the parent chain of `dirPath` and checks each prefix against the set of registered archive paths (one hash lookup per component). The archive is parsed on first enumeration and cached as `shared_ptr<const CArchiveTree>` under a mutex. Parse failures are cached too, so an error is reported once.
3. **Command-line targets.** `CMaskGrouper` and `CDirectoryLister::List` call `CArchiveFileSystem::IsDirectory` and `Exists` instead of `filesystem::is_directory`/`exists` when the switch is on. These split the path at the first real file with an archive extension (research R5).
4. **Per-entry OS calls.** `CDirectoryInfo::m_fInArchive` indicates archive origin. `ResolveReparseTarget`, `HandleFileMatchStreams`, `GetFileOwner`, `IsUnderSyncRoot` and `GetDiskFreeSpaceEx` are skipped for them; `CDriveInfo` and the volume footer use the real archive file path.
5. **Errors.** An archive that fails to parse produces `Error:   <path>: <reason>` through the existing `ColorPrintf` `{Error}` style. In multi-threaded mode the HRESULT and message are stored on the `CDirectoryInfo` (`Status::Error`) and printed by the main thread, the same as an inaccessible directory. This is the existing error path.
6. **Thread safety.** `CArchiveTree` is immutable after construction. `CArchiveFileSystem` guards its maps with a `std::mutex`; two workers asking for the same archive wait on one parse (`std::shared_future`).
7. **libarchive binding.** `CLibArchiveReader` loads `archiveint.dll` with `LoadLibraryExW (L"archiveint.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32)` once (`std::call_once`), resolves the function pointers, and reports `HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED)` when it is absent. `CArchiveFormat` recognizes libarchive extensions only when the DLL loads.

## Complexity Tracking

| Item | Why Needed | Simpler Alternative Rejected Because |
|------|------------|-------------------------------------|
| Native UDF parser | Real-world .iso files (Windows media, most Linux ISOs) keep their contents in UDF | libarchive has no UDF support; mounting violates FR-012 |
| Native .zip parser in addition to libarchive | .zip is P1 and must work on Windows 10 | `archiveint.dll` is absent on Windows 10 |
