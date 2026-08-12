# Feature Specification: Nerd Font Install/Uninstall

**Feature Branch**: `010-nerd-fonts-install`
**Created**: 2026-06-21
**Status**: Shipped (TCDir 5.6.0)
**Input**: User description: "Add the ability to download and install the CaskaydiaCove Nerd Font system-wide, configure terminal profiles, and uninstall later."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Download and Install Nerd Font (Priority: P1)

A user runs `tcdir --Install-NerdFonts`. The tool resolves the latest Nerd Font release from GitHub, downloads the Cascadia Code Nerd Font zip, extracts it, and installs all font files to the system Fonts folder. If the user is not an administrator, the tool requests elevation via UAC to complete the system-wide installation.

**Why this priority**: Core value — without the font installed on the system, nothing else (icons, glyph display) works for users who don't already have a Nerd Font.

**Independent Test**: Run `tcdir --Install-NerdFonts` on a machine without any Nerd Font installed. Verify the tool downloads, extracts, and installs the fonts to `%WINDIR%\Fonts\`, registering them with both GDI and the system font registry.

**Acceptance Scenarios**:

1. **Given** no Nerd Font is installed, **When** the user runs `tcdir --Install-NerdFonts`, **Then** the tool resolves the latest release tag from GitHub, downloads the zip, extracts it, and installs the fonts system-wide
2. **Given** the user is not an administrator, **When** the tool needs to install fonts, **Then** it requests elevation via UAC and waits for the elevated child process to complete
3. **Given** fonts are already installed in the system Fonts folder, **When** the user runs `tcdir --Install-NerdFonts`, **Then** the tool detects this and skips the font installation step, reporting "already installed"
4. **Given** the GitHub API returns a rate limit response (403/429), **When** the user runs `tcdir --Install-NerdFonts`, **Then** the tool displays a rate limit message and exits gracefully
5. **Given** the user has no network connection, **When** the user runs `tcdir --Install-NerdFonts`, **Then** the tool displays a network error message and exits gracefully

---

### User Story 2 - Configure Terminal Profiles (Priority: P1)

After installing the fonts, the tool presents an interactive list of terminal profiles (Windows Terminal, PowerShell pwsh, Windows PowerShell, Command Prompt). The user selects which profiles to configure, and the tool sets each profile's font to the Nerd Font. Already-configured profiles are detected and shown as locked/pre-selected.

**Why this priority**: Installing the font is useless if the user's terminals don't actually use it. Profile configuration delivers the observable value.

**Independent Test**: Run `tcdir --Install-NerdFonts` (fonts already installed). Verify the interactive profile selection prompt appears, user can select/deselect profiles, and selected profiles get their font configuration updated.

**Acceptance Scenarios**:

1. **Given** fonts are installed and no terminal is configured, **When** the user runs `tcdir --Install-NerdFonts`, **Then** all four terminal targets appear in the selection prompt and the user can choose which to configure
2. **Given** Windows Terminal is already configured with the Nerd Font, **When** the user runs `tcdir --Install-NerdFonts`, **Then** Windows Terminal appears as locked (pre-selected, cannot be deselected) and is skipped during apply
3. **Given** the user selects Windows Terminal and configures it, **When** the settings.json is updated, **Then** `profiles.defaults.font.face` is set to `"CaskaydiaCove Nerd Font"` and ligatures (calt, liga) are disabled
4. **Given** the user selects a Console Host profile (pwsh, PowerShell, or cmd), **When** the profile is configured, **Then** the registry key under `HKCU\Console\<profile>` has `FaceName` set to `"CaskaydiaCove NF"`, `FontFamily` to 54, and `FontWeight` to 400
5. **Given** Windows Terminal settings.json does not exist, **When** the user selects Windows Terminal, **Then** the tool skips it with a message

---

### User Story 3 - Uninstall Nerd Font (Priority: P2)

A user runs `tcdir --Uninstall-NerdFonts`. The tool detects which terminal profiles are currently configured with the Nerd Font and prompts the user to select which to unconfigure. After unconfiguring profiles, the tool asks whether to remove the font files from the system Fonts folder. Font files are not removed if any configured profiles remain unselected.

**Why this priority**: Users should be able to fully reverse what `--Install-NerdFonts` did. This is lower priority than install since it's only needed when users want to undo.

**Independent Test**: Run `tcdir --Install-NerdFonts` to configure all profiles, then run `tcdir --Uninstall-NerdFonts` to unconfigure all profiles and remove fonts. Verify all profile configurations are restored and font files are removed.

**Acceptance Scenarios**:

1. **Given** Nerd Fonts are configured for some profiles, **When** the user runs `tcdir --Uninstall-NerdFonts`, **Then** the tool shows configured profiles and lets the user select which to unconfigure
2. **Given** profiles are unconfigured, **When** the tool prompts to remove font files, **Then** the user can confirm or decline; on confirmation, the font files are removed from the system Fonts folder
3. **Given** some profiles are still configured (user didn't select all), **When** the tool considers font file removal, **Then** font files are NOT removed and the user is informed
4. **Given** no profiles are configured and no font files are found, **When** the user runs `tcdir --Uninstall-NerdFonts`, **Then** the tool reports nothing to uninstall and exits
5. **Given** the user cancels the profile selection prompt, **When** the user presses cancel, **Then** the tool aborts cleanly without making changes

---

### User Story 4 - Validation and Command Restrictions (Priority: P2)

The install and uninstall flags are standalone operations — they cannot be combined with listing switches or file masks. Attempting to combine them produces an error.

**Why this priority**: Prevents confusion from ambiguous command-line invocations. The install/uninstall operations are full-screen interactive wizards, not directory listing modifiers.

**Independent Test**: Run `tcdir --Install-NerdFonts /Tree` and verify an error is shown.

**Acceptance Scenarios**:

1. **Given** the user runs `tcdir --Install-NerdFonts --Uninstall-NerdFonts`, **When** the tool validates arguments, **Then** it reports an error that install and uninstall are mutually exclusive
2. **Given** the user runs `tcdir --Install-NerdFonts /Tree`, **When** the tool validates arguments, **Then** it reports an error that the install flag cannot be combined with other switches
3. **Given** the user runs `tcdir --Install-NerdFonts *.txt`, **When** the tool validates arguments, **Then** it reports an error that the install flag cannot be combined with file masks

---

### Edge Cases

- **Font files are installed but manifest is missing**: The three-tier discovery (source dir, manifest, glob pattern) ensures uninstall can still find and remove the fonts
- **UAC prompt is declined**: The tool reports the elevation was denied and fails gracefully
- **Partial profile configuration failure**: The tool continues configuring other targets and reports per-target success/failure
- **PowerShell pwsh installed via WindowsApps**: The tool also configures the alternate WindowsApps console profile key for pwsh
- **settings.json has comments and custom formatting**: JSONC-aware editing preserves all user comments, key order, and formatting
- **Already installed + no profiles changed**: The tool reports "no changes were made" rather than implying work was done
- **User cancels mid-install (Ctrl+C)**: The tool aborts cleanly; any downloaded temp files are cleaned up on next run

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST provide an `--Install-NerdFonts` command that downloads, installs, and configures the CaskaydiaCove Nerd Font
- **FR-002**: System MUST provide an `--Uninstall-NerdFonts` command that reverses the font configuration and optionally removes font files
- **FR-003**: The `--Install-NerdFonts` and `--Uninstall-NerdFonts` flags MUST be mutually exclusive and cannot be combined with any other switch or file mask
- **FR-004**: System MUST resolve the latest Nerd Font release tag from the ryanoasis/nerd-fonts GitHub repository
- **FR-005**: System MUST download the `CascadiaCode.zip` package from the resolved release
- **FR-006**: System MUST extract the downloaded zip and install all `.ttf` font files to the system Fonts folder
- **FR-007**: System MUST register installed fonts with both GDI and the system font registry (for DirectWrite and Settings discovery)
- **FR-008**: System MUST request UAC elevation when the current process lacks administrator privileges needed for system-wide font installation
- **FR-009**: System MUST detect which terminal profiles are already configured with the Nerd Font before prompting
- **FR-010**: System MUST present an interactive profile selection prompt allowing the user to choose which terminals to configure/unconfigure
- **FR-011**: Already-configured profiles MUST appear as locked (pre-selected, cannot be deselected) during install prompts
- **FR-012**: System MUST configure Windows Terminal by setting `profiles.defaults.font.face` to `"CaskaydiaCove Nerd Font"` and disabling ligatures (calt=0, liga=0)
- **FR-013**: System MUST configure Console Host profiles (pwsh, PowerShell, cmd) by setting `FaceName` to `"CaskaydiaCove NF"`, `FontFamily` to 54, and `FontWeight` to 400 in the per-profile registry key
- **FR-014**: System MUST handle the alternate WindowsApps profile key for pwsh when present
- **FR-015**: System MUST edit Windows Terminal settings.json in a JSONC-aware manner, preserving comments, key order, and formatting
- **FR-016**: System MUST track installed font files in an uninstall manifest (registry-based) to enable reliable removal
- **FR-017**: System MUST use three-tier font file discovery during uninstall: source extract directory, manifest, and glob pattern in system Fonts folder
- **FR-018**: Font files MUST NOT be removed during uninstall if any configured terminal profile remains unselected
- **FR-019**: System MUST display a user-friendly message when GitHub API rate limiting (403/429) is encountered
- **FR-020**: System MUST report per-target configuration status (success/failure) after applying changes
- **FR-021**: System MUST remind the user to restart affected terminal windows after configuration changes
- **FR-022**: System MUST skip font installation if fonts are already present in the system Fonts folder
- **FR-023**: System MUST report "no changes were made" when fonts are already installed and no profile configuration changed
- **FR-024**: System MUST accept `--Install-NerdFonts` and `--Install-Nerd-Fonts` (with and without the hyphen between "Nerd" and "Fonts") as equivalent; same alias pair for uninstall
- **FR-025**: System MUST use the tool's own version in the HTTP user agent string (e.g., `TCDir/5.6.0`)

### Key Entities

- **Nerd Font Release**: A versioned release from the ryanoasis/nerd-fonts GitHub repository, containing the `CascadiaCode.zip` package with CaskaydiaCove Nerd Font `.ttf` files
- **Terminal Target**: A configurable terminal application (Windows Terminal, PowerShell pwsh, Windows PowerShell, Command Prompt) that can have its font face set to the Nerd Font
- **Target State**: The per-target configuration tracking whether a terminal is detected, already configured, selected by the user, and whether the apply succeeded
- **Uninstall Manifest**: A registry entry (`HKCU\Software\TCDir\NerdFonts\InstalledFontFiles`) tracking the font files installed during this session, enabling reliable removal later
- **Console Host Profile Key**: A registry path under `HKCU\Console\<path>` that controls the font settings for a specific Console Host window. The path is the executable's full path with backslashes replaced by underscores

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A user can go from "no Nerd Font installed" to "fully configured" by running a single command (`tcdir --Install-NerdFonts`) and following the prompts
- **SC-002**: Font installation completes within 2 minutes on a typical broadband connection (download + extract + register)
- **SC-003**: All four terminal targets (Windows Terminal, pwsh, PowerShell, cmd) can be configured independently
- **SC-004**: Uninstall fully reverses the install — profile configurations are restored and font files are removed
- **SC-005**: Re-running install after fonts are already present completes quickly (no re-download) and reports no changes needed
- **SC-006**: Windows Terminal settings.json edits preserve all user comments and formatting
- **SC-007**: The tool handles GitHub API rate limiting gracefully with a clear user message
- **SC-008**: Users can restart their terminals and see the Nerd Font icons immediately after installation and configuration

## Assumptions

- Windows is the target platform; font installation uses the Windows Fonts folder and Windows registry
- PowerShell is available on the system for zip extraction
- The user has internet access to reach the GitHub API and download the font package
- Windows Terminal, if installed, stores its settings at `%LOCALAPPDATA%\Packages\Microsoft.WindowsTerminal_8wekyb3d8bbwe\LocalState\settings.json`
- The CaskaydiaCove Nerd Font is distributed by ryanoasis/nerd-fonts on GitHub
- `ShellExecuteExW` with `runas` verb triggers UAC elevation
- The font's full name can be extracted from the TTF binary (name table, platform 3, encoding 1, name ID 4)
- Console Host profile registry keys may not exist yet; they are created on first configuration
- The tool uses two distinct font face names: `"CaskaydiaCove NF"` for GDI/Console and `"CaskaydiaCove Nerd Font"` for DirectWrite/Windows Terminal
