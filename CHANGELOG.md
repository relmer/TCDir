# Changelog

All notable changes to TCDir are documented in this file.

## [5.1.1106] - 2026-02-21

### Added
- `--Tree` switch: hierarchical directory tree view with Unicode box-drawing connectors (`├──`, `└──`, `│`)
- `--Depth=N` switch: limit tree recursion depth (e.g., `--Depth=2` shows two levels)
- `--TreeIndent=N` switch: configurable indent width per tree level (1–8, default 4)
- `--Size=Auto` switch: Explorer-style abbreviated file sizes (e.g., `8.90 KB`, `1.00 MB`, `2.38 GB`) with fixed 7-character width — default in tree mode
- `--Size=Bytes` switch: explicit opt-in for exact comma-separated sizes (existing default for non-tree modes)
- Tree connector color (`TreeConnector`) configurable via `TCDIR` environment variable
- TCDIR env var support for `Tree`, `Tree-`, `Depth=N`, `TreeIndent=N`, `Size=Auto`, `Size=Bytes`
- Thread-safe empty subdirectory pruning when file masks are active (producer-side upward propagation via parent back-pointers and condition variables)
- Reparse-point cycle guard: junction/symlink directories are listed but not expanded, preventing infinite loops in both `-S` and `--Tree` modes
- Interleaved sort in tree mode: directories and files sorted together (not grouped)
- Per-directory summary at each tree level plus grand total at end
- Comprehensive test suite: 408 tests covering tree connectors, depth limiting, pruning, streams, icons, reparse points, and column alignment

### Changed
- Minor version bump from 5.0 to 5.1
- Column spacing adjustment: added 1 space between attributes and size column, reduced gap between size and icon from 3 to 2
- `<DIR>` shifted left within its field for better visual alignment with abbreviated sizes

### Incompatibilities
- `--Tree` cannot be combined with `-W` (wide), `-B` (bare), `-S` (recurse), or `--Owner`
- `--Size=Bytes` cannot be used with `--Tree` (tree mode requires fixed-width sizes)
- `--Depth` and `--TreeIndent` require `--Tree`

## [5.0.1038] - 2026-02-15

### Added
- Nerd Font file and folder icons in directory listings (normal, wide, and bare modes)
- ~200 file extension icon mappings aligned with Terminal-Icons default theme
- ~65 well-known directory icon mappings (Terminal-Icons aligned with 5 intentional deviations)
- Auto-detection of Nerd Font availability via GDI glyph canary probe and system font enumeration
- WezTerm auto-detection (bundles Nerd Font symbols natively)
- ConPTY terminal detection (Windows Terminal, VS Code, etc.)
- `/Icons` and `/Icons-` CLI switches for manual icon override
- `TCDIR=Icons` / `TCDIR=Icons-` env var switch for persistent icon preference
- `TCDIR=.ext=Color,U+XXXX` icon override syntax for per-extension custom icons
- Cloud status Nerd Font glyph upgrade (☁/✓/📌 → nf-md glyphs when NF available)
- Icon display in `/config` and `/env` diagnostics
- Rust language support (.rs, .toml, .cargo)
- IconMapping module (IconMapping.h/cpp) with compile-time sorted lookup tables
- NerdFontDetector module (NerdFontDetector.h/cpp) with GDI and heuristic detection
- Comprehensive icon tests: mapping lookups, NF detection, display integration (307 total tests)

### Changed
- Major version bump from 4.x to 5.0

### Fixed
- Nerd Font suffix detection now matches abbreviated suffixes (NF, NFM, NFP)

## [4.2.1436] - 2026-02-13

### Fixed
- TCDIR env var: entries with invalid background color (e.g., `Chartreuse`) now reject the entire entry instead of falling back to black-on-black
- TCDIR env var: entries where foreground and background are the same color (e.g., `Blue on Blue`) are now rejected as unreadable
- `--config` now shows validation errors after the configuration table, matching `--env` convention
- Foreground colors matching the terminal background are now auto-corrected with a contrasting background in all display paths (listing, `--config`, `--env`)

## [4.2.1414] - 2026-02-05

### Changed
- Multi-mask processing: masks targeting the same directory are now grouped and processed together with deduplication
- Directory counts (both per-directory and grand total) now reflect subdirectories whose names match the pattern, not all traversed directories
- CDirectoryInfo simplified with in-class member initializers

### Added
- Comprehensive directory count and mask matching tests

## [4.2.1322] - 2026-02-01

### Fixed
- TCDIR env var error message now uses correct switch prefix (`--env` vs `/env`) based on user's choice
- TCDIR env var error "see --env for help" hint no longer shown when already viewing `--env` output

## [4.2.1318] - 2026-02-01

### Changed
- Refactored console output to use `ColorPrintf`/`ColorPuts` with embedded color markers (`{EAttributeName}`)
- `--env` help text now uses `{Information}`/`{InformationHighlight}` colors consistently with `-?` output
- Wide listing: `GetWideFormattedName` now returns `wstring_view` with caller-provided stack buffer (thread-safe)
- Cloud status display refactored to use lookup table instead of switch statement

### Fixed
- `ColorPrintf` no longer appends trailing newline (use `ColorPuts` for that)

## [4.2.1284] - 2026-01-31

### Added
- Colorized help output: `/? ` now displays switches, attribute letters, and cloud status symbols in color

## [4.2.1195] - 2026-01-29

### Fixed
- Cloud status symbols no longer appear for files outside cloud sync roots (e.g., files copied from OneDrive that retained stale attributes)
- IncrementVersion.ps1: Add robustness with atomic writes, retry logic, and auto-recovery from git for corrupted Version.h

## [4.2] - 2026-01-28

### Added
- Cloud file visualization: ○/◐/● symbols show sync status for OneDrive, iCloud, etc.
- Cloud attribute filters: `/A:O` (cloud-only), `/A:L` (local), `/A:V` (pinned/always available)
- Extended attribute filters: `/A:X` (not indexed), `/A:I` (integrity), `/A:B` (no scrub), `/A:F` (sparse), `/A:U` (unpinned)
- `/T:` switch to select time field for display and sorting (Creation, Access, Write)
- `--owner` switch to display file ownership (DOMAIN\User)
- `--streams` switch to display NTFS alternate data streams
- `--debug` switch for raw hex attribute display (debug builds only)
- Decoded settings display in `--env` output

### Changed
- TCDIR env var: switch names no longer require prefix (`W` instead of `-W`)

## [4.1] - 2026-01-15

### Added
- `/B` switch for bare listing format (filenames only, no headers/footers)
- Seamless support for `-` or `/` switch prefix (reflects user's choice in help)

## [4.0] - 2026-01-16

### Added
- Multi-threaded directory enumeration (`/M`, enabled by default)
- ARM64 native support
- `--env` switch to display TCDIR environment variable help and current value
- `--config` switch to display current color configuration
- TCDIR env var: support for default switches (W, S, P, M)
- TCDIR env var: file attribute color overrides (`Attr:H=DarkGrey`)
- Structured error reporting with visual underlining for TCDIR parsing errors

### Changed
- Split configuration display from env var help (`--env` vs `--config`)

## [3.x] - 2020-2025

### Added
- `/P` switch to display performance timing
- Unit test framework
- TCDIR environment variable for color customization

### Changed
- Upgraded to C++latest (/std:c++latest)
- Refactored to use std::filesystem
- Project restructured: TCDirCore library + TCDir executable

## [2.x] - 2020

Modernization of the classic TCDir codebase.

## [1.x] - 1990s

Initial versions, inspired by ZDir

---

*For the complete commit history, see the [GitHub repository](https://github.com/relmer/TCDir).*
