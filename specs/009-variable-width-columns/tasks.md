# Tasks: Variable-Width Columns in Wide Mode

**Input**: Design documents from `/specs/009-variable-width-columns/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, quickstart.md

**Tests**: Unit tests are included per Constitution Principle II (Testing Discipline).

**Organization**: Tasks are grouped by user story to enable independent implementation and testing.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

---

## Phase 1: Setup

**Purpose**: Declare new data structures and method signatures

- [X] T001 Define `SColumnLayout` struct and new method signatures in `TCDirCore/ResultsDisplayerWide.h`
- [X] T002 Add `ResultsDisplayerWideTests.cpp` to `UnitTest/UnitTest.vcxproj` and create empty test file in `UnitTest/ResultsDisplayerWideTests.cpp`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Pure helper functions that all user stories depend on

**⚠️ CRITICAL**: Column layout and display width helpers must be complete before any user story rendering work.

- [X] T003 Implement `ComputeDisplayWidth()` helper in `TCDirCore/ResultsDisplayerWide.cpp` — computes per-entry display width from `WIN32_FIND_DATA` attributes, icons-active flag, icon-suppressed flag, and sync-root flag (includes directory brackets, icon +2 only when `!m_fIconSuppressed`, cloud status +2)
- [X] T004 [P] Write unit tests for `ComputeDisplayWidth()` in `UnitTest/ResultsDisplayerWideTests.cpp` — test plain file, directory with brackets (icons off), directory with icons (no brackets), file with cloud status, file with icons + cloud status, file with icon suppressed (no +2)
- [X] T005 Implement `ComputeColumnLayout()` pure function in `TCDirCore/ResultsDisplayerWide.cpp` — takes vector of display widths, console width, returns `SColumnLayout` with per-column widths. Uses GNU ls-style column-major fitting: try column counts from max feasible down to 2, compute per-column widths, take first that fits. Distributes leftover space evenly across inter-column gaps.
- [X] T006 [P] Write unit tests for `ComputeColumnLayout()` in `UnitTest/ResultsDisplayerWideTests.cpp` — test: uniform widths produce equal columns (FR-007), mixed widths produce variable columns, single entry returns 1 column, all entries wider than console returns 1 column, leftover space distributed across gaps with remainder bonus to first gaps, column-major index mapping correct, SC-001 validation (100 entries: 95×15-char + 5×55-char at 120-width → assert columns ≥ 150% of uniform-width count)

**Checkpoint**: Pure layout computation is tested and correct before any rendering changes.

---

## Phase 3: User Story 1 — Better Space Utilization in Wide Mode (Priority: P1) 🎯 MVP

**Goal**: Replace uniform-width column layout with variable-width layout in `DisplayFileResults`.

**Independent Test**: Run `tc /W` on a directory with mixed-length filenames and verify more columns than before.

### Implementation for User Story 1

- [X] T007 [US1] Refactor `GetColumnInfo()` to call `ComputeDisplayWidth()` per entry and `ComputeColumnLayout()` instead of the old uniform-width formula in `TCDirCore/ResultsDisplayerWide.cpp`
- [X] T008 [US1] Update `DisplayFileResults()` to use `SColumnLayout.vColumnWidths[col]` for per-column padding instead of uniform `cxColumnWidth` in `TCDirCore/ResultsDisplayerWide.cpp`
- [X] T009 [US1] Update `DisplayFile()` signature to accept per-column width from the layout (replace uniform `cxColumnWidth` parameter) in `TCDirCore/ResultsDisplayerWide.cpp` and `TCDirCore/ResultsDisplayerWide.h`
- [X] T010 [US1] Write unit tests for column-major ordering with variable-width layout in `UnitTest/ResultsDisplayerWideTests.cpp` — verify entries map to correct columns in column-major order (entry i → column i/nRows)
- [X] T011 [US1] Build and run full test suite via `Build + Test Debug (current arch)` task
- [X] T011a [US1] Run Code Analysis via `process: Run Code Analysis (current arch)` task — must pass with zero warnings before committing Phase 3

**Checkpoint**: Wide mode produces variable-width columns. Uniform-width directories produce identical output to before.

---

## Phase 4: User Story 2 — Column-Major Ordering Preserved (Priority: P1)

**Goal**: Verify column-major ordering works correctly with variable-width columns.

**Independent Test**: Run `tc /W` on a 10-entry directory and confirm top-to-bottom fill order.

**Note**: Column-major ordering is inherent in the `ComputeColumnLayout()` algorithm from Phase 2 and the existing `DisplayFileResults()` loop. This phase validates correctness via targeted tests.

### Implementation for User Story 2

- [X] T012 [US2] Write unit tests for column-major ordering edge cases in `UnitTest/ResultsDisplayerWideTests.cpp` — test: 10 entries in 3 columns (entries 1–4 col 1, 5–7 col 2, 8–10 col 3), entries not evenly divisible by column count, single-row layout, 2-entry layout

**Checkpoint**: Column-major ordering proven correct for all entry-count / column-count combinations.

---

## Phase 5: User Story 3 — Correct Width Accounting for Icons and Cloud Status (Priority: P2)

**Goal**: Ensure per-entry display width correctly accounts for icons and cloud status on a per-entry basis.

**Independent Test**: Run `tc /W /I` on a directory with icons and verify no truncation or misalignment.

### Implementation for User Story 3

- [X] T013 [US3] Write unit tests for display width with icons and cloud status in `UnitTest/ResultsDisplayerWideTests.cpp` — test: entries with icons (+2), entries with cloud status (+2), entries with both (+4), mixed entries where some have icons and some don't
- [X] T014 [US3] Write unit tests for column layout with per-entry icon/cloud width variations in `UnitTest/ResultsDisplayerWideTests.cpp` — verify column widths account for per-entry icon/cloud width, not global flags

**Checkpoint**: Icons and cloud status correctly contribute to per-entry width in variable-width layout.

---

## Phase 6: Outlier Truncation (FR-008)

**Goal**: Truncate filenames exceeding 2× median display width with ellipsis; respect `--Ellipsize-` opt-out.

**Independent Test**: Run `tc /W` on a directory with one 63-char name and many short names; verify outlier is truncated with `…`.

### Implementation

- [X] T015 Implement `ComputeMedianDisplayWidth()` helper in `TCDirCore/ResultsDisplayerWide.cpp` — takes vector of display widths, returns median using `nth_element`
- [X] T016 [P] Write unit tests for `ComputeMedianDisplayWidth()` in `UnitTest/ResultsDisplayerWideTests.cpp` — test: odd count, even count (lower middle), single entry, two entries, all same width
- [X] T017 Implement outlier truncation in `ComputeColumnLayout()` — when ellipsis enabled, compute median, cap entries > `max(2× median, 20)` at truncCap in `TCDirCore/ResultsDisplayerWide.cpp`
- [X] T018 Update `DisplayFile()` to right-truncate outlier names with `…` at render time when their display width exceeds `truncCap` in `TCDirCore/ResultsDisplayerWide.cpp`
- [X] T019 [P] Write unit tests for outlier truncation in `UnitTest/ResultsDisplayerWideTests.cpp` — test: one outlier truncated, no outliers (all within threshold), all entries same width (no truncation), `--Ellipsize-` disables truncation
- [X] T020 Build and run full test suite via `Build + Test Debug (current arch)` task
- [X] T020a Run Code Analysis via `process: Run Code Analysis (current arch)` task — must pass with zero warnings before committing Phase 6

**Checkpoint**: Outlier filenames truncated with `…`; opt-out via `--Ellipsize-` works.

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Edge cases, build verification, and cleanup.

- [X] T021 [P] Write edge case unit tests in `UnitTest/ResultsDisplayerWideTests.cpp` — test: empty directory (0 entries), single file, terminal width < shortest filename, terminal width = 1
- [X] T022 Build Release for both architectures via `process: Build All Release (x64 + ARM64) (no deploy)` task
- [X] T022a Run Code Analysis via `process: Run Code Analysis (current arch)` task — final clean analysis before merge
- [X] T023 Run quickstart.md validation — manually verify the examples in `specs/009-variable-width-columns/quickstart.md` match actual `tc /W` output

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: No dependencies — start immediately
- **Phase 2 (Foundational)**: Depends on Phase 1 — BLOCKS all user stories
- **Phase 3 (US1)**: Depends on Phase 2
- **Phase 4 (US2)**: Depends on Phase 3 (validates ordering in the refactored code)
- **Phase 5 (US3)**: Depends on Phase 3 (validates icon/cloud width in the refactored code)
- **Phase 6 (FR-008)**: Depends on Phase 3 (builds on the variable-width algorithm)
- **Phase 7 (Polish)**: Depends on Phases 3–6

### Parallel Opportunities

- T004 and T006 can run in parallel (independent test functions)
- T013 and T014 can run in parallel (independent test scopes)
- T016 and T019 can run in parallel with T015/T017 (test-first for independent helpers)
- T021 can run in parallel with T022

### Within Each Phase

- Tests can be written before implementation (TDD where marked [P])
- Build verification tasks gate phase completion

---

## Parallel Example: Phase 2

```text
# These can be developed in parallel (different functions, different test cases):
T003: Implement ComputeDisplayWidth()     T005: Implement ComputeColumnLayout()
T004: Test ComputeDisplayWidth()          T006: Test ComputeColumnLayout()
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001–T002)
2. Complete Phase 2: Foundational (T003–T006)
3. Complete Phase 3: User Story 1 (T007–T011)
4. **STOP and VALIDATE**: `tc /W` produces variable-width columns; uniform-width directories unchanged
5. This is a deployable MVP — variable-width columns work without outlier truncation

### Incremental Delivery

1. Setup + Foundational → Layout helpers ready
2. Add User Story 1 → Variable-width columns working (MVP!)
3. Add User Story 2 → Column ordering validated
4. Add User Story 3 → Icon/cloud width verified
5. Add Outlier Truncation → Full feature complete
6. Polish → Edge cases covered, both architectures verified

---

## Notes

- [P] tasks = different files or independent functions, no dependencies
- [Story] label maps task to specific user story for traceability
- Constitution Principle V: Keep all new functions under 50 lines
- Constitution Principle II: All tests use synthetic data (no real filesystem)
- Commit after each phase completion
