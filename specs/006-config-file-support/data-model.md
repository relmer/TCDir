# Data Model: Config File Support

## Entities

### Modified: `CConfig::EAttributeSource` (enum)

**Current**:
```
Default, Environment
```

**New**:
```
Default, ConfigFile, Environment
```

Used in: `m_rgAttributeSources[]`, `m_mapExtensionSources`, `m_mapExtensionIconSources`, `m_mapWellKnownDirIconSources`, `FileAttrMap`

**Impact**: All existing source-tracking parallel maps gain a third possible value. Display code in Usage.cpp must render three labels.

---

### Modified: `CConfig::ErrorInfo` (struct)

**Current fields**:
| Field | Type | Description |
|-------|------|-------------|
| `message` | `wstring` | Error description |
| `entry` | `wstring` | Full "Key=Value" segment |
| `invalidText` | `wstring` | Portion to underline |
| `invalidTextOffset` | `size_t` | Position of invalidText within entry |

**New fields**:
| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `sourceFilePath` | `wstring` | `L""` | Config file path (empty for env var errors) |
| `lineNumber` | `size_t` | `0` | 1-based line number (0 for env var errors) |

**Grouping rule**: Errors with non-empty `sourceFilePath` are config file errors; others are env var errors. Display code groups and headers accordingly.

---

### Modified: `CConfig` (class)

**New public members**:

| Member | Type | Description |
|--------|------|-------------|
| `m_strConfigFilePath` | `wstring` | Resolved path to `.tcdirconfig` (empty if not found) |
| `m_fConfigFileLoaded` | `bool` | Whether config file was successfully loaded |

**New protected members**:

| Member | Type | Description |
|--------|------|-------------|
| `m_configFileParseResult` | `ValidationResult` | Errors from config file parsing (separate from env var errors) |
| `m_configFileReader` | `CConfigFileReader` | Direct member instance for parsing file bytes into lines |

**New source tracking members** (public):

| Member | Type | Description |
|--------|------|-------------|
| `m_rgSwitchSources[9]` | `EAttributeSource[9]` | Source tracking for each of the 9 boolean switches (W, B, S, P, M, Owner, Streams, Icons, Tree) |
| `m_eMaxDepthSource` | `EAttributeSource` | Source tracking for the Depth parameter |
| `m_eTreeIndentSource` | `EAttributeSource` | Source tracking for the TreeIndent parameter |
| `m_eSizeFormatSource` | `EAttributeSource` | Source tracking for the Size parameter |

**New public constants**:

| Member | Type | Description |
|--------|------|-------------|
| `SWITCH_COUNT` | `static constexpr size_t` | Value `9` — number of boolean switches |
| `s_switchMemberOrder[SWITCH_COUNT]` | `static const optional<bool> CConfig::*` | Ordered member-pointers for switch-to-source-index mapping |

**New public methods**:

| Method | Signature | Description |
|--------|-----------|-------------|
| `LoadConfigFile` | `HRESULT LoadConfigFile()` | Resolve `.tcdirconfig` path from `%USERPROFILE%`, open file via `CreateFileW`, read bytes via `ReadFile`, pass to `m_configFileReader.ReadLines`, then call `ProcessConfigLines`. Silently skips if USERPROFILE not set or file not found. |
| `ValidateConfigFile` | `ValidationResult ValidateConfigFile()` | Return config file parse errors |
| `GetConfigFilePath` | `const wstring& GetConfigFilePath() const` | Return resolved config file path |
| `IsConfigFileLoaded` | `bool IsConfigFileLoaded() const` | Whether config file was found and loaded |

**Modified protected methods**:

| Method | Change |
|--------|--------|
| `ApplyUserColorOverrides` | Keep existing name; add `EAttributeSource source` parameter with default `EAttributeSource::Environment` so the same method handles both config file and env var entries |
| `ProcessColorOverrideEntry` | Add `EAttributeSource source` parameter with default `EAttributeSource::Environment`; all source-map writes use this parameter instead of hardcoded `Environment` |
| All methods that write to source maps | Thread the `source` parameter through the call chain (all with default `EAttributeSource::Environment`) |

**New protected methods**:

| Method | Signature | Description |
|--------|-----------|-------------|
| `ProcessConfigLines` | `void ProcessConfigLines(const vector<wstring>& lines)` | Line-by-line processing loop: trim, skip blanks/comments, strip inline comments, pass entries to `ProcessColorOverrideEntry` with `ConfigFile` source, tag errors with line numbers |

---

### New: `CConfigFileReader` (class)

A concrete class (no interface) responsible for parsing raw file bytes into lines. File I/O (opening, reading) is handled by `CConfig::LoadConfigFile` using Win32 `CreateFileW` / `ReadFile` / `GetFileSizeEx` with `AutoHandle` for RAII. The reader receives the raw bytes and handles:
- BOM detection (UTF-8 BOM skipped, UTF-16 LE/BE BOM rejected with error)
- UTF-8 to wide conversion via `MultiByteToWideChar`
- Line splitting on `\r\n`, `\n`, `\r`
- `<fstream>` added to `pch.h` (available but not used for config file I/O)

```
CConfigFileReader
├── ReadLines(bytes, lines, errorMessage) → HRESULT
├── (private) CheckAndStripBom(bytes, errorMessage) → HRESULT
├── (private) ConvertUtf8ToWide(bytes, wideContent, errorMessage) → HRESULT
├── (private) SplitLines(content, lines) → void
```

| Method | Signature | Description |
|--------|-----------|-------------|
| `ReadLines` | `HRESULT ReadLines(const string& bytes, vector<wstring>& lines, wstring& errorMessage)` | Parse raw bytes: check/strip BOM, convert UTF-8 to wide, split into lines. Returns `S_OK` on success, `E_FAIL` with errorMessage on encoding error. |
| `CheckAndStripBom` | `HRESULT CheckAndStripBom(string& bytes, wstring& errorMessage)` | Detect and strip UTF-8 BOM; reject UTF-16 BOM with descriptive error message. |
| `ConvertUtf8ToWide` | `HRESULT ConvertUtf8ToWide(const string& bytes, wstring& wideContent, wstring& errorMessage)` | Convert UTF-8 bytes to wide string via `MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, ...)`. |
| `SplitLines` | `void SplitLines(const wstring& content, vector<wstring>& lines)` | Split wide string on `\r\n`, `\n`, or `\r` line endings. |

**Testability**: Unit tests pass raw byte strings directly to `ReadLines` without any file I/O. No interface or mock class is needed — the reader has no file system dependency.

---

### Modified: `CCommandLine` (class)

**New member**:

| Member | Type | Description |
|--------|------|-------------|
| `m_fSettings` | `bool` | `/settings` switch — display merged configuration tables |

**Modified**: `HandleLongSwitch` adds `"settings"` to the long switch table mapping to `m_fSettings`.

---

### Modified: `CUsage` (class)

**New static methods**:

| Method | Description |
|--------|-------------|
| `DisplayConfigFileHelp` | Config file syntax reference + color/icon format reference + example file + env var override note + file path and load status + errors (new `/config` output) |
| `DisplayConfigFileIssues` | Render config file errors with line numbers (grouped separately). Accepts `fShowHint` parameter — when `true`, appends `(see /config for help)` to the error header |
| `DisplaySettings` | Merged configuration tables with 3-source column (new `/settings` output, replaces old `DisplayCurrentConfiguration`). Shows "No config file or TCDIR..." message when neither source is set. |

**Modified static methods**:

| Method | Change |
|--------|--------|
| `DisplayCurrentConfiguration` | Renamed/repurposed → becomes `DisplaySettings` |
| `DisplayEnvVarIssues` | Gains `fShowHint` parameter (same pattern as `DisplayConfigFileIssues`) |

---

## State Transitions

```
Startup Flow:
  [Initialize defaults] → [LoadConfigFile] → [ApplyEnvVarOverrides] → [ApplyConfigDefaults to CLI] → [Parse CLI]
       Default               ConfigFile          Environment              (merge switches)            (CLI wins)

Error Accumulation:
  LoadConfigFile → m_configFileParseResult.errors (with lineNumber + sourceFilePath)
  ApplyEnvVarOverrides → m_lastParseResult.errors (lineNumber=0, sourceFilePath=empty)

Display at end of run:
  1. Config file errors (if any) — grouped with header "There are some problems with your config file (see /config for help):"
  2. Env var errors (if any) — grouped with header "There are some problems with your TCDIR environment variable (see /env for help):"
```

## Validation Rules

- Config file path: resolved from `%USERPROFILE%` env var + `\.tcdirconfig`
- File not found: silent skip (FR-014), `m_fConfigFileLoaded = false`
- File I/O error: single `ErrorInfo` in `m_configFileParseResult`, `m_fConfigFileLoaded = false`
- Per-line parsing: identical rules to env var entry parsing, plus line number tracking
- Duplicate settings: last occurrence wins (both within config file, and env var over config file)
