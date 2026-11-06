# Tasks: PowerShell Argument Completion

**Input**: Design documents from `specs/014-pwsh-arg-completion/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/cli.md, quickstart.md

**Tests**: Required by the constitution (Principle II). The completer takes requests as data, so no test starts PowerShell or reads the profile.

**Format**: `[ID] [P?] [Story] Description`. Every new `.cpp`/`.h` is added to `TCDirCore/TCDirCore.vcxproj` and `.filters` (or `UnitTest/UnitTest.vcxproj` and `.filters`) in the task that creates it.

## Phase 1: Setup

- [X] T001 Capture `tcdir -? --Icons-` and `tcdir /? /Icons-` from the current Debug build into the scratch folder, without the version header lines, for the before/after comparison in T006

## Phase 2: Foundational (blocks all stories)

- [X] T002 Add `ESwitchForm`, `ESwitchSection`, `SSwitchValue` and `SSwitchDefinition` per data-model.md, and `CSwitchCatalog` with a `constexpr` table of every switch in the `-?` help (descriptions and continuations with `{0}` short and `{1}` long prefix placeholders, value lists for `-A`, `-O`, `-T`, `--Size`, `--TreeIndent`), plus `Find (form, name)`, `PlainText (markup)` and `FormatHelpText (definition, shortPrefix, longPrefix)`, in TCDirCore/SwitchCatalog.h and TCDirCore/SwitchCatalog.cpp
- [X] T003 [P] Write tests in UnitTest/SwitchCatalogTests.cpp: every catalog switch parses with `CCommandLine::Parse` (with its first value when it takes one), every name `CCommandLine::IsRecognizedLongSwitch` accepts is in the catalog, every value of `-A`, `-O` and `-T` parses, `PlainText` removes color markers, `Find` is case insensitive
- [X] T004 Build the option rows in `BuildPrimaryOptionRows` and `BuildAliasRows`, and the attribute, sort and time grids in `CUsage::DisplayUsage`, from `CSwitchCatalog`, in TCDirCore/Usage.cpp
- [X] T005 Make `CCommandLine::IsRecognizedLongSwitch` accessible to tests (public static) in TCDirCore/CommandLine.h
- [X] T006 Compare `-?` and `/?` output with T001; the output is identical. Run all tests and commit the phase

**Checkpoint**: The help comes from the catalog and prints the same text.

## Phase 3: User Stories 1 and 2 - Complete switches, keep path completion (P1) 🎯 MVP

**Goal**: `tcdir --Complete w:<word>` prints switch candidates, and nothing for other words.

**Independent Test**: Quickstart scenarios 1 to 4 with the script from contracts/cli.md pasted into a session.

- [ ] T007 [P] [US1] Write tests in UnitTest/CompleterTests.cpp: `--Tr` → `--Tree` only; `--` → every long switch in catalog casing; `-` → short switches then long switches with `--`; `/Str` → `/Streams`; matching ignores case; tooltips are plain text from the help; `--Debug` only in Debug builds; the output line format is three tab-separated fields
- [ ] T008 [P] [US2] Write tests in UnitTest/CompleterTests.cpp: an empty word, `C:\Pro`, `.\-notes` and `foo` → no candidates
- [ ] T009 [US1] Implement `SCompletionRequest`, `SCompletionCandidate` and `CCompleter::Complete (request, candidates)` and `CCompleter::FormatLine` in TCDirCore/Completer.h and TCDirCore/Completer.cpp
- [ ] T010 [US1] Add `CCompleter::Run (argc, argv)` (parses `--Complete w:<word> <words>`, writes the lines to stdout) and call it from `wmain` in TCDirCore/TCDir.cpp before `ApplyConfigDefaults` when `argv[1]` is `--Complete`
- [ ] T011 [US1] Run quickstart scenarios 1 to 4 and 12, build, run all tests, commit the phase

**Checkpoint**: Switch completion works once the script is registered manually.

## Phase 4: User Story 3 - Setup through the alias installer (P1)

**Goal**: `--set-aliases` registers completion for `tcdir` and every alias; `--get-aliases` reports it.

**Independent Test**: Quickstart scenarios 8 to 10.

- [ ] T012 [P] [US3] Write tests in UnitTest/CompletionScriptTests.cpp for `CCompletionScript::Generate (invocation, commands, lines)`: one registration per command, alias flags quoted before `@others`, `w:` word prefix, `2>$null`, the invocation quoted with `&`
- [ ] T013 [US3] Implement `CCompletionScript` with `SCompletionCommand { name, flags }` in TCDirCore/CompletionScript.h and TCDirCore/CompletionScript.cpp
- [ ] T014 [P] [US3] Write tests in UnitTest/AliasBlockGeneratorTests.cpp: the block contains registrations for `tcdir`, `tcdir.exe`, the root alias and each enabled sub-alias with its flags, inside the block markers
- [ ] T015 [US3] Append the registrations in `CAliasBlockGenerator::Generate` in TCDirCore/AliasBlockGenerator.cpp
- [ ] T016 [US3] Report `Tab completion: installed` or `not installed` per block in `CAliasManager::DisplayProfileAliases` in TCDirCore/AliasManager.cpp, with a test in UnitTest/AliasManagerTests.cpp if the existing seams allow it
- [ ] T017 [US3] Run quickstart scenarios 8 to 10 in PowerShell 7 and 5.1, build, run all tests, commit the phase

## Phase 5: User Story 4 - Setup without aliases (P2)

**Goal**: `tcdir --Completion pwsh` prints the registration script.

**Independent Test**: Quickstart scenarios 1 and 11.

- [ ] T018 [P] [US4] Write tests in UnitTest/CommandLineTests.cpp: `--Completion pwsh`, `--Completion=pwsh`, `--Completion powershell`, `--Completion bash` (error listing `pwsh`), `--Completion` without a value
- [ ] T019 [US4] Parse `--Completion <shell>` into `m_strCompletionShell` with validation in TCDirCore/CommandLine.h and TCDirCore/CommandLine.cpp
- [ ] T020 [US4] Print `CCompletionScript::Generate` output for `tcdir` and `tcdir.exe` with the running exe's full path from `ProcessCommandLine` in TCDirCore/TCDir.cpp, and add the `--Completion` row (with `--Complete` documented in its continuation) to the catalog
- [ ] T021 [US4] Run quickstart scenarios 1 and 11, build, run all tests, commit the phase

## Phase 6: User Story 5 - Complete switch values (P2)

**Goal**: Values complete after `-O:`, `-A:`, `-T:`, `--Size=` and `--TreeIndent=`.

**Independent Test**: Quickstart scenarios 5 and 6.

- [ ] T022 [P] [US5] Write tests in UnitTest/CompleterTests.cpp: `-O:` → each key with and without `-`; `-A:H` extends `H` with each other letter and `-`; `-T:` → `C`, `A`, `W`; `--Size=` → `Auto` and `Bytes`; `--TreeIndent=` → `1` to `8`; `-W:` → nothing; value tooltips are the help descriptions
- [ ] T023 [US5] Complete values from the catalog in `CCompleter::Complete` in TCDirCore/Completer.cpp
- [ ] T024 [US5] Run quickstart scenarios 5 and 6, build, run all tests, commit the phase

## Phase 7: User Story 6 - Context-aware candidates (P3)

**Goal**: Present and conflicting switches aren't offered.

**Independent Test**: Quickstart scenario 7.

- [ ] T025 [P] [US6] Write tests in UnitTest/CompleterTests.cpp: after `-W`, `-W` isn't offered; after `--Tree`, `-W`, `-B`, `-S` and `--Owner` aren't offered; `-A:` stays after `-A:H`; neither `--Icons` nor `--Icons-` after `--Icons`; alias flags in the context count as present; every candidate offered with `--Tree` parses with it (SC-005)
- [ ] T026 [US6] Filter candidates through the catalog (present, repeatable, negatable) and `CCommandLine::Parse` of the context plus the candidate in TCDirCore/Completer.cpp
- [ ] T027 [US6] Run quickstart scenario 7, build, run all tests, commit the phase

## Phase 8: Polish

- [ ] T028 [P] Document completion setup in README.md and CHANGELOG.md
- [ ] T029 Build Release x64 and ARM64, run Code Analysis with zero warnings, run all tests
- [ ] T030 Run the prose check on new comments and docs, then commit the phase

## Dependencies

- Phase 1 → Phase 2 → US1/US2 (Phase 3).
- US3 and US4 depend on Phase 3 and share `CCompletionScript` (US3 creates it).
- US5 and US6 depend on Phase 3 only.

## Implementation Strategy

1. MVP: Phases 1 to 3, switch completion with a pasted script.
2. US3 and US4 make setup a single step.
3. US5 and US6 refine the candidates.
