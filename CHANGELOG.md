# Changelog

All notable changes to TCDir are documented in this file.

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
