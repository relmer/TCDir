# Implementation Plan: Config File Support

**Branch**: `006-config-file-support` | **Date**: 2026-04-04 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/006-config-file-support/spec.md`

## Summary

Add config file support to tcdir so users can place settings in `%USERPROFILE%\.tcdirconfig` instead of cramming everything into the `TCDIR` environment variable. The config file uses the same entry syntax as the env var (one entry per line, `#` comments). Precedence: built-in defaults < config file < TCDIR env var < CLI flags. Error reporting includes file path + line numbers. The `/config` command is repurposed for config file diagnostics, and a new `/settings` command replaces the existing merged configuration view.

## Technical Context

**Language/Version**: C++ (stdcpplatest, MSVC v145+)  
**Primary Dependencies**: Windows SDK, STL only — no third-party libraries  
**Storage**: Single flat file (`%USERPROFILE%\.tcdirconfig`), UTF-8 with optional BOM  
**Testing**: Microsoft C++ Unit Test Framework (CppUnitTestFramework)  
**Target Platform**: Windows 10/11, x64 and ARM64  
**Project Type**: CLI tool (native Windows console application)  
**Performance Goals**: Config file parsing adds no perceptible startup delay (50 settings < 1ms)  
**Constraints**: Zero external dependencies; file I/O via STL `<fstream>`  
**Scale/Scope**: Config files expected 20-150 lines max

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Code Quality | PASS | EHM patterns for HRESULT-returning functions; single exit point; formatting rules |
| II. Testing Discipline | PASS | All new parsing and file I/O code will have unit tests in `UnitTest/ConfigFileTests.cpp`; mock via `IEnvironmentProvider` + new `IConfigFileReader` interface |
| III. User Experience Consistency | PASS | Config file syntax matches env var; `/config` parallels `/env`; errors follow existing underline pattern with added line numbers |
| IV. Performance Requirements | PASS | Single file read at startup; no measurable impact on directory listing perf |
| V. Simplicity & Maintainability | PASS | No external dependencies; reuses existing `ProcessColorOverrideEntry` for parsing; new code is a thin file-reading layer + line splitting |

No violations. Gate passes.

## Project Structure

### Documentation (this feature)

```text
specs/006-config-file-support/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
└── tasks.md             # Phase 2 output (created by /speckit.tasks)
```

### Source Code (repository root)

```text
TCDirCore/
├── Config.h                 # Extended: config file loading methods, source tracking enum
├── Config.cpp               # Extended: LoadConfigFile, line-oriented parsing, source tracking
├── ConfigFileReader.h        # NEW: IConfigFileReader interface + CConfigFileReader implementation
├── ConfigFileReader.cpp      # NEW: File I/O — read lines, BOM handling, error wrapping
├── CommandLine.h            # Extended: m_fSettings switch
├── CommandLine.cpp          # Extended: /settings handling, switch table update
├── Usage.h                  # Extended: DisplayConfigFileHelp, DisplaySettings declarations
├── Usage.cpp                # Extended: /config repurposed, /settings new, error grouping
├── TCDir.cpp                # Extended: config file loaded before env var in Initialize

UnitTest/
├── ConfigFileReaderTests.cpp # NEW: file reading, BOM handling, I/O error tests
├── ConfigFileTests.cpp       # NEW: line parsing, comments, blanks, precedence, errors
├── ConfigTests.cpp           # Extended: source tracking in existing override tests
├── CommandLineTests.cpp      # Extended: /settings switch
```

**Structure Decision**: No new projects. All changes are within the existing `TCDirCore` static library and `UnitTest` test project. The only new files are `ConfigFileReader.h/.cpp` (file I/O abstraction) and the corresponding test files.

## Complexity Tracking

No constitution violations. No complexity justifications needed.
