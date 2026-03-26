# Data Model: PowerShell Alias Configuration

**Feature**: 005-powershell-aliases | **Date**: 2026-03-25

## Entities

### EPowerShellVersion (enum)

Detected PowerShell version of the calling shell.

| Value | Description |
|-------|-------------|
| `PS7Plus` | PowerShell 7+ (`pwsh.exe`). Profile dir: `PowerShell\` |
| `PS51` | Windows PowerShell 5.1 (`powershell.exe`). Profile dir: `WindowsPowerShell\` |
| `Unknown` | Parent process is neither pwsh.exe nor powershell.exe |

---

### EProfileScope (enum)

One of the four standard PowerShell profile scopes.

| Value | Variable Name | Filename |
|-------|--------------|----------|
| `CurrentUserCurrentHost` | `$PROFILE.CurrentUserCurrentHost` | `Microsoft.PowerShell_profile.ps1` |
| `CurrentUserAllHosts` | `$PROFILE.CurrentUserAllHosts` | `profile.ps1` |
| `AllUsersCurrentHost` | `$PROFILE.AllUsersCurrentHost` | `Microsoft.PowerShell_profile.ps1` |
| `AllUsersAllHosts` | `$PROFILE.AllUsersAllHosts` | `profile.ps1` |

**Constraints**: AllUsers scopes require administrator privileges for write operations.

---

### SProfileLocation (struct)

A resolved profile file path with metadata.

| Field | Type | Description |
|-------|------|-------------|
| `eScope` | `EProfileScope` | Which of the four scopes |
| `strVariableName` | `wstring` | Display name (e.g., `$PROFILE.CurrentUserAllHosts`) |
| `strResolvedPath` | `wstring` | Full filesystem path |
| `fExists` | `bool` | Whether the file currently exists on disk |
| `fRequiresAdmin` | `bool` | Whether writing requires admin privileges |
| `fHasAliasBlock` | `bool` | Whether tcdir marker block was found in the file |

**Derived from**: `EPowerShellVersion` + `SHGetKnownFolderPath(FOLDERID_Documents | FOLDERID_ProgramData)` + `EProfileScope`

---

### SAliasDefinition (struct)

A single alias (root or sub) to be generated.

| Field | Type | Description |
|-------|------|-------------|
| `strName` | `wstring` | Alias function name (e.g., `d`, `dd`, `ds`) |
| `strFlags` | `wstring` | tcdir flags to prepend (empty for root; e.g., `/a:d` for dirs-only) |
| `fEnabled` | `bool` | Whether selected by user in the checkbox step |

**Validation Rules**:
- Root alias: 1-4 alphanumeric characters (FR-021)
- Sub-alias name: root + suffix (suffix from fixed set: `d`, `s`, `sb`, `w`)

---

### SAliasConfig (struct)

The complete user configuration from the TUI wizard.

| Field | Type | Description |
|-------|------|-------------|
| `strRootAlias` | `wstring` | Root alias name chosen by user (default: `d`) |
| `strTcDirInvocation` | `wstring` | How to invoke tcdir (`tcdir` or full path) |
| `rgSubAliases` | `vector<SAliasDefinition>` | Sub-aliases with enabled/disabled state |
| `eTargetScope` | `EProfileScope` | Chosen profile location (or session-only sentinel) |
| `strTargetPath` | `wstring` | Resolved path for the chosen profile |
| `fSessionOnly` | `bool` | True if "Current session only" was chosen |
| `fWhatIf` | `bool` | Dry-run mode — preview only, no file writes |

---

### SAliasBlock (struct)

A parsed alias block found in an existing profile file.

| Field | Type | Description |
|-------|------|-------------|
| `iStartLine` | `size_t` | 0-based line index of the opening marker |
| `iEndLine` | `size_t` | 0-based line index of the closing marker |
| `strRootAlias` | `wstring` | Detected root alias name (parsed from block content) |
| `rgAliasNames` | `vector<wstring>` | All function names found in the block |
| `strVersion` | `wstring` | tcdir version that generated the block |

---

## State Transitions

### TUI Wizard Steps (--set-aliases)

```
[Start] → Step1_RootAlias → Step2_SubAliases → Step3_ProfileLocation → Step4_Preview → [Write/WhatIf]
                                                                                          ↓
[Escape at any step] ──────────────────────────────────────────────────────────→ [Cancelled]
```

| State | Input Widget | Output |
|-------|-------------|--------|
| Step1_RootAlias | Text input (default: `d`) | `strRootAlias` → recalculate sub-alias names |
| Step2_SubAliases | Checkbox list | `rgSubAliases` with enabled states |
| Step3_ProfileLocation | Radio button list | `eTargetScope` + `strTargetPath` |
| Step4_Preview | Confirmation (Y/N) | Proceed to write or cancel |

### Remove Wizard Steps (--remove-aliases)

```
[Start] → Scan profiles → [No aliases found → exit] 
                         → [Aliases found] → Step1_SelectProfile → Step2_Confirm → [Remove/WhatIf]
                                                                                      ↓
[Escape] ──────────────────────────────────────────────────────────────────→ [Cancelled]
```

---

## Relationships

```
EPowerShellVersion ──determines──→ SProfileLocation (4 per version)
                                        │
SAliasConfig ──references──→ SProfileLocation (chosen target)
    │
    ├── contains ──→ SAliasDefinition[] (root + sub-aliases)
    │
    └── produces ──→ Alias Block text (via AliasBlockGenerator)

SAliasBlock ──parsed from──→ existing profile file content
```
