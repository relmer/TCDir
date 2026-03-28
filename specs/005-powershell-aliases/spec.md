# Feature Specification: PowerShell Alias Configuration

**Feature Branch**: `005-powershell-aliases`
**Created**: 2026-03-25
**Status**: Draft
**Input**: User description: "Interactive PowerShell alias configuration: tcdir --set-aliases, --get-aliases, --remove-aliases with TUI for configuring root alias, sub-aliases, and profile storage location"

## Clarifications

### Session 2026-03-25

- Q: What should happen when the user's profile already contains manually-written tcdir aliases (no marker comments)? → A: Ignore them entirely — only manage marker-delimited blocks
- Q: What should happen when the terminal does not support ANSI VT escape sequences? → A: Not applicable — VT support is a baseline requirement for tcdir; no fallback needed
- Q: What should happen when marker comments exist but content between them was manually altered? → A: Replace the entire marker-delimited block unconditionally; the timestamped `.bak` backup protects the user. Block delimiters are required (multi-line functions). Marker comments must explicitly warn that tcdir manages and replaces the block.
- Q: How should the generated root function resolve the tcdir executable? → A: At setup time, check the path of the running tcdir.exe instance. If that path is on PATH, generate functions using the short name `tcdir`. If not on PATH, bake the full path into the generated functions.
- Q: Should the tool support Windows PowerShell 5.1 profile paths in addition to PowerShell 7+? → A: Yes, fully support both. Auto-detect which PS version launched tcdir by inspecting the parent process image name (`pwsh.exe` = 7+, `powershell.exe` = 5.1). `PSModulePath` env var is NOT usable — PS 7+ includes both `\PowerShell\` and `\WindowsPowerShell\` paths. All three commands (`--set-aliases`, `--get-aliases`, `--remove-aliases`) scope to the detected PS version's profile paths only.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - First-Time Alias Setup (Priority: P1)

A user installs tcdir via winget and wants to set up short aliases so they can type `d` instead of `tcdir`, and `dd` for directory-only listings, `ds` for recursive search, etc. They run `tcdir --set-aliases` and are guided through an interactive menu to choose their root alias, select which sub-aliases to enable, pick where to save them, and optionally preview the changes before applying.

**Why this priority**: This is the core value proposition — making tcdir accessible through short, memorable commands. Without this, users must always type the full `tcdir` command.

**Independent Test**: Can be fully tested by running `tcdir --set-aliases` on a machine with no existing aliases. The user completes the wizard, a new alias block appears in their chosen profile, and after reloading the profile the aliases work.

**Acceptance Scenarios**:

1. **Given** tcdir is installed and no aliases exist, **When** the user runs `tcdir --set-aliases`, **Then** an interactive TUI launches showing step-by-step configuration screens.
2. **Given** the user is on the root alias step, **When** they accept the default `d`, **Then** the root alias is set to `d` and sub-aliases are derived from it (`dt`, `dw`, `dd`, `ds`, `dsb`).
3. **Given** the user is on the root alias step, **When** they type `tc` as a custom root, **Then** sub-aliases are recalculated (`tct`, `tcw`, `tcd`, `tcs`, `tcsb`).
4. **Given** the user is on the sub-aliases step, **When** they toggle off `dsb` and proceed, **Then** only the selected sub-aliases are included in the output.
5. **Given** the user chooses "Current User, All Hosts" as the storage location, **When** they confirm, **Then** the alias block is appended to that profile file with marker comments.
6. **Given** the user has `--whatif` specified, **When** the wizard completes, **Then** the output shows what would be written but no files are modified.
7. **Given** the profile file does not yet exist, **When** the user confirms writing, **Then** the file and its parent directory are created automatically.

---

### User Story 2 - View Current Aliases (Priority: P1)

A user wants to see what tcdir aliases are currently configured — both in their profile files and in the running session. They run `tcdir --get-aliases` and see a summary.

**Why this priority**: Users need visibility into their current alias state before making changes, and for troubleshooting. This also supports the remove flow (which needs to scan for existing aliases).

**Independent Test**: Can be tested by setting up aliases in a profile, running `tcdir --get-aliases`, and verifying the output shows the profile location, version, and actual function definitions.

**Acceptance Scenarios**:

1. **Given** aliases are stored in a profile file with tcdir marker comments, **When** the user runs `tcdir --get-aliases`, **Then** the output shows the profile variable name and path on one line, the generating version, and the actual `function xxx { ... }` lines from the alias block.
2. **Given** no tcdir aliases exist anywhere, **When** the user runs `tcdir --get-aliases`, **Then** the output says no tcdir aliases were found and suggests running `--set-aliases`.
3. **Given** aliases exist in multiple profile files, **When** the user runs `tcdir --get-aliases`, **Then** each profile location is shown with its function definitions, separated by 2 blank lines between profiles.

---

### User Story 3 - Update Existing Aliases (Priority: P2)

A user already has aliases configured with root `d` but now wants to change the root to `tc`, or wants to enable a sub-alias they previously skipped. They run `tcdir --set-aliases` and the system detects the existing configuration and offers to replace it.

**Why this priority**: Users' preferences change over time. Without update support, they'd need to manually edit their profile or remove-then-re-add.

**Independent Test**: Can be tested by setting aliases with root `d`, then running `--set-aliases` again choosing root `tc`, and verifying the old block is replaced with the new one.

**Acceptance Scenarios**:

1. **Given** tcdir aliases already exist in a profile with root `d`, **When** the user runs `tcdir --set-aliases`, **Then** the system detects the existing aliases and shows them, offering to update or replace.
2. **Given** the user chooses a different root alias than the existing one, **When** they confirm, **Then** the old alias block is removed and the new one is written in its place.
3. **Given** the user chooses the same root but toggles different sub-aliases, **When** they confirm, **Then** the alias block is updated with the new selection.

---

### User Story 4 - Remove Aliases (Priority: P2)

A user wants to remove tcdir aliases from their profile and/or current session. They run `tcdir --remove-aliases` and the system scans for existing aliases, shows where they were found, and lets the user choose which locations to clean up.

**Why this priority**: Clean uninstall support is important for user trust. Users should be able to fully undo what `--set-aliases` did.

**Independent Test**: Can be tested by having aliases in a profile file, running `--remove-aliases`, selecting the profile, and verifying the alias block is removed while the rest of the profile is untouched.

**Acceptance Scenarios**:

1. **Given** tcdir aliases exist in one or more profile files, **When** the user runs `tcdir --remove-aliases`, **Then** only profiles containing tcdir aliases are listed as checkbox options, defaulting to unchecked (opt-in removal).
2. **Given** the user selects one or more profile files for removal, **When** they confirm, **Then** the tcdir alias block (between marker comments) is removed from each selected profile and no other profile content is modified.
3. **Given** no tcdir aliases are found anywhere, **When** the user runs `tcdir --remove-aliases`, **Then** the output says no tcdir aliases were found.
4. **Given** aliases exist in a profile, **When** `--remove-aliases --whatif` is used, **Then** the tool shows what would be removed without making changes.

---

### User Story 5 - Dry Run with --whatif (Priority: P2)

A user wants to preview exactly what `--set-aliases` or `--remove-aliases` would do before committing. They append `--whatif` to the command and see a full preview of the changes.

**Why this priority**: Gives users confidence before modifying their profile files. Prevents accidental changes.

**Independent Test**: Can be tested by running `tcdir --set-aliases --whatif`, completing the wizard, and verifying no files were created or modified on disk while the console output shows the exact content that would be written.

**Acceptance Scenarios**:

1. **Given** the user runs `tcdir --set-aliases --whatif`, **When** the wizard completes, **Then** the console displays the profile path, the alias block content, and a message confirming no changes were made.
2. **Given** the user runs `tcdir --remove-aliases --whatif`, **When** they select a profile, **Then** the console shows which lines would be removed and confirms no changes were made.

---

### User Story 6 - Alias Conflict Detection (Priority: P3)

During setup, the system checks whether the chosen root alias or sub-aliases conflict with existing PowerShell commands, aliases, or functions. If a conflict is found, the user is warned and can choose to override or pick a different name.

**Why this priority**: Prevents silent breakage of the user's existing environment. Important for safety but less common than the happy path.

**Independent Test**: Can be tested by choosing a root alias that matches an existing PowerShell alias (e.g., `r` which aliases `Invoke-History`), and verifying the warning appears with the conflicting command's identity.

**Acceptance Scenarios**:

1. **Given** the user chooses root alias `r`, **When** `r` is already a built-in PowerShell alias, **Then** the system displays a warning showing what `r` is currently bound to and asks the user to confirm override or choose a different name.
2. **Given** the user confirms the override, **When** the aliases are written, **Then** the alias block includes the overriding definition.
3. **Given** no conflicts exist, **When** the user proceeds through setup, **Then** no warnings are shown.

---

### Edge Cases

- What happens when the profile file exists but is read-only or locked by another process? → **Resolved**: Handled by FR-073 — display a clear error message and exit without partial writes.
- What happens when the profile file uses an encoding other than UTF-8 (e.g., UTF-16, BOM)? → **Resolved**: Check first bytes for BOM. `FF FE` or `FE FF` (UTF-16) → bail with clear error directing user to convert to UTF-8. `EF BB BF` (UTF-8 BOM) → read as UTF-8, preserve BOM on write. No BOM → read as UTF-8 (handles both UTF-8 and ASCII).
- What happens when the user's profile already contains tcdir-related code that was manually written (not via `--set-aliases`, so no marker comments)? → **Resolved**: The tool ignores them entirely. Only marker-delimited blocks (per FR-040) are managed. Manual aliases are invisible to the tool.
- What happens when the user's terminal does not support ANSI escape sequences (very old conhost without VT mode)? → **Resolved**: Not applicable. VT support is a baseline requirement for tcdir; the alias wizard inherits this requirement.
- What happens when the marker comments are present but the content between them has been manually altered? → **Resolved**: The entire marker-delimited block is replaced unconditionally. The timestamped `.bak` backup (FR-070) preserves the previous state.
- What happens if the profile directory path contains spaces or special characters? → **Resolved**: Paths with spaces are handled by proper quoting in generated PowerShell code (`& "full path"`). Windows NTFS path characters are inherently limited; no special handling needed beyond quoting.
- What happens when the user presses Ctrl+C or closes the terminal mid-wizard? → **Resolved**: Console mode and cursor visibility are restored via RAII guard. No files are modified until the final write step, so mid-wizard abort leaves no partial state.
- What happens when `--set-aliases` and `--remove-aliases` are both specified? → **Resolved**: Handled by FR-005 — mutual exclusivity validation produces an error.
- What happens when `--whatif` is used without `--set-aliases` or `--remove-aliases`? → **Resolved**: Display an error stating `--whatif` is only valid with `--set-aliases` or `--remove-aliases`.

## Requirements *(mandatory)*

### Functional Requirements

#### Command-Line Interface

- **FR-001**: tcdir MUST accept `--set-aliases` to launch the interactive alias configuration wizard
- **FR-002**: tcdir MUST accept `--get-aliases` to display all currently configured tcdir aliases and their source locations
- **FR-003**: tcdir MUST accept `--remove-aliases` to launch the interactive alias removal wizard
- **FR-004**: tcdir MUST accept `--whatif` as a modifier to `--set-aliases` and `--remove-aliases` to preview changes without applying them
- **FR-005**: `--set-aliases`, `--get-aliases`, and `--remove-aliases` MUST be mutually exclusive with each other and with normal directory listing operations

#### Interactive TUI — General

- **FR-010**: The TUI MUST use `❯` as the focus indicator for the currently highlighted option
- **FR-011**: The TUI MUST use `(●)` / `( )` for radio button (single-select) widgets
- **FR-012**: The TUI MUST use `[✓]` / `[ ]` for checkbox (multi-select) widgets
- **FR-013**: The TUI MUST respond to arrow keys (↑/↓) for navigation, Space for toggling, Enter for confirming, and Escape for cancelling
- **FR-014**: The TUI MUST hide the console cursor during interactive menu display and restore it on exit
- **FR-015**: Pressing Escape at any step MUST cancel the entire operation without modifying any files, displaying a cancellation message
- **FR-016**: The TUI MUST use ANSI escape sequences for cursor movement and line updates

#### Set Aliases Flow

- **FR-020**: The set-aliases wizard MUST present a text input for the root alias with a default value of `d`
- **FR-021**: The root alias input MUST accept any string of 1-4 alphanumeric characters
- **FR-022**: After the root alias is chosen, the wizard MUST present a checkbox list of sub-aliases, each derived by appending a suffix to the root alias
- **FR-023**: The predefined sub-alias suffixes MUST be: `t` (tree view, maps to `--tree`), `d` (directories only, maps to `-a:d`), `s` (recursive, maps to `-s`), `sb` (recursive bare, maps to `-s -b`), `w` (wide format, maps to `-w`)
- **FR-024**: All sub-aliases MUST default to selected (checked)
- **FR-025**: The wizard MUST present a radio button list of PowerShell profile storage locations, showing both the profile variable name and the resolved file path
- **FR-026**: The default storage location MUST be "Current User, All Hosts" (`$PROFILE.CurrentUserAllHosts`)
- **FR-027**: The storage location list MUST include an option for "Current session only (not persisted)"
- **FR-028**: Profile locations requiring administrator privileges MUST be visually marked with "(requires admin)"
- **FR-029**: Before writing, the wizard MUST display a preview of the complete alias block and target file path. In normal mode, explicit confirmation is required. In `--whatif` mode, the preview is shown directly without a confirmation prompt.
- **FR-030**: If existing tcdir aliases are detected in any profile (via marker comments), the wizard MUST inform the user and offer to replace them

#### Alias Block Format

- **FR-040**: The persisted alias block MUST be wrapped in an 80-character-wide `#` banner. The header banner includes the version, a DO NOT EDIT warning, and usage instructions. The footer banner contains `# End TCDir Aliases`. The scanner identifies the block by looking for `#  TCDir Aliases` in the header and `# End TCDir Aliases` in the footer. See alias-block format example in quickstart.md.
- **FR-041**: At setup time, the tool MUST determine the path of the currently running tcdir.exe. If that path is reachable via PATH, the root alias function MUST invoke `tcdir` by short name. If not on PATH, the root alias function MUST invoke the full resolved path to tcdir.exe.
- **FR-042**: The root alias MUST be defined as a simple passthrough PowerShell function that invokes tcdir (by name or full path per FR-041) and passes all arguments via `@args`. No fallback behavior (e.g., `Get-ChildItem`) or post-invocation output (e.g., `Write-Host`)
- **FR-043**: Each sub-alias MUST be defined as a PowerShell function that invokes the root function with the appropriate flags prepended
- **FR-044**: The alias block header MUST include the tcdir version that generated it (e.g., `tcdir v5.2.1150`). The minor version bumps to 5.2 for this feature.

#### Remove Aliases Flow

- **FR-050**: The remove wizard MUST scan all PowerShell profile file paths for the detected PS version for tcdir marker comments before presenting options
- **FR-051**: The remove wizard MUST only list profile locations that contain tcdir aliases
- **FR-052**: Under each profile location, the remove wizard MUST display the specific alias names found
- **FR-053**: If no tcdir aliases are found in any location, the tool MUST display a message stating so and exit
- **FR-054**: After removal, the marker comments and all content between them MUST be deleted from the profile file
- **FR-055**: The rest of the profile file content MUST remain completely unchanged after removal

#### Get Aliases (Non-Interactive)

- **FR-060**: `--get-aliases` MUST scan all PowerShell profile paths for the detected PS version for tcdir alias blocks
- **FR-061**: For each location containing aliases, the output MUST show the profile variable name and resolved path on one line in the format `$PROFILE.Xxx  (C:\path\to\file.ps1)` (matching the display format used in `--set-aliases` and `--remove-aliases` prompts), followed by a "Generated by tcdir vX.Y.Z" line, then the actual `function xxx { ... }` lines from the alias block (indented 4 spaces). Output spacing: 1 blank line after the command, 1 blank line between header and function lines, 1 blank line after the last function line, 2 blank lines between profile dumps when multiple profiles have aliases
- **FR-062**: `--get-aliases` MUST NOT require interactive input — it runs and exits

#### File Operations Safety

- **FR-070**: Before modifying any profile file, the tool MUST create a timestamped backup copy named `{filename}.{YYYY-MM-DD-HH-MM-SS}.bak`
- **FR-071**: The tool MUST check the first bytes of a profile file for a BOM. If a UTF-16 BOM is detected (`FF FE` or `FE FF`), the tool MUST display an error and refuse to modify the file. If a UTF-8 BOM (`EF BB BF`) is present, it MUST be preserved on write-back. If no BOM is present, the file MUST be read and written as UTF-8.
- **FR-072**: If the target profile file does not exist, the tool MUST create it (and any missing parent directories)
- **FR-073**: If the target profile file cannot be written (permissions, locked), the tool MUST display a clear error message and exit without partial writes
- **FR-074**: Conflict detection MUST scan for existing PowerShell aliases, functions, or commands that match the chosen alias names, and warn the user with the identity of the conflicting command

#### Profile Path Resolution

- **FR-080**: The tool MUST resolve all PowerShell profile paths without spawning a PowerShell child process. For per-user (CurrentUser) paths, use `SHGetKnownFolderPath(FOLDERID_Documents)` correctly handling folder redirection (e.g., OneDrive). For all-users (AllUsers) paths, use `$PSHOME` — the parent PowerShell process's installation directory (e.g., `C:\Program Files\PowerShell\7\`), determined by extracting the directory from the parent process image path.
- **FR-081**: The tool MUST resolve profile paths for both PowerShell 7+ (`Documents\PowerShell\`) and Windows PowerShell 5.1 (`Documents\WindowsPowerShell\`). For each version, the four profile paths are: Current User Current Host, Current User All Hosts, All Users Current Host, All Users All Hosts
- **FR-082**: The tool MUST auto-detect the calling PowerShell version by inspecting the parent process image name (`pwsh.exe` → 7+, `powershell.exe` → 5.1). If the parent is neither (e.g., CMD, Explorer), display an error directing the user to run from PowerShell. All alias commands MUST scope profile paths to the detected version only. The "Current Host" profile filename is `Microsoft.PowerShell_profile.ps1` for both `pwsh.exe` and `powershell.exe`

### Key Entities

- **Root Alias**: The primary short command (e.g., `d`, `tc`) that maps to invoking tcdir with passthrough arguments
- **Sub-Alias**: A derived command that prepends specific tcdir flags before the user's arguments (e.g., `dd` → `d -a:d`)
- **Alias Block**: The delimited section of PowerShell code written to a profile file, bounded by marker comments, containing all alias function definitions
- **Profile Location**: One of the four standard PowerShell profile file paths, identified by scope (user/machine) and host (current/all)

## Assumptions

- Users invoke `--set-aliases` from a PowerShell prompt (not CMD). If invoked from CMD, the tool displays a message directing the user to run from PowerShell.
- The tool targets both PowerShell 7+ (`pwsh`) and Windows PowerShell 5.1 (`powershell`). Profile directories differ (`PowerShell` vs `WindowsPowerShell`) but the generated alias code is compatible with both. The calling PS version is auto-detected via parent process inspection (`PSModulePath` is not usable — PS 7+ includes both version paths). Path resolution uses Windows APIs to handle Documents folder redirection (OneDrive, etc.).
- The winget portable install places `tcdir.exe` in a directory already on `PATH` via the winget links mechanism, so the alias function can reference `tcdir` by name rather than full path.
- ANSI VT escape sequences are supported in the user's terminal (Windows Terminal, VS Code terminal, modern conhost). If VT mode cannot be enabled, the TUI falls back gracefully or errors with a clear message.
- Profile files use UTF-8 encoding (with or without BOM). This is the default for PowerShell 7+.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A new user can go from `winget install relmer.TCDir` to having working aliases in under 2 minutes
- **SC-002**: 100% of alias content written by `--set-aliases` is cleanly removable by `--remove-aliases` with zero residual changes to the profile
- **SC-003**: `--whatif` accurately represents every change that the real operation would make — no differences between preview and execution
- **SC-004**: Users can complete the full set-aliases wizard without consulting documentation — the TUI is self-explanatory
- **SC-005**: Alias configuration does not break any existing content in the user's profile file
- **SC-006**: `--get-aliases` output gives users full visibility into their current alias state within 1 second
