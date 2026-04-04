# Research: Config File Support

## R-001: File I/O Approach

**Decision**: Use C++ `std::ifstream` in binary mode for reading, then convert UTF-8 bytes to wide strings via `MultiByteToWideChar()`. Add `<fstream>` to `pch.h`. Use flag-checking (not exceptions) for error handling to integrate cleanly with EHM.

**Rationale**: C++ streams provide RAII file handling (auto-close on scope exit), are portable for a future Linux port, and are idiomatic modern C++. The existing `_wfopen_s` / `fread` pattern in `ProfileFileManager` is C-style and was not intentionally chosen over C++ I/O. There's no performance reason to prefer Win32 `CreateFileW` / `ReadFile` for a sub-1KB config file read at startup. MSVC's `std::ifstream` constructor accepts `wstring` paths natively.

**EHM integration**: `ifstream` does not throw by default — failed operations set internal state flags. Extract flag checks into local variables before passing to EHM macros (never call functions inside macros). Use `_doserrno` with `HRESULT_FROM_WIN32()` for specific error codes after open failure. Verify `_doserrno` reliability during implementation; fallback to `std::filesystem::exists()` to distinguish file-not-found from other open errors if needed. Exception mode (`file.exceptions(...)`) was considered but rejected because try/catch mixed with EHM goto-cleanup is awkward and inconsistent with the rest of the codebase.

**Alternatives considered**:
- `_wfopen_s` / `fread` / `fclose`: C-style, no RAII, not portable. Exists in ProfileFileManager but not a deliberate pattern choice.
- Windows `CreateFileW` / `ReadFile`: Over-engineered, not portable, no benefit for small files.
- `ifstream` with exceptions: Cleaner failure detection but mixes try/catch with EHM goto-cleanup; not used anywhere in the project.

## R-002: BOM Handling

**Decision**: Reuse the BOM detection logic from `CProfileFileManager::ReadProfileFile` — check first 3 bytes for UTF-8 BOM (`EF BB BF`) and skip if present. Reject UTF-16 BOMs (`FF FE` / `FE FF`) with an error since config files are UTF-8 only.

**Rationale**: ProfileFileManager already implements BOM detection logic. The same checks apply to config files. With `std::ifstream` in binary mode, BOM bytes are read as raw data and can be detected identically.

## R-003: Comment Character Safety

**Decision**: `#` is safe for comment lines and inline comments. No disambiguation needed.

**Rationale**: The existing color parser (`Config::ParseColorName`) accepts only named colors (Black, Blue, LightGreen, etc.) — no hex color codes. Icon code points use the `U+XXXX` prefix format, not `#`. There is no valid setting value that contains `#`, so stripping everything from `#` onward (after trimming the setting) is unambiguous.

## R-004: Source Tracking Architecture

**Decision**: Extend the existing `EAttributeSource` enum from 2 to 3 values: `Default`, `ConfigFile`, `Environment`. The config file applies first (producing `ConfigFile` source markers), then the env var applies on top (overwriting to `Environment` where it conflicts).

**Rationale**: The codebase already tracks sources via `EAttributeSource` and parallel `m_mapExtensionSources` / `m_rgAttributeSources` maps. Adding a third enum value is the minimal change. The `ProcessColorOverrideEntry` function already updates source maps on every call — it just needs to be told which source it's applying for.

**Key insight**: `ApplyUserColorOverrides()` currently hardcodes `EAttributeSource::Environment` when writing source maps. Refactor to accept a source parameter so the same method handles both config file and env var entries.

## R-005: Config File Path Resolution

**Decision**: Use `GetEnvironmentVariableW(L"USERPROFILE", ...)` to resolve `%USERPROFILE%`, then append `\.tcdirconfig`.

**Rationale**: `SHGetKnownFolderPath(FOLDERID_Profile)` requires COM initialization and CoTaskMemFree — overhead for a simple path lookup. `GetEnvironmentVariableW` is simpler, and `%USERPROFILE%` is always set on Windows. The `IEnvironmentProvider` interface already abstracts env var access, so this naturally goes through the existing mock infrastructure for testing.

**Alternatives considered**:
- `SHGetKnownFolderPath(FOLDERID_Profile)`: More "correct" but heavier; requires COM. Overkill for a path that's always available as an env var.
- Hardcode `C:\Users\...`: Fragile, breaks on non-standard Windows installs.

## R-006: Parsing Architecture — Reuse Strategy

**Decision**: Reuse `ProcessColorOverrideEntry()` directly for each config file line. The config file parser's job is only: read file → split lines → strip comments → pass each line to the existing entry processor.

**Rationale**: The spec requires identical syntax between config file entries and env var entries. The existing `ProcessColorOverrideEntry()` already handles all entry types (switches, colors, icons, parameterized values). Reusing it avoids duplicating parsing logic and guarantees syntax parity.

**Line number tracking**: Wrap `ProcessColorOverrideEntry()` calls in a loop that tracks line numbers. Errors pushed to `m_lastParseResult.errors` need the `ErrorInfo` struct extended with an optional line number and source file path.

## R-007: Error Model Extension

**Decision**: Extend `ErrorInfo` with two optional fields: `configFilePath` (wstring, empty for env var errors) and `lineNumber` (size_t, 0 for env var errors). Grouping logic in `DisplayEnvVarIssues` / new `DisplayConfigFileIssues` checks these fields.

**Rationale**: Minimal change to existing struct. Env var errors continue working unchanged (new fields default to empty/0). Config file errors populate both fields. Display code groups by source presence.

**Alternative considered**: Separate error vectors for config file vs env var. Rejected because it would require splitting `m_lastParseResult` into two, complicating the validation API.

## R-008: Initialization Order

**Decision**: Config file is loaded first, then env var overrides, then CLI overrides. Specifically:
1. `CConfig::Initialize()` → load built-in defaults
2. `CConfig::LoadConfigFile()` → NEW: read `.tcdirconfig`, process entries with `EAttributeSource::ConfigFile`
3. `CConfig::ApplyUserColorOverrides()` → existing: read TCDIR env var, process entries with `EAttributeSource::Environment`
4. `CCommandLine::ApplyConfigDefaults()` → merge switch states into CLI defaults
5. `CCommandLine::Parse()` → CLI flags override everything

**Rationale**: The existing flow is Initialize → ApplyUserColorOverrides → CLI. Inserting config file loading between Initialize and ApplyUserColorOverrides naturally produces the correct precedence (config < env var < CLI).
