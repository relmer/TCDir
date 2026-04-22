# Feature Specification: Ellipsize Long Link Target Paths

**Feature Branch**: `008-ellipsize-targets`
**GitHub Issue**: [#11](https://github.com/relmer/TCDir/issues/11)
**Created**: 2026-04-19
**Status**: Shipped
**Input**: User description: "Middle-truncate long link target paths to prevent line wrapping"

## Clarifications

### Session 2026-04-19

- Q: How should the middle-truncation algorithm select the split point? → A: Priority order: (1) keep first two dirs + …\ + leaf dir + filename, (2) first two dirs + …\ + filename, (3) first dir + …\ + filename. Always keep as much as fits at the highest priority level.
- Q: What happens when even the minimum truncation form doesn't fit? → A: Show the leaf filename truncated with a trailing `…` to indicate cutoff (e.g., `→ DesktopStickerEdito…`)
- Q: What color should the ellipsis character use? → A: Default attribute, NOT the source file color — must be visually distinct so it's obvious it's not part of the actual path

## User Scenarios & Testing *(mandatory)*

### User Story 1 — Middle-Truncate Long Target Paths (Priority: P1)

A user runs `tcdir` in a directory containing AppExecLink entries or symlinks with long absolute target paths (e.g., `C:\Program Files\WindowsApps\PythonSoftwareFoundation.Python.3.12_3.12.1024.0_x64__qbz5n2kfra8p0\python3.12.exe`). Instead of the line wrapping past the terminal edge, the target path is middle-truncated with `…` so the entire line fits within the terminal width. The user sees the drive/root prefix and the leaf filename, which are the most actionable parts.

**Why this priority**: Core value proposition — preventing line wrap is the entire feature.

**Independent Test**: Run `tcdir` in `%LOCALAPPDATA%\Microsoft\WindowsApps` at a standard terminal width (120 chars). Verify that AppExecLink targets show `→ C:\Program Files\…\python3.12.exe` instead of wrapping.

**Acceptance Scenarios**:

1. **Given** an AppExecLink `python.exe` with target `C:\Program Files\WindowsApps\PythonSoftwareFoundation.Python.3.12_3.12.1024.0_x64__qbz5n2kfra8p0\python3.12.exe`, **When** the user runs `tcdir` at 120-char terminal width, **Then** the target is displayed as `C:\Program Files\…\python3.12.exe` (or similar middle-truncation that fits within the terminal width)
2. **Given** a junction with a short target path `C:\Dev\Projects`, **When** the user runs `tcdir`, **Then** the target is displayed in full — no truncation applied
3. **Given** a file symlink `config.yml → ..\shared\config.yml`, **When** the user runs `tcdir`, **Then** the relative target is displayed in full (relative paths are typically short)
4. **Given** a directory with no reparse points, **When** the user runs `tcdir`, **Then** output is identical to current behavior — no change

---

### User Story 2 — Ellipsize in Tree Mode (Priority: P2)

A user runs `tcdir --Tree` in a directory tree containing links with long target paths. The target path is middle-truncated just as in normal mode, accounting for the tree connector prefix width in the available-width calculation.

**Why this priority**: Tree mode has even less horizontal space due to indentation, making truncation more important.

**Independent Test**: Run `tcdir --Tree` in a tree containing a junction with a long target path. Verify the target is truncated and the line does not wrap.

**Acceptance Scenarios**:

1. **Given** a tree with a junction at depth 2 whose target is a long absolute path, **When** the user runs `tcdir --Tree`, **Then** the target is middle-truncated to fit within the remaining terminal width after tree connectors and metadata columns
2. **Given** a tree with a symlink whose target is short, **When** the user runs `tcdir --Tree`, **Then** the target is shown in full

---

### User Story 3 — Disable Truncation with --Ellipsize- (Priority: P3)

A user who needs to see full target paths can disable middle-truncation using the `--Ellipsize-` switch. This causes targets to display in full, wrapping as they did before this feature.

**Why this priority**: Opt-out mechanism — most users want truncation, but power users may need full paths for scripting or debugging.

**Independent Test**: Run `tcdir --Ellipsize-` in a directory with long targets. Verify full paths are shown and lines wrap as before.

**Acceptance Scenarios**:

1. **Given** a long target path that would normally be truncated, **When** the user runs `tcdir --Ellipsize-`, **Then** the full target path is displayed without truncation
2. **Given** `Ellipsize-` set in `.tcdirconfig`, **When** the user runs `tcdir`, **Then** truncation is disabled by default
3. **Given** `Ellipsize-` set in the `TCDIR` environment variable, **When** the user runs `tcdir`, **Then** truncation is disabled by default
4. **Given** `Ellipsize-` set in `.tcdirconfig`, **When** the user runs `tcdir --Ellipsize`, **Then** the CLI switch overrides the config and truncation is active

---

### Edge Cases

- **Target path is just a leaf filename** (relative symlink like `../config.yml`): Never truncated — already short
- **Target path has only two components** (e.g., `C:\file.exe`): Never truncated — nothing to elide from the middle
- **Terminal width is very narrow** (< 80): Truncation still works; in extreme cases, even the leaf filename may be the only thing shown after `…`
- **`…` would be longer than the truncated middle**: Don't truncate — show full path (truncation must actually save space)
- **Wide mode (`/W`) and bare mode (`/B`)**: No target paths displayed in these modes, so ellipsize is not applicable
- **Filename itself is very long**: Filename is never truncated; only the target path is subject to ellipsize. If even the leaf filename of the target doesn't fit, truncate it with a trailing `…` to indicate cutoff.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST middle-truncate link target paths using `…` (U+2026) when the full line (metadata + filename + ` → ` + target) would exceed the terminal width
- **FR-002**: Truncation MUST follow this priority order for what to preserve:
  1. First two directory components + `…\` + last directory component + leaf filename (e.g., `C:\Program Files\…\Notepad\Notepad.exe`)
  2. First two directory components + `…\` + leaf filename (e.g., `C:\Program Files\…\Notepad.exe`)
  3. First directory component + `…\` + leaf filename (e.g., `C:\…\Notepad.exe`)
  The algorithm MUST use the highest-priority form that fits within the available width.
- **FR-003**: System MUST NOT truncate target paths that fit within the terminal width. Paths with fewer than 3 components (e.g., `C:\file.exe`) are never truncated — there are no intermediate directories to elide.
- **FR-004**: System MUST NOT truncate the source filename — only the target path is subject to ellipsize
- **FR-005**: System MUST apply ellipsize in both normal mode and tree mode
- **FR-006**: In tree mode, the available width calculation MUST account for tree connector prefix width and indentation depth
- **FR-007**: System MUST provide an `--Ellipsize` switch (default: on) and `--Ellipsize-` to disable truncation
- **FR-008**: The `Ellipsize` switch MUST be configurable via `.tcdirconfig` and the `TCDIR` environment variable, following existing switch precedence (defaults < config file < env var < CLI)
- **FR-009**: System MUST document the `--Ellipsize` switch in `-?` help output and `--Settings` display
- **FR-010**: When truncation is disabled (`--Ellipsize-`), target paths MUST display in full, wrapping as before
- **FR-011**: The `…` ellipsis character MUST be rendered using the `Default` color attribute, NOT the source file's color — it must be visually distinct from the surrounding path text to signal that path components were elided

### Key Entities

- **Available Width**: The number of characters remaining on the current line after metadata columns, icon, filename, and ` → ` have been rendered. This is the maximum space the target path can occupy without wrapping.
- **Ellipsis Character**: `…` (U+2026, ELLIPSIS) — a single character used to replace elided path components.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: No line wrapping occurs for any link target in a default `tcdir` listing at standard terminal widths (120+ characters)
- **SC-002**: Users can identify the target's location (root/drive) and actual filename (leaf) from the truncated display
- **SC-003**: Short target paths (under available width) are never modified — zero information loss for typical cases
- **SC-004**: All existing unit tests continue to pass with no modifications
- **SC-005**: New unit tests cover truncation logic: long path truncation, short path passthrough, edge cases (two-component paths, very narrow terminals)
- **SC-006**: `--Ellipsize-` switch correctly disables truncation and shows full paths

## Assumptions

- The `…` (U+2026) character renders as a single character width in all target terminal environments — consistent with existing Unicode symbol usage in TCDir
- The metadata column widths (date, time, attributes, size, cloud status, icon) are deterministic per line and can be computed or measured to calculate available width
- Ellipsize is only relevant for normal and tree modes — wide and bare modes do not display link targets
- The feature builds on the existing `m_strReparseTarget` field and `→` display from spec 007

## Test Data (from `%LOCALAPPDATA%\Microsoft\WindowsApps`)

The following real filename/target pairs from the developer's machine MUST be used as unit test inputs to validate truncation behavior with realistic data:

| Source Filename | Target Path | Notes |
|----------------|-------------|-------|
| `MicrosoftWindows.DesktopStickerEditorCentennial.exe` | `C:\Windows\SystemApps\MicrosoftWindows.Client.CBS_cw5n1h2txyewy\DesktopStickerEditorWin32Exe\DesktopStickerEditorWin32Exe.exe` | Long filename + long target — most aggressive truncation |
| `wingetcreate.exe` | `C:\Program Files\WindowsApps\Microsoft.WindowsPackageManagerManifestCreator_1.12.8.0_x64__8wekyb3d8bbwe\WingetCreateCLI\WingetCreateCLI.exe` | Short filename + very long target |
| `GameBarElevatedFT_Alias.exe` | `C:\Program Files\WindowsApps\Microsoft.XboxGamingOverlay_7.326.4151.0_arm64__8wekyb3d8bbwe\GameBarElevatedFT.exe` | Medium filename + long target |
| `wt.exe` | `C:\Program Files\WindowsApps\Microsoft.WindowsTerminal_1.24.10921.0_arm64__8wekyb3d8bbwe\wt.exe` | Short filename + medium target |
| `notepad.exe` | `C:\Program Files\WindowsApps\Microsoft.WindowsNotepad_11.2601.26.0_arm64__8wekyb3d8bbwe\Notepad\Notepad.exe` | Short filename + medium target with subdirectory |
| `winget.exe` | `C:\Program Files\WindowsApps\Microsoft.DesktopAppInstaller_1.29.30.0_arm64__8wekyb3d8bbwe\winget.exe` | Short target — should NOT be truncated at 120-wide |
| `AzureVpn.exe` | `C:\Windows\system32\SystemUWPLauncher.exe` | Short target — should never be truncated |

## Release Checklist

The following items MUST be completed before this feature is considered shipped:

- Bump the minor version number (5.4 → 5.5) — done at branch creation
- Update `CHANGELOG.md` with the new version entry and feature description
- Update `README.md` "What's New" table with a v5.5 row
- Update `specs/sync-status.md` with spec 008 row (TCDir status, version, GH issue #11)
- Add `--Ellipsize` to Usage.cpp help output
- Add `Ellipsize` to `--Settings` display
- Associate the implementation branch/PR with GitHub issue #11 (use `Closes #11` in merge commit)
- Close GitHub issue #11 upon merge to master
