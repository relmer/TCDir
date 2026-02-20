# TCDir Development Guidelines

Auto-generated from all feature plans. Last updated: 2026-02-14

## Active Technologies
- C++ (stdcpplatest / MSVC v145+) + Windows SDK, STL only (no third-party libraries) (004-tree-view)
- N/A (filesystem enumeration, no persistent storage) (004-tree-view)

- stdcpplatest (MSVC v145+) + Windows SDK (+ `gdi32.lib` for font detection), STL only — no third-party libraries (003-file-icons)

## Project Structure

```text
src/
tests/
```

## Commands

# Add commands for stdcpplatest (MSVC v145+)

## Code Style

All coding conventions are defined in [/.github/copilot-instructions.md](/.github/copilot-instructions.md). Key rules:
- EHM macros (CHR, CBR, CWRA, etc.) for HRESULT functions with single `Error:` exit point
- NEVER delete blank lines between constructs, NEVER break column alignment
- Preserve exact indentation; match existing whitespace
- `pch.h` must be the first #include in every .cpp file
- Quoted includes only (no angle brackets except in pch.h)
- Prefer `unique_ptr` for exclusive ownership
- Use VS Code build tasks, not direct msbuild

## Recent Changes
- 004-tree-view: Added C++ (stdcpplatest / MSVC v145+) + Windows SDK, STL only (no third-party libraries)

- 003-file-icons: Added stdcpplatest (MSVC v145+) + Windows SDK (+ `gdi32.lib` for font detection), STL only — no third-party libraries

<!-- MANUAL ADDITIONS START -->
<!-- MANUAL ADDITIONS END -->
