# Tasks: Archive Browsing

**Input**: Design documents from `specs/012-archive-browsing/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/cli.md, quickstart.md

**Tests**: Required by the constitution (Principle II). Every new unit has a test task. Tests never touch disk: readers use `CMemoryByteSource`, listers use `ScopedFileSystemMock` plus `MockArchiveFileAccess`.

**Format**: `[ID] [P?] [Story] Description`. Paths are relative to the repository root. Every new `.cpp`/`.h` is added to `TCDirCore/TCDirCore.vcxproj` and `.filters` (or `UnitTest/UnitTest.vcxproj` and `.filters`) in the same task that creates it.

## Phase 1: Setup

- [ ] T001 Add `ArchiveEntry.h` with `EArchiveFormat { None, Zip, Iso, LibArchive }`, `SArchiveEntry` (`m_strPath`, `m_fDirectory`, `m_ullSize`, `m_ftLastWrite` UTC, `m_dwAttributes`, `m_fEncrypted`) and `SArchiveReadResult` (`m_vEntries`, `m_fListingEncrypted`) in TCDirCore/ArchiveEntry.h; register it in TCDirCore/TCDirCore.vcxproj and TCDirCore/TCDirCore.vcxproj.filters
- [ ] T002 [P] Add `IByteSource` (`Size()`, `ReadAt (offset, pBuffer, cb)` with "full read or ERROR_HANDLE_EOF"), `CFileByteSource` (`CreateFileW` `GENERIC_READ`, share read/write/delete, `FILE_FLAG_RANDOM_ACCESS`) and `CMemoryByteSource` in TCDirCore/ByteSource.h and TCDirCore/ByteSource.cpp
- [ ] T003 [P] Add `UnitTest/Mocks/ArchiveBuilders.h/.cpp` with an in-memory zip builder (local headers, central directory, EOCD; options for ZIP64, UTF-8 flag, Unicode path extra `0x7075`, NTFS time extra `0x000A`, Unix host attributes, encryption flag, comment) and register it in UnitTest/UnitTest.vcxproj and .filters

**Checkpoint**: Solution builds; no behavior change.

## Phase 2: Foundational (blocks all stories)

- [ ] T004 [P] Write tests for `CWildcardMatcher::Matches (name, spec)`: `*`, `?`, `*.*` matches names without a dot, case-insensitive, trailing-dot spec, empty spec, literal match in UnitTest/WildcardMatcherTests.cpp
- [ ] T005 [P] Implement `CWildcardMatcher` (FindFirstFile-compatible, no 8.3 matching) in TCDirCore/WildcardMatcher.h and TCDirCore/WildcardMatcher.cpp
- [ ] T006 [P] Write tests for `CArchiveFormat::FromFileName (name, fLibArchiveAvailable)`: `.zip`, `.ISO`, `.tar.gz` longest suffix, `.tgz`, `.7z` only when available, bare `.gz` and `.docx` return `None` in UnitTest/ArchiveFormatTests.cpp
- [ ] T007 [P] Implement `CArchiveFormat` with one `constexpr` suffix table (research R4 list) in TCDirCore/ArchiveFormat.h and TCDirCore/ArchiveFormat.cpp
- [ ] T008 [P] Write tests for `CArchiveTree::Build`, `FindFolder` and `EnumerateChildren`: implied folders, explicit folder after implied updates time and attributes, `/` and `\` separators, `..`/drive/root prefixes sanitized, duplicates kept, case-insensitive lookup, file component returns `nullptr`, wildcard filtering, synthesized `WIN32_FIND_DATAW` (size high/low, three times equal last-write, `FILE_ATTRIBUTE_NORMAL` when none, long names truncated to `MAX_PATH - 1`), empty archive in UnitTest/ArchiveTreeTests.cpp
- [ ] T009 Implement `CArchiveTree` per data-model.md in TCDirCore/ArchiveTree.h and TCDirCore/ArchiveTree.cpp
- [ ] T010 Add `FileInfo::m_fInArchive`, `CDirectoryInfo::m_fInArchive` and `CDirectoryInfo::m_strError` (column-aligned with the existing members) in TCDirCore/DirectoryInfo.h
- [ ] T011 Add `IArchiveFileAccess` (`GetAttributes`, `Read (path, format, result)`, `IsLibArchiveAvailable`) in TCDirCore/ArchiveFileAccess.h, and `UnitTest/Mocks/MockArchiveFileAccess.h` that maps paths to attributes and in-memory images
- [ ] T012 Write tests for `CArchiveFileSystem`: `IsArchiveFile` (extension, directory excluded, `FILE_ATTRIBUTE_OFFLINE` and `FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS` excluded), `TrySplit` (archive root, inner path, inner mask, real directory with archive name wins, non-archive file returns false), `GetTree` parses once (mock counts reads), failure cached with message, concurrent `GetTree` calls from several threads parse once, `TrySplit` on a placeholder archive reports the offline error, `Resolve` of a registered archive child path in UnitTest/ArchiveFileSystemTests.cpp
- [ ] T013 Implement `CArchiveFileSystem` (known-archive set, `TrySplit`, `Resolve`, `GetTree` with mutex plus `shared_future`, `IsDirectory`, `Exists`, error text per contracts/cli.md) in TCDirCore/ArchiveFileSystem.h and TCDirCore/ArchiveFileSystem.cpp
- [ ] T014 Implement `CFileEnumerator::Enumerate (dirPath, spec, callback)`: real paths call `FindFirstFileW`/`FindNextFileW` with the existing error semantics; with a non-null `CArchiveFileSystem`, real archive files get `FILE_ATTRIBUTE_DIRECTORY` and are registered; archive locations are served from `CArchiveTree::EnumerateChildren`; an archive error is returned as an HRESULT plus message in TCDirCore/FileEnumerator.h and TCDirCore/FileEnumerator.cpp
- [ ] T015 Replace the `FindFirstFile` loops in `CDirectoryLister::CollectMatchingFilesAndDirectories` and `CDirectoryLister::RecurseIntoSubdirectories` with `CFileEnumerator` in TCDirCore/DirectoryLister.cpp and TCDirCore/DirectoryLister.h
- [ ] T016 Replace the `FindFirstFile` loops in `CMultiThreadedLister::EnumerateMatchingFiles` and `CMultiThreadedLister::EnumerateSubdirectories` with `CFileEnumerator`, add a `CArchiveFileSystem *` constructor parameter (null when the switch is off) and pass it from `CDirectoryLister::ProcessDirectoryMultiThreaded`, in TCDirCore/MultiThreadedLister.cpp, TCDirCore/MultiThreadedLister.h and TCDirCore/DirectoryLister.cpp
- [ ] T017 Run the full unit test suite; the existing `DirectoryListerScenarioTests` and `DirectoryListerTests` pass with no change in output (switch-off path, SC-002)

**Checkpoint**: Archive infrastructure exists and is tested; output without the switch is identical.

## Phase 3: User Story 1 - List the Contents of an Archive (P1) 🎯 MVP

**Goal**: `tcdir --Archives release.zip` lists the archive root like a directory; archives in a listed directory appear as directories.

**Independent Test**: Scenario tests with a mocked `release.zip` containing `readme.txt`, `app.exe` and `docs\`; quickstart scenarios 1-2.

- [ ] T018 [P] [US1] Write tests for `CZipReader::Read (IByteSource &, SArchiveReadResult &)`: EOCD with and without comment, ZIP64 count/offset/sizes, UTF-8 flag, CP437 name, Unicode path extra, NTFS UTC time, DOS local time converted to UTC, DOS attributes from hosts 0/10/14, Unix host `S_IFDIR` and read-only mapping, trailing `/` folder, encryption flag sets `m_fEncrypted`, truncated central directory returns `ERROR_BAD_FORMAT`, non-zip bytes return `ERROR_BAD_FORMAT`, 10,000 entries in UnitTest/ZipReaderTests.cpp
- [ ] T019 [US1] Implement `CZipReader` per research R2 (backward EOCD scan over the last 65,557 bytes, ZIP64 locator, one central-directory read, `TzSpecificLocalTimeToSystemTime` for DOS times) in TCDirCore/ZipReader.h and TCDirCore/ZipReader.cpp
- [ ] T020 [US1] Implement `CArchiveFileAccess` (`GetAttributes` via `GetFileAttributesW`; `Read` opens `CFileByteSource` and dispatches `Zip` to `CZipReader`; other formats return `ERROR_NOT_SUPPORTED` until US5) in TCDirCore/ArchiveFileAccess.cpp
- [ ] T021 [US1] Add `bool m_fArchives = false;` and parse `--Archives` / `--Archives-` in `CCommandLine::HandleLongSwitch` (copy the `ellipsize` block), add `archives` to `IsRecognizedLongSwitch` in TCDirCore/CommandLine.h and TCDirCore/CommandLine.cpp
- [ ] T022 [P] [US1] Write tests for `--Archives`, `--archives`, `/Archives`, `--Archives-` and the "Did you mean" hint for `-archives` in UnitTest/CommandLineTests.cpp
- [ ] T023 [US1] Create `CArchiveFileSystem` in `RunDirectoryListing` when `m_fArchives` is set and pass it to `CMaskGrouper::GroupMasksByDirectory` and `CDirectoryLister` in TCDirCore/TCDir.cpp
- [ ] T024 [US1] Make `CMaskGrouper::SplitPureMask` and `SplitQualifiedMask` treat an archive file (and paths inside it) as a directory through an optional `const CArchiveFileSystem *` parameter in TCDirCore/MaskGrouper.h and TCDirCore/MaskGrouper.cpp
- [ ] T025 [US1] Make `CDirectoryLister::List` accept archive locations via `CArchiveFileSystem::IsDirectory`/`Exists`, construct `CDriveInfo` from the real archive file path, and set `CDirectoryInfo::m_fInArchive` in TCDirCore/DirectoryLister.cpp
- [ ] T026 [US1] Skip `ResolveReparseTarget` and `HandleFileMatchStreams` for `FileInfo::m_fInArchive` entries in `CDirectoryLister::AddMatchToList` in TCDirCore/DirectoryLister.cpp
- [ ] T027 [US1] Skip `IsUnderSyncRoot` and `GetFileOwners` for `CDirectoryInfo::m_fInArchive`, and pass `CDriveInfo`'s root path instead of `di.m_dirPath` to `GetDiskFreeSpaceEx` in `DisplayVolumeFooter` when `m_fInArchive` is set, in TCDirCore/ResultsDisplayerWithHeaderAndFooter.cpp, TCDirCore/ResultsDisplayerNormal.cpp, TCDirCore/ResultsDisplayerTree.cpp and TCDirCore/ResultsDisplayerWide.cpp
- [ ] T028 [US1] Print archive errors (`Error:   <path> is not a valid archive`, encrypted listing, not available offline for a directly targeted cloud placeholder, I/O) per contracts/cli.md through `m_consolePtr->ColorPrintf` in `CDirectoryLister::List`/`ProcessDirectory` in TCDirCore/DirectoryLister.cpp
- [ ] T029 [P] [US1] Write scenario tests: `--Archives release.zip` lists root entries with sizes and dates; without the switch `release.zip` is one file line; `--Archives` on the parent directory shows `release.zip` as `<DIR>`; wide and bare output; `/O:S` sorts archive entries by size; `/A:D` lists only archive folders; a directly targeted offline placeholder archive prints the offline error; `--Owner` and `--Streams` leave archive columns blank; empty archive prints the empty-directory message; corrupt archive prints one error and other targets still list in UnitTest/ArchiveListingScenarioTests.cpp
- [ ] T030 [US1] Add the `--Archives` help row from contracts/cli.md to `BuildPrimaryOptionRows` in TCDirCore/Usage.cpp
- [ ] T031 [US1] Build Debug x64, run all tests, commit the phase (include `TCDirCore/Version.h`)

**Checkpoint**: MVP: .zip archives are browsable at the root level.

## Phase 4: User Story 2 - Navigate Into Folders and Filter Inside an Archive (P2)

**Goal**: `x.zip\docs`, `x.zip\docs\*.md` and `x.zip\*.md` work like real paths.

**Independent Test**: Scenario tests on a nested mocked archive; quickstart scenario 3.

- [ ] T032 [P] [US2] Write scenario tests: `--Archives release.zip\docs` lists `guide.md` and `<DIR> api`; `release.zip\docs\*.md` lists `guide.md` only; `release.zip\*.md` matches root only; archive with no explicit folder entries shows implied folders; `release.zip\missing` prints the current "does not exist" error; without the switch `release.zip\docs` prints the current "does not exist" error; header contains `...\release.zip\docs` in UnitTest/ArchiveListingScenarioTests.cpp
- [ ] T033 [US2] Handle the inner-path forms in `CArchiveFileSystem::TrySplit` and `CMaskGrouper` so that the last component becomes the spec when it has wildcards or is not a folder in the archive, and verify grouping of several masks into one archive folder in TCDirCore/ArchiveFileSystem.cpp and TCDirCore/MaskGrouper.cpp
- [ ] T034 [P] [US2] Add mask grouping tests for archive paths (`x.zip\docs\*.md` and `x.zip\docs\*.txt` share one group) in UnitTest/MaskGroupingTests.cpp
- [ ] T035 [US2] Build, run all tests, commit the phase (include `TCDirCore/Version.h`)

## Phase 5: User Story 3 - Recursive and Tree Listings Through Archives (P2)

**Goal**: `-S` and `--Tree` descend into archives only with the switch, in both single-threaded (`-M-`) and multi-threaded modes.

**Independent Test**: Scenario tests on a mocked tree `src\lib.zip` with three levels; quickstart scenarios 4-6 and 8.

- [ ] T036 [US3] Store archive errors on `CDirectoryInfo` (`m_hr`, `m_strError`, `Status::Error`) in `CMultiThreadedLister::EnumerateDirectoryNode` and print them on the main thread in `WaitForNodeCompletion` and the tree path, with the contracts/cli.md text instead of the raw HRESULT line, in TCDirCore/MultiThreadedLister.cpp
- [ ] T037 [US3] Set `m_fInArchive` on child `CDirectoryInfo` objects created for archive locations in `CMultiThreadedLister::EnqueueChildDirectory` and in `CDirectoryLister::ProcessDirectory` in TCDirCore/MultiThreadedLister.cpp and TCDirCore/DirectoryLister.cpp
- [ ] T038 [P] [US3] Write scenario tests: `--Archives -S src` lists every folder in `lib.zip` with subtotals and the grand total includes archive files (multi-threaded and `-M-`); `--Archives --Tree src` draws archive contents under `lib.zip` and honors `--Depth`; without the switch both are identical to current output; `--Archives -S src\*.cpp` matches inside archives; a corrupt archive in the tree prints one error and recursion continues; nested `outer.zip\inner.zip` lists `inner.zip` as a file in UnitTest/ArchiveListingScenarioTests.cpp
- [ ] T039 [US3] Make nested archives inside an archive list as files (no directory marking for entries already inside an archive) in TCDirCore/FileEnumerator.cpp
- [ ] T040 [US3] Build, run all tests, commit the phase (include `TCDirCore/Version.h`)

## Phase 6: User Story 4 - Turn the Switch On by Default (P3)

**Goal**: `Archives` / `Archives-` in TCDIR and the config file; `--Env`, `--Config` and `--Settings` rows.

**Independent Test**: Config tests with a mocked environment provider; quickstart scenario 9.

- [ ] T041 [US4] Add `optional<bool> m_fArchives`, bump `SWITCH_COUNT` and `m_rgSwitchSources`, and add `archives`/`archives-` to `s_switchMappings` and `s_switchMemberOrder` in TCDirCore/Config.h and TCDirCore/Config.cpp
- [ ] T042 [US4] Apply `config.m_fArchives` in `CCommandLine::ApplyConfigDefaults` in TCDirCore/CommandLine.cpp
- [ ] T043 [US4] Add the Archives rows to `s_kSwitchInfos`, `DisplaySwitchConfiguration`, `DisplayEnvVarSwitchesSection` and `HasEnvVarSwitches`, and add `Archives` to the `<Switch>` lists in `DisplayEnvVarHelp` and `DisplayConfigFileHelp` in TCDirCore/Usage.cpp
- [ ] T044 [P] [US4] Write tests: `TCDIR=Archives` sets the default, `--Archives-` overrides it, source tracking reports the environment variable, config file `Archives` line is applied in UnitTest/ConfigTests.cpp and UnitTest/ConfigFileTests.cpp
- [ ] T045 [US4] Build, run all tests, commit the phase (include `TCDirCore/Version.h`)

## Phase 7: User Story 5 - Other Common Archive Formats (P3)

**Goal**: .iso (ISO 9660, Joliet, UDF) and the libarchive formats.

**Independent Test**: Reader tests on in-memory images; quickstart scenarios 4 and 7.

- [ ] T046 [P] [US5] Extend UnitTest/Mocks/ArchiveBuilders.cpp with an in-memory ISO 9660 image builder (primary and Joliet volume descriptors, nested directories, hidden flag, `;1` suffix) and a minimal UDF 1.02 builder (VRS, AVDP at sector 256, partition and logical volume descriptors, file set descriptor, file entries, FIDs with 8-bit and 16-bit OSTA names)
- [ ] T047 [P] [US5] Write tests for `CIsoReader`: primary-only image, Joliet preferred over primary, `;1` and trailing `.` removed, hidden flag, read-only attribute on all entries, multi-extent sizes, corrupt descriptor returns `ERROR_BAD_FORMAT` in UnitTest/IsoReaderTests.cpp
- [ ] T048 [US5] Implement `CIsoReader` (ISO 9660 + Joliet, directory-record walk with a visited-extent set against cycles) in TCDirCore/IsoReader.h and TCDirCore/IsoReader.cpp
- [ ] T049 [P] [US5] Write tests for `CUdfReader`: UDF preferred over ISO 9660 bridge, nested folders, 8-bit and 16-bit names, Extended File Entry, metadata partition returns `ERROR_NOT_SUPPORTED` and `CIsoReader` falls back to the ISO 9660 tree in UnitTest/IsoReaderTests.cpp
- [ ] T050 [US5] Implement `CUdfReader` per research R3 and call it from `CIsoReader` in TCDirCore/UdfReader.h and TCDirCore/UdfReader.cpp
- [ ] T051 [P] [US5] Write tests for the pure `CLibArchiveReader::ConvertEntry` (filetype directory, size not set, mtime to FILETIME, encrypted flag, perm without owner write → read-only) in UnitTest/LibArchiveReaderTests.cpp
- [ ] T052 [US5] Implement `CLibArchiveReader` (`std::call_once` `LoadLibraryExW (L"archiveint.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32)`, typed function pointers, `archive_read_support_filter_all` + `archive_read_support_format_all`, `archive_read_open_filename_w`, header loop with `archive_read_data_skip`, `archive_read_has_encrypted_entries`, `archive_read_free`) in TCDirCore/LibArchiveReader.h and TCDirCore/LibArchiveReader.cpp
- [ ] T053 [US5] Dispatch `Iso` and `LibArchive` in `CArchiveFileAccess::Read` and implement `IsLibArchiveAvailable`; print the Windows 11 23H2 message from contracts/cli.md when a libarchive extension is targeted directly and the DLL is absent, in TCDirCore/ArchiveFileAccess.cpp and TCDirCore/ArchiveFileSystem.cpp
- [ ] T054 [US5] Build, run all tests, commit the phase (include `TCDirCore/Version.h`)

## Phase 8: Polish and Cross-Cutting

- [ ] T055 [P] Document `--Archives` (formats, Windows 11 requirement for libarchive formats, examples) in README.md and add a CHANGELOG.md entry
- [ ] T056 Run the quickstart.md scenarios manually on a Debug x64 build, compare each format's listing against the extracted source tree (SC-005), and include the 10,000-entry timing (SC-003) and the `-P` regression check (SC-006)
- [ ] T057 Build Release x64 and ARM64, run Code Analysis with zero warnings, fix any findings
- [ ] T058 Update specs/sync-status.md with the 012 row and commit

## Dependencies

- Phase 1 → Phase 2 → US1 (Phase 3). US1 is the MVP and blocks the rest because it adds the switch and `CZipReader`.
- US2, US3 and US4 depend only on US1 and can proceed in any order.
- US5 depends on Phase 2 (`CArchiveFileSystem`, `CArchiveFileAccess`) and on US1 for end-to-end checks.
- Polish depends on all stories.

## Parallel Opportunities

- Phase 2: T004/T005, T006/T007 and T008 touch separate files.
- US1: T018 and T022 can be written while T019-T021 are in progress.
- US5: the ISO builder (T046), ISO tests (T047) and libarchive conversion tests (T051) are independent.

## Implementation Strategy

1. Phases 1-2, then US1, then stop and validate (MVP: .zip root listing).
2. US2 and US3 next (navigation and recursion deliver most of the remaining value).
3. US4, then US5, then Polish.
4. Commit at the end of every phase with the build and tests passing.
