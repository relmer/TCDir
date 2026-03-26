# Tasks: PowerShell Alias Configuration

**Input**: Design documents from `/specs/005-powershell-aliases/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, quickstart.md

**Tests**: Included — the spec requires unit tests (Constitution Principle II: Testing Discipline).

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project scaffolding, new headers in pch.h, new source files added to project

- [X] T001 Add `<tlhelp32.h>` and `<shlobj.h>` includes to `TCDirCore/pch.h`
- [X] T002 Add new switch bool members (`m_fSetAliases`, `m_fGetAliases`, `m_fRemoveAliases`, `m_fWhatIf`) to `TCDirCore/CommandLine.h`
- [X] T003 Add long switch entries for `set-aliases`, `get-aliases`, `remove-aliases`, `whatif` to `s_krgLongSwitches[]` in `TCDirCore/CommandLine.cpp`
- [X] T004 Add mutual exclusivity validation for alias switches and `--whatif` in `ValidateSwitchCombinations()` in `TCDirCore/CommandLine.cpp`
- [X] T005 [P] Add data model structs and enums (`EPowerShellVersion`, `EProfileScope`, `SProfileLocation`, `SAliasDefinition`, `SAliasConfig`, `SAliasBlock`) to `TCDirCore/AliasManager.h`
- [X] T006 [P] Create empty `TCDirCore/ProfilePathResolver.h` and `TCDirCore/ProfilePathResolver.cpp` with class skeleton, add to `TCDirCore/TCDirCore.vcxproj`
- [X] T007 [P] Create empty `TCDirCore/ProfileFileManager.h` and `TCDirCore/ProfileFileManager.cpp` with class skeleton, add to `TCDirCore/TCDirCore.vcxproj`
- [X] T008 [P] Create empty `TCDirCore/AliasBlockGenerator.h` and `TCDirCore/AliasBlockGenerator.cpp` with class skeleton, add to `TCDirCore/TCDirCore.vcxproj`
- [X] T009 [P] Create empty `TCDirCore/TuiWidgets.h` and `TCDirCore/TuiWidgets.cpp` with class skeleton, add to `TCDirCore/TCDirCore.vcxproj`
- [X] T010 [P] Create empty `TCDirCore/AliasManager.cpp` with class skeleton, add to `TCDirCore/TCDirCore.vcxproj`
- [X] T011 Add unit test for new command-line switches (parse, validate mutual exclusivity, `--whatif` without alias switch errors) in `UnitTest/CommandLineTests.cpp`
- [X] T012 Build and verify all tests pass

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure modules that ALL user stories depend on

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

### ProfilePathResolver (Layer 0)

- [X] T013 Implement parent process detection: `CreateToolhelp32Snapshot` → find parent PID → `OpenProcess` → `QueryFullProcessImageNameW` → extract exe name → return `EPowerShellVersion` in `TCDirCore/ProfilePathResolver.cpp`
- [X] T014 Implement profile path resolution: `SHGetKnownFolderPath(FOLDERID_Documents)` for per-user paths, `SHGetKnownFolderPath(FOLDERID_ProgramData)` for all-users paths, build 4 `SProfileLocation` structs for detected PS version in `TCDirCore/ProfilePathResolver.cpp`
- [X] T015 Implement admin privilege detection for AllUsers scopes (check write access or token elevation) in `TCDirCore/ProfilePathResolver.cpp`
- [X] T016 [P] Create `UnitTest/ProfilePathResolverTests.cpp`: test path construction for PS7+ and PS5.1, test admin detection, test Unknown parent handling; add to `UnitTest/UnitTest.vcxproj`

### AliasBlockGenerator (Layer 0)

- [X] T017 Implement `Generate()`: given `SAliasConfig`, produce complete alias block string with opening/closing markers (FR-040), version comment (FR-044), root function (FR-042), and sub-alias functions (FR-043) in `TCDirCore/AliasBlockGenerator.cpp`
- [X] T018 Implement tcdir invocation resolution: `GetModuleFileNameW` for exe path, `SearchPathW` to check PATH reachability, set `SAliasConfig::strTcDirInvocation` accordingly (FR-041) in `TCDirCore/AliasManager.cpp` (result passed to generator via `SAliasConfig`)
- [X] T019 [P] Create `UnitTest/AliasBlockGeneratorTests.cpp`: test generated block format, marker comments, version stamp, root alias variations, sub-alias toggling, short-name vs full-path invocation; add to `UnitTest/UnitTest.vcxproj`

### ProfileFileManager (Layer 1)

- [X] T020 Implement `ReadProfileFile()`: read UTF-8 file (with/without BOM) into `vector<wstring>` lines in `TCDirCore/ProfileFileManager.cpp`
- [X] T021 Implement `FindAliasBlock()`: scan lines for opening/closing markers, return `SAliasBlock` with line indices and parsed alias names in `TCDirCore/ProfileFileManager.cpp`
- [X] T022 Implement `WriteProfileFile()`: create timestamped `.bak` backup (FR-070), write lines back preserving encoding, create parent directories if needed (FR-072) in `TCDirCore/ProfileFileManager.cpp`
- [X] T023 Implement `ReplaceAliasBlock()`: remove lines[start..end], insert new block at same position in `TCDirCore/ProfileFileManager.cpp`
- [X] T024 Implement `AppendAliasBlock()`: append block at end of file in `TCDirCore/ProfileFileManager.cpp`
- [X] T025 Implement `RemoveAliasBlock()`: delete lines[start..end] inclusive (FR-054, FR-055) in `TCDirCore/ProfileFileManager.cpp`
- [X] T026 [P] Create `UnitTest/ProfileFileManagerTests.cpp`: test read/write round-trip, BOM preservation, marker detection, backup creation, block replace/append/remove, error on locked file; add to `UnitTest/UnitTest.vcxproj`

### TuiWidgets (Layer 2)

- [X] T027 Implement `TuiInit()` / `TuiCleanup()`: save/restore console mode and cursor visibility, set raw input mode, flush input buffer in `TCDirCore/TuiWidgets.cpp`
- [X] T028 Implement `TextInput` widget: prompt with default value, accept 1-4 alphanumeric chars, Enter confirms, Escape cancels (FR-020, FR-021) in `TCDirCore/TuiWidgets.cpp`
- [X] T029 Implement `CheckboxList` widget: render `[●]`/`[ ]` items with `❯` focus indicator, arrow keys navigate, Space toggles, Enter confirms, Escape cancels (FR-012, FR-013, FR-022, FR-024) in `TCDirCore/TuiWidgets.cpp`
- [X] T030 Implement `RadioButtonList` widget: render `(●)`/`( )` items with `❯` focus indicator, arrow keys navigate, Enter selects, Escape cancels (FR-011, FR-013, FR-025) in `TCDirCore/TuiWidgets.cpp`
- [X] T031 Implement `ConfirmationPrompt` widget: display preview text, Y/N prompt, Enter confirms, Escape cancels (FR-029) in `TCDirCore/TuiWidgets.cpp`
- [X] T032 [P] Create `UnitTest/TuiWidgetsTests.cpp`: test widget state transitions with simulated key input sequences, test Escape cancellation, test cursor visibility restore; add to `UnitTest/UnitTest.vcxproj`

- [X] T033 Build and verify all foundational tests pass

**Checkpoint**: Foundation ready — user story implementation can now begin

---

## Phase 3: User Story 1 — First-Time Alias Setup (Priority: P1) 🎯 MVP

**Goal**: User runs `tcdir --set-aliases`, completes the wizard, and working aliases appear in their chosen profile.

**Independent Test**: Run `tcdir --set-aliases` with no existing aliases. Complete wizard, reload profile, verify aliases work.

- [X] T034 [US1] Implement `SetAliases()` orchestration in `TCDirCore/AliasManager.cpp`: detect PS version → resolve paths → scan for existing blocks → run TUI wizard (root alias → sub-aliases → profile location → preview) → generate block → write to profile
- [X] T035 [US1] Wire TUI wizard steps: call `TextInput` for root alias, recalculate sub-alias names, call `CheckboxList` for sub-aliases, call `RadioButtonList` for profile location (with admin markers per FR-028), call `ConfirmationPrompt` for preview in `TCDirCore/AliasManager.cpp`
- [X] T036 [US1] Handle "session only" storage option (FR-027): if selected, output alias block to console with instructions to paste, skip file write in `TCDirCore/AliasManager.cpp`
- [X] T037 [US1] Handle existing alias detection (FR-030): if marker block found during scan, inform user with current aliases and offer to replace in `TCDirCore/AliasManager.cpp`
- [X] T038 [US1] Add dispatch in `TCDir.cpp`: if `m_fSetAliases` is set, call `CAliasManager::SetAliases()` and exit before directory listing
- [X] T039 [US1] Add `--set-aliases` help text to `TCDirCore/Usage.cpp`
- [X] T040 [P] [US1] Create `UnitTest/AliasManagerTests.cpp`: test SetAliases flow end-to-end with mocked file system (new profile, existing profile with block, session-only mode); add to `UnitTest/UnitTest.vcxproj`
- [X] T041 [US1] Build and verify all US1 tests pass

**Checkpoint**: User Story 1 complete — first-time setup works end-to-end

---

## Phase 4: User Story 2 — View Current Aliases (Priority: P1)

**Goal**: User runs `tcdir --get-aliases` and sees a formatted summary of all tcdir aliases in profile files.

**Independent Test**: Set up aliases in a profile, run `tcdir --get-aliases`, verify output shows alias names, mappings, and source locations.

- [X] T042 [US2] Implement `GetAliases()` in `TCDirCore/AliasManager.cpp`: detect PS version → resolve paths → scan all 4 profiles for marker blocks → format and display results grouped by profile (FR-060, FR-061, FR-062)
- [X] T043 [US2] Handle "no aliases found" case: display message suggesting `--set-aliases` (FR-062, spec US2 scenario 2) in `TCDirCore/AliasManager.cpp`
- [X] T044 [US2] Add dispatch in `TCDir.cpp`: if `m_fGetAliases` is set, call `CAliasManager::GetAliases()` and exit
- [X] T045 [US2] Add `--get-aliases` help text to `TCDirCore/Usage.cpp`
- [X] T046 [US2] Add GetAliases tests to `UnitTest/AliasManagerTests.cpp`: test with aliases in one profile, multiple profiles, no aliases found

**Checkpoint**: User Story 2 complete — users can inspect their alias state

---

## Phase 5: User Story 3 — Update Existing Aliases (Priority: P2)

**Goal**: User runs `tcdir --set-aliases` again with a different root or sub-alias selection, and the existing block is cleanly replaced.

**Independent Test**: Set aliases with root `d`, run `--set-aliases` again with root `tc`, verify old block replaced with new one.

- [X] T047 [US3] Enhance `SetAliases()` flow: when existing block detected, show current aliases, pre-populate wizard defaults from existing config (root alias, enabled sub-aliases, current profile) in `TCDirCore/AliasManager.cpp`
- [X] T048 [US3] Implement block replacement path: use `ProfileFileManager::ReplaceAliasBlock()` instead of `AppendAliasBlock()` when existing block found in `TCDirCore/AliasManager.cpp`
- [X] T049 [US3] Add update tests to `UnitTest/AliasManagerTests.cpp`: test root change (d→tc), sub-alias toggle change, same-root different-subs

**Checkpoint**: User Story 3 complete — update flow works

---

## Phase 6: User Story 4 — Remove Aliases (Priority: P2)

**Goal**: User runs `tcdir --remove-aliases`, selects a profile, and the alias block is cleanly removed.

**Independent Test**: Have aliases in a profile, run `--remove-aliases`, verify block removed and rest of profile untouched.

- [X] T050 [US4] Implement `RemoveAliases()` in `TCDirCore/AliasManager.cpp`: detect PS version → resolve paths → scan for blocks → if none found display message and exit (FR-053) → present radio list of profiles with aliases (FR-051, FR-052) → confirm → remove block via `ProfileFileManager::RemoveAliasBlock()`
- [X] T051 [US4] Add dispatch in `TCDir.cpp`: if `m_fRemoveAliases` is set, call `CAliasManager::RemoveAliases()` and exit
- [X] T052 [US4] Add `--remove-aliases` help text to `TCDirCore/Usage.cpp`
- [X] T053 [US4] Add RemoveAliases tests to `UnitTest/AliasManagerTests.cpp`: test successful removal, no-aliases-found case, profile content preservation

**Checkpoint**: User Story 4 complete — clean uninstall of aliases works

---

## Phase 7: User Story 5 — Dry Run with --whatif (Priority: P2)

**Goal**: User appends `--whatif` to `--set-aliases` or `--remove-aliases` and sees a preview without file modifications.

**Independent Test**: Run `--set-aliases --whatif`, complete wizard, verify no files modified and console shows preview.

- [X] T054 [US5] Add `--whatif` integration to `SetAliases()`: after wizard completes and block is generated, display block content and target path, print "What if: No changes were made" message, skip all file operations in `TCDirCore/AliasManager.cpp`
- [X] T055 [US5] Add `--whatif` integration to `RemoveAliases()`: after profile selected, display lines that would be removed, print "What if: No changes were made" message, skip file operations in `TCDirCore/AliasManager.cpp`
- [X] T056 [US5] Add WhatIf tests to `UnitTest/AliasManagerTests.cpp`: test set-aliases --whatif produces output but no file changes, test remove-aliases --whatif produces output but no file changes

**Checkpoint**: User Story 5 complete — dry-run previews work accurately

---

## Phase 8: User Story 6 — Alias Conflict Detection (Priority: P3)

**Goal**: During setup, if chosen alias names conflict with existing PowerShell commands, the user is warned.

**Independent Test**: Choose root alias `r` (conflicts with `Invoke-History`), verify warning appears.

- [X] T057 [US6] Implement conflict scanning: given list of alias names, check `SearchPathW` for matching executables and check known PowerShell built-in alias list for matches (FR-074) in `TCDirCore/AliasManager.cpp`
- [X] T058 [US6] Integrate conflict warning into `SetAliases()` wizard: after root alias and sub-alias selection, run conflict check, display warnings with conflicting command identity, offer to override or choose different name in `TCDirCore/AliasManager.cpp`
- [X] T059 [US6] Add conflict detection tests to `UnitTest/AliasManagerTests.cpp`: test known conflict (built-in alias), test no-conflict path, test override confirmation

**Checkpoint**: User Story 6 complete — safety warnings prevent accidental breakage

---

## Phase 9: Polish & Cross-Cutting Concerns

**Purpose**: Final integration, edge case handling, documentation

- [X] T060 [P] Handle Ctrl+C / terminal close during wizard: ensure console mode and cursor visibility are restored via RAII guard in `TCDirCore/TuiWidgets.cpp`
- [X] T061 [P] Handle file permission errors gracefully (FR-073): clear error message and clean exit in `TCDirCore/ProfileFileManager.cpp`
- [X] T062 [P] Handle paths with spaces and special characters: ensure all path operations use proper quoting in generated PowerShell code in `TCDirCore/AliasBlockGenerator.cpp`
- [X] T063 Verify all existing tests still pass (regression check)
- [X] T064 Run quickstart.md validation: build from clean, execute all three commands, verify output

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: No dependencies — start immediately
- **Phase 2 (Foundational)**: Depends on Phase 1 — BLOCKS all user stories
- **Phase 3 (US1)**: Depends on Phase 2 — MVP delivery
- **Phase 4 (US2)**: Depends on Phase 2 — can run in parallel with US1
- **Phase 5 (US3)**: Depends on Phase 3 (extends SetAliases)
- **Phase 6 (US4)**: Depends on Phase 2 — can run in parallel with US1/US2
- **Phase 7 (US5)**: Depends on Phase 3 and Phase 6 (modifies both flows)
- **Phase 8 (US6)**: Depends on Phase 3 (extends SetAliases wizard)
- **Phase 9 (Polish)**: Depends on all user stories being complete

### User Story Dependencies

```
Phase 2 (Foundation) ──→ US1 (Set Aliases) ──→ US3 (Update Existing)
                     │                     └──→ US5 (WhatIf, set path)
                     │                     └──→ US6 (Conflict Detection)
                     ├──→ US2 (Get Aliases)     [independent of US1]
                     └──→ US4 (Remove Aliases) ──→ US5 (WhatIf, remove path)
```

### Parallel Opportunities

- **Phase 1**: T005–T010 all [P] — file skeletons can be created in parallel
- **Phase 2**: T016, T019, T026, T032 — test files can be created in parallel with implementation
- **US1 + US2 + US4**: Can proceed in parallel after Phase 2
- **US3 + US5 + US6**: Sequential after US1, but US5 (remove path) can overlap with US3/US6

---

## Implementation Strategy

**MVP**: Phase 1 + Phase 2 + Phase 3 (User Story 1) = working `--set-aliases` wizard

**Incremental delivery**:
1. MVP: `--set-aliases` (new profile, first-time setup)
2. Add `--get-aliases` for visibility (US2)
3. Add `--remove-aliases` for clean uninstall (US4)
4. Add update support to `--set-aliases` (US3)
5. Add `--whatif` preview to both set and remove (US5)
6. Add conflict detection for safety (US6)
