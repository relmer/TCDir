# Implementation Plan: Ellipsize Long Link Target Paths

**Branch**: `008-ellipsize-targets` | **Date**: 2026-04-19 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `specs/008-ellipsize-targets/spec.md`

## Summary

Middle-truncate long link target paths using `…` (U+2026) to prevent line wrapping in normal and tree modes. Truncation preserves first two directory components and leaf filename where possible, falling back gracefully. New `--Ellipsize` switch (default on) with `--Ellipsize-` to disable. Ellipsis rendered in `Default` color to be visually distinct.

## Technical Context

**Language/Version**: C++ (stdcpplatest, MSVC v145+)
**Primary Dependencies**: Windows SDK, STL only
**Storage**: N/A
**Testing**: Microsoft C++ Unit Test Framework (CppUnitTestFramework)
**Target Platform**: Windows 10/11, x64 and ARM64
**Project Type**: CLI tool (desktop)
**Performance Goals**: Pure string operations — no I/O, no measurable cost
**Constraints**: Truncation logic must be a pure function testable with synthetic data
**Scale/Scope**: One new pure function, switch plumbing in 4 files, display changes in 2 displayers

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Code Quality | PASS | EHM patterns where applicable, pure function for truncation logic |
| II. Testing Discipline | PASS | Pure truncation function testable with synthetic data; real WindowsApps paths as test inputs; no system state accessed |
| III. UX Consistency | PASS | New `--Ellipsize` switch follows `--Icons`/`--Tree` pattern; documented in `-?` help and `--Settings` |
| IV. Performance | PASS | String operations only, called 0-5 times per directory — negligible |
| V. Simplicity | PASS | One pure function, minimal display changes, follows existing switch infrastructure |

**Gate result: PASS** — No violations.

## Project Structure

### Documentation (this feature)

```text
specs/008-ellipsize-targets/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md        # Phase 1 output
├── quickstart.md        # Phase 1 output
└── tasks.md             # Phase 2 output (/speckit.tasks)
```

### Source Code (files affected)

```text
TCDirCore/
├── UnicodeSymbols.h             # Add Ellipsis constant
├── PathEllipsis.h               # New — EllipsizePath() pure function declaration
├── PathEllipsis.cpp             # New — EllipsizePath() implementation
├── ResultsDisplayerNormal.cpp   # Call EllipsizePath before rendering target
├── ResultsDisplayerTree.cpp     # Call EllipsizePath with tree-aware available width
├── CommandLine.h                # Add m_fEllipsize member
├── CommandLine.cpp              # Parse --Ellipsize/--Ellipsize- switch
├── Config.h                     # Add m_fEllipsize optional<bool>
├── Config.cpp                   # Add Ellipsize to s_switchMappings and s_switchMemberOrder
├── Usage.cpp                    # Document --Ellipsize in help output

UnitTest/
├── PathEllipsisTests.cpp        # New — pure function tests with real WindowsApps paths
```

**Structure Decision**: New `PathEllipsis.h/.cpp` encapsulates the truncation logic as a pure function — keeps displayers clean and makes the algorithm independently testable without mocks.

## Complexity Tracking

No constitution violations — this section is empty.
