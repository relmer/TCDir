# Tasks: Ellipsize Long Link Target Paths

**Input**: Design documents from `specs/008-ellipsize-targets/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, quickstart.md

**Tests**: Yes — spec SC-005 requires unit tests for truncation logic with real WindowsApps paths.

**Organization**: Tasks grouped by user story to enable independent implementation and testing.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Phase 1: Setup

**Purpose**: Version bump, new constants, and core truncation function

- [x] T001 Bump minor version from 5.4 to 5.5 in TCDirCore/Version.h (pre-completed — already applied)
- [ ] T002 [P] Add `Ellipsis` constant (U+2026) to TCDirCore/UnicodeSymbols.h
- [ ] T003 Create TCDirCore/PathEllipsis.h — declare `SEllipsizedPath` struct and `EllipsizePath()` function
- [ ] T004 Implement `EllipsizePath()` in TCDirCore/PathEllipsis.cpp — priority-based truncation algorithm: (1) first two dirs + `…\` + leaf dir + filename, (2) first two dirs + `…\` + filename, (3) first dir + `…\` + filename, (4) leaf filename only
- [ ] T005 Add PathEllipsis.h/.cpp to TCDirCore/TCDirCore.vcxproj (ClInclude and ClCompile entries)

---

## Phase 2: Foundational (Switch Infrastructure)

**Purpose**: Add `--Ellipsize` / `--Ellipsize-` switch plumbing — MUST complete before display work

- [ ] T006 Add `optional<bool> m_fEllipsize` to CConfig in TCDirCore/Config.h
- [ ] T007 Add `Ellipsize` and `Ellipsize-` entries to `s_switchMappings[]` in TCDirCore/Config.cpp; add to `s_switchMemberOrder[]`; bump `SWITCH_COUNT` from 9 to 10
- [ ] T008 Add `optional<bool> m_fEllipsize` to CCommandLine in TCDirCore/CommandLine.h
- [ ] T009 Parse `--Ellipsize` / `--Ellipsize-` in `HandleLongSwitch()` in TCDirCore/CommandLine.cpp; add to `IsRecognizedLongSwitch()` list; add to `ApplyConfigDefaults()`
- [ ] T010 [P] Add `--Ellipsize` synopsis entry, detail entry, and `s_kSwitchInfos[]` entry in TCDirCore/Usage.cpp (note: `--Settings` display is auto-covered by adding to `s_switchMemberOrder` in T007)

**Checkpoint**: Build compiles, `--Ellipsize` recognized by CLI, shows in `-?` help. No display effect yet.

---

## Phase 3: Unit Tests for EllipsizePath (Foundational)

**Purpose**: Test the pure truncation function with real WindowsApps data

- [ ] T011 Create UnitTest/PathEllipsisTests.cpp — test short path passthrough: `AzureVpn.exe` target `C:\Windows\system32\SystemUWPLauncher.exe` at width 80 returns full path, `fTruncated = false`
- [ ] T012 [P] Extend PathEllipsisTests.cpp — test priority 1 truncation (two dirs + leaf dir + filename): `notepad.exe` target at width 60, verify `C:\Program Files\…\Notepad\Notepad.exe`
- [ ] T013 [P] Extend PathEllipsisTests.cpp — test priority 2 truncation (two dirs + filename): `wingetcreate.exe` target at width 40, verify `C:\Program Files\…\WingetCreateCLI.exe`
- [ ] T014 [P] Extend PathEllipsisTests.cpp — test priority 3 truncation (one dir + filename): use crafted path `C:\LongDirName\SubDir1\SubDir2\target.exe` at a width that forces exactly priority 3 (first dir + `…\` + filename fits, but first two dirs do not)
- [ ] T015 [P] Extend PathEllipsisTests.cpp — test priority 4 fallback (leaf only): target at very narrow width, verify just `WingetCreateCLI.exe`
- [ ] T016 [P] Extend PathEllipsisTests.cpp — test two-component path `C:\file.exe` at narrow width: never truncated (nothing to elide)
- [ ] T017 [P] Extend PathEllipsisTests.cpp — test relative path `..\..\shared\config.yml`: passthrough (short paths not truncated)
- [ ] T018 [P] Extend PathEllipsisTests.cpp — test `MicrosoftWindows.DesktopStickerEditorCentennial.exe` target at width 50 (long filename + long target — most aggressive truncation)
- [ ] T019 [P] Extend PathEllipsisTests.cpp — test `GameBarElevatedFT_Alias.exe` target at 120-wide to verify correct priority level selected
- [ ] T020 Add PathEllipsisTests.cpp to UnitTest/UnitTest.vcxproj
- [ ] T020a [P] Add unit test(s) validating the available-width calculation formula in ResultsDisplayerNormal — use known metadata column widths (date=21, attrs=7, size=9, cloud=3, icon=3) and verify the computed available width matches expectations for a given console width and filename length

**Checkpoint**: All EllipsizePath tests pass. Build with `Build + Test Debug (current arch)`.

---

## Phase 4: User Story 1 — Normal Mode Ellipsize (Priority: P1) MVP

**Goal**: Ellipsize long target paths in normal mode to prevent line wrapping

**Independent Test**: Run `tcdir` in `%LOCALAPPDATA%\Microsoft\WindowsApps` at 120-char width; long targets fit on one line

### Implementation for User Story 1

- [ ] T021 [US1] Implement available-width calculation helper in TCDirCore/ResultsDisplayerNormal.cpp — compute remaining chars after metadata columns + filename + ` → `
- [ ] T022 [US1] Call `EllipsizePath()` before rendering target in TCDirCore/ResultsDisplayerNormal.cpp; render prefix in `textAttr`, `…` in `Default`, suffix in `textAttr`; skip ellipsize when `m_fEllipsize` is explicitly false
- [ ] T023 [US1] Build and manually verify normal mode output in `%LOCALAPPDATA%\Microsoft\WindowsApps` — targets fit within terminal width

**Checkpoint**: Normal mode shows truncated targets for long paths. Short paths unaffected. All tests pass.

---

## Phase 5: User Story 2 — Tree Mode Ellipsize (Priority: P2)

**Goal**: Ellipsize long target paths in tree mode, accounting for tree prefix width

### Implementation for User Story 2

- [ ] T024 [US2] Implement available-width calculation in TCDirCore/ResultsDisplayerTree.cpp — same as normal mode plus tree prefix deduction (`depth * m_cTreeIndent - 1`)
- [ ] T025 [US2] Call `EllipsizePath()` before rendering target in TCDirCore/ResultsDisplayerTree.cpp; same split-color rendering as normal mode; skip when `m_fEllipsize` is explicitly false
- [ ] T026 [US2] Build and manually verify tree mode output with long-target links

**Checkpoint**: Tree mode shows truncated targets. Normal mode still works. All tests pass.

---

## Phase 6: User Story 3 — --Ellipsize- Disable Switch (Priority: P3)

**Goal**: Verify `--Ellipsize-` disables truncation and shows full paths

### Verification

- [ ] T027 [US3] Manually verify `tcdir --Ellipsize-` in `%LOCALAPPDATA%\Microsoft\WindowsApps` shows full wrapping target paths
- [ ] T028 [US3] Add unit test verifying that EllipsizePath is not called (or returns full path) when ellipsize is disabled — test the displayer logic, not the pure function

**Checkpoint**: All user stories complete. All tests pass.

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Release readiness, documentation, and validation

- [ ] T029 Run full test suite: `Build + Test Debug (current arch)` and `Build + Test Release (current arch)` — SC-004 compliance
- [ ] T030 Update CHANGELOG.md with v5.5 entry describing ellipsize feature
- [ ] T031 Update README.md "What's New" table with v5.5 row
- [ ] T032 Update specs/sync-status.md with spec 008 row (TCDir: Shipped, v5.5, GH issue #11)
- [ ] T033 Run quickstart.md validation — verify all files listed in quickstart.md were touched
- [ ] T034 Final build: `Build All Release (x64 + ARM64)` — verify both architectures compile clean
- [ ] T035 Run Code Analysis locally to verify no new warnings

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — can start immediately
- **Foundational (Phase 2)**: Independent of Phase 1 (different files). Can proceed in parallel.
- **Unit Tests (Phase 3)**: Depends on Phase 1 (T003-T005, the EllipsizePath function)
- **User Story 1 (Phase 4)**: Depends on Phases 1-3 (needs EllipsizePath + switch infrastructure)
- **User Story 2 (Phase 5)**: Depends on Phase 4 pattern. Independent of US1 but reuses same approach.
- **User Story 3 (Phase 6)**: Depends on US1/US2 (needs display code to exist for switch verification)
- **Polish (Phase 7)**: Depends on all user stories being complete

### Within Each Phase

- T003 (header) must precede T004 (implementation)
- T006-T009 (switch plumbing) are sequential (each builds on the previous)
- T010 (Usage.cpp) is parallel with T006-T009
- T011-T019 (tests) can all run in parallel after T004
- T021 (width calc) must precede T022 (display integration)

### Parallel Opportunities

```text
Phase 1 + Phase 2 can proceed in parallel (different files):
T002 Ellipsis constant      ─┐
T003 PathEllipsis.h          ├─► T004 EllipsizePath impl → T005 vcxproj
                              │
T006-T009 Switch plumbing    ─┘
T010 Usage.cpp               (parallel with T006-T009)

T011-T019 tests (all parallel after T004)
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup (T001-T005)
2. Complete Phase 2: Switch Infrastructure (T006-T010)
3. Complete Phase 3: Unit Tests (T011-T020)
4. Complete Phase 4: Normal Mode (T021-T023)
5. **STOP and VALIDATE**: `tcdir` in WindowsApps shows truncated targets

### Incremental Delivery

1. Setup + Switch infra → EllipsizePath tested and switch recognized
2. Add US1 (Normal mode) → MVP: long targets truncated in default listing
3. Add US2 (Tree mode) → Truncation in tree view too
4. Add US3 (Verification) → --Ellipsize- works
5. Polish → Release-ready with docs, changelog, full test suite

---

## Notes

- [P] tasks = different files, no dependencies — can run in parallel
- [Story] label maps task to specific user story for traceability
- EllipsizePath is pure (no I/O) and independently testable
- All 7 WindowsApps test data entries from the spec MUST appear in PathEllipsisTests.cpp
- Commit after each phase or logical group
- Stop at any checkpoint to validate independently
