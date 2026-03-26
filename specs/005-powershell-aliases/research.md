# Research: PowerShell Alias Configuration

**Feature**: 005-powershell-aliases | **Date**: 2026-03-25

## R1: Parent Process Detection (PS Version Auto-Detection)

**Decision**: Use `CreateToolhelp32Snapshot` + `QueryFullProcessImageNameW`

**Rationale**: Toolhelp API is user-mode, requires no special privileges, and is well-documented. NtQueryInformationProcess is undocumented and unnecessarily complex for this use case.

**Alternatives Considered**:
- `NtQueryInformationProcess` — undocumented, requires winternl.h, overly complex
- `PSModulePath` environment variable — PS 7+ includes both `\PowerShell\` and `\WindowsPowerShell\` paths, making it ambiguous

**Implementation Pattern**:
1. `CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)` → walk with `Process32First`/`Process32Next` to find current PID's parent
2. `OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, ...)` on parent PID
3. `QueryFullProcessImageNameW(hProcess, ...)` to get full image path
4. Extract filename — compare against `pwsh.exe` (7+) or `powershell.exe` (5.1)

**Error Handling**: Parent may have exited. `OpenProcess` fails → fall back to error message directing user to run from PowerShell.

**Required Headers**: `<tlhelp32.h>` (not currently in pch.h)

---

## R2: Profile Path Resolution via Shell APIs

**Decision**: Use `SHGetKnownFolderPath(FOLDERID_Documents)` and `SHGetKnownFolderPath(FOLDERID_ProgramData)`

**Rationale**: Correctly handles OneDrive Known Folder Move (KFM) redirection transparently. No PowerShell child process needed.

**Alternatives Considered**:
- Expanding `%USERPROFILE%\Documents` — fails when OneDrive KFM redirects Documents
- Shelling out to PowerShell (`$PROFILE`) — creates "which PS version" problem + process overhead

**Path Construction**:

| Scope | PS 7+ | PS 5.1 |
|-------|-------|--------|
| Current User, Current Host | `{Documents}\PowerShell\Microsoft.PowerShell_profile.ps1` | `{Documents}\WindowsPowerShell\Microsoft.PowerShell_profile.ps1` |
| Current User, All Hosts | `{Documents}\PowerShell\profile.ps1` | `{Documents}\WindowsPowerShell\profile.ps1` |
| All Users, Current Host | `{ProgramData}\PowerShell\Microsoft.PowerShell_profile.ps1` | `{ProgramData}\WindowsPowerShell\Microsoft.PowerShell_profile.ps1` |
| All Users, All Hosts | `{ProgramData}\PowerShell\profile.ps1` | `{ProgramData}\WindowsPowerShell\profile.ps1` |

**Key Finding**: "Current Host" filename is `Microsoft.PowerShell_profile.ps1` for both `pwsh.exe` and `powershell.exe`. The directory differs, not the filename.

**Memory Management**: `CoTaskMemFree(pszPath)` required. Needs `ole32.lib`.

**Required Headers**: `<shlobj.h>` (not currently in pch.h)

---

## R3: Console Input for TUI

**Decision**: Use `ReadConsoleInputW` with raw mode (`SetConsoleMode`)

**Rationale**: TCDir already uses `SetConsoleMode` for VT output. Extending to raw input mode is natural. `ReadConsoleInputW` provides low-level keyboard events for arrow keys, space, enter, escape.

**Alternatives Considered**:
- `_getch()` / `_getwch()` — C runtime, doesn't handle extended keys cleanly
- Third-party TUI library — violates constitution (no external dependencies)

**Console Mode Setup** (for TUI input):
- Disable: `ENABLE_LINE_INPUT`, `ENABLE_ECHO_INPUT`, `ENABLE_PROCESSED_INPUT`
- Enable: `ENABLE_EXTENDED_FLAGS`, `ENABLE_WINDOW_INPUT`
- Save original mode → restore on exit (including Ctrl+C / Escape abort)

**Key Handling**: Virtual key codes (`VK_UP`, `VK_DOWN`, `VK_SPACE`, `VK_RETURN`, `VK_ESCAPE`). Ctrl+C detected via `KEY_EVENT` with control key state + `'C'`.

**Cursor**: Hide via `SetConsoleCursorInfo(bVisible=FALSE)` during TUI, restore on exit.

**Existing Infrastructure**: `CConsole` already has `GetConsoleMode`/`SetConsoleMode` and screen buffer info. TUI input can build on same handle management.

---

## R4: Executable PATH Resolution

**Decision**: Use `GetModuleFileNameW(nullptr, ...)` + `SearchPathW` for PATH check

**Rationale**: `GetModuleFileNameW` reliably returns the running exe's full path. `SearchPathW(nullptr, L"tcdir.exe", ...)` checks if the same name is findable via PATH without manual string parsing.

**Alternatives Considered**:
- Parsing `PATH` env var manually and comparing directories — fragile, case-sensitivity edge cases
- `where.exe tcdir.exe` via CreateProcess — process overhead, unnecessary

**Logic**:
1. `GetModuleFileNameW(nullptr, szExePath, MAX_PATH)` → full path of running tcdir.exe
2. `SearchPathW(nullptr, L"tcdir.exe", nullptr, MAX_PATH, szFound, nullptr)` → if found, compare resolved path
3. If `szFound` matches `szExePath`: tcdir is on PATH → generate `function d { tcdir @args }`
4. If not on PATH: generate `function d { & "C:\full\path\to\tcdir.exe" @args }`

---

## R5: Profile File I/O Patterns

**Decision**: Read entire file into memory, manipulate as `vector<wstring>` of lines, write back atomically

**Rationale**: Profile files are small (typically <1KB, rarely >10KB). In-memory manipulation is simpler and safer than streaming. Atomic write (write to temp → rename) prevents partial writes on crash.

**Marker Block Handling**:
- Opening: `# >>> tcdir aliases — DO NOT EDIT: this block is managed by tcdir and will be replaced <<<`
- Closing: `# >>> end tcdir aliases <<<`
- Scan for opening marker → find closing marker → extract or replace range
- For append: add block at end of file
- For replace: remove lines[open..close] inclusive, insert new block at same position

**Backup Strategy**: Copy original to `{filename}.bak` before any modification (FR-070).

**Encoding**: Read/write as UTF-8. Detect BOM on read (0xEF 0xBB 0xBF) and preserve it on write.

---

## Codebase Integration Notes

### Headers to Add to pch.h
```cpp
#include <tlhelp32.h>      // ToolHelp process snapshot (R1)
#include <shlobj.h>        // SHGetKnownFolderPath, FOLDERID_* (R2)
```

### Existing Infrastructure to Leverage
- `CConsole` — output buffering, ANSI escape sequences, console mode management
- `CCommandLine` — switch parsing framework (long switch table, validation)
- EHM macros — `CHR`, `CWRA`, `CBRAEx` for all HRESULT flows
