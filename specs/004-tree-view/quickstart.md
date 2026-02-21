# Quickstart: Tree View Display Mode

**Feature**: 004-tree-view  
**Date**: 2026-02-19

## Prerequisites

- Visual Studio 2026 / MSBuild (MSVC v145+)
- VS Code with C++ extension and configured build tasks
- Windows 10/11 (x64 or ARM64)
- PowerShell 7 (`pwsh`)

## Build & Test

```powershell
# From repo root — use VS Code tasks for standard workflow:
# "Build + Test Debug (current arch)" task

# Or from terminal:
.\scripts\Build.ps1 -Target BuildDebug
.\scripts\RunTests.ps1 -Configuration Debug -Platform Auto
```

## Quick Verification

After implementing the feature, verify with:

```powershell
# Basic tree (should show hierarchy with connectors)
tcdir --Tree

# Depth-limited tree
tcdir --Tree --Depth=2

# Tree with custom indent
tcdir --Tree --TreeIndent=2

# Tree with metadata options
tcdir --Tree --Owner --Icons --Streams

# Abbreviated sizes (Explorer-style, default in tree mode)
tcdir --Tree                # Auto mode — sizes as KB/MB/GB/TB, 7-char fixed width
tcdir --Tree --Size=Bytes   # Exact byte counts with commas in tree mode
tcdir --Size=Auto           # Abbreviated sizes in non-tree mode
tcdir --Size=Bytes          # Explicit exact bytes (default in non-tree)

# Verify size alignment in tree mode
tcdir --Tree .              # All size columns should be uniformly 7 chars wide
tcdir --Tree --Depth=3      # Alignment preserved across depth levels

# Error cases (should all produce clear error messages)
tcdir --Tree -W       # Error: incompatible with wide
tcdir --Tree -B       # Error: incompatible with bare
tcdir --Tree -S       # Error: incompatible with recurse
tcdir --Depth=3       # Error: requires --Tree
tcdir --TreeIndent=10 # Error: value out of range
tcdir --Size=Invalid  # Error: invalid value

# Environment variable configuration
$env:TCDIR = "Tree Depth=2"
tcdir                 # Should show tree with depth 2
$env:TCDIR = "Size=Auto"
tcdir                 # Should show abbreviated sizes
$env:TCDIR = ""
```

## Key Files to Modify

| File | What to change |
|------|---------------|
| `TCDirCore/CommandLine.h` | Add `m_fTree`, `m_cMaxDepth`, `m_cTreeIndent`, `m_eSizeFormat` members |
| `TCDirCore/CommandLine.cpp` | Parse new switches (`--Tree`, `--Depth=N`, `--TreeIndent=N`, `--Size=Auto\|Bytes`); add `ValidateSwitchCombinations()` |
| `TCDirCore/Config.h` | Add `m_fTree`, `m_cMaxDepth`, `m_cTreeIndent`, `m_eSizeFormat` optionals; `EAttribute::TreeConnector` |
| `TCDirCore/Config.cpp` | Parse `Tree`/`Depth=N`/`TreeIndent=N`/`Size=Auto\|Bytes` from TCDIR env var |
| `TCDirCore/TreeConnectorState.h` | **NEW** — `STreeConnectorState` struct with `GetPrefix()`, `Push()`, `Pop()` |
| `TCDirCore/ResultsDisplayerTree.h` | **NEW** — `CResultsDisplayerTree` derived from `CResultsDisplayerNormal` |
| `TCDirCore/ResultsDisplayerTree.cpp` | **NEW** — Tree display flow (`DisplayResults` override) + tree-prefixed file rendering (`DisplayFileResults` override) |
| `TCDirCore/UnicodeSymbols.h` | Add tree connector character constants |
| `TCDirCore/DirectoryLister.cpp` | Route `--Tree` to MT lister; instantiate `CResultsDisplayerTree` instead of Normal |
| `TCDirCore/MultiThreadedLister.h` | Add tree state parameter to `PrintDirectoryTree`/`ProcessChildren` |
| `TCDirCore/MultiThreadedLister.cpp` | Thread connector state through recursion; depth checks |
| `TCDirCore/ResultsDisplayerNormal.h` | Make `DisplayFileStreams` virtual (for tree override) |
| `TCDirCore/ResultsDisplayerNormal.cpp` | Add `FormatAbbreviatedSize` (Explorer-style 3-sig-digit, 1024-based, 7-char fixed width); wire into `DisplayResultsNormalFileSize` for `Auto` mode |
| `TCDirCore/IResultsDisplayer.h` | No signature changes needed |
| `TCDirCore/FileComparator.cpp` | Add interleaved sort mode for tree |
| `TCDirCore/Usage.cpp` | Document new switches (`--Tree`, `--Depth`, `--TreeIndent`, `--Size`) in help |

## New Files

| File | Purpose |
|------|---------|
| `TCDirCore/TreeConnectorState.h` | Encapsulates tree prefix generation logic — `Push`/`Pop` depth levels, generate `├── `/`└── `/`│   ` prefixes |
| `TCDirCore/ResultsDisplayerTree.h` | Tree displayer class declaration — derives from `CResultsDisplayerNormal` |
| `TCDirCore/ResultsDisplayerTree.cpp` | Tree displayer implementation — overrides `DisplayResults` for tree-walking flow and `DisplayFileResults` for tree prefix insertion; reuses inherited column helpers |

## Testing Order

1. **Unit tests first**: CommandLine parsing, Config parsing, TreeConnectorState prefix generation
2. **Integration**: FileComparator interleaved sort, switch validation
3. **Scenario**: End-to-end tree output with known directory structures
4. **Manual**: Visual verification of connector alignment across multiple depth levels

## Implementation Sequence

1. Tree connector constants in `UnicodeSymbols.h` + `STreeConnectorState` struct + unit tests
2. Switch parsing (`CommandLine` + `Config`) + validation + unit tests
3. `CResultsDisplayerTree` class (derives from Normal) + basic `DisplayFileResults` override
4. Wire tree state through `MultiThreadedLister::PrintDirectoryTree`; instantiate tree displayer in `DirectoryLister`
5. Interleaved sort in `FileComparator`
6. Depth limiting
7. Stream continuation lines (override in tree displayer)
8. Usage help text
9. Reparse point (cycle) guard (shared path — protects both `-S` and `--Tree`)
10. Environment variable configuration
