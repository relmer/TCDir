# Implementation Plan: PowerShell Argument Completion

**Branch**: `014-pwsh-arg-completion` | **Date**: 2026-11-06 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/014-pwsh-arg-completion/spec.md`

## Summary

A switch catalog becomes the source of the `-?` help rows and of the
completion candidates. `tcdir --Complete w:<word> <other words>` prints
candidates for a PowerShell native completer, filtered through
`CCommandLine::Parse` so that conflicting switches aren't offered. The alias
profile block registers the completer for `tcdir` and every alias, and
`tcdir --Completion pwsh` prints the registration for users without aliases.

## Technical Context

**Language/Version**: C++ (stdcpplatest, MSVC v145+); PowerShell for the registration script

**Primary Dependencies**: Windows SDK, STL

**Storage**: The managed block in the PowerShell profile (existing alias installer)

**Testing**: Microsoft C++ Unit Test Framework; no test starts PowerShell

**Target Platform**: Windows 10/11, x64 and ARM64; PowerShell 7 and Windows PowerShell 5.1

**Project Type**: CLI

**Performance Goals**: Candidates within 100 ms of Tab (SC-001)

**Constraints**: `--Complete` doesn't read files or apply `TCDIR` defaults, and prints only candidates. `-?` output stays identical

**Scale/Scope**: About 30 switches and 30 values

## Constitution Check

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Code Quality | Pass | EHM, alignment and spacing rules |
| II. Testing Discipline | Pass | Catalog, completer, help output and alias block are tested in memory. The completer takes the request as data |
| III. UX Consistency | Pass | `--Completion` is in `-?`. `--Complete` is documented under it. Help output is compared before and after the refactor |
| IV. Performance | Pass | One short process per Tab press. Listings are unaffected |
| V. Simplicity | Pass | One catalog for help and completion. Conflict rules come from the parser, not a second copy |

## Project Structure

### Documentation (this feature)

```text
specs/014-pwsh-arg-completion/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   └── cli.md
└── tasks.md
```

### Source Code

```text
TCDirCore/
├── SwitchCatalog.h/.cpp       # New: switch definitions and value lists
├── Completer.h/.cpp           # New: candidates for a completion request
├── CompletionScript.h/.cpp    # New: PowerShell registration lines
├── Usage.cpp                  # Help rows and value grids from the catalog; --Completion row
├── CommandLine.h/.cpp         # --Completion=<shell>
├── TCDir.cpp                  # --Complete before parsing; --Completion output
├── AliasBlockGenerator.cpp    # Registration lines in the profile block
└── AliasManager.cpp           # --get-aliases completion status

UnitTest/
├── SwitchCatalogTests.cpp     # New: catalog vs parser
├── CompleterTests.cpp         # New
├── CompletionScriptTests.cpp  # New
├── SwitchCatalogTests.cpp     # Catalog rows match the help labels
├── AliasBlockGeneratorTests.cpp
└── CommandLineTests.cpp
```

**Structure Decision**: Each new class has one job and lives in `TCDirCore`
with a matching test file, as for the other features.

## Design Notes

- **Help comparison first**: The `-?` and `/?` output is captured from the
  build before the catalog refactor, with icons off, and compared after it
  character for character.
- **`--Complete` dispatch**: `wmain` checks `argv[1]` before `ApplyConfigDefaults`
  and `Parse`, and calls `CCompleter::Run`, which writes to stdout with
  `WriteConsoleW` when attached to a console and `WriteFile` otherwise
  (the script reads a pipe).
- **Prefix**: The candidate prefix follows the word: `--`, `-` or `/`. An
  empty word doesn't get candidates, so PowerShell completes paths.
- **Short vs long under `-`**: `-` lists short switches, then long ones with
  `--`. `/` lists both with `/`.
- **Value words**: `-O:x`, `-A:xy`, `-T:x`, `--Size=x` and `--TreeIndent=x`
  are recognized from the catalog's separator and value list.
- **Alias flags**: Each alias registration passes its flags before the typed
  words, so `dw --Tree` (an alias with `-W`) doesn't offer switches that
  conflict with `-W`.

## Complexity Tracking

No constitution violations.
