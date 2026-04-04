# Feature Specification: Config File Support

**Feature Branch**: `006-config-file-support`  
**Created**: 2026-03-29  
**Status**: Draft  
**Input**: User description: "Using the environment variable to configure more than a few items is cumbersome. A config file would make that much easier." (GitHub issue #9)

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Basic Config File Loading (Priority: P1)

A user creates a config file in a well-known location and places their preferred settings in it. When they run tcdir, the tool reads the config file and applies those settings — switches, color overrides, icon overrides, and parameterized values — exactly as if they had been specified in the TCDIR environment variable, but in a much more readable, multi-line format.

**Why this priority**: This is the core value of the feature. Without config file loading, nothing else matters. A single config file that replaces the env var satisfies the primary pain point.

**Independent Test**: Can be fully tested by creating a config file with several settings (e.g., switches, color overrides for extensions) and verifying tcdir applies them correctly.

**Acceptance Scenarios**:

1. **Given** a config file exists at the default location with color and switch settings, **When** the user runs tcdir, **Then** all settings from the config file are applied to the output.
2. **Given** no config file exists and the TCDIR environment variable is not set, **When** the user runs tcdir, **Then** built-in defaults are used without errors or warnings.
3. **Given** a config file exists at the default location, **When** the user runs tcdir, **Then** the config file is loaded without noticeable delay to the user.

---

### User Story 2 - Environment Variable Overrides Config File (Priority: P1)

A user has a config file with their baseline preferences. For a specific terminal session, they set the TCDIR environment variable with a few overrides. The environment variable settings take precedence over the config file, allowing session-specific customization on top of persistent defaults.

**Why this priority**: The env var is the existing mechanism and must remain functional. Users need predictable layering so they can keep a config file for defaults while still doing quick per-session tweaks via the env var.

**Independent Test**: Can be tested by setting a color for `.cpp` in the config file, then setting a different color for `.cpp` in the TCDIR env var, and verifying the env var value wins.

**Acceptance Scenarios**:

1. **Given** the config file sets `.cpp=LightGreen` and the env var sets `.cpp=Yellow`, **When** the user runs tcdir, **Then** `.cpp` files display in Yellow (env var wins).
2. **Given** the config file sets `w` (wide listing) and the env var sets `w-` (disable wide), **When** the user runs tcdir, **Then** wide listing is disabled (env var wins).
3. **Given** the config file sets several values and the env var sets one additional value not in the config file, **When** the user runs tcdir, **Then** both the config file values and the env var value are applied (merged).

---

### User Story 3 - Readable Multi-Line Format (Priority: P1)

A user opens the config file in a text editor and can easily understand and modify their settings. The file supports comments, blank lines for grouping, and each setting on its own line, making complex configurations maintainable.

**Why this priority**: Readability is the primary motivation for the feature. If the config file format is just as cryptic as the semicolon-delimited env var, the feature fails its purpose.

**Independent Test**: Can be tested by creating a config file with comments, blank lines, and settings on separate lines, and verifying tcdir parses it correctly, ignoring comments and blanks.

**Acceptance Scenarios**:

1. **Given** a config file with comment lines (starting with `#`), **When** the user runs tcdir, **Then** comment lines are ignored during parsing.
2. **Given** a config file with blank lines separating groups of settings, **When** the user runs tcdir, **Then** blank lines are ignored and all settings are applied.
3. **Given** a config file with inline comments after a setting, **When** the user runs tcdir, **Then** the setting is applied and the inline comment is ignored.

---

### User Story 4 - Config File Error Reporting (Priority: P2)

A user makes a typo or invalid entry in their config file. When they run tcdir, the tool reports the error clearly — indicating the file path, line number, and what went wrong — so the user can quickly fix the issue. Errors are shown on every run (same behavior as env var errors), not suppressed after the first display.

**Why this priority**: Good error messages are critical for usability, but the feature is still functional without them (settings would just not apply). This builds on the P1 stories.

**Independent Test**: Can be tested by placing an invalid color name in the config file and verifying tcdir shows an actionable error message with line number.

**Acceptance Scenarios**:

1. **Given** a config file contains an invalid color name on line 5, **When** the user runs tcdir, **Then** an error message is displayed referencing the config file path and line 5, with the invalid text indicated.
2. **Given** a config file contains a malformed entry (e.g., missing `=`), **When** the user runs tcdir, **Then** an error message describes the formatting issue and the problematic line.
3. **Given** a config file has one invalid line and several valid lines, **When** the user runs tcdir, **Then** valid settings are still applied and only the invalid line is reported as an error.
4. **Given** both the config file and the TCDIR env var contain errors, **When** the user runs tcdir, **Then** errors are displayed in two separate groups — config file errors first, then env var errors — each with its own header.
5. **Given** the config file cannot be opened (permissions, I/O error), **When** the user runs tcdir, **Then** a single error line is displayed describing the file-level issue (e.g., "Cannot open ~/.tcdirconfig: Access denied"), the entire file is skipped, and the env var and defaults still apply.

---

### User Story 5 - Diagnostic Command Restructuring (Priority: P2)

With two configuration sources (config file and env var), the existing diagnostic commands need restructuring for clarity. The `/config` command is repurposed as the config file equivalent of `/env` — showing config file syntax reference, file location/status, decoded settings from the file, and config file errors. A new `/settings` command takes over the merged configuration view that `/config` provides today, with the source column extended to show three sources (Default, Config file, Environment).

**Why this priority**: Users need clear diagnostic tools to understand and troubleshoot their configuration. Without this restructuring, there's no way to inspect what the config file contributed vs. the env var.

**Independent Test**: Can be tested by setting up both a config file and env var, running each of the three commands (`/env`, `/config`, `/settings`), and verifying each shows the expected scope of information.

**Acceptance Scenarios**:

1. **Given** a config file exists with settings, **When** the user runs `/config`, **Then** the output shows config file syntax reference, the file path, decoded settings from the file, and any config file parse errors.
2. **Given** no config file exists, **When** the user runs `/config`, **Then** the output shows the config file syntax reference and indicates no config file was found at the expected location.
3. **Given** both a config file and env var are set, **When** the user runs `/settings`, **Then** the merged configuration tables show each setting's source as Default, Config file, or Environment.
4. **Given** the user runs `/env`, **When** the env var is set, **Then** the output is unchanged from today — env var syntax help, current value decoded, and env var errors.

---
### Edge Cases

- What happens when the config file exists but is empty? (No settings applied, no error.)
- What happens when the config file has only comments and blank lines? (No settings applied, no error.)
- What happens when the config file is not readable due to permissions? (Single error line shown with the I/O error, entire config file skipped, built-in defaults and env var still used.)
- What happens when the config file contains a BOM? (Handled gracefully — BOM is skipped.)
- What happens when the config file contains very long lines or very large content? (Reasonable limits; degrade gracefully.)
- What happens when the same setting appears multiple times in the config file? (Last occurrence wins, consistent with standard config file behavior.)

## Config File Format *(mandatory)*

The config file uses a **flat, line-oriented plain-text format** with the same entry syntax as the existing TCDIR environment variable. Each line contains one setting entry. Comments and blank lines are supported for readability and organization.

**Format choice rationale**: Evaluated JSON, YAML, TOML, INI, and flat formats. JSON was rejected (no comments). YAML was rejected (whitespace-sensitive, complex parser, surprising type coercions). TOML was rejected (requires parser library/dependency). INI was rejected (forces users to know which section each setting belongs in). The flat format was chosen because it requires no new dependencies, reuses the existing env var syntax identically, and delivers the primary value — multi-line readability with comments — with minimal parser complexity.

### Example Config File

```
# Switches
w
tree
icons

# Extension colors
.cpp = LightGreen
.h   = Yellow on Blue
.rs  = LightCyan

# Display attribute colors
D = LightBlue

# File attribute overrides
attr:h = DarkGrey

# Icon overrides
.cpp = LightGreen, e795
.py  = LightGreen, e73c

# Parameterized settings
Depth = 3
TreeIndent = 4
Size = Auto
```

### Format Rules

- One setting entry per line
- Each entry uses the same syntax as a single TCDIR environment variable entry (what would be between semicolons)
- Lines beginning with `#` are comments (ignored)
- Inline comments: text after `#` on a setting line is ignored
- Blank lines and whitespace-only lines are ignored
- Leading and trailing whitespace on each line is trimmed before parsing
- Users are encouraged to use comments as section headers for organization, but no structural sections are enforced

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST load settings from a config file at a well-known default location in the user's home directory.
- **FR-002**: System MUST support all settings currently available via the TCDIR environment variable: switches, color overrides (extension, display attribute, file attribute), icon overrides, and parameterized values (Depth, TreeIndent, Size).
- **FR-003**: System MUST support a flat, line-oriented config file format where each line contains one setting entry using the same syntax as the TCDIR environment variable.
- **FR-004**: System MUST support comment lines beginning with `#` and inline comments (text after `#` on a setting line).
- **FR-005**: System MUST ignore blank lines and whitespace-only lines.
- **FR-006**: System MUST trim leading and trailing whitespace from each line before parsing.
- **FR-007**: System MUST apply the TCDIR environment variable settings after config file settings, so that the env var overrides the config file for any conflicting keys.
- **FR-008**: System MUST report config file parse errors with the file path, line number, and a description of the issue.
- **FR-009**: System MUST continue applying valid settings from the config file even when some lines contain errors.
- **FR-010**: System MUST display config file errors on every run (same behavior as env var errors), not suppress them after the first display.
- **FR-011**: System MUST display config file errors and env var errors in separate groups, each with its own header (e.g., "Config file errors:" and "Environment variable errors:"). Config file errors are shown first.
- **FR-012**: System MUST include the line number in each config file error message.
- **FR-013**: System MUST display file-level errors (cannot open, permission denied, encoding conversion failure) as a single error line identifying the file and the issue. The entire config file is skipped in this case.
- **FR-014**: System MUST silently skip config file loading when no config file exists (no error, no warning).
- **FR-015**: System MUST handle config files with a UTF-8 BOM by skipping the BOM before parsing.
- **FR-016**: System MUST treat duplicate settings within the config file using last-occurrence-wins semantics.

#### Diagnostic Commands

- **FR-017**: The `/config` command MUST be repurposed as the config file diagnostic command (parallel to `/env` for the environment variable). It MUST show: config file syntax reference, the resolved config file path, whether the file was found/loaded, decoded settings from the config file grouped by type (switches, display items, file attributes, extensions, icons), and any config file parse errors.
- **FR-018**: A new `/settings` command MUST be introduced to display the merged configuration tables — the output that `/config` produces today — showing all effective settings with their sources.
- **FR-019**: The `/settings` source column MUST distinguish three sources: Default, Config file, and Environment.
- **FR-020**: The `/settings` command MUST show icon status and any errors from both config file and env var.
- **FR-021**: The `/env` command MUST remain unchanged — showing env var syntax help, current value decoded, and env var errors.
- **FR-022**: The `/help` output MUST be updated to reflect the new command names and descriptions.

### Key Entities

- **Config File**: A flat plain-text file containing one setting per line with optional `#` comments; located at `%USERPROFILE%\.tcdirconfig`. Uses the same entry syntax as the TCDIR environment variable. No structural sections — organization via comment headers is optional and user-driven.
- **Setting Entry**: A single configuration directive (switch toggle, color override, icon override, or parameterized value) occupying one line in the config file. Identical syntax to a single semicolon-delimited segment from the TCDIR env var.
- **Configuration Layer**: The precedence model — built-in defaults < config file < TCDIR environment variable < command-line arguments.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Users can configure all settings currently supported by the TCDIR environment variable through the config file.
- **SC-002**: Users can set up a config file with 20+ settings and have all of them applied correctly.
- **SC-003**: Configuration errors are reported with sufficient detail (file, line number, issue description) that users can fix them on the first attempt.
- **SC-004**: tcdir startup time with a config file containing 50 settings is indistinguishable from startup without a config file (no perceptible delay).
- **SC-005**: Existing users who rely solely on the TCDIR environment variable experience no change in behavior.
- **SC-006**: Users can transition from env var configuration to config file configuration without learning new syntax for individual settings.

## Assumptions

- The config file uses the same key=value syntax for individual entries as the current TCDIR environment variable, just one entry per line instead of semicolon-delimited.
- The default config file is `%USERPROFILE%\.tcdirconfig` — a single dotfile (not a directory), following the convention of `.gitconfig`, `.wslconfig`, and `.editorconfig`.
- The config file is encoded as UTF-8 (with or without BOM).
- Config file parsing happens once at startup; there is no file-watching or live-reload mechanism.
- Command-line arguments continue to have the highest precedence, overriding both config file and env var settings.
