# Implementation Plan: Cross-Platform Support (Linux & macOS)

**Branch**: `010-cross-platform-pal` | **Date**: 2026-06-25 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `specs/010-cross-platform-pal/spec.md`

## Summary

Bring TCDir's colorized directory listing to Linux (then macOS) without rewriting
the application logic and without changing the Windows product. Approach:
**keep `TCDirCore` wide (`wchar_t`/`wstring`) and platform-agnostic**, and introduce
a **Platform Abstraction Layer (PAL)** that owns all OS interaction. The Windows
PAL backend is a thin passthrough to today's Win32 calls (behaviour unchanged);
a new POSIX backend implements the same seams with `readdir`/`lstat`/`termios`/etc.
Enumeration is **per-directory and bulk**: `TCDirCore` hands the PAL a directory
(or an explicit, shell-expanded path list) and receives a **vector of fully
classified, enriched entries** in one call. File **attributes are a PAL-owned,
name-addressed domain** — `TCDirCore` is attribute-agnostic except for two
structural predicates (directory/symlink) it needs for traversal and a name-based
hook for configurable theming.

## Technical Context

**Language/Version**: C++ `stdcpplatest` (MSVC v145+ on Windows; clang/libc++ or
gcc/libstdc++ on Linux/macOS). The wide-string core compiles on POSIX with
`wchar_t` = 32-bit (UTF-32); the PAL transcodes UTF-8 ↔ UTF-32 at the boundary.
**Primary Dependencies**: Windows SDK (Windows backend); POSIX/libc only for the
POSIX backend core listing. No third-party libraries for the MVP. (Post-MVP
convenience features may add per-terminal/per-shell adapters; cloud status on
macOS uses `st_flags`.)
**Storage**: N/A (filesystem listing tool; reads directory metadata only).
**Testing**: Microsoft C++ Unit Test Framework on Windows (existing 684 tests);
a portable test runner (e.g. a lightweight harness or ctest-driven) for the POSIX
build. The PAL is an injectable interface, so logic tests run against a **mock PAL
backend** with synthetic entries — no real filesystem (satisfies Test Isolation).
**Target Platform**: Windows 10/11 (x64, ARM64) — unchanged; **Linux** (x64,
arm64) first; **macOS** (arm64, x64) shortly after.
**Project Type**: Single C++ CLI/desktop application (core library + thin exe +
unit tests), now multi-backend.
**Performance Goals**: No regression on Windows. On Linux, a large directory
(thousands of entries) lists within ~1.5× the Windows build (SC-007). Preserve the
issue #12 hot-path allocation profile: the PAL fills `m_vMatches` once per
directory; classification/filter/theme-match are computed in the single native
enumeration pass — **no per-entry PAL callbacks on the hot path**.
**Constraints**: Keep Windows behaviour byte-identical (FR-014). Wide-core stays
wide (no narrow rewrite). Hot path stays allocation-lean.
**Scale/Scope**: ~88 core source files; ~6–8 PAL seams × 2 backends; ~40-site
entry-model accessor refactor; ~7 Windows-only subsystems gated.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

**Strengthened by this design:**
- **II. Testing Discipline / Test Isolation (NON-NEGOTIABLE)** — the PAL is exactly
  the injectable seam the constitution demands. Logic becomes testable against a
  mock PAL with synthetic data; no real FS/registry/process access. ✅ (improves)
- **IV. Performance** — per-directory bulk enumeration minimizes boundary crossings
  and preserves the #12 hot-path allocation work. ✅
- **V. Simplicity / Single Responsibility / Members-over-free-functions** — PAL
  seams are cohesive classes; no anonymous namespaces; the core sheds OS detail. ✅
- **I. Code Quality (EHM, single-exit, formatting)** — unchanged; PAL methods
  return `HRESULT` and use EHM. ✅

**Deviations requiring a constitution amendment (flagged, not blocking):**

| Constitution clause | Tension | Resolution |
|---|---|---|
| Tech Constraints: *Target Platforms: Windows 10/11* | Feature adds Linux/macOS | Amend Target Platforms (the feature's entire purpose; tracked in Complexity Tracking) |
| Tech Constraints: *Build System: VS 2026 / MSBuild* | Adds CMake for POSIX | **CMake alongside** the existing `.sln`/`.vcxproj` (Windows build unchanged); amend to list both |
| Tech Constraints: *Testing Framework: MS C++ Unit Test* | Windows-only | Keep on Windows; add a portable runner for POSIX over the same PAL-mocked logic |
| Perf: *Prefer `WriteConsoleW`* | POSIX uses `write()`/ANSI | The `IConsole` seam abstracts it; Windows still uses `WriteConsoleW` |
| Tech Constraints: *no third-party libraries* | Core MVP: still none (POSIX/libc only) | Holds for MVP; revisit only for post-MVP convenience adapters |

These are intrinsic to "support other platforms" and are recorded in Complexity
Tracking. Recommend a constitution MINOR amendment once the MVP lands.

## Project Structure

### Documentation (this feature)

```text
specs/010-cross-platform-pal/
├── plan.md                     # This file
├── spec.md                     # Feature spec (clarified)
├── research.md                 # Phase 0 — decisions + rationale
├── data-model.md               # Phase 1 — entities & relationships
├── quickstart.md               # Phase 1 — build/run/smoke per platform
├── contracts/
│   └── pal-interfaces.md       # Phase 1 — the PAL seam contracts
├── win32-surface-inventory.md  # Prior research input (the PAL/shim contract)
└── checklists/requirements.md  # Spec quality checklist
```

### Source Code (repository root)

```text
TCDirCore/                 # platform-AGNOSTIC core, stays wide; calls the PAL
  Pal/                     # PAL interface headers (IPlatform + seam interfaces)
  Pal/Windows/             # Windows backend (FindFirstFileW, WriteConsoleW, reg…)
  Pal/Posix/               # Linux/macOS backend (readdir/lstat, write, termios…)
  …                        # existing displayers, comparator, config, lister, etc.
TCDir/                     # thin executable entry point (wmain/main)
UnitTest/                  # tests + a Mock PAL backend (synthetic entries)
CMakeLists.txt             # cross-platform build (NEW; alongside TCDir.sln)
TCDir.sln / *.vcxproj      # Windows build (UNCHANGED)
```

**Structure Decision**: Single project, multi-backend. The PAL interface lives in
`TCDirCore/Pal/`; exactly one backend (`Windows/` or `Posix/`) is compiled per
target. `TCDirCore` depends only on the interface. The Windows MSBuild build is
untouched; CMake drives the POSIX (and optionally Windows) builds. Threading,
work queue, sort, dedup, and display stay in `TCDirCore` (already std/portable).

## Complexity Tracking

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| Adds Linux/macOS targets (vs Windows-only constitution) | The feature *is* cross-platform support (issue #8) | N/A — it is the goal; amend constitution |
| CMake added alongside MSBuild | MSBuild cannot build for Linux/macOS toolchains | Keeping MSBuild-only blocks the feature; CMake-alongside keeps Windows build unchanged |
| Portable test runner added | MS C++ Unit Test Framework is Windows-only | Tests must run on the POSIX build too; same PAL-mocked logic is reused |
| New PAL indirection layer | Required to isolate OS calls behind one seam | A pervasive `#ifdef` sprinkle was rejected: it would bury platform logic across ~40 files, defeat testability, and entangle the wide core with POSIX detail |
