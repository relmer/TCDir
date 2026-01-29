# Implementation Plan: CMD Dir Compatibility & Cloud File Visualization

**Branch**: `001-dir-compat-cloud` | **Date**: 2026-01-24 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/001-dir-compat-cloud/spec.md`

## Summary

Add CMD `dir` compatibility features and cloud file visualization to TCDir. This includes:
- New `/A:` attribute filters for cloud status (O, F, U), content indexing (X), and ReFS attributes (I, B)
- Cloud status visualization column with ☁/✓/● symbols between file size and filename
- Time field selection (`/T:C|A|W`) for creation, access, or write time
- File ownership display (`--owner`) and alternate data streams (`--streams`)

**Technical Approach**: Extend existing patterns in `CommandLine.cpp` (attribute arrays, switch handlers), `FileComparator.cpp` (time field sorting), and `ResultsDisplayer*` classes (cloud status column). Use `CDriveInfo::GetFileSystemName()` for NTFS/ReFS validation.

## Technical Context

**Language/Version**: stdcpplatest (MSVC v145+)  
**Primary Dependencies**: Windows SDK, STL only  
**Storage**: N/A (filesystem enumeration only)  
**Testing**: Microsoft C++ Unit Test Framework (CppUnitTestFramework)  
**Target Platform**: Windows 10/11, x64 and ARM64  
**Project Type**: Single project (native Windows console application)  
**Performance Goals**: No regression in directory enumeration speed; `--owner` and `--streams` are opt-in due to per-file API calls  
**Constraints**: Cloud file APIs require Windows 10 v1709+; ADS requires NTFS; ReFS attributes require ReFS volume  
**Scale/Scope**: Extends existing ~20 source files; adds ~500-800 LOC across CommandLine, ResultsDisplayer, and test files

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| **I. Code Quality** | ✅ PASS | Will use EHM patterns, preserve formatting, follow existing switch/attribute patterns |
| **II. Testing Discipline** | ✅ PASS | SC-011 requires unit tests for all new switches and attributes |
| **III. UX Consistency** | ✅ PASS | New switches follow existing `-X`/`/X` patterns; documented in Usage.cpp; colors via CConsole |
| **IV. Performance** | ✅ PASS | Slow features (`--owner`, `--streams`) are opt-in; core enumeration unchanged |
| **V. Simplicity** | ✅ PASS | Extends existing classes; no new dependencies; factored helper functions |

## Project Structure

### Documentation (this feature)

```text
specs/001-dir-compat-cloud/
├── plan.md              # This file
├── research.md          # Phase 0: API research for cloud attributes, ADS, ownership
├── data-model.md        # Phase 1: New enums, attribute mappings
├── quickstart.md        # Phase 1: Developer guide for the changes
├── contracts/           # Phase 1: N/A (no external APIs)
└── tasks.md             # Phase 2: Implementation tasks (/speckit.tasks)
```

### Source Code (repository root)

```text
TCDirCore/
├── CommandLine.cpp      # Extend attribute arrays, add /T: handler, add --owner/--streams
├── CommandLine.h        # Add ETimeField enum, m_timeField, m_fShowOwner, m_fShowStreams
├── Config.cpp           # Add cloud status colors to attribute map
├── Config.h             # Add EAttribute entries for cloud status colors
├── FileComparator.cpp   # Respect m_timeField for date sorting
├── ResultsDisplayerNormal.cpp    # Add cloud status column, time field display
├── ResultsDisplayerWide.cpp      # Add cloud status indicator (if applicable)
├── ResultsDisplayerBare.cpp      # No changes (bare mode suppresses cloud status)
├── Usage.cpp            # Document all new switches and attributes
├── DriveInfo.h          # Expose IsNTFS(), IsReFS() helper methods

UnitTest/
├── CommandLineTests.cpp # Tests for new /A: filters, /T: switch, --owner, --streams
├── ConfigTests.cpp      # Tests for cloud status color configuration
├── FileComparatorTests.cpp  # Tests for time field sorting
├── ResultsDisplayerTests.cpp # Tests for cloud status column output
```

**Structure Decision**: Extend existing files following established patterns. No new source files needed for core functionality.

## Complexity Tracking

No constitution violations. All changes extend existing patterns.

---

## Phase Completion Status

| Phase | Status | Artifacts |
|-------|--------|-----------|
| Phase 0: Research | ✅ COMPLETE | [research.md](research.md) |
| Phase 1: Design | ✅ COMPLETE | [data-model.md](data-model.md), [quickstart.md](quickstart.md) |
| Phase 2: Tasks | ✅ COMPLETE | [tasks.md](tasks.md) |

**Ready for implementation**: Start with Phase 1 Setup tasks
