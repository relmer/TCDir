# Implementation Plan: PowerShell Alias Configuration

**Branch**: `005-powershell-aliases` | **Date**: 2026-03-25 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/005-powershell-aliases/spec.md`

## Summary

Add three new long switches (`--set-aliases`, `--get-aliases`, `--remove-aliases`) to tcdir that manage PowerShell alias functions in the user's profile file. `--set-aliases` provides an interactive TUI wizard for configuring a root alias and sub-aliases. New source modules handle PS version detection (parent process inspection), profile path resolution (Windows shell APIs), profile file I/O (marker-delimited blocks), alias block generation, and a stepped TUI built on Console ReadInput APIs.

## Technical Context

**Language/Version**: C++ /std:c++latest (MSVC v145+)
**Primary Dependencies**: Windows SDK only (Console API, Shell API, Process API, TlHelp32)
**Storage**: User's PowerShell profile files (text files on disk)
**Testing**: Microsoft C++ Unit Test Framework (CppUnitTestFramework)
**Target Platform**: Windows 10/11, x64 and ARM64
**Project Type**: Single solution (TCDirCore static lib + TCDir exe + UnitTest)
**Performance Goals**: Interactive responsiveness (<50ms per keypress); `--get-aliases` completes in <1s
**Constraints**: No external dependencies; no PowerShell child processes for path resolution
**Scale/Scope**: ~8 profile paths to scan (4 per PS version × 2 versions, scoped to detected version at runtime = 4); TUI has 4 wizard steps

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Code Quality | ✅ PASS | New files follow EHM patterns, formatting rules, single exit point. No existing code modified except adding switch entries to CommandLine and dispatch in TCDir.cpp |
| II. Testing Discipline | ✅ PASS | All new modules (ProfilePathResolver, ProfileFileManager, AliasBlockGenerator, TUI widgets, PS version detector) will have unit tests. Integration tests for end-to-end wizard flow |
| III. UX Consistency | ✅ PASS | Uses CConsole for output. New `--` long switches follow existing pattern. `--set-aliases`/`--get-aliases`/`--remove-aliases` added to Usage.cpp help. Mutual exclusivity enforced in ValidateSwitchCombinations |
| IV. Performance | ✅ PASS | No hot-path changes. Alias operations are one-shot interactive; profile scanning is 4 file reads max. No impact on directory listing performance |
| V. Simplicity | ✅ PASS | Each new module has single responsibility. No external dependencies added. Windows APIs used directly (SHGetKnownFolderPath, CreateToolhelp32Snapshot, ReadConsoleInput) |

**Gate Result: PASS — no violations. Proceed to Phase 0.**

## Project Structure

### Documentation (this feature)

```text
specs/005-powershell-aliases/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
└── tasks.md             # Phase 2 output (/speckit.tasks command)
```

### Source Code (new files in existing project)

```text
TCDirCore/
├── AliasManager.h/.cpp          # Top-level orchestrator for --set/--get/--remove-aliases
├── ProfilePathResolver.h/.cpp   # PS version detection + profile path resolution
├── ProfileFileManager.h/.cpp    # Read/write/backup profile files, marker block parsing
├── AliasBlockGenerator.h/.cpp   # Generate PowerShell alias function code
├── TuiWidgets.h/.cpp            # Interactive TUI components (text input, radio, checkbox, confirmation, spinner)
├── CommandLine.h/.cpp           # (MODIFIED) Add new switch entries + validation
├── AnsiCodes.h                  # (MODIFIED) Add cursor hide/show, erase line, spinner color codes
├── UnicodeSymbols.h             # (MODIFIED) Add focus indicator, radio/check marks, spinner frames
├── TCDir.cpp                    # (MODIFIED) Add dispatch to AliasManager
├── Usage.cpp                    # (MODIFIED) Add --set-aliases/--get-aliases/--remove-aliases help text
└── pch.h                        # (MODIFIED) Add TlHelp32.h, shlobj.h if not present

UnitTest/
├── ProfilePathResolverTests.cpp
├── ProfileFileManagerTests.cpp
├── AliasBlockGeneratorTests.cpp
├── TuiWidgetsTests.cpp
├── AliasManagerTests.cpp
└── CommandLineTests.cpp         # (MODIFIED) Add tests for new switches
```

**Structure Decision**: New source files added to existing TCDirCore project. No new projects needed. Each module is a focused class with a clear single responsibility: path resolution, file I/O, code generation, TUI rendering, and orchestration.

## Complexity Tracking

> No constitution violations — this table is empty.

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| (none)    |            |                                     |

## Contracts

N/A — This feature has no REST/GraphQL API surface. It is a native C++ tool that reads/writes local profile files. The "contract" is the alias block format defined in FR-040 through FR-044 of the spec.

## Post-Design Constitution Re-Check

| Principle | Pre-Design | Post-Design | Delta |
|-----------|-----------|-------------|-------|
| I. Code Quality | ✅ PASS | ✅ PASS | No change. New modules follow EHM, single exit, formatting rules |
| II. Testing Discipline | ✅ PASS | ✅ PASS | No change. 5 new test files + CommandLineTests extension |
| III. UX Consistency | ✅ PASS | ✅ PASS | No change. CConsole output, `--` switches, Usage.cpp updated |
| IV. Performance | ✅ PASS | ✅ PASS | No change. No hot-path impact |
| V. Simplicity | ✅ PASS | ✅ PASS | No change. 5 focused modules, no external deps |

**Post-design gate: PASS**
