# Tasks: Config File Support

**Input**: Design documents from `/specs/006-config-file-support/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/

## Phase 1: Setup

**Purpose**: Add `<fstream>` to pch.h, create new source files and add them to the build

- [ ] T001 Add `<fstream>` to TCDirCore/pch.h
- [ ] T002 [P] Create IConfigFileReader interface and CConfigFileReader class in TCDirCore/ConfigFileReader.h
- [ ] T003 [P] Create CConfigFileReader implementation stub in TCDirCore/ConfigFileReader.cpp
- [ ] T004 Add ConfigFileReader.h and ConfigFileReader.cpp to TCDirCore/TCDirCore.vcxproj and .filters
- [ ] T005 [P] Create UnitTest/ConfigFileReaderTests.cpp with test class stub and add to UnitTest project
- [ ] T006 [P] Create UnitTest/ConfigFileTests.cpp with test class stub and add to UnitTest project
- [ ] T007 Build and verify no compilation errors

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure changes that all user stories depend on

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [ ] T008 Extend CConfig::EAttributeSource enum with ConfigFile value in TCDirCore/Config.h
- [ ] T009 Extend CConfig::ErrorInfo struct with sourceFilePath (wstring) and lineNumber (size_t) fields in TCDirCore/Config.h
- [ ] T010 Add source parameter to ProcessColorOverrideEntry and thread through all downstream methods that write to source maps in TCDirCore/Config.cpp
- [ ] T011 Update ApplyUserColorOverrides to pass EAttributeSource::Environment as source parameter in TCDirCore/Config.cpp
- [ ] T012 Add config file members to CConfig: m_strConfigFilePath, m_fConfigFileLoaded, m_configFileParseResult, m_pConfigFileReader in TCDirCore/Config.h
- [ ] T013 Add public methods to CConfig: LoadConfigFile, ValidateConfigFile, GetConfigFilePath, IsConfigFileLoaded, SetConfigFileReader in TCDirCore/Config.h
- [ ] T014 Update existing ConfigTests.cpp to verify source parameter threading does not break current env var override tests in UnitTest/ConfigTests.cpp
- [ ] T015 Build and run all existing tests to verify no regressions

**Checkpoint**: Foundation ready — source tracking supports 3 values, ErrorInfo has line numbers, ProcessColorOverrideEntry accepts source parameter

---

## Phase 3: User Story 1 — Basic Config File Loading (Priority: P1) 🎯 MVP

**Goal**: Users can place settings in `%USERPROFILE%\.tcdirconfig` and have them applied on every run

**Independent Test**: Create a config file with switches and color overrides, run tcdir, verify settings are applied

### Implementation

- [ ] T016 Implement CConfigFileReader::ReadLines in TCDirCore/ConfigFileReader.cpp — ifstream binary read, BOM detection, MultiByteToWideChar, line splitting
- [ ] T017 Implement CConfig::LoadConfigFile in TCDirCore/Config.cpp — resolve path via USERPROFILE env var, call reader, strip comments, trim whitespace, pass entries to ProcessColorOverrideEntry with ConfigFile source, populate ErrorInfo.lineNumber and ErrorInfo.sourceFilePath for each error
- [ ] T018 Insert LoadConfigFile call into CConfig::Initialize between default initialization and ApplyUserColorOverrides in TCDirCore/Config.cpp
- [ ] T019 Write CTestConfigFileReader mock in UnitTest/ConfigFileReaderTests.cpp — in-memory lines, simulate not-found and I/O errors
- [ ] T020 Write ConfigFileReader unit tests in UnitTest/ConfigFileReaderTests.cpp — UTF-8 read, BOM skip, UTF-16 BOM rejection, empty file, line splitting (CRLF, LF, CR)
- [ ] T021 Write config file loading unit tests in UnitTest/ConfigFileTests.cpp — switches applied, color overrides applied, icon overrides applied, parameterized values applied
- [ ] T022 Write comment and blank line unit tests in UnitTest/ConfigFileTests.cpp — comment lines skipped, inline comments stripped, blank lines skipped, whitespace-only lines skipped
- [ ] T023 Build and run all tests

**Checkpoint**: Config file loads and applies settings. No env var interaction tested yet.

---

## Phase 4: User Story 2 — Environment Variable Overrides Config File (Priority: P1)

**Goal**: TCDIR env var settings take precedence over config file for conflicting keys; non-conflicting settings merge

**Independent Test**: Set `.cpp=LightGreen` in config file and `.cpp=Yellow` in env var, verify Yellow wins

### Implementation

- [ ] T024 [US2] Write precedence unit tests in UnitTest/ConfigFileTests.cpp — env var overrides config file color, env var overrides config file switch, non-conflicting settings merge from both sources
- [ ] T025 [US2] Write source tracking unit tests in UnitTest/ConfigFileTests.cpp — verify EAttributeSource::ConfigFile for config-only settings, EAttributeSource::Environment for env-var-overridden settings
- [ ] T026 [US2] Build and run all tests

**Checkpoint**: Precedence model verified. Config file + env var merge correctly.

---

## Phase 5: User Story 3 — Readable Multi-Line Format (Priority: P1)

**Goal**: Config file supports comments, blank lines, and per-line settings for readable organization

**Independent Test**: Create a config file with comment headers, grouped settings, inline comments — verify all parsed correctly

### Implementation

- [ ] T027 [US3] Write inline comment edge case tests in UnitTest/ConfigFileTests.cpp — setting with inline comment, setting with multiple # characters, comment-only lines with leading whitespace
- [ ] T028 [US3] Write whitespace handling tests in UnitTest/ConfigFileTests.cpp — leading/trailing whitespace trimmed, whitespace around = in key=value, tabs as whitespace
- [ ] T029 [US3] Write duplicate setting tests in UnitTest/ConfigFileTests.cpp — last occurrence wins within config file
- [ ] T030 [US3] Build and run all tests

**Checkpoint**: All format rules validated. Stories 1-3 form a complete, testable MVP.

---

## Phase 6: User Story 4 — Config File Error Reporting (Priority: P2)

**Goal**: Parse errors include file path and line number; errors shown on every run; config file and env var errors grouped separately

**Independent Test**: Place an invalid color name in config file, verify error message shows file path and line number

### Implementation

- [ ] T031 [US4] Implement DisplayConfigFileIssues in TCDirCore/Usage.cpp — render config file errors with line numbers, using existing underline pattern
- [ ] T032 [US4] Update error display at end of normal runs in TCDirCore/TCDir.cpp — call DisplayConfigFileIssues before DisplayEnvVarIssues, skip group if no errors
- [ ] T033 [US4] Implement file-level I/O error reporting in CConfig::LoadConfigFile — single ErrorInfo for open/read/encoding failures in TCDirCore/Config.cpp
- [ ] T034 [US4] Write error reporting unit tests in UnitTest/ConfigFileTests.cpp — invalid color name shows line number, malformed entry shows line number, valid lines still apply alongside errors
- [ ] T035 [US4] Write error grouping tests in UnitTest/ConfigFileTests.cpp — config file errors separate from env var errors, config file errors listed first
- [ ] T036 [US4] Write I/O error tests in UnitTest/ConfigFileReaderTests.cpp — permission denied produces single error, file not found produces silent skip
- [ ] T037 [US4] Build and run all tests

**Checkpoint**: Error reporting complete with line numbers and grouped display.

---

## Phase 7: User Story 5 — Diagnostic Command Restructuring (Priority: P2)

**Goal**: `/config` shows config file diagnostics; `/settings` shows merged configuration tables with 3-source column; `/env` unchanged

**Independent Test**: Set up config file + env var, run each of `/config`, `/settings`, `/env` and verify expected output scope

### Implementation

- [ ] T038 [US5] Add m_fSettings bool to CCommandLine in TCDirCore/CommandLine.h
- [ ] T039 [US5] Add "settings" to long switch table in CCommandLine::HandleLongSwitch in TCDirCore/CommandLine.cpp
- [ ] T040 [US5] Add "settings" to informational switch list for mutual exclusion validation in TCDirCore/CommandLine.cpp
- [ ] T041 [US5] Implement DisplayConfigFileHelp in TCDirCore/Usage.cpp — config file syntax reference, file path, load status, decoded settings grouped by type, config file parse errors
- [ ] T042 [US5] Repurpose /config handler in TCDirCore/TCDir.cpp — call DisplayConfigFileHelp instead of DisplayCurrentConfiguration
- [ ] T043 [US5] Rename DisplayCurrentConfiguration to DisplaySettings in TCDirCore/Usage.h and TCDirCore/Usage.cpp
- [ ] T044 [US5] Update DisplaySettings source column to render three values: Default, Config file, Environment in TCDirCore/Usage.cpp
- [ ] T045 [US5] Add /settings handler in TCDirCore/TCDir.cpp — call DisplaySettings when m_fSettings is set
- [ ] T046 [US5] Update DisplaySettings to show both config file and env var errors at the bottom in TCDirCore/Usage.cpp
- [ ] T047 [US5] Update /help text in DisplayUsage to reflect new /config description and new /settings command in TCDirCore/Usage.cpp
- [ ] T048 [US5] Write /settings switch parsing test in UnitTest/CommandLineTests.cpp
- [ ] T049 [US5] Build and run all tests

**Checkpoint**: All three diagnostic commands work correctly.

---

## Phase 8: Polish & Cross-Cutting Concerns

**Purpose**: Final validation and edge case hardening

- [ ] T050 [P] Write edge case tests in UnitTest/ConfigFileTests.cpp — empty file, file with only comments, file with only blank lines, very long lines
- [ ] T051 [P] Write BOM edge case tests in UnitTest/ConfigFileReaderTests.cpp — UTF-16 LE BOM rejected with clear error, UTF-16 BE BOM rejected with clear error
- [ ] T052 [P] Write BOM end-to-end test in UnitTest/ConfigFileTests.cpp — UTF-8 BOM file with settings parses correctly through LoadConfigFile
- [ ] T053 Verify config file does not exist scenario — no error, no warning, defaults used. Already tested but validate end-to-end.
- [ ] T054 Run quickstart.md validation — create config file, verify all commands work as documented
- [ ] T055 Full build (Debug + Release, x64 + ARM64) and run all tests on all configurations

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — can start immediately
- **Foundational (Phase 2)**: Depends on Phase 1 — BLOCKS all user stories
- **US1 (Phase 3)**: Depends on Phase 2 — the MVP
- **US2 (Phase 4)**: Depends on Phase 3 (needs loading to work to test precedence)
- **US3 (Phase 5)**: Depends on Phase 3 (needs loading to work to test format rules)
- **US4 (Phase 6)**: Depends on Phase 3 (needs loading to work to test error reporting)
- **US5 (Phase 7)**: Depends on Phase 6 (needs error model complete for display)
- **Polish (Phase 8)**: Depends on all prior phases

### Within Each Phase

- Tasks marked [P] can run in parallel
- Tests before or alongside implementation (write tests that exercise the new code)
- Build verification at end of each phase

### Parallel Opportunities

After Phase 2 (Foundational) completes:
- US2 (Phase 4) and US3 (Phase 5) can run in parallel after US1 (Phase 3) completes
- US4 (Phase 6) can run in parallel with US2/US3 after US1 completes

---

## Implementation Strategy

### MVP First (User Stories 1-3)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL — blocks all stories)
3. Complete Phase 3: User Story 1 — Basic Loading
4. Complete Phase 4: User Story 2 — Precedence
5. Complete Phase 5: User Story 3 — Format Rules
6. **STOP and VALIDATE**: Test end-to-end with a real `.tcdirconfig` file
7. Continue to Phase 6-8 for error reporting and diagnostics

### Incremental Delivery

- After Phase 3: Config file works, no error diagnostics yet
- After Phase 5: Full MVP — loading, precedence, format all validated
- After Phase 7: Complete feature — all diagnostics and commands
- After Phase 8: Production-ready — all edge cases and platforms validated
