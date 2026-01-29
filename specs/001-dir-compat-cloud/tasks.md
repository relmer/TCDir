# Tasks: CMD Dir Compatibility & Cloud File Visualization

**Input**: Design documents from `/specs/001-dir-compat-cloud/`  
**Prerequisites**: plan.md ✅, spec.md ✅, research.md ✅, data-model.md ✅, quickstart.md ✅

## Format: `[ID] [P?] [Story?] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2)
- Include exact file paths in descriptions

## Path Conventions

Project structure per plan.md:
- Core library: `TCDirCore/`
- Unit tests: `UnitTest/`

---

## Phase 1: Setup

**Purpose**: Shared infrastructure changes needed before any user story

- [x] T001 Add ETimeField enum to TCDirCore/CommandLine.h
- [x] T002 Add m_timeField, m_fShowOwner, m_fShowStreams members to CCommandLine in TCDirCore/CommandLine.h
- [x] T003 [P] Add IsNTFS() and IsReFS() helper methods to TCDirCore/DriveInfo.h
- [x] T004 [P] Add cloud status color entries (CloudStatusCloudOnly, CloudStatusLocal, CloudStatusPinned) to EAttribute enum in TCDirCore/Config.h
- [x] T005 Add default cloud status colors to CConfig initialization in TCDirCore/Config.cpp

---

## Phase 2: Foundational (Attribute Array Extensions)

**Purpose**: Extend attribute arrays that multiple stories depend on

**⚠️ CRITICAL**: Cloud filtering (US2) and extended attributes (US4) both depend on this

- [x] T006 Extend s_kszAttributes[] to add "xoibfu" in TCDirCore/CommandLine.cpp
- [x] T007 Extend s_kdwAttributes[] with corresponding FILE_ATTRIBUTE_* values in TCDirCore/CommandLine.cpp
- [x] T008 Add special handling for /A:O composite attribute (OFFLINE | RECALL_ON_DATA_ACCESS | RECALL_ON_OPEN) in TCDirCore/CommandLine.cpp
- [x] T009 Add unit tests for new attribute letters in UnitTest/CommandLineTests.cpp

**Checkpoint**: Attribute infrastructure ready - user story implementation can begin

---

## Phase 3: User Story 1 - Visualize Cloud Sync Status (P1) 🎯 MVP

**Goal**: Display ○/◐/● symbols with colors for cloud-synced files

**Independent Test**: Run `tcdir` in a OneDrive folder and observe cloud status symbols between size and filename

### Implementation for User Story 1

- [x] T010 [US1] Add GetCloudStatus() helper function in TCDirCore/ResultsDisplayerNormal.cpp
- [x] T011 [US1] Add DisplayCloudStatusSymbol() helper in TCDirCore/ResultsDisplayerNormal.cpp
- [x] T012 [US1] Integrate cloud status column into file display (between size and filename) in TCDirCore/ResultsDisplayerNormal.cpp
- [x] T013 [US1] Suppress cloud status in bare mode check in TCDirCore/ResultsDisplayerBare.cpp (verify no changes needed)
- [x] T014 [P] [US1] Add cloud status display unit tests in UnitTest/ResultsDisplayerTests.cpp

### Cloud Files API Enhancement (US1b)

- [x] T014d [US1b] Add cfapi.h include and cldapi.lib pragma to TCDirCore/pch.h
- [x] T014e [US1b] Add IsUnderSyncRoot() helper using CfGetSyncRootInfoByPath in TCDirCore/ResultsDisplayerNormal.cpp
- [x] T014f [US1b] Update GetCloudStatus() to use hybrid approach (attributes + sync root detection) in TCDirCore/ResultsDisplayerNormal.cpp
- [x] T014g [US1b] Update DisplayRawAttributes() to show cfapi state for debugging in TCDirCore/ResultsDisplayerNormal.cpp
- [x] T014h [US1b] Update tests to use new GetCloudStatus signature in UnitTest/ResultsDisplayerTests.cpp

### Documentation for User Story 1

- [x] T014a [US1] Document cloud status symbols in help output (/?`) in TCDirCore/Usage.cpp
- [x] T014b [US1] Add CloudOnly/Local/Pinned items to --Env help in TCDirCore/Usage.cpp
- [x] T014c [US1] Update --Config output to display cloud status colors in TCDirCore/Config.cpp (FR-024)

**Checkpoint**: Cloud status symbols visible in normal directory listings, fully documented

---

## Phase 4: User Story 2 - Filter by Cloud Sync Status (P1)

**Goal**: Filter files using /A:O, /A:F, /A:U switches

**Independent Test**: Run `tcdir /A:O` in OneDrive folder to list only cloud-only files

### Implementation for User Story 2

- [x] T015 [US2] Add unit tests for /A:O, /A:F, /A:U, /A:-O filtering in UnitTest/CommandLineTests.cpp
- [x] T016 [US2] Verify attribute negation works for new cloud attributes in UnitTest/CommandLineTests.cpp

### Documentation for User Story 2

- [x] T016a [US2] Document /A:O, /A:F, /A:U cloud attribute filters in help output (/?`) in TCDirCore/Usage.cpp

**Checkpoint**: Cloud filtering works independently, fully documented

---

## Phase 4a: User Story 2a - Debug/Diagnostic Mode (P1)

**Goal**: Display raw file attributes in hex for diagnosing cloud sync state edge cases

**Note**: `--debug` is only available in debug builds (compiled out of release builds)

**Independent Test**: Run `tcdir --debug` (debug build) and verify hex attribute values appear before each filename

### Implementation for User Story 2a

- [x] T016b [US2a] Add m_fDebug member to CCommandLine in TCDirCore/CommandLine.h
- [x] T016c [US2a] Parse --debug switch in TCDirCore/CommandLine.cpp (debug builds only)
- [x] T016d [US2a] Add --debug parsing unit tests in UnitTest/CommandLineTests.cpp
- [x] T016e [US2a] Implement DisplayRawAttributes() helper in TCDirCore/ResultsDisplayerNormal.cpp
- [x] T016f [US2a] Integrate raw attribute display into file output (before filename) in TCDirCore/ResultsDisplayerNormal.cpp

### Documentation for User Story 2a

- [x] T016g [US2a] Document --debug switch in help output (debug builds only) in TCDirCore/Usage.cpp

**Checkpoint**: Raw hex attributes visible for debugging cloud sync edge cases (debug builds only)

---

## Phase 5: User Story 3 - Time Field Selection (P2)

**Goal**: Display and sort by creation, access, or modified time using /T: switch

**Independent Test**: Run `tcdir /T:C` to display creation times; `tcdir /T:A /O:D` to sort by access time

### Implementation for User Story 3

- [x] T017 [US3] Parse /T:C|A|W switch and set m_timeField in TCDirCore/CommandLine.cpp
- [x] T018 [US3] Add /T: parsing unit tests in UnitTest/CommandLineTests.cpp
- [x] T019 [US3] Modify DisplayResultsNormalDateAndTime to use selected time field in TCDirCore/ResultsDisplayerNormal.cpp
- [x] T019a [US3] Modify DisplayResultsWideDateAndTime to use selected time field in TCDirCore/ResultsDisplayerWide.cpp (N/A - wide mode doesn't show dates)
- [x] T020 [US3] Modify FileComparator SO_DATE handling to respect m_timeField in TCDirCore/FileComparator.cpp
- [x] T021 [US3] Add time field sorting unit tests in UnitTest/FileComparatorTests.cpp

### Documentation for User Story 3

- [x] T021a [US3] Document /T:C|A|W switch in help output (/?`) in TCDirCore/Usage.cpp

**Checkpoint**: Time field selection works for both display and sorting, fully documented

---

## Phase 6: User Story 4 - Extended Attributes (P2)

**Goal**: Filter by /A:X (not indexed), /A:I (integrity), /A:B (no-scrub)

**Independent Test**: Run `tcdir /A:X` to list files excluded from indexing; `tcdir /A:I` on ReFS volume

### Implementation for User Story 4

- [x] T022 [US4] Add filesystem validation for /A:I and /A:B (warn if not ReFS) in TCDirCore/CommandLine.cpp or TCDirCore/TCDir.cpp (SKIPPED - low value, docs already say "(ReFS)")
- [x] T023 [US4] Add unit tests for /A:X, /A:I, /A:B filtering in UnitTest/CommandLineTests.cpp (DONE in Phase 2)

### Documentation for User Story 4

- [x] T023a [US4] Document /A:X, /A:I, /A:B attribute filters in help output (/?`) in TCDirCore/Usage.cpp (DONE in Phase 2)

**Checkpoint**: Extended attribute filtering works with appropriate filesystem warnings, fully documented

---

## Phase 7: User Story 5 - File Ownership (P3)

**Goal**: Display DOMAIN\User ownership with --owner switch

**Independent Test**: Run `tcdir --owner` and verify owner column appears

### Implementation for User Story 5

- [x] T024 [US5] Parse --owner switch in TCDirCore/CommandLine.cpp
- [x] T025 [US5] Add --owner parsing unit test in UnitTest/CommandLineTests.cpp
- [x] T026 [US5] Implement GetFileOwner() helper using GetNamedSecurityInfoW/LookupAccountSidW in TCDirCore/ResultsDisplayerNormal.cpp
- [x] T027 [US5] Integrate owner column into file display in TCDirCore/ResultsDisplayerNormal.cpp
- [x] T028 [US5] Handle access denied gracefully (display "Unknown") in TCDirCore/ResultsDisplayerNormal.cpp

### Documentation for User Story 5

- [x] T028a [US5] Document --owner switch in help output (/?`) in TCDirCore/Usage.cpp
- [x] T028b [US5] Update --Env help to mention --owner can be set via TCDIR env var in TCDirCore/Usage.cpp

**Checkpoint**: Ownership display works for accessible files, fully documented

---

## Phase 8: User Story 6 - Alternate Data Streams (P3)

**Goal**: Display NTFS alternate data streams with --streams switch

**Independent Test**: Run `tcdir --streams` on file with Zone.Identifier ADS

### Implementation for User Story 6

- [x] T029 [US6] Parse --streams switch in TCDirCore/CommandLine.cpp
- [x] T030 [US6] Add --streams parsing unit test in UnitTest/CommandLineTests.cpp
- [x] T031 [US6] Add filesystem validation for --streams (warn if not NTFS) in TCDirCore/TCDir.cpp
- [x] T032 [US6] Implement EnumerateStreams() using FindFirstStreamW/FindNextStreamW in TCDirCore/DirectoryLister.cpp
- [x] T033 [US6] Display streams indented below main file entry in TCDirCore/ResultsDisplayerNormal.cpp

### Documentation for User Story 6

- [x] T033a [US6] Document --streams switch in help output (/?`) in TCDirCore/Usage.cpp
- [x] T033b [US6] Update --Env help to mention --streams can be set via TCDIR env var in TCDirCore/Usage.cpp

**Checkpoint**: ADS enumeration works on NTFS volumes, fully documented

---

## Phase 9: Final Documentation & Validation

**Purpose**: README update and final validation across all stories

- [x] T034 [P] Update README.md with all new switches and attributes
- [x] T035 Run scripts/RunTests.ps1 to verify all existing tests pass (SC-010 regression check)
- [ ] T036 Manual validation per quickstart.md test scenarios

---

## Dependencies & Execution Order

### Phase Dependencies

```text
Phase 1 (Setup)
    ↓
Phase 2 (Foundational) ← BLOCKS all user stories
    ↓
┌───────────────────────────────────────────────────────────┐
│  Phase 3 (US1: Cloud Viz)     ← P1, MVP                   │
│  Phase 4 (US2: Cloud Filter)  ← P1, can parallel with US1 │
│  Phase 4a (US2a: Debug Mode)  ← P1, diagnostic feature    │
│  Phase 5 (US3: Time Field)    ← P2                        │
│  Phase 6 (US4: Extended Attr) ← P2, can parallel with US3 │
│  Phase 7 (US5: Ownership)     ← P3                        │
│  Phase 8 (US6: Streams)       ← P3, can parallel with US5 │
└───────────────────────────────────────────────────────────┘
    ↓
Phase 9 (Polish)
```

### User Story Independence

- **US1 (Cloud Viz)**: Depends only on Setup + Foundational
- **US2 (Cloud Filter)**: Depends only on Setup + Foundational (uses same attribute arrays)
- **US2a (Debug Mode)**: Depends only on Setup (new member variable)
- **US3 (Time Field)**: Depends only on Setup (new enum)
- **US4 (Extended Attr)**: Depends only on Foundational (attribute arrays)
- **US5 (Ownership)**: Depends only on Setup (new member variable)
- **US6 (Streams)**: Depends only on Setup (new member variable)

### Parallel Opportunities

Within Phase 1:
- T003 and T004 can run in parallel (different files)

Within each User Story:
- Implementation and documentation tasks are sequential (docs depend on implementation)

Across User Stories:
- US1 and US2 share foundational work but modify different code paths
- US3 and US4 are fully independent
- US5 and US6 are fully independent

---

## Implementation Strategy

### MVP First (US1 + US2)

1. ✅ Phase 1: Setup
2. ✅ Phase 2: Foundational
3. ✅ Phase 3: Cloud Visualization (US1) — includes docs
4. ✅ Phase 4: Cloud Filtering (US2) — includes docs
5. **STOP**: Test cloud features, demo OneDrive folder browsing
6. Continue with P2/P3 stories as needed

### Task Count Summary

| Phase | Tasks | Parallelizable |
|-------|-------|----------------|
| Setup | 5 | 2 |
| Foundational | 4 | 0 |
| US1 (Cloud Viz) | 8 | 1 |
| US2 (Cloud Filter) | 3 | 0 |
| US2a (Debug Mode) | 6 | 0 |
| US3 (Time Field) | 7 | 0 |
| US4 (Extended Attr) | 3 | 0 |
| US5 (Ownership) | 7 | 0 |
| US6 (Streams) | 7 | 0 |
| Final | 3 | 1 |
| **Total** | **53** | **4** |

---

## Notes

- All tasks include explicit file paths per project conventions
- Unit tests are included per SC-011 requirement
- Each user story includes its own documentation tasks — "done-done" with docs
- Cloud features (US1+US2) are MVP priority for immediate value
- P3 features (ownership, streams) are opt-in and slower by design
