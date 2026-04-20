# Implementation Plan: Variable-Width Columns in Wide Mode

**Branch**: `009-variable-width-columns` | **Date**: 2026-04-19 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/009-variable-width-columns/spec.md`

## Summary

Replace the uniform-width column layout in wide mode (`/W`) with a variable-width algorithm that computes per-column widths based on actual entry display widths. Outlier filenames exceeding `max(2× median display width, 20)` are truncated with ellipsis (unless `--Ellipsize-` is specified). Leftover horizontal space is distributed evenly across inter-column gaps.

## Technical Context

**Language/Version**: C++ (stdcpplatest, MSVC v145+)  
**Primary Dependencies**: Windows SDK, STL only  
**Storage**: N/A  
**Testing**: Microsoft C++ Unit Test Framework (CppUnitTestFramework)  
**Target Platform**: Windows 10/11, x64 and ARM64  
**Project Type**: CLI tool (console application)  
**Performance Goals**: Column layout calculation must be imperceptible; no regression on `/W` display performance  
**Constraints**: No third-party libraries; display-only change scoped to `ResultsDisplayerWide`  
**Scale/Scope**: Single file change (`ResultsDisplayerWide.cpp/.h`) plus new unit tests

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Code Quality | PASS | EHM patterns preserved in `DisplayFileResults`; formatting rules apply to all edits |
| II. Testing Discipline | PASS | New unit tests required for column layout logic; test isolation via synthetic data (no real filesystem) |
| III. User Experience Consistency | PASS | `/W` output improves density; backward compat preserved when all names equal width (FR-007); ellipsis opt-out via `--Ellipsize-` |
| IV. Performance Requirements | PASS | Column-fitting loop runs once per directory (negligible cost); no hot-path allocations added |
| V. Simplicity & Maintainability | PASS | Change scoped to one displayer class; column layout extracted to pure testable function; functions kept under 50 lines |

## Project Structure

### Documentation (this feature)

```text
specs/009-variable-width-columns/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
└── tasks.md             # Phase 2 output (/speckit.tasks command)
```

### Source Code (affected files)

```text
TCDirCore/
├── ResultsDisplayerWide.h      # Modified: new method signatures for variable-width layout
└── ResultsDisplayerWide.cpp    # Modified: column layout algorithm + outlier truncation

UnitTest/
└── ResultsDisplayerWideTests.cpp  # New: unit tests for column layout logic
```

**Note**: `PathEllipsis.h/cpp` is not modified — its path-oriented middle-truncation is not suitable for filename right-truncation in wide mode (see research.md §3).

**Structure Decision**: No new projects or directories needed. The change is contained within the existing `CResultsDisplayerWide` class in `TCDirCore`, plus a new test file in `UnitTest`.

## Complexity Tracking

No constitution violations. Table not applicable.
