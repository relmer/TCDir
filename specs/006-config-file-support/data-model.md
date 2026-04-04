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

**New protected members**:

| Member | Type | Description |
|--------|------|-------------|
| `m_strConfigFilePath` | `wstring` | Resolved path to `.tcdirconfig` (empty if not found) |
| `m_fConfigFileLoaded` | `bool` | Whether config file was successfully loaded |
| `m_configFileParseResult` | `ValidationResult` | Errors from config file parsing (separate from env var errors) |

**New public methods**:

| Method | Signature | Description |
|--------|-----------|-------------|
| `LoadConfigFile` | `HRESULT LoadConfigFile()` | Read `.tcdirconfig`, parse lines, apply settings with `ConfigFile` source |
| `ValidateConfigFile` | `ValidationResult ValidateConfigFile()` | Return config file parse errors |
| `GetConfigFilePath` | `const wstring& GetConfigFilePath() const` | Return resolved config file path |
| `IsConfigFileLoaded` | `bool IsConfigFileLoaded() const` | Whether config file was found and loaded |

**Modified protected methods**:

| Method | Change |
|--------|--------|
| `ApplyUserColorOverrides` | Renamed to `ApplyOverridesFromEnvVar` for clarity (or keep name, add source parameter) |
| `ProcessColorOverrideEntry` | Add `EAttributeSource source` parameter; all source-map writes use this parameter instead of hardcoded `Environment` |
| All methods that write to source maps | Thread the `source` parameter through the call chain |

---

### New: `IConfigFileReader` (interface)

```
IConfigFileReader
├── ReadLines(path, lines, error) → HRESULT
```

| Method | Signature | Description |
|--------|-----------|-------------|
| `ReadLines` | `HRESULT ReadLines(const wstring& path, vector<wstring>& lines, wstring& errorMessage)` | Read file, handle BOM, split into lines. Returns `S_OK` on success, `S_FALSE` if file not found, `E_FAIL` with errorMessage on I/O error. |

**Purpose**: Abstracts file I/O for unit testing. Production implementation uses `std::ifstream` (binary mode) / `MultiByteToWideChar`. Test mock returns in-memory lines.

---

### New: `CConfigFileReader` (class)

Implements `IConfigFileReader`. Handles:
- `std::ifstream` in binary mode (MSVC accepts `wstring` paths)
- BOM detection (UTF-8 BOM skipped, UTF-16 BOM rejected with error)
- Raw bytes → `MultiByteToWideChar` → wstring
- Line splitting on `\r\n`, `\n`, `\r`
- `<fstream>` added to `pch.h`

---

### New: `CTestConfigFileReader` (test class)

Implements `IConfigFileReader` for unit tests. Stores lines in-memory via a `Set()` method, returns them from `ReadLines()`. Can simulate file-not-found (`S_FALSE`) and I/O errors (`E_FAIL`).

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
| `DisplayConfigFileHelp` | Config file syntax reference + status + decoded settings + errors (new `/config` output) |
| `DisplayConfigFileIssues` | Render config file errors with line numbers (grouped separately) |
| `DisplaySettings` | Merged configuration tables with 3-source column (new `/settings` output, replaces old `DisplayCurrentConfiguration`) |

**Modified static methods**:

| Method | Change |
|--------|--------|
| `DisplayCurrentConfiguration` | Renamed/repurposed → becomes `DisplaySettings` |

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
  1. Config file errors (if any) — grouped with header "Config file issues in <path>:"
  2. Env var errors (if any) — grouped with header "Environment variable issues:"
```

## Validation Rules

- Config file path: resolved from `%USERPROFILE%` env var + `\.tcdirconfig`
- File not found: silent skip (FR-014), `m_fConfigFileLoaded = false`
- File I/O error: single `ErrorInfo` in `m_configFileParseResult`, `m_fConfigFileLoaded = false`
- Per-line parsing: identical rules to env var entry parsing, plus line number tracking
- Duplicate settings: last occurrence wins (both within config file, and env var over config file)
