# Quickstart: Config File Support

## Build & Test

```powershell
# Build (from VS Code tasks or terminal)
# Uses existing build infrastructure — no new build steps required

# Run tests
scripts\RunTests.ps1 -Configuration Debug -Platform x64
```

## Try It

1. Create a config file:
```powershell
@"
# My tcdir config
tree
icons

# Colors
.cpp = LightGreen
.h   = Yellow on Blue
D    = LightCyan
"@ | Set-Content "$env:USERPROFILE\.tcdirconfig" -Encoding UTF8
```

2. Run tcdir — settings from the file should be applied:
```powershell
tcdir
```

3. Override with env var — env var wins for conflicting keys:
```powershell
$env:TCDIR = ".cpp=Red"
tcdir    # .cpp is now Red (env var), but tree/icons/.h/D still from config file
```

4. Inspect config file diagnostics:
```powershell
tcdir /config
```

5. View merged settings with sources:
```powershell
tcdir /settings
```

## Key Files

| File | Purpose |
|------|---------|
| `TCDirCore/ConfigFileReader.h` | `CConfigFileReader` class (BOM, UTF-8 conversion, line splitting) |
| `TCDirCore/ConfigFileReader.cpp` | Byte parsing: BOM check, MultiByteToWideChar, line split |
| `TCDirCore/Config.h` | Extended: `LoadConfigFile`, source tracking, switch/param source arrays |
| `TCDirCore/Config.cpp` | Extended: config file loading (Win32 I/O), `ProcessConfigLines`, 3-source model |
| `TCDirCore/CommandLine.cpp` | Extended: `/settings` switch |
| `TCDirCore/Usage.cpp` | Extended: `/config` repurposed, `/settings` new, error display with `fShowHint` |
| `TCDirCore/TCDir.cpp` | Entry point: initialization flow, command dispatch, end-of-run error display |
| `UnitTest/ConfigFileReaderTests.cpp` | Byte parsing tests (BOM, line splitting, UTF-8) |
| `UnitTest/ConfigFileTests.cpp` | Config file parsing tests (loading, comments, precedence, errors) |

## Implementation Order

1. `CConfigFileReader` + tests (byte parsing layer — BOM, UTF-8, line splitting)
2. `CConfig::LoadConfigFile` + `ProcessConfigLines` + source tracking + tests (file I/O + parsing layer)
3. Error model extension (`ErrorInfo` line numbers) + tests
4. `/settings` command + `/config` repurposing + tests
5. Help text updates
