# Quickstart: Symlink & Junction Target Display

**Date**: 2026-04-18
**Feature**: 007-symlink-junction-targets

## What's Changing

TCDir will display the target path for junctions, symlinks, and AppExecLink entries in normal and tree display modes. After the filename, a `→` arrow and the target path appear:

```
04/18/2026  12:30 PM    <DIR>  RA P      Projects → C:\Dev\Projects
04/18/2026  12:30 PM         0 R  P      config.yml → ..\shared\config.yml
04/18/2026  12:30 PM         0 R  P      python.exe → C:\Program Files\WindowsApps\...\python3.12.exe
```

## Files to Modify

| File | Change |
|------|--------|
| `TCDirCore/DirectoryInfo.h` | Add `wstring m_strReparseTarget` to `FileInfo` |
| `TCDirCore/ReparsePointResolver.h` | **New** — `ResolveReparseTarget()` declaration, `REPARSE_DATA_BUFFER` definition |
| `TCDirCore/ReparsePointResolver.cpp` | **New** — reparse buffer reading and parsing for all 3 tag types |
| `TCDirCore/DirectoryLister.cpp` | Call `ResolveReparseTarget()` in `AddMatchToList()` |
| `TCDirCore/UnicodeSymbols.h` | Add `RightArrow` constant (U+2192) |
| `TCDirCore/ResultsDisplayerNormal.cpp` | Display `→ target` after filename |
| `TCDirCore/ResultsDisplayerTree.cpp` | Display `→ target` after filename |
| `TCDirCore/Config.h` | Expose `GetTextAttrForExtension()` |
| `TCDirCore/Config.cpp` | Implement `GetTextAttrForExtension()` |
| `UnitTest/ReparsePointResolverTests.cpp` | **New** — buffer parsing tests |
| `UnitTest/ResultsDisplayerTests.cpp` | Extend — display format tests |

## Build & Test

```powershell
# Build
# Use VS Code task: "Build + Test Debug (current arch)"

# Or command line:
pwsh -NoProfile -File scripts\Build.ps1 -Configuration Debug -Platform x64 -Target Build
pwsh -NoProfile -File scripts\RunTests.ps1 -Configuration Debug -Platform x64
```

## Key Design Decisions

1. **PrintName preferred over SubstituteName** — PrintName is user-friendly, no `\??\` prefix
2. **Resolution happens at enumeration time** — not at display time; keeps displayers I/O-free
3. **Stack-allocated 16KB buffer** — no heap allocation for reparse reads
4. **No new switches or config keys** — targets always shown in normal/tree modes
5. **Graceful degradation** — any failure to read reparse data → filename without target, no error
