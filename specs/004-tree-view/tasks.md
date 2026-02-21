# Tasks: Tree View Display Mode

**Input**: Design documents from `/specs/004-tree-view/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, quickstart.md

## Format: `[ID] [P?] [Story?] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

---

## Phase 1: Setup

**Purpose**: Add tree connector constants and the new `STreeConnectorState` struct — shared infrastructure used by all user stories.

- [x] T001 Add Unicode tree connector constants (`├`, `└`, `│`, `─`) in `TCDirCore/UnicodeSymbols.h`
- [x] T002 Create `STreeConnectorState` struct with `m_vAncestorHasSibling`, `m_cTreeIndent`, `GetPrefix()`, `GetStreamContinuation()`, `Push()`, `Pop()`, `Depth()` in `TCDirCore/TreeConnectorState.h`
- [x] T003 Add unit tests for `STreeConnectorState`: prefix generation at depths 0–3, `Push`/`Pop` state transitions, last-entry vs middle-entry connectors, custom indent widths 1–8 in `UnitTest/TreeConnectorStateTests.cpp`

**Checkpoint**: Tree connector state is implemented and tested in isolation.

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Switch parsing, config parsing, and validation — these MUST be complete before any user story display work.

**⚠️ CRITICAL**: No user story display work can begin until both Phase 1 and Phase 2 are complete. Phase 1 and Phase 2 are independent and can run in parallel.

- [x] T004 Add `m_fTree` (bool), `m_cMaxDepth` (int), `m_cTreeIndent` (int) members to `CCommandLine` in `TCDirCore/CommandLine.h`
- [x] T005 Parse `--Tree` boolean long switch in `HandleLongSwitch` in `TCDirCore/CommandLine.cpp`
- [x] T006 Parse `--Depth=N` and `--TreeIndent=N` parameterized long switches (support both `=` and space separators) in `TCDirCore/CommandLine.cpp`
- [x] T007 Add post-parse validation: `--Tree` + `-W`/`-B`/`-S` conflicts, `--Depth` without `--Tree`, `--TreeIndent` without `--Tree`, `--TreeIndent` outside [1,8], `--Depth` ≤ 0 in `TCDirCore/CommandLine.cpp`
- [x] T008 [P] Add `m_fTree` (`optional<bool>`), `m_cMaxDepth` (`optional<int>`), `m_cTreeIndent` (`optional<int>`) members and `EAttribute::TreeConnector` to `CConfig` in `TCDirCore/Config.h`
- [x] T009 [P] Parse `Tree`/`Tree-`/`Depth=N`/`TreeIndent=N` from TCDIR environment variable in `TCDirCore/Config.cpp`
- [x] T010 Apply `CConfig` overrides to `CCommandLine` (Tree, Depth, TreeIndent) following existing override pattern in `TCDirCore/CommandLine.cpp` or `TCDirCore/Config.cpp`
- [x] T011 [P] Add unit tests for `--Tree`, `--Depth=N`, `--Depth N`, `--TreeIndent=N` parsing in `UnitTest/CommandLineTests.cpp`
- [x] T012 [P] Add unit tests for switch conflict validation (Tree+Wide, Tree+Bare, Tree+Recurse, Depth-without-Tree, TreeIndent-without-Tree, TreeIndent-out-of-range, Depth ≤ 0) in `UnitTest/CommandLineTests.cpp`
- [x] T013 [P] Add unit tests for TCDIR env var parsing of `Tree`/`Tree-`/`Depth=N`/`TreeIndent=N` in `UnitTest/ConfigTests.cpp`

**Checkpoint**: All switch parsing and validation works. `tcdir --Tree -W` produces an error. `tcdir --Depth=3` without `--Tree` produces an error. Env var configuration round-trips correctly.

---

## Phase 3: User Story 1 — Basic Tree Listing (Priority: P1) 🎯 MVP

**Goal**: `tcdir --Tree` displays directory contents hierarchically with tree connectors. All metadata columns (date, time, attributes, size) present at every level.

**Independent Test**: Run `tcdir --Tree` in a directory with 2+ nesting levels. Verify connectors, column alignment, and per-dir summaries.

### Implementation for User Story 1

- [x] T014 [US1] Create `CResultsDisplayerTree` class declaration (derives from `CResultsDisplayerNormal`) in `TCDirCore/ResultsDisplayerTree.h`
- [x] T015 [US1] Implement `CResultsDisplayerTree::DisplayFileResults` override — iterate `m_vMatches`, call inherited column helpers (date/time, attrs, size, cloud, debug, owner), prepend tree prefix from `STreeConnectorState` before icon/filename in `TCDirCore/ResultsDisplayerTree.cpp`
- [x] T016 [US1] Implement `CResultsDisplayerTree::DisplayResults` override — tree-walking flow: drive header for root, recursive traversal of `m_vChildren` calling `DisplayFileResults` at each level, per-dir summaries indented within tree, grand total at end in `TCDirCore/ResultsDisplayerTree.cpp`
- [x] T017 [US1] Add `m_fInterleavedSort` to `CFileComparator` and implement conditional sort (dirs+files together when true) in `TCDirCore/FileComparator.h` and `TCDirCore/FileComparator.cpp`
- [x] T018 [US1] Set `m_fInterleavedSort = true` when tree mode is active in sort setup path in `TCDirCore/MultiThreadedLister.cpp`
- [x] T019 [US1] Route `--Tree` to MT lister path (force MT even if `-M-`); instantiate `CResultsDisplayerTree` instead of `CResultsDisplayerNormal` when tree mode is active in `TCDirCore/DirectoryLister.cpp`
- [x] T020 [US1] Thread `STreeConnectorState` through `PrintDirectoryTree` → `ProcessChildren` recursion in `TCDirCore/MultiThreadedLister.h` and `TCDirCore/MultiThreadedLister.cpp`
- [x] T021 [P] [US1] Add unit tests for `CFileComparator` interleaved sort mode (dirs and files sorted together, not grouped) in `UnitTest/FileComparatorTests.cpp`
- [x] T022 [US1] Add scenario tests: basic tree output with 2–3 levels, last-entry `└──` vs middle `├──`, `│` continuation lines, per-dir summary placement in `UnitTest/DirectoryListerScenarioTests.cpp`
- [x] T022a [US1] Add scenario tests for file masks in tree mode: `tcdir --Tree *.cpp` shows only matching files while preserving directory structure; leaf directories with zero matching files and no matching descendants are pruned; intermediate directories are preserved in `UnitTest/DirectoryListerScenarioTests.cpp`

**Checkpoint**: `tcdir --Tree` produces correct hierarchical output with connectors and metadata. Per-dir summaries + grand total shown.

---

## Phase 4: User Story 2 — Depth-Limited Tree Listing (Priority: P1)

**Goal**: `tcdir --Tree --Depth=N` limits tree recursion to N levels. Directories at the depth limit appear as entries but are not expanded.

**Independent Test**: Run `tcdir --Tree --Depth=1` on a deep tree. Verify only 1 level of subdirectory contents shown.

### Implementation for User Story 2

- [x] T023 [US2] Add depth check in tree recursion — compare `STreeConnectorState::Depth()` against `m_cMaxDepth` before expanding children in `TCDirCore/MultiThreadedLister.cpp`
- [x] T024 [US2] Add scenario tests: `--Depth=1` stops at first level, `--Depth=3` shows exactly 3 levels, unlimited depth without `--Depth` in `UnitTest/DirectoryListerScenarioTests.cpp`

**Checkpoint**: Depth limiting works. Directories at the limit are listed but not expanded.

---

## Phase 5: User Story 3 — Tree View with Metadata Columns (Priority: P1)

**Goal**: Verify all optional metadata columns (`--Icons`, `--Owner`, `--Streams` display baseline) render correctly in tree mode alongside tree connectors.

**Independent Test**: Run `tcdir --Tree --Owner --Icons` and verify all columns present and aligned.

### Implementation for User Story 3

- [x] T025 [US3] Ensure icon rendering in `CResultsDisplayerTree::DisplayFileResults` places tree connector before icon glyph, with connector vertical lines aligning below parent folder icons in `TCDirCore/ResultsDisplayerTree.cpp`
- [x] T026 [US3] Ensure owner column renders via inherited `DisplayFileOwner` in tree mode in `TCDirCore/ResultsDisplayerTree.cpp`
- [x] T027 [US3] Add scenario tests: tree + icons (connector alignment below folder icons), tree + owner (owner column present at all depths) in `UnitTest/DirectoryListerScenarioTests.cpp`

**Checkpoint**: All metadata columns work in tree mode. Icon/connector alignment verified.

---

## Phase 6: User Story 4 — Tree View with Alternate Data Streams (Priority: P2)

**Goal**: `tcdir --Tree --Streams` shows stream entries below their parent file with `│` vertical continuation prefix instead of horizontal connectors.

**Independent Test**: Run `tcdir --Tree --Streams` on files with alternate data streams. Verify streams show `│` continuation, not `├──`/`└──`.

### Implementation for User Story 4

- [x] T028 [US4] Make `DisplayFileStreams` virtual in `CResultsDisplayerNormal` in `TCDirCore/ResultsDisplayerNormal.h`
- [x] T029 [US4] Implement `CResultsDisplayerTree::DisplayFileStreamsWithTreePrefix` — prepend `STreeConnectorState::GetStreamContinuation()` prefix to each stream line in `TCDirCore/ResultsDisplayerTree.cpp`
- [x] T030 [US4] Call `DisplayFileStreamsWithTreePrefix` instead of inherited `DisplayFileStreams` in tree `DisplayFileResults` loop in `TCDirCore/ResultsDisplayerTree.cpp`
- [x] T031 [US4] Add scenario tests: streams with tree connectors, stream continuation `│` prefix, streams followed by next sibling file resuming normal connectors in `UnitTest/DirectoryListerScenarioTests.cpp`

**Checkpoint**: Streams display correctly in tree mode with vertical continuation lines.

---

## Phase 7: User Story 5 — Incompatible Switch Detection (Priority: P2)

**Goal**: `tcdir --Tree -W`, `--Tree -B`, `--Tree -S` all produce clear, specific error messages without listing.

**Independent Test**: Run each incompatible combination and verify error text names both conflicting switches.

### Implementation for User Story 5

- [x] T032 [US5] Verify error messages from T007 validation name both conflicting switches (e.g., "`--Tree` and `-W` cannot be used together") in `TCDirCore/CommandLine.cpp`
- [x] T033 [US5] Add scenario tests: each incompatible combination produces correct error, compatible combinations (`--Tree --Owner --Icons`) produce no error in `UnitTest/CommandLineTests.cpp`

**Checkpoint**: All incompatible switch pairs detected with clear errors. Compatible combos work.

---

## Phase 8: User Story 6 — Environment Variable Configuration (Priority: P3)

**Goal**: `TCDIR=Tree;Depth=2` enables tree mode with depth 2 by default. CLI overrides env var.

**Independent Test**: Set `TCDIR=Tree`, run `tcdir` without flags, verify tree mode. Set `TCDIR=Tree` and run `tcdir --Tree-`, verify tree disabled.

### Implementation for User Story 6

- [x] T034 [US6] Add scenario tests: `TCDIR=Tree` activates tree, `TCDIR=Tree;Depth=3` limits depth, CLI `--Tree-` overrides env var, `TCDIR=Depth=2` without Tree is silently ignored in `UnitTest/ConfigTests.cpp` or `UnitTest/DirectoryListerScenarioTests.cpp`

**Checkpoint**: Env var configuration works. CLI overrides verified.

---

## Phase 8a: Bug Fixes (Completed During Implementation)

**Purpose**: Fixes discovered and applied during tree-mode implementation. Documented here for RCDir port reference.

- [x] T034a **Streaming Output Fix**: Add `CConsole::Flush()` call before recursing into child directories in `PrintDirectoryTreeMode` in `TCDirCore/MultiThreadedLister.cpp` — ensures parent entries are visible before child enumeration begins (see research.md R11)
- [x] T034b **Flush After Entry Loop**: Add `CConsole::Flush()` call after the entry display loop in `TCDirCore/MultiThreadedLister.cpp` — ensures all entries for current directory are flushed before proceeding
- [x] T034c **Display State Preservation**: Add `SaveDirectoryState()`/`RestoreDirectoryState()` around child directory recursion in `TCDirCore/MultiThreadedLister.cpp` — preserves parent's `m_cchStringLengthOfMaxFileSize`, `m_fDisplayedFirstEntry`, `m_cVisibleEntries`, `m_uliCurrentDirectoryTotalSize` across recursion (see research.md R12)

**Checkpoint**: Tree output streams progressively and column alignment is preserved across recursive directory transitions.

---

## Phase 9: Abbreviated Size Display (`--Size` Switch)

**Purpose**: Add `--Size=Auto|Bytes` switch for fixed-width abbreviated file sizes (Explorer-style 3-significant-digit format). This solves tree-mode column alignment without pre-scanning the entire directory tree.

### Size switch parsing

- [x] T041 Add `ESizeFormat` enum (`Default`, `Auto`, `Bytes`) and `m_eSizeFormat` member to `CCommandLine` in `TCDirCore/CommandLine.h`
- [x] T042 Parse `--Size=Auto` and `--Size=Bytes` parameterized long switch in `HandleLongSwitch` in `TCDirCore/CommandLine.cpp`
- [x] T043 [P] Add `m_eSizeFormat` (`optional<ESizeFormat>`) to `CConfig` in `TCDirCore/Config.h`
- [x] T044 [P] Parse `Size=Auto`/`Size=Bytes` from TCDIR environment variable in `TCDirCore/Config.cpp`
- [x] T045 Apply `CConfig` override for `Size` to `CCommandLine` following existing override pattern in `TCDirCore/CommandLine.cpp` or `TCDirCore/Config.cpp`
- [x] T046 [P] Add unit tests for `--Size=Auto`, `--Size=Bytes`, `--Size=Invalid` parsing in `UnitTest/CommandLineTests.cpp`
- [x] T047 [P] Add unit tests for TCDIR env var parsing of `Size=Auto`/`Size=Bytes` in `UnitTest/ConfigTests.cpp`

### Abbreviated size formatter

- [x] T048 Implement `FormatAbbreviatedSize` function: Explorer-style 3-sig-digit formatting (1024-based, B/KB/MB/GB/TB suffixes, 7-char fixed width, number right-justified / unit left-justified) in `TCDirCore/ResultsDisplayerNormal.cpp` or a new helper
- [x] T049 Format `<DIR>` centered in 7-char field (` <DIR> `) when abbreviated mode active
- [x] T050 Wire formatter into `DisplayResultsNormalFileSize`: check `m_eSizeFormat`, call abbreviated formatter when `Auto`, existing comma-separated formatter when `Bytes`
- [x] T051 Default to `Auto` in tree mode, `Bytes` in non-tree mode (when user does not explicitly specify `--Size`)

### Revert pre-scan approach

- [x] T052 ~~Remove `ComputeGlobalMaxFileSize` from `TCDirCore/MultiThreadedLister.cpp` and `.h`~~ (superseded by abbreviated sizes)
- [x] T053 ~~Remove `SetGlobalMaxFileSize`/`m_cchGlobalMaxFileSize` from `TCDirCore/ResultsDisplayerTree.h` and `.cpp`~~
- [x] T054 ~~Revert `BeginDirectory` in `ResultsDisplayerTree.cpp` to use `GetStringLengthOfMaxFileSize(...)` only~~

### Tests

- [x] T055 [P] Add unit tests for `FormatAbbreviatedSize`: 0 B, 999 B, 1 KB, 9.99 KB, 99.9 KB, 999 KB, 1.00 MB, boundary values in `UnitTest/ResultsDisplayerTests.cpp`
- [x] T056 [P] Add scenario test: tree mode output uses 7-char fixed-width size column, all rows aligned in `UnitTest/DirectoryListerScenarioTests.cpp`

**Checkpoint**: `--Size=Auto` produces Explorer-style abbreviated sizes. Tree mode defaults to Auto. All sizes uniformly 7 characters wide. Column alignment guaranteed without pre-scan.

---

## Phase 10: Polish & Cross-Cutting Concerns

**Purpose**: Help text, cycle detection, final validation.

- [x] T035 [P] Document `--Tree`, `--Depth=N`, `--TreeIndent=N`, `--Size=Auto|Bytes` in help output in `TCDirCore/Usage.cpp`
- [x] T036 Add reparse-point cycle guard: check `FILE_ATTRIBUTE_REPARSE_POINT` before recursing in `EnumerateDirectoryNode`; skip expansion for reparse points and render `[→ target]` indicator after directory name (protects both `-S` and `--Tree`) in `TCDirCore/MultiThreadedLister.cpp`
- [ ] T036a [P] Add scenario test for inaccessible directory in tree mode: access-denied directory is listed as entry but not expanded, inline error shown in `UnitTest/DirectoryListerScenarioTests.cpp` *(DEFERRED: requires mock enhancement to simulate ERROR_ACCESS_DENIED from FindFirstFileW)*
- [x] T037 [P] Add unit test for reparse-point guard (mock directory with reparse point attribute is listed but not expanded) in `UnitTest/DirectoryListerScenarioTests.cpp`
- [x] T038 [P] Add `TreeConnector` default color (LightGray) to color attribute defaults in `TCDirCore/Config.cpp`
- [x] T039 Run full test suite (`Build + Test Debug (current arch)` task) and verify all existing tests still pass — **405/405 passing**
- [x] T040 Run quickstart.md manual verification commands (all switch combinations, error cases, env var config)

**Checkpoint**: Feature complete. All tests pass. Help text updated. Cycle detection protects both `-S` and `--Tree`.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: No dependencies — start immediately
- **Phase 2 (Foundational)**: No dependencies on Phase 1 — can run in parallel with Phase 1. BLOCKS all user story display work.
- **Phase 3 (US1)**: Depends on Phase 1 + Phase 2 — core tree display
- **Phase 4 (US2)**: Depends on Phase 3 (needs working tree display to add depth limiting)
- **Phase 5 (US3)**: Depends on Phase 3 (needs working tree display to verify metadata columns)
- **Phase 6 (US4)**: Depends on Phase 3 (needs working tree display for stream continuation)
- **Phase 7 (US5)**: Depends on Phase 2 only (validation is in command-line parsing, not display)
- **Phase 8 (US6)**: Depends on Phase 2 (env var parsing) + Phase 3 (needs tree display to verify)
- **Phase 9 (Polish)**: Depends on all desired user stories being complete

### User Story Independence

- **US1 (Basic Tree)**: Foundation for all other stories — must complete first
- **US2 (Depth Limiting)**: Independent of US3–US6 after US1
- **US3 (Metadata Columns)**: Independent of US2, US4–US6 after US1
- **US4 (Streams)**: Independent of US2, US3, US5–US6 after US1
- **US5 (Switch Conflicts)**: Independent of US1–US4, US6 — only needs Phase 2
- **US6 (Env Var Config)**: Independent of US2–US5 after US1

### Within Each User Story

- Implementation tasks before scenario tests
- Core rendering before optional features
- Commit after each task or logical group

### Parallel Opportunities

Phase 1 (Setup) and Phase 2 (Foundational) are fully independent and can run in parallel.

Within Phase 2:
- T008, T009 (Config changes) can run in parallel with T004–T007 (CommandLine changes)
- T011, T012, T013 (tests) can run in parallel once their respective implementations complete

After Phase 3 (US1) completes:
- US2 (depth), US3 (metadata), US4 (streams) can run in parallel
- US5 (switch conflicts) can run in parallel starting from Phase 2 completion

Within Phase 9:
- T035 (Usage), T037 (reparse test), T038 (color default) can all run in parallel

---

## Parallel Example: After Phase 2 Completes

```text
# These can run simultaneously:
T014–T022 (US1: Basic Tree Listing)
T032–T033 (US5: Switch Conflict Detection — only needs Phase 2)
T035      (Usage help text — only needs Phase 2 for switch names)

# After US1 completes, these can run simultaneously:
T023–T024 (US2: Depth Limiting)
T025–T027 (US3: Metadata Columns)
T028–T031 (US4: Streams)
T034      (US6: Env Var Config)
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (TreeConnectorState + UnicodeSymbols)
2. Complete Phase 2: Foundational (switch parsing + validation)
3. Complete Phase 3: User Story 1 (basic tree display)
4. **STOP and VALIDATE**: `tcdir --Tree` produces correct hierarchical output
5. Build and test on both architectures

### Incremental Delivery

1. Setup + Foundational → Switch parsing works, connectors tested
2. User Story 1 → Basic tree display (MVP!)
3. User Story 2 → Depth limiting
4. User Story 3 → Metadata column verification
5. User Story 4 → Stream continuation lines
6. User Story 5 → Error messages for incompatible switches
7. User Story 6 → Environment variable defaults
8. Polish → Help text, cycle detection, final validation

Each story adds value without breaking previous stories.

---

## Notes

- [P] tasks = different files, no dependencies on in-progress tasks
- [Story] label maps task to specific user story for traceability
- All new code follows EHM patterns, formatting rules, single exit points per constitution
- All console output through `CConsole` — never use raw `std::wcout`/`std::wcerr`
- Commit after each task or logical group using conventional commits with scope
- Run `Build + Test Debug (current arch)` task after each phase to catch regressions
