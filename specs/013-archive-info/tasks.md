# Tasks: Archive Info Mode

**Input**: Design documents from `specs/013-archive-info/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/cli.md, quickstart.md

**Tests**: Required by the constitution (Principle II). Every new unit has a test task. Tests never touch disk: readers use `CMemoryByteSource` with `CZipBuilder` and `CIsoImageBuilder`, the finder and builder use `ScopedFileSystemMock` plus `CMockArchiveFileAccess`, and the libarchive tests use in-memory archives as `LibArchiveReaderTests.cpp` does.

**Format**: `[ID] [P?] [Story] Description`. Paths are relative to the repository root. Every new `.cpp`/`.h` is added to `TCDirCore/TCDirCore.vcxproj` and `.filters` (or `UnitTest/UnitTest.vcxproj` and `.filters`) in the same task that creates it.

## Phase 1: Setup

- [X] T001 Add `ArchiveDetails.h` with `SMethodGroup`, `EZipEncryption`, `SZipDetails`, `EDiscTree`, `SDiscDetails`, `SLibArchiveDetails` and `SArchiveDetails` per data-model.md in TCDirCore/ArchiveDetails.h; register it in TCDirCore/TCDirCore.vcxproj and .filters
- [X] T002 Add `shared_ptr<SArchiveDetails> m_detailsPtr` to `SArchiveReadResult` (null means "don't collect"), column-aligned with the existing members, in TCDirCore/ArchiveEntry.h
- [X] T003 [P] Add `SExtensionGroup`, `SArchiveStatistics`, `EChecksumCoverage`, `SDamagedEntry`, `SVerifyResult` and `SArchiveReport` per data-model.md in TCDirCore/ArchiveReport.h; register it in TCDirCore/TCDirCore.vcxproj and .filters

**Checkpoint**: Solution builds. No behavior change.

## Phase 2: Foundational (blocks all stories)

- [X] T004 [P] Write tests for `--Info` and `--Verify` parsing (`--Info`, `--info`, `/Info`, `--Verify`), `--Verify` without `--Info` ("--Verify requires --Info."), each listing switch from research R6 with `--Info` ("--Info and <switch> cannot be used together." with the switch as typed), `-s`, `-p`, `-m-` and `--Archives` accepted with `--Info`, and `TCDIR` defaults `W`, `B`, `Tree`, `Icons` ignored under `--Info` while `S` applies, in UnitTest/CommandLineTests.cpp
- [X] T005 Add `m_fInfo`, `m_fVerify` and `m_strInfoConflictSwitch` to `CCommandLine`; parse `--Info` and `--Verify` in `HandleLongSwitch` and `IsRecognizedLongSwitch`; record the first listing switch typed in `ParseSwitch` and `HandleLongSwitch`; add `ValidateInfoCombinations` that reports the conflict or resets the listing members to their defaults; call it from `ValidateSwitchCombinations`, in TCDirCore/CommandLine.h and TCDirCore/CommandLine.cpp
- [X] T006 Add `HRESULT Verify (const SArchiveSource & source, EArchiveFormat format, SVerifyResult & result)` to `IArchiveFileAccess` and `CArchiveFileAccess` (returns `E_NOTIMPL` until US7), and make `CMockArchiveFileAccess` fill `m_detailsPtr` and verify results from per-path configured values, in TCDirCore/ArchiveFileAccess.h, TCDirCore/ArchiveFileAccess.cpp and UnitTest/Mocks/MockArchiveFileAccess.h
- [X] T007 Write tests for `CArchiveFileSystem::ReadDetails`: real archive, nested archive (bytes from the parent), details requested from the mock, read error returns the `FormatError` text, nested archive over the size limit returns `ERROR_FILE_TOO_LARGE`, in UnitTest/ArchiveFileSystemTests.cpp
- [X] T008 Implement `CArchiveFileSystem::ReadDetails (path, SArchiveReadResult &, wstring & strError)` and `CArchiveFileSystem::Verify` through `OpenSource`. Size on disk comes from the find data the finder supplies, in TCDirCore/ArchiveFileSystem.h and TCDirCore/ArchiveFileSystem.cpp
- [X] T009 Run the full unit test suite. Listing output is identical (SC-006)

**Checkpoint**: Command line and read paths exist and are tested. Listing output is identical.

## Phase 3: User Story 1 - Statistics for one archive (P1) 🎯 MVP

**Goal**: `tcdir --Info foo.zip` prints one block with the common statistics.

**Independent Test**: Quickstart scenarios 1, 2, 11, 12, 13 and 16.

- [X] T010 [P] [US1] Write tests for `CArchiveStatistics::Compute`: files, explicit and implied folders, nested archives counted by suffix, uncompressed total, largest file with tie on the first entry, deepest path counted in components, oldest and newest with entries lacking a time left out, no timestamps at all, encrypted count, listing-encrypted flag, extension groups lowercase with "(none)" and sorted by size then extension, empty archive, in UnitTest/ArchiveStatisticsTests.cpp
- [X] T011 [US1] Implement `CArchiveStatistics::Compute (const SArchiveReadResult &, SArchiveStatistics &)` in one pass with private static helpers for depth, extension and folder collection, in TCDirCore/ArchiveStatistics.h and TCDirCore/ArchiveStatistics.cpp
- [X] T012 [P] [US1] Write tests for `CArchiveInfoBuilder::Build`: report for a mocked archive (size on disk, statistics), read error becomes the report error text, encrypted listing reports size and format without statistics, in UnitTest/ArchiveInfoBuilderTests.cpp
- [X] T013 [US1] Implement `CArchiveInfoBuilder` (takes `CArchiveFileSystem &` and the verify flag, sets `m_detailsPtr`, calls `ReadDetails`, then `CArchiveStatistics::Compute`) in TCDirCore/ArchiveInfoBuilder.h and TCDirCore/ArchiveInfoBuilder.cpp
- [X] T014 [P] [US1] Write tests for `CArchiveInfoDisplayer` line builders: heading, label width (longest label plus 3), size on disk, contents with singular and plural ("1 nested archive"), uncompressed with ratio, "none" for empty values, date range in the listing's date and time format, encryption line, extension rows with aligned columns, error block in `{Error}`, output through `TestConsole` with the configured colors, in UnitTest/ArchiveInfoDisplayerTests.cpp
- [X] T015 [US1] Implement `CArchiveInfoDisplayer::Display (const SArchiveReport &)` with private static line builders that return strings, using `FormatNumberWithSeparators` and the listing date format, per contracts/cli.md, in TCDirCore/ArchiveInfoDisplayer.h and TCDirCore/ArchiveInfoDisplayer.cpp
- [X] T016 [US1] Add `CArchiveInfo::Run` in TCDirCore/ArchiveInfo.h and TCDirCore/ArchiveInfo.cpp: create `CArchiveFileSystem`, build and print the report for each target that is an archive path, and return the failure code. Call it from TCDirCore/TCDir.cpp instead of `RunDirectoryListing` when `m_fInfo` is set
- [X] T017 [US1] Add `--Info` and `--Verify` rows from contracts/cli.md to the help in TCDirCore/Usage.cpp
- [X] T018 [US1] Run quickstart scenarios 1, 2, 11, 12, 13 and 16 against TestData\Archives
- [X] T019 [US1] Build Debug x64, run all tests, commit the phase (include `TCDirCore/Version.h`)

**Checkpoint**: MVP: one block per archive path.

## Phase 4: User Story 2 - Masks and several targets (P1)

**Goal**: Masks, folders and several targets each produce blocks in order, with a summary.

**Independent Test**: Quickstart scenarios 3 and 14.

- [X] T020 [P] [US2] Write tests for `CArchiveInfoFinder::Find (targets, callback)` without `-s`: archive path, mask in a folder (non-archives skipped, name order), folder treated as `folder\*`, no target treated as `*`, several targets in order, a target with no archives returns "No archives match <target>" and the run continues, mask inside an archive (`outer.zip\*.7z`), in UnitTest/ArchiveInfoFinderTests.cpp
- [X] T021 [US2] Implement `CArchiveInfoFinder` (splits targets with `CMaskGrouper::GroupMasksByDirectory`, enumerates folders with `CFileEnumerator` and archive folders with `CArchiveTree::EnumerateChildren`, filters matches with `CArchiveFileSystem::IsArchiveFile`) in TCDirCore/ArchiveInfoFinder.h and TCDirCore/ArchiveInfoFinder.cpp
- [X] T022 [P] [US2] Write tests for the summary line (archive count, combined size on disk, combined uncompressed size) and its absence for a single block in UnitTest/ArchiveInfoDisplayerTests.cpp
- [X] T023 [US2] Add `CArchiveInfoDisplayer::DisplaySummary` and use `CArchiveInfoFinder` in `CArchiveInfo::Run`, printing each block as it is found, in TCDirCore/ArchiveInfoDisplayer.h, TCDirCore/ArchiveInfoDisplayer.cpp and TCDirCore/TCDir.cpp
- [X] T024 [US2] Run quickstart scenarios 3 and 14, build, run all tests, commit the phase

**Checkpoint**: Masks and several targets work.

## Phase 5: User Story 3 - Nested archives and recursion (P2)

**Goal**: Archives at any depth are reported through explicit nested paths and `-s`.

**Independent Test**: Quickstart scenarios 4, 5 and 6.

- [X] T025 [P] [US3] Write tests for `CArchiveInfoFinder` with `-s`: subfolders searched depth first after the folder's archives, archives inside matching archives reported after their parent with full paths, archives that don't match the mask searched but not reported (`*.7z` inside a ZIP), a nested archive that fails to open reported with its error, in UnitTest/ArchiveInfoFinderTests.cpp
- [X] T026 [US3] Add the `-s` walk to `CArchiveInfoFinder` with a private static helper per level (disk folder, archive folder), in TCDirCore/ArchiveInfoFinder.cpp
- [X] T027 [P] [US3] Write a test that a nested report's size on disk equals the entry size in its parent in UnitTest/ArchiveInfoBuilderTests.cpp
- [X] T028 [US3] Run quickstart scenarios 4, 5 and 6, build, run all tests, commit the phase

**Checkpoint**: Nested archives are reachable both ways.

## Phase 6: User Story 4 - ZIP details (P2)

**Goal**: ZIP blocks add methods, encryption, creator, ZIP64, UTF-8, multi-disk, prefix and comment.

**Independent Test**: ZipReaderTests with built archives. Quickstart scenario 1 includes the methods line.

- [X] T029 [US4] Extend `CZipBuilder::SEntryOptions` with `m_wMethod`, `m_ullCompressedSize`, `m_bAesStrength` (emits extra 0x9901 and method 99), `m_fStrongEncryption` (flag bit 6), `m_bVersion`, and add `SetPrefix (bytes)` and `SetDiskNumbers (wDisk, wDirectoryDisk)`, in UnitTest/Mocks/ZipBuilder.h and UnitTest/Mocks/ZipBuilder.cpp
- [X] T030 [P] [US4] Write tests for ZIP details: method groups with compressed and uncompressed sizes, AES method taken from the 0x9901 extra, ZipCrypto, AES-128/192/256, PKWARE strong, mixed encryption counts, ZIP64 locator flag, UTF-8 flag, multi-disk, most common creator host and version, prefix size from the smallest local header offset, comment decoded as UTF-8 and as code page 437, compressed size from the ZIP64 extra, details not collected when `m_detailsPtr` is null, in UnitTest/ZipReaderTests.cpp
- [X] T031 [US4] Collect `SZipDetails` in `CZipReader` per research R2 with new offset constants (`s_kibMethod`, `s_kibCompressedSize`, `s_kibLocalHeaderOffset`, `s_kibEndDiskNumber`, `s_kibEndDirectoryDisk`, `s_kwExtraAes`) and private static helpers for encryption type and comment decoding, in TCDirCore/ZipReader.h and TCDirCore/ZipReader.cpp
- [X] T032 [P] [US4] Write tests for the ZIP lines: format line with creator and UTF-8, methods rows, encryption per type, ZIP64 and multi-disk lines omitted when false, prefix line, comment with several lines indented under the first, in UnitTest/ArchiveInfoDisplayerTests.cpp
- [X] T033 [US4] Add the ZIP lines to `CArchiveInfoDisplayer` in TCDirCore/ArchiveInfoDisplayer.cpp
- [X] T034 [US4] Run `--Info` on TestData\Archives\zip, build, run all tests, commit the phase

**Checkpoint**: ZIP blocks are complete.

## Phase 7: User Story 5 - Disc image details (P3)

**Goal**: ISO and UDF blocks add file systems, IDs, dates, sectors and boot platforms.

**Independent Test**: IsoReaderTests with built images. Quickstart scenario 7.

- [X] T035 [US5] Test the disc parsers with synthetic sectors in UnitTest/IsoDetailsTests.cpp instead of extending `CIsoImageBuilder` with PVD and Joliet ID fields, volume dates, a Rock Ridge "SP" and "RR" system use area on the root record, an El Torito boot record and catalog with chosen platform IDs, and a UDF domain revision and logical volume label, in UnitTest/Mocks/IsoImageBuilder.h and UnitTest/Mocks/IsoImageBuilder.cpp
- [X] T036 [P] [US5] Write tests for disc details: IDs with trailing spaces removed, Joliet values replacing primary values, blank IDs empty, dates with UTC offset and all-zero dates unset, sector size and count, Rock Ridge detection, boot platforms BIOS, UEFI and BIOS plus UEFI, no boot record, UDF revision and label, listed tree for each `EUdfVariant` and for Joliet and primary images, in UnitTest/IsoReaderTests.cpp
- [X] T037 [US5] Collect `SDiscDetails` in `CIsoDetails` (volume fields, Rock Ridge, El Torito) in TCDirCore/IsoDetails.h and TCDirCore/IsoDetails.cpp, called from `CIsoReader::Read`, which sets the listed tree
- [X] T038 [US5] Read the UDF revision and logical volume label in `CIsoDetails::ReadUdf`, using `CUdfReader::DecodeOstaName`
- [X] T039 [P] [US5] Write tests for the disc lines: file systems with the listed tree, blank IDs omitted, dates, sectors, boot platforms with the number for unknown platforms, in UnitTest/ArchiveInfoDisplayerTests.cpp
- [X] T040 [US5] Add the disc lines to `CArchiveInfoDisplayer` in TCDirCore/ArchiveInfoDisplayer.cpp
- [X] T041 [US5] Run quickstart scenario 7, build, run all tests, commit the phase

**Checkpoint**: Disc image blocks are complete.

## Phase 8: User Story 6 - Details for other formats (P3)

**Goal**: libarchive blocks add format, filters, stream totals, encryption, links, owners, permissions and attribute counts.

**Independent Test**: LibArchiveReaderTests with in-memory archives. Quickstart scenario 8.

- [ ] T042 [US6] Load the optional exports from research R4 in `CLibArchiveReader::LoadApi` (the pointer for a missing export is null) in TCDirCore/LibArchiveReader.h and TCDirCore/LibArchiveReader.cpp
- [ ] T043 [P] [US6] Write tests for libarchive details with in-memory archives: format name for tar, filter name for tar.gz, stream byte totals present with a filter and absent for zip, symlink and hard link counts, owners and groups, permission sets, details not collected when `m_detailsPtr` is null; skipped when `archiveint.dll` is missing, in UnitTest/LibArchiveReaderTests.cpp
- [ ] T044 [US6] Collect `SLibArchiveDetails` in `CLibArchiveReader::ReadEntries` with a private static helper per group (stream, links, ownership, attributes), in TCDirCore/LibArchiveReader.cpp
- [ ] T045 [P] [US6] Write tests for the libarchive lines: format with filters, stream totals with ratio, owners and groups sorted by count, permissions in octal, lines with no data omitted, in UnitTest/ArchiveInfoDisplayerTests.cpp
- [ ] T046 [US6] Add the libarchive lines to `CArchiveInfoDisplayer` in TCDirCore/ArchiveInfoDisplayer.cpp
- [ ] T047 [US6] Run `--Info` on every libarchive folder in TestData\Archives, build, run all tests, commit the phase

**Checkpoint**: Every format has its details.

## Phase 9: User Story 7 - Verification (P3)

**Goal**: `--Verify` reports passed, damaged and skipped entries.

**Independent Test**: Quickstart scenarios 9 and 10.

- [ ] T048 [P] [US7] Write tests for `CLibArchiveReader::Verify` with in-memory archives: intact zip all passed, zip with one damaged byte reports that entry with the libarchive reason, encrypted entry skipped, coverage per format per research R5 (zip per entry, tar.gz stream, tar headers only), in UnitTest/LibArchiveReaderTests.cpp
- [ ] T049 [US7] Implement `CLibArchiveReader::Verify (source, format, SVerifyResult &)` (reads each entry with `archive_read_data`, treats WARN or FAILED during data reads as damage, skips encrypted entries) and `CArchiveFileAccess::Verify` (dispatches every format to libarchive, returns `Unavailable` for UDF-only images and the missing-libarchive error without `archiveint.dll`), in TCDirCore/LibArchiveReader.h, TCDirCore/LibArchiveReader.cpp and TCDirCore/ArchiveFileAccess.cpp
- [ ] T050 [P] [US7] Write tests for verify in `CArchiveInfoBuilder` (run only with the flag, nested archives verified from memory) in UnitTest/ArchiveInfoBuilderTests.cpp, and for the verify line and damaged entries in `{Error}` in UnitTest/ArchiveInfoDisplayerTests.cpp
- [ ] T051 [US7] Call `Verify` from `CArchiveInfoBuilder`, add the verify lines to `CArchiveInfoDisplayer`, and return the failure code for damaged entries from `CArchiveInfo::Run`, in TCDirCore/ArchiveInfoBuilder.cpp, TCDirCore/ArchiveInfoDisplayer.cpp and TCDirCore/TCDir.cpp
- [ ] T052 [US7] Run quickstart scenarios 9 and 10, build, run all tests, commit the phase

**Checkpoint**: Verification works for every format libarchive reads.

## Phase 10: Polish

- [ ] T053 [P] Document `--Info` and `--Verify` in README.md and CHANGELOG.md
- [ ] T054 Run quickstart scenario 15 and check SC-002 (under 1 second for ZIP and ISO)
- [ ] T055 Build Release x64 and ARM64, run Code Analysis with zero warnings, run all tests
- [ ] T056 Run the prose check on every new comment and the docs, then commit the phase

## Dependencies

- Phase 1 → Phase 2 → US1 (Phase 3).
- US2 depends on US1 (`CArchiveInfo::Run`). US3 depends on US2 (finder).
- US4, US5 and US6 depend on US1 only and can run in any order or in parallel. They touch different readers, but all add lines to `ArchiveInfoDisplayer.cpp`, so their displayer tasks run one after another.
- US7 depends on US1 and uses the libarchive work from US6 (T042).
- Polish runs last.

## Parallel Examples

- Phase 1: T003 with T001 and T002.
- US1: T010, T012 and T014 (tests in different files) together.
- After US1: T030 (ZIP tests), T036 (disc tests) and T043 (libarchive tests).

## Implementation Strategy

1. MVP: Phases 1 to 3. One block per archive path with common statistics.
2. Add US2 and US3 for targets and recursion.
3. Add the format details (US4, US5, US6), then verification (US7).
4. Commit after each phase, as the constitution requires.
