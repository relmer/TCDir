# Quickstart: Ellipsize Long Link Target Paths

**Date**: 2026-04-19
**Feature**: 008-ellipsize-targets

## What's Changing

Long link target paths are middle-truncated with `…` to prevent line wrapping:

```
Before (wraps at 120 chars):
python.exe → C:\Program Files\WindowsApps\Microsoft.DesktopAppInstaller_1.29.30.0_arm64__8wekyb3d8bbwe\AppInstallerPythonRedirector.exe

After (fits in 120 chars):
python.exe → C:\Program Files\…\AppInstallerPythonRedirector.exe
```

New switch: `--Ellipsize` (default on), `--Ellipsize-` to disable.

## Files to Modify

| File | Change |
|------|--------|
| `TCDirCore/UnicodeSymbols.h` | Add `Ellipsis` constant (U+2026) |
| `TCDirCore/PathEllipsis.h` | **New** — `EllipsizePath()` declaration, `SEllipsizedPath` struct |
| `TCDirCore/PathEllipsis.cpp` | **New** — truncation algorithm implementation |
| `TCDirCore/ResultsDisplayerNormal.cpp` | Compute available width, call EllipsizePath, render with split colors |
| `TCDirCore/ResultsDisplayerTree.cpp` | Same as Normal + account for tree prefix width |
| `TCDirCore/CommandLine.h` | Add `m_fEllipsize` member |
| `TCDirCore/CommandLine.cpp` | Parse `--Ellipsize` / `--Ellipsize-` switch |
| `TCDirCore/Config.h` | Add `m_fEllipsize` member |
| `TCDirCore/Config.cpp` | Add to `s_switchMappings`, `s_switchMemberOrder`, bump `SWITCH_COUNT` |
| `TCDirCore/Usage.cpp` | Document `--Ellipsize` in help output |
| `UnitTest/PathEllipsisTests.cpp` | **New** — pure function tests with real WindowsApps paths |

## Build & Test

```powershell
# Use VS Code task: "Build + Test Debug (current arch)"
```

## Key Design Decisions

1. **Pure function** — `EllipsizePath` takes a path string and available width, returns a struct with prefix/suffix split
2. **Arithmetic width calculation** — no character counter needed; compute from known column widths
3. **Priority-based truncation** — first two dirs + leaf dir + filename > first two dirs + filename > first dir + filename > leaf only
4. **Ellipsis color** — `Default` attribute, not file color, so it's visually distinct
5. **Default on** — most users benefit from truncation; `--Ellipsize-` opts out
