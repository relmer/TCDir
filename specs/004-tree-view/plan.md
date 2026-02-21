# Implementation Plan: Tree View Display Mode

**Branch**: `004-tree-view` | **Date**: 2026-02-19 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/004-tree-view/spec.md`

## Summary

Add a `--Tree` display mode that renders directory contents hierarchically with Unicode box-drawing connectors, configurable depth (`--Depth=N`), configurable indent width (`--TreeIndent=N`), and configurable connector color. Add a `--Size=Auto|Bytes` switch for fixed-width abbreviated file sizes (Explorer-style 3-significant-digit format, 1024-based) that defaults to `Auto` in tree mode and `Bytes` in non-tree mode, ensuring column alignment without pre-scanning the directory tree. The implementation reuses the existing multi-threaded producer/consumer enumeration model and introduces a new `CResultsDisplayerTree` class (derived from `CResultsDisplayerNormal`) that overrides the display flow for tree-walking while reusing all inherited column rendering helpers.

## Technical Context

**Language/Version**: C++ (stdcpplatest / MSVC v145+)  
**Primary Dependencies**: Windows SDK, STL only (no third-party libraries)  
**Storage**: N/A (filesystem enumeration, no persistent storage)  
**Testing**: Microsoft C++ Unit Test Framework (CppUnitTestFramework) in `UnitTest/` project  
**Target Platform**: Windows 10/11, x64 and ARM64  
**Project Type**: Single native console application (TCDirCore static lib + TCDir exe + UnitTest)  
**Performance Goals**: Tree view must not regress performance of existing modes; measurable via `-P` flag  
**Constraints**: No external dependencies; all console output through `CConsole`; EHM patterns for HRESULT functions  
**Scale/Scope**: Targets directories with 1000+ files across 50+ subdirectories; deterministic output with same sort order
**Size Display**: Explorer-style abbreviated sizes (1024-based, 3 significant digits, B/KB/MB/GB/TB, 7-char fixed width); `--Size=Auto` default in tree mode, `--Size=Bytes` (comma-separated exact) default in non-tree mode

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Pre-Research Check

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Code Quality | PASS | Will follow EHM patterns, formatting rules, single exit points |
| II. Testing Discipline | PASS | Unit tests planned for all new public functions and code paths |
| III. UX Consistency | PASS | Uses `CConsole` for output, new switches documented in `Usage.cpp`, error messages to stderr |
| IV. Performance | PASS | Reuses MT model, no new hot-path allocations beyond tree state vector, measurable via `-P` |
| V. Simplicity | PASS | No external deps, one new displayer class + one new struct, reuses existing patterns |
| Technology Constraints | PASS | MSVC/MSBuild, Windows SDK + STL only, x64 and ARM64 |
| Development Workflow | PASS | Build via VS Code tasks, tests via CppUnitTestFramework |

### Post-Design Check

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Code Quality | PASS | New code follows EHM, formatting, smart pointer patterns |
| II. Testing Discipline | PASS | Tests cover: switch parsing, validation, connector generation, depth limiting, interleaved sort |
| III. UX Consistency | PASS | `--Tree`/`--Depth`/`--TreeIndent` follow long-switch convention; Usage.cpp updated; TCDIR env var extended |
| IV. Performance | PASS | Tree connector state is a small `vector<bool>` threaded through recursion; no per-file allocations |
| V. Simplicity | PASS | No new classes beyond tree connector state; new displayer derived from Normal reuses column helpers |
| Technology Constraints | PASS | Pure C++ with Windows SDK; no new dependencies |
| Development Workflow | PASS | Standard build/test cycle; both architectures verified |

## Project Structure

### Documentation (this feature)

```text
specs/004-tree-view/
├── plan.md              # This file
├── research.md          # Phase 0 output (completed)
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
├── contracts/           # Phase 1 output (N/A — no API contracts for CLI tool)
└── tasks.md             # Phase 2 output (/speckit.tasks command)
```

### Source Code (affected files)

```text
TCDirCore/
├── CommandLine.h          # Add m_fTree, m_cMaxDepth, m_cTreeIndent, m_eSizeFormat members
├── CommandLine.cpp        # Parse --Tree, --Depth=N, --TreeIndent=N, --Size=Auto|Bytes; validate conflicts
├── Config.h               # Add m_fTree, m_cMaxDepth, m_cTreeIndent, m_eSizeFormat optionals; EAttribute::TreeConnector
├── Config.cpp             # Parse Tree/Depth/TreeIndent/Size from TCDIR env var
├── TreeConnectorState.h   # NEW: Tree connector state tracker (vector<bool> + prefix generation)
├── ResultsDisplayerTree.h      # NEW: Tree displayer (derives from CResultsDisplayerNormal)
├── ResultsDisplayerTree.cpp    # NEW: Tree display flow + tree-prefixed file rendering
├── DirectoryLister.cpp    # Route --Tree to MT lister path; instantiate CResultsDisplayerTree
├── MultiThreadedLister.h  # Add depth parameter to PrintDirectoryTree/ProcessChildren
├── MultiThreadedLister.cpp# Thread tree state through PrintDirectoryTree recursion; depth checks
├── ResultsDisplayerNormal.h    # Make DisplayFileStreams virtual (for tree override)
├── ResultsDisplayerNormal.cpp  # Add FormatAbbreviatedSize; wire into DisplayResultsNormalFileSize for Auto mode
├── ResultsDisplayerWithHeaderAndFooter.h   # No signature changes needed
├── ResultsDisplayerWithHeaderAndFooter.cpp # No changes (tree flow is in CResultsDisplayerTree)
├── IResultsDisplayer.h    # No signature changes needed
├── FileComparator.h       # Add interleaved sort mode (no dir-first grouping)
├── FileComparator.cpp     # Conditional sort behavior for tree mode
├── Usage.cpp              # Document --Tree, --Depth, --TreeIndent, --Size in help output
├── UnicodeSymbols.h       # Add tree connector constants (├── └── │)
└── pch.h                  # No changes expected

UnitTest/
├── CommandLineTests.cpp   # Test --Tree, --Depth, --TreeIndent, --Size parsing and validation
├── ConfigTests.cpp        # Test TCDIR env var Tree/Depth/TreeIndent/Size entries
├── TreeConnectorStateTests.cpp  # Test STreeConnectorState prefix generation, Push/Pop, indent widths
├── DirectoryListerTests.cpp         # Test reparse-point guard and tree traversal
├── DirectoryListerScenarioTests.cpp # End-to-end tree output verification
├── ResultsDisplayerTests.cpp        # Existing displayer tests + FormatAbbreviatedSize unit tests
└── FileComparatorTests.cpp          # Test interleaved sort mode
```

**Structure Decision**: No new projects or directories. The feature extends the existing `TCDirCore` static library and `UnitTest` project. One new header (`TreeConnectorState.h`) encapsulates tree prefix logic. Two new files (`ResultsDisplayerTree.h`/`.cpp`) implement the tree displayer derived from `CResultsDisplayerNormal`, inheriting all column rendering helpers and overriding the display flow.

## Complexity Tracking

No constitution violations. No complexity justifications needed.
