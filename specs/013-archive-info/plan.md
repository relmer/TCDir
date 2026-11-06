# Implementation Plan: Archive Info Mode

**Branch**: `013-archive-info` | **Date**: 2026-11-06 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/013-archive-info/spec.md`

## Summary

`--Info` replaces the listing with one block of statistics per archive.
The ZIP, ISO/UDF and libarchive readers gain an optional details record that
they fill while reading the entry list, so a block costs one read of the
index. A pure statistics function computes the common values from the entry
list. A finder resolves targets and walks folders and archives under `-s`
with the rule `--Archives -s` uses. `--Verify` reads every entry through
libarchive, which checks the stored checksums. A displayer prints the blocks
through `CConsole`.

## Technical Context

**Language/Version**: C++ (stdcpplatest, MSVC v145+)

**Primary Dependencies**: Windows SDK, STL, `archiveint.dll` loaded at run
time (Windows 11 23H2 and later)

**Storage**: N/A

**Testing**: Microsoft C++ Unit Test Framework, in-memory archives and
`CMockArchiveFileAccess`

**Target Platform**: Windows 10/11, x64 and ARM64

**Project Type**: CLI

**Performance Goals**: `--Info` on a ZIP or ISO under 1 GB in under 1 second
(SC-002). No change to listing timing (SC-006)

**Constraints**: Listings don't request details, so the readers do no extra
work for them. Nested archives use the 256 MB extraction limit

**Scale/Scope**: Archives with millions of entries. Statistics are computed
in one pass without copying entries

## Constitution Check

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Code Quality | Pass | EHM macros, single exit, alignment rules from `.github/copilot-instructions.md` |
| II. Testing Discipline | Pass | Statistics, detail parsing, conflicts, target resolution and output are tested with synthetic bytes and the mock. The libarchive detail and verify tests use in-memory archives, as `LibArchiveReaderTests.cpp` does |
| III. UX Consistency | Pass | Output through `CConsole` with configured colors. Errors in `{Error}` inline. `--Info` and `--Verify` documented in `-?` |
| IV. Performance | Pass | Details are opt-in per read. One index read per archive |
| V. Simplicity | Pass | New classes each have one job. Helpers are private static members. No new dependencies |

Re-check after Phase 1 design: Pass. One spec refinement is noted below.

**Spec refinement**: FR-028 states that verify reports whether every entry
reads for formats without checksums. libarchive can't read UDF-only images,
so for those the verify line reports that verification isn't available
(research R5).

## Project Structure

### Documentation (this feature)

```text
specs/013-archive-info/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   └── cli.md
└── tasks.md
```

### Source Code

```text
TCDirCore/
├── ArchiveEntry.h            # SArchiveReadResult gains m_detailsPtr
├── ArchiveDetails.h          # New: SArchiveDetails, SZipDetails, SDiscDetails, SLibArchiveDetails
├── ArchiveStatistics.h/.cpp  # New: CArchiveStatistics::Compute
├── ArchiveInfoFinder.h/.cpp  # New: target resolution and -s walk
├── ArchiveInfoBuilder.h/.cpp # New: builds SArchiveReport for one archive
├── ArchiveVerifier.h/.cpp    # New: --Verify through libarchive
├── ArchiveInfoDisplayer.h/.cpp # New: block and summary output
├── ZipReader.h/.cpp          # Methods, compressed sizes, encryption, creator, comment, prefix
├── IsoReader.h/.cpp          # Volume fields, Rock Ridge, El Torito, listed tree
├── UdfReader.h/.cpp          # UDF revision and label
├── LibArchiveReader.h/.cpp   # Format, filters, byte totals, owners, links, attributes; verify read loop
├── ArchiveFileSystem.h/.cpp  # Read with details for a resolved archive path
├── CommandLine.h/.cpp        # --Info, --Verify, conflict tracking, config reset
├── Usage.cpp                 # Help text
└── TCDir.cpp                 # RunArchiveInfo dispatch

UnitTest/
├── ArchiveStatisticsTests.cpp   # New
├── ArchiveInfoFinderTests.cpp   # New
├── ArchiveInfoDisplayerTests.cpp # New
├── ZipReaderTests.cpp           # Detail parsing
├── IsoReaderTests.cpp           # Detail parsing
├── UdfReaderTests.cpp           # Revision and label
├── LibArchiveReaderTests.cpp    # Details and verify with in-memory tar and zip
├── CommandLineTests.cpp         # Conflicts and config reset
└── Mocks/MockArchiveFileAccess.h # Details and verify hooks
```

**Structure Decision**: The feature follows the archive browsing layout:
readers and the archive file system in `TCDirCore`, tests in `UnitTest`
grouped by class.

## Design Notes

- **`IArchiveFileAccess`** gains `Verify (source, format, SVerifyResult &)`.
  `Read` returns details when `result.m_detailsPtr` is set before the call.
  The mock fills both from its configured data.
- **`CArchiveFileSystem`** gains `ReadDetails (path, SArchiveReadResult &)`,
  which resolves nested paths through `OpenSource` and reads the archive
  directly. The cached tree serves the walk; details aren't cached, because
  each archive is reported once.
- **`CArchiveInfoBuilder`** fills one report: size on disk from the file or
  the parent entry, statistics from `CArchiveStatistics`, details from the
  read, and verify when requested. Errors become the report's error text
  through `CArchiveFileSystem::FormatError`.
- **`CArchiveInfoFinder`** takes a callback that receives each archive path
  in output order, so blocks print while the walk continues.
- **`CArchiveInfoDisplayer`** formats sizes with the listing's digit grouping
  and dates with the listing's date and time format. Its helpers that build
  each line return strings, so tests check text without a console.
- **`RunArchiveInfo`** in `TCDir.cpp` runs instead of `RunDirectoryListing`
  when `m_fInfo` is set, and returns the failure code for FR-005 and FR-027.
- **Ctrl+C**: tcdir doesn't install a console control handler, so Ctrl+C
  ends the process at any point. The spec's edge case doesn't require new
  code.

## Complexity Tracking

No constitution violations.
