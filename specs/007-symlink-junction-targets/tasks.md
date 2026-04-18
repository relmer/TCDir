# Tasks: Symlink & Junction Target Display

**Input**: Design documents from `specs/007-symlink-junction-targets/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, quickstart.md

**Tests**: Yes — spec SC-006 requires unit tests for buffer parsing, prefix stripping, color selection, and error handling.

**Organization**: Tasks grouped by user story to enable independent implementation and testing.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Phase 1: Setup

**Purpose**: Version bump and foundational data model changes

- [x] T001 Bump minor version from 5.3 to 5.4 in TCDirCore/Version.h (pre-completed — already applied)
- [x] T002 [P] Add `RightArrow` constant (U+2192) to TCDirCore/UnicodeSymbols.h
- [x] T003 [P] Add `wstring m_strReparseTarget` field to `FileInfo` struct in TCDirCore/DirectoryInfo.h

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Reparse buffer parsing and target resolution — MUST complete before any display work

**CRITICAL**: All user stories depend on the reparse resolver being complete

- [x] T004 Create TCDirCore/ReparsePointResolver.h — declare `REPARSE_DATA_BUFFER` struct, `ParseJunctionBuffer()`, `ParseSymlinkBuffer()`, `ParseAppExecLinkBuffer()`, `ResolveReparseTarget()`, and `StripDevicePrefix()` functions
- [x] T005 Implement `StripDevicePrefix()` pure function in TCDirCore/ReparsePointResolver.cpp — strip `\??\` prefix from paths
- [x] T006 Implement `ParseJunctionBuffer()` in TCDirCore/ReparsePointResolver.cpp — extract PrintName from MountPointReparseBuffer, fall back to SubstituteName with prefix stripped
- [x] T007 Implement `ParseSymlinkBuffer()` in TCDirCore/ReparsePointResolver.cpp — extract PrintName from SymbolicLinkReparseBuffer, fall back to SubstituteName with prefix stripped
- [x] T008 Implement `ParseAppExecLinkBuffer()` in TCDirCore/ReparsePointResolver.cpp — parse version + 3 NUL-terminated strings from GenericReparseBuffer, return third string (target exe path)
- [x] T009 Implement `ResolveReparseTarget()` in TCDirCore/ReparsePointResolver.cpp — CreateFileW + DeviceIoControl flow, dispatch to correct parser by reparse tag, return empty string on any failure
- [x] T010 Call `ResolveReparseTarget()` from `AddMatchToList()` in TCDirCore/DirectoryLister.cpp — resolve only when `FILE_ATTRIBUTE_REPARSE_POINT` is set and tag is supported
- [x] T011 [P] Add `GetTextAttrForExtension()` method to CConfig in TCDirCore/Config.h and TCDirCore/Config.cpp — extract extension from path, look up in `m_mapExtensionToTextAttr`, return default attr if not found

### Unit Tests for Foundational Phase

- [x] T012 [P] Create UnitTest/ReparsePointResolverTests.cpp — test `StripDevicePrefix()` with `\??\C:\path`, `\??\UNC\server\share`, empty string, and path without prefix
- [x] T013 [P] Extend UnitTest/ReparsePointResolverTests.cpp — test `ParseJunctionBuffer()` with synthetic REPARSE_DATA_BUFFER containing known PrintName and SubstituteName
- [x] T014 [P] Extend UnitTest/ReparsePointResolverTests.cpp — test `ParseSymlinkBuffer()` with both absolute (Flags=0) and relative (Flags=SYMLINK_FLAG_RELATIVE) synthetic buffers
- [x] T015 [P] Extend UnitTest/ReparsePointResolverTests.cpp — test `ParseAppExecLinkBuffer()` with synthetic version-3 buffer containing 3 NUL-terminated strings; test version mismatch returns empty
- [x] T016 [P] Extend UnitTest/ReparsePointResolverTests.cpp — test `ParseJunctionBuffer()` with empty PrintName falls back to SubstituteName with `\??\` stripped
- [x] T017 [P] Extend UnitTest/ReparsePointResolverTests.cpp — test `ParseAppExecLinkBuffer()` with truncated buffer (string extends past buffer bounds) returns empty

**Checkpoint**: Reparse resolver compiles, all parser tests pass. Build with `Build + Test Debug (current arch)`.

---

## Phase 3: User Story 1 — Normal Mode Target Display (Priority: P1) MVP

**Goal**: Display `→ target` after filenames for junctions, symlinks, and AppExecLinks in normal mode

**Independent Test**: Run `tcdir` in a directory containing a junction and symlink; verify `→ target` appears

### Implementation for User Story 1

- [x] T018 [US1] Modify filename rendering in TCDirCore/ResultsDisplayerNormal.cpp — split `Printf(textAttr, L"%s\n", cFileName)` into filename + conditional `→ target` + newline when `m_strReparseTarget` is non-empty
- [x] T019 [US1] Build and manually verify normal mode output with junctions and symlinks on the local filesystem
- [x] T019a [US1] Manually verify AppExecLink display: run `tcdir` in `%LOCALAPPDATA%\Microsoft\WindowsApps` and confirm `→ target_exe_path` appears for Store app aliases

### Unit Tests for User Story 1

- [x] T020 [US1] Add display test in UnitTest/ResultsDisplayerTests.cpp — verify `FileInfo` with populated `m_strReparseTarget` renders `filename → target\n` in captured output
- [x] T021 [US1] Add display test in UnitTest/ResultsDisplayerTests.cpp — verify `FileInfo` with empty `m_strReparseTarget` renders `filename\n` only (no arrow, no change)

**Checkpoint**: Normal mode shows targets for junctions/symlinks. All tests pass.

---

## Phase 4: User Story 2 — Tree Mode Target Display (Priority: P2)

**Goal**: Display `→ target` after filenames in tree mode, preserving tree connectors and non-recursion behavior

**Independent Test**: Run `tcdir --Tree` in a directory tree with a junction; verify target appears and junction is not expanded

### Implementation for User Story 2

- [x] T022 [US2] Modify filename rendering in TCDirCore/ResultsDisplayerTree.cpp — same split pattern as Normal mode: filename + conditional `→ target` + newline when `m_strReparseTarget` is non-empty
- [x] T023 [US2] Build and manually verify tree mode output — confirm junctions show targets and are not recursed into

### Unit Tests for User Story 2

- [x] T024 [US2] Add tree display test in UnitTest/ResultsDisplayerTests.cpp — verify tree entry with `m_strReparseTarget` renders `filename → target\n` with tree connectors intact
- [x] T025 [US2] Add tree display test in UnitTest/ResultsDisplayerTests.cpp — verify tree entry without reparse target renders normally (no arrow)

**Checkpoint**: Tree mode shows targets. Normal mode still works. All tests pass.

---

## Phase 5: User Story 3 — Color-Coded Target Paths (Priority: P3)

**Goal**: Arrow uses `Information` color; target path uses directory color or extension color

**Independent Test**: Run `tcdir` with a junction and a `.cpp` file symlink; verify arrow color differs from target color

### Implementation for User Story 3

- [x] T026 [US3] Update target rendering in TCDirCore/ResultsDisplayerNormal.cpp — use `CConfig::EAttribute::Information` for `→` arrow, `CConfig::EAttribute::Directory` for directory targets, `GetTextAttrForExtension()` for file targets
- [x] T027 [US3] Update target rendering in TCDirCore/ResultsDisplayerTree.cpp — same color logic as normal mode
- [x] T028 [US3] Build and manually verify colored output — arrow in Information color, targets in appropriate dir/extension colors

### Unit Tests for User Story 3

- [x] T029 [P] [US3] Add color test in UnitTest/ReparsePointResolverTests.cpp or UnitTest/ConfigTests.cpp — verify `GetTextAttrForExtension()` returns correct color for `.cpp`, `.py`, unknown extension, and extensionless path

**Checkpoint**: Colors correct in both modes. All tests pass.

---

## Phase 6: User Story 4 — Path Prefix Stripping (Priority: P3)

**Goal**: `\??\` device prefix is stripped from displayed junction targets

**Independent Test**: Already covered by T005/T012 (foundational phase) — `StripDevicePrefix()` is called during resolution, so targets stored in `m_strReparseTarget` are already clean

### Verification

- [x] T030 [US4] Verify end-to-end: create a junction, run `tcdir`, confirm target path does not show `\??\` prefix
- [x] T031 [US4] Verify `ParseJunctionBuffer()` fallback path: synthetic buffer with empty PrintName uses SubstituteName with `\??\` stripped (covered by T016)

**Checkpoint**: All user stories complete and independently verified.

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Release readiness, documentation, and validation

- [x] T032 Verify wide mode (`tcdir /W`) does NOT show `→ target` — FR-009 compliance
- [x] T033 Verify bare mode (`tcdir /B`) does NOT show `→ target` — FR-009 compliance
- [x] T034 Verify recursive mode (`tcdir -S`) shows targets but does not recurse into junctions/symlinks — FR-010 compliance
- [x] T035 Run full test suite: `Build + Test Debug (current arch)` and `Build + Test Release (current arch)` — SC-005 compliance
- [x] T036 Update CHANGELOG.md with v5.4 entry describing symlink/junction target display feature
- [x] T037 Update README.md "What's New" table with v5.4 row and feature comparison table with symlink target column
- [x] T038 Update specs/sync-status.md with spec 007 row (TCDir: Shipped, v5.4)
- [x] T039 Run quickstart.md validation — verify files listed in quickstart.md were all touched
- [x] T040 Final build: `Build All Release (x64 + ARM64)` — verify both architectures compile clean

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — can start immediately
- **Foundational (Phase 2)**: Depends on Phase 1 (T001–T003). BLOCKS all user stories.
- **User Story 1 (Phase 3)**: Depends on Foundational (T004–T017)
- **User Story 2 (Phase 4)**: Depends on Foundational (T004–T017). Independent of US1.
- **User Story 3 (Phase 5)**: Depends on US1 and/or US2 (needs display code to exist for color changes)
- **User Story 4 (Phase 6)**: Already implemented in Foundational; this phase is verification only
- **Polish (Phase 7)**: Depends on all user stories being complete

### Within Each Phase

- T004 (header) must precede T005–T009 (implementations)
- T005–T008 (parsers) can be written in parallel [P] — different functions
- T009 (resolver) depends on T005–T008 (calls all parsers)
- T010 (integration) depends on T009
- T011 (GetTextAttrForExtension) is independent [P]
- T012–T017 (tests) can all run in parallel [P] — different test methods

### Parallel Opportunities

```text
# After T004 (header), these can all proceed in parallel:
T005 StripDevicePrefix      ─┐
T006 ParseJunctionBuffer     ├─► T009 ResolveReparseTarget → T010 Integration
T007 ParseSymlinkBuffer      │
T008 ParseAppExecLinkBuffer ─┘

T011 GetTextAttrForExtension  (independent, parallel with T005–T010)

T012–T017 parser tests        (parallel with each other, after their respective implementation)
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001–T003)
2. Complete Phase 2: Foundational (T004–T017)
3. Complete Phase 3: User Story 1 (T018–T021)
4. **STOP and VALIDATE**: `tcdir` in a directory with junctions/symlinks shows `→ target`
5. This alone delivers the core value proposition

### Incremental Delivery

1. Setup + Foundational → Reparse resolver tested and working
2. Add US1 (Normal mode) → MVP: targets visible in default listing
3. Add US2 (Tree mode) → Targets visible in tree view too
4. Add US3 (Colors) → Arrow and targets color-coded properly
5. Add US4 (Verification) → Confirm prefix stripping works end-to-end
6. Polish → Release-ready with docs, changelog, full test suite

---

## Notes

- [P] tasks = different files, no dependencies — can run in parallel
- [Story] label maps task to specific user story for traceability
- All parser functions are pure (no I/O) and independently testable
- `ResolveReparseTarget()` is the only function with Win32 I/O — tested via integration, not unit mocks
- Commit after each phase or logical group
- Stop at any checkpoint to validate independently
