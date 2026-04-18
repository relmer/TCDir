# Implementation Plan: Symlink & Junction Target Display

**Branch**: `007-symlink-junction-targets` | **Date**: 2026-04-18 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `specs/007-symlink-junction-targets/spec.md`

## Summary

Display symlink, junction, and AppExecLink target paths after filenames in normal and tree modes using `→` (U+2192). Target paths are read from NTFS reparse data via `FSCTL_GET_REPARSE_POINT`. Arrow uses `Information` color; target uses directory color (for dir targets) or extension-based color (for file targets). No new switches or config keys.

## Technical Context

**Language/Version**: C++ (stdcpplatest, MSVC v145+)
**Primary Dependencies**: Windows SDK, STL only
**Storage**: N/A
**Testing**: Microsoft C++ Unit Test Framework (CppUnitTestFramework)
**Target Platform**: Windows 10/11, x64 and ARM64
**Project Type**: CLI tool (desktop)
**Performance Goals**: Zero overhead in directories with no reparse points; microseconds per reparse point when present
**Constraints**: Stack-allocated 16KB buffer per reparse read; no heap allocation in hot path
**Scale/Scope**: Typical directory has 0–5 reparse points; no batch processing concern

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Code Quality | PASS | EHM patterns for HRESULT functions, smart pointers (AutoHandle for CreateFileW), formatting rules followed |
| II. Testing Discipline | PASS | Pure-function buffer parsers tested with synthetic byte arrays (no mocking needed); display tested via CCapturingConsole; no real filesystem access in any unit test |
| III. UX Consistency | PASS | Uses existing `Information` attribute for arrow, existing extension/dir colors for target; no new switches; `-?` help not affected (no switch to document) |
| IV. Performance | PASS | Zero cost when no reparse points; one CreateFile + DeviceIoControl per link (stack buffer, no heap); measurable with `-P` |
| V. Simplicity | PASS | One new field on FileInfo, one new resolution function, display changes in two displayers; no new abstractions beyond what's needed |

**Gate result: PASS** — No violations. Proceed to Phase 0.

## Project Structure

### Documentation (this feature)

```text
specs/007-symlink-junction-targets/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
└── tasks.md             # Phase 2 output (/speckit.tasks)
```

### Source Code (files affected)

```text
TCDirCore/
├── DirectoryInfo.h          # Add m_strReparseTarget to FileInfo
├── ReparsePointResolver.h   # New — ResolveReparseTarget() + REPARSE_DATA_BUFFER definition
├── ReparsePointResolver.cpp # New — reparse buffer reading and parsing
├── DirectoryLister.cpp      # Call resolver in AddMatchToList()
├── UnicodeSymbols.h         # Add RightArrow constant
├── ResultsDisplayerNormal.cpp  # Display → target after filename
├── ResultsDisplayerTree.cpp    # Display → target after filename (tree mode)
├── Config.h                 # Expose extension color lookup for target paths
├── Config.cpp               # Add GetTextAttrForExtension() helper

UnitTest/
├── ReparsePointResolverTests.cpp  # New — buffer parsing, prefix stripping, error handling
├── ResultsDisplayerTests.cpp      # New — verify → target rendering with mock console
```

**Structure Decision**: No new projects. New `ReparsePointResolver.h/.cpp` encapsulates all reparse I/O and buffer parsing — keeps DirectoryLister clean. Buffer parsers are pure functions testable with synthetic byte arrays; Win32 I/O (`ResolveReparseTarget`) is tested via integration only.

## Complexity Tracking

No constitution violations — this section is empty.
