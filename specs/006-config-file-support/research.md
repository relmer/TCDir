# Research: Config File Support

## R-001: File I/O Approach

**Decision**: Use Win32 `CreateFileW` / `ReadFile` / `GetFileSizeEx` for opening and reading the file, with `AutoHandle` for RAII. After reading raw bytes into a `std::string` buffer, pass them to `CConfigFileReader::ReadLines` which handles BOM detection, UTF-8 to wide conversion via `MultiByteToWideChar()`, and line splitting. `<fstream>` was added to `pch.h` but is not used for config file I/O.

**Rationale**: Win32 file APIs provide direct access to `GetLastError()` for distinguishing file-not-found from other open errors, which integrates cleanly with the EHM pattern (no need for `_doserrno` workarounds or `std::filesystem::exists()` fallbacks). `AutoHandle` provides RAII for the file handle, matching existing code patterns. The separation of file I/O (in `LoadConfigFile`) from byte parsing (in `CConfigFileReader`) keeps the reader testable — unit tests pass raw byte strings directly to `ReadLines` without any file system interaction.

**EHM integration**: `CreateFileW` returns `INVALID_HANDLE_VALUE` on failure; `GetLastError()` provides the specific error code. `ReadFile` and `GetFileSizeEx` return `BOOL` results compatible with EHM `CWRA` macros. No try/catch or exception handling needed.

**Alternatives considered**:
- `std::ifstream` (binary mode): Originally planned, but `GetLastError()` integration is less clean — requires `_doserrno` after open failure, which was found unreliable for distinguishing error codes. Would need `std::filesystem::exists()` pre-check as fallback.
- `_wfopen_s` / `fread` / `fclose`: C-style, no RAII, not portable. Exists in ProfileFileManager but not a deliberate pattern choice.
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

**Line number tracking**: `ProcessConfigLines` (a separate protected method) iterates lines, tracks line numbers, and after each `ProcessColorOverrideEntry` call, tags any newly appended errors in `m_configFileParseResult.errors` with the config file path and 1-based line number.

## R-007: Error Model Extension

**Decision**: Extend `ErrorInfo` with two fields: `sourceFilePath` (wstring, empty for env var errors) and `lineNumber` (size_t, 0 for env var errors). Config file errors go to `m_configFileParseResult` and env var errors go to `m_lastParseResult` — separate `ValidationResult` instances. Display functions `DisplayConfigFileIssues` and `DisplayEnvVarIssues` each query their respective result object.

**Rationale**: Separate error containers for config file vs env var simplifies grouping in display code — each display function queries one result object. The `sourceFilePath` and `lineNumber` fields in `ErrorInfo` are still present for rendering purposes (line number display in config file errors). Display functions accept a `fShowHint` parameter: when `true` (normal listing runs), error headers include `(see /config for help)` or `(see /env for help)`; when `false` (inside `/settings`), the hint is omitted.

**Alternative considered**: Single error vector for both sources with grouping by `sourceFilePath` presence. Rejected because it would require runtime filtering on every display and complicates the validation API.

## R-008: Initialization Order

**Decision**: Config file is loaded first, then env var overrides, then CLI overrides. Specifically:
1. `CConfig::Initialize()` → load built-in defaults
2. `CConfig::LoadConfigFile()` → NEW: read `.tcdirconfig`, process entries with `EAttributeSource::ConfigFile`
3. `CConfig::ApplyUserColorOverrides()` → existing: read TCDIR env var, process entries with `EAttributeSource::Environment`
4. `CCommandLine::ApplyConfigDefaults()` → merge switch states into CLI defaults
5. `CCommandLine::Parse()` → CLI flags override everything

**Rationale**: The existing flow is Initialize → ApplyUserColorOverrides → CLI. Inserting config file loading between Initialize and ApplyUserColorOverrides naturally produces the correct precedence (config < env var < CLI).
