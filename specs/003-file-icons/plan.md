# Implementation Plan: Nerd Font File & Folder Icons

**Branch**: `003-file-icons` | **Date**: 2026-02-14 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/003-file-icons/spec.md`

## Summary

Add Nerd Font file/folder icons to directory listings, auto-detecting font availability via a layered strategy (CLI override → env var → WezTerm detection → conhost canary probe → system font enumeration → OFF). Icons display before filenames in all modes (normal, wide, bare) using the same color precedence chain. Cloud status symbols upgrade to NF glyphs when icons are active. Configuration via the existing `TCDIR` env var with extended `[color][,icon]` comma syntax. This is a **major version** bump.

**Coding Conventions**: All code follows [.github/copilot-instructions.md](../../.github/copilot-instructions.md) — EHM patterns, single exit point, formatting preservation, pch.h first, no angle-bracket includes outside pch.h.

**Technical Approach**: Extend `CConfig` with parallel icon mapping tables (extension→glyph, well-known dir→glyph, attribute→glyph, type fallbacks). Extend `GetTextAttrForFile()` to return a `SFileDisplayStyle` struct containing both resolved color and icon. Add a new `CNerdFontDetector` class (with protected virtual GDI methods for test derivation) implementing the 6-step detection chain using `GetGlyphIndicesW` (canary probe) and `EnumFontFamiliesExW` (font enumeration). Modify all three displayers to emit `icon + space` before filenames when icons are active. Extend `CCommandLine` with `/Icons` and `/Icons-` switches. Extend `CUsage` for diagnostics and help.

## Technical Context

**Language/Version**: stdcpplatest (MSVC v145+)
**Primary Dependencies**: Windows SDK (+ `gdi32.lib` for font detection), STL only — no third-party libraries
**Storage**: N/A (filesystem enumeration only)
**Testing**: Microsoft C++ Unit Test Framework (CppUnitTestFramework)
**Target Platform**: Windows 10/11, x64 and ARM64
**Project Type**: Single project (native Windows console application)
**Performance Goals**: Icon feature adds <5% overhead on 1000+ file directories (SC-004); detection runs once at startup; icon lookup is O(1) hash map
**Constraints**: `wchar_t` is 16-bit (surrogate pairs required for Material Design 5-digit code points); ConPTY hides real rendering font from Win32 APIs; `gdi32.lib` link dependency added for font detection
**Scale/Scope**: Extends ~20 existing source files; adds 4 new source files + 2 new test files; estimated ~1500–2000 LOC of production code + ~1000 LOC of tests

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Notes |
|-----------|--------|-------|
| **I. Code Quality** | ✅ PASS | EHM patterns for all HRESULT functions (canary probe, font enumeration). Formatting preserved. New functions follow existing patterns. Smart pointers not needed (GDI handles cleaned up via RAII or goto Error). Single exit point via `Error:` label. |
| **II. Testing Discipline** | ✅ PASS | New `IconMappingTests.cpp` and `NerdFontDetectorTests.cpp` test files. Extended `ConfigTests.cpp`, `CommandLineTests.cpp`, `ResultsDisplayerTests.cpp`. Test derivation pattern (like `ConfigProbe`) for `CNerdFontDetector` GDI overrides. `CTestEnvironmentProvider` reused for env var testing. |
| **III. UX Consistency** | ✅ PASS | `/Icons` and `/Icons-` follow existing switch convention. Documented in `Usage.cpp` (`/?`, `/env`, `/config`). Colors via `CConsole`. Backward compatible — no icon comma = no behavior change. |
| **IV. Performance** | ✅ PASS | Icon lookup is O(1) hash map. Detection runs once at startup. No per-file allocations in hot path. Static constexpr default tables. `EnumFontFamiliesExW` is sub-millisecond. |
| **V. Simplicity** | ✅ PASS | 4 new source files, each with single clear purpose (icon mapping data, font detection). Parallels existing color infrastructure. No external dependencies. `INerdFontDetector` interface adds testability without over-engineering. |

## Project Structure

### Documentation (this feature)

```text
specs/003-file-icons/
├── plan.md              # This file
├── spec.md              # Feature specification (completed)
├── research.md          # Terminal host detection & font config research (completed)
├── nerd-font-glyphs.md  # Glyph reference with code points (completed)
├── glyph-preview.html   # Visual glyph preview (completed)
├── data-model.md        # Phase 1: Data model & entity design
├── quickstart.md        # Phase 1: Developer implementation guide
└── tasks.md             # Phase 2 output (/speckit.tasks — NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
TCDirCore/
├── IconMapping.h            # NEW — Icon code point constants, default mapping table, lookup helpers
├── IconMapping.cpp          # NEW — Default table initialization, icon resolution functions
├── NerdFontDetector.h       # NEW — CNerdFontDetector with protected virtual GDI methods
├── NerdFontDetector.cpp     # NEW — Layered detection chain (canary probe, font enumeration)
├── Config.h                 # MODIFIED — Add icon override maps, m_fIcons, SFileDisplayStyle
├── Config.cpp               # MODIFIED — Parse icon syntax from TCDIR, unified precedence resolver
├── CommandLine.h            # MODIFIED — Add /Icons, /Icons- switches
├── CommandLine.cpp          # MODIFIED — Add long switch table entry for Icons
├── ResultsDisplayerNormal.cpp   # MODIFIED — Insert icon glyph before filename
├── ResultsDisplayerWide.cpp     # MODIFIED — Insert icon, suppress brackets, add cloud status
├── ResultsDisplayerBare.cpp     # MODIFIED — Insert icon before filename
├── ResultsDisplayerWithHeaderAndFooter.h   # MODIFIED — Pass icon active state
├── ResultsDisplayerWithHeaderAndFooter.cpp # MODIFIED — Pass icon active state
├── TCDir.cpp                # MODIFIED — Detection orchestration, displayer wiring
├── Usage.cpp                # MODIFIED — Document /Icons flags, icon syntax, config display
├── Usage.h                  # MODIFIED — Add icon-related display helpers if needed
├── Version.h                # MODIFIED — Major version bump
├── pch.h                    # MODIFIED — Add #pragma comment(lib, "gdi32.lib")

UnitTest/
├── IconMappingTests.cpp         # NEW — Icon lookup, precedence, surrogate encoding, fallbacks
├── NerdFontDetectorTests.cpp    # NEW — Detection chain with mock env provider (derivation pattern)
├── CommandLineTests.cpp         # MODIFIED — /Icons, /Icons- parsing tests
├── ConfigTests.cpp              # MODIFIED — TCDIR icon syntax, duplicates, precedence tests
├── ResultsDisplayerTests.cpp    # MODIFIED — Icon display in all modes
├── DirectoryListerScenarioTests.cpp  # MODIFIED — End-to-end scenarios with icons
```

**Structure Decision**: Extend existing project structure. Two new logical components (IconMapping, NerdFontDetector) get their own files following single-responsibility principle. All other changes extend existing files following established patterns.

## Complexity Tracking

No constitution violations. All changes extend existing patterns or add well-scoped new components.

---

## Constitution Re-Check (Post Phase 1 Design)

*GATE: Re-evaluated after data model, quickstart, and research are complete.*

| Principle | Status | Post-Design Notes |
|-----------|--------|-------------------|
| **I. Code Quality** | ✅ PASS | `ProbeConsoleFontForGlyph` and `IsNerdFontInstalled` follow EHM with `Error:` label and GDI handle cleanup. `constexpr CodePointToWideChars` verified at compile time via `static_assert`. New `k_rgAttributePrecedenceOrder[]` is `constexpr` like `k_rgFileAttributeMap`. Formatting conventions preserved — column alignment in Config.h members, blank lines between groups. |
| **II. Testing Discipline** | ✅ PASS | 42 specific test scenarios documented in quickstart.md. Existing `ConfigProbe` derivation pattern extended for both icon config and font detector testing. `CTestEnvironmentProvider` reused for all env var tests. `FileSystemMock` + IAT patching reused for displayer tests. GDI calls overridable via protected virtual methods. |
| **III. UX Consistency** | ✅ PASS | `/Icons` and `/Icons-` use existing long switch infrastructure (`s_krgLongSwitches[]`). `/?` help, `/env` documentation, and `/config` display all extended via existing `CUsage` functions. Icon color inherits filename color — no new color configuration concept. Backward compat: entries without comma = zero behavior change. |
| **IV. Performance** | ✅ PASS | Icon lookup: O(1) `unordered_map`. Detection: runs once at startup — `EnumFontFamiliesExW` sub-ms, `GetGlyphIndicesW` sub-ms. `CodePointToWideChars` is constexpr. No heap allocations in the per-file display hot path (stack `wchar_t[3]` buffer). Static tables avoid runtime initialization cost. |
| **V. Simplicity** | ✅ PASS | 4 new source files with clear single responsibilities: IconMapping (data), NerdFontDetector (detection). No over-engineering — no template metaprogramming, no complex inheritance, no test-only interfaces. GDI methods are protected virtual for test derivation (existing pattern). Attribute precedence reorder is a deliberate spec decision, documented in changelog. |

**Post-Design Gate**: ✅ ALL PASS — no violations, no complexity justification needed.

---

## Phase Completion Status

| Phase | Status | Artifacts |
|-------|--------|-----------|
| Phase 0: Research | ✅ COMPLETE | [research.md](research.md) (appended with R1–R7 implementation research) |
| Phase 1: Design | ✅ COMPLETE | [data-model.md](data-model.md), [quickstart.md](quickstart.md) |
| Phase 2: Tasks | ✅ COMPLETE | [tasks.md](tasks.md) (34 tasks across 9 phases) |

**Ready for**: Implementation — start with Phase 1 (Setup) and Phase 2 (Foundational)
