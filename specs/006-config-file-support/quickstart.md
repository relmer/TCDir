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
| `TCDirCore/ConfigFileReader.h` | `IConfigFileReader` interface + `CConfigFileReader` |
| `TCDirCore/ConfigFileReader.cpp` | File I/O: read, BOM, line split |
| `TCDirCore/Config.h` | Extended: `LoadConfigFile`, source tracking |
| `TCDirCore/Config.cpp` | Extended: config file parsing, 3-source model |
| `TCDirCore/CommandLine.cpp` | Extended: `/settings` switch |
| `TCDirCore/Usage.cpp` | Extended: `/config` repurposed, `/settings` new |
| `UnitTest/ConfigFileReaderTests.cpp` | File reader tests |
| `UnitTest/ConfigFileTests.cpp` | Config file parsing tests |

## Implementation Order

1. `ConfigFileReader` + tests (file I/O layer)
2. `CConfig::LoadConfigFile` + source tracking + tests (parsing layer)
3. Error model extension (`ErrorInfo` line numbers) + tests
4. `/settings` command + `/config` repurposing + tests
5. Help text updates
