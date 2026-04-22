# Feature Specification: Symlink & Junction Target Display

**Feature Branch**: `007-symlink-junction-targets`
**GitHub Issue**: [#6](https://github.com/relmer/TCDir/issues/6)
**Created**: 2026-04-18
**Status**: Shipped
**Input**: User description: "Display symlink and junction targets in directory listings"

## Clarifications

### Session 2026-04-18

- Q: What exact spacing format between filename and arrow/target? → A: `filename → target` (single space each side of arrow)
- Q: How should non-junction/non-symlink reparse tags be handled? → A: Support AppExecLink (show resolved target exe path); silently ignore all other reparse types (WCI, DEDUP, AF_UNIX, LX_SYMLINK, LX_FIFO, CLOUD already handled)

## User Scenarios & Testing *(mandatory)*

### User Story 1 — Normal Mode Target Display (Priority: P1)

A user runs `tcdir` in a directory containing junctions and symlinks. After each junction or symlink name, they see a `→` arrow followed by the target path. This tells them at a glance where each link points without needing to run a separate command.

**Why this priority**: Core value proposition — showing link targets is the entire feature. Without this, there is nothing.

**Independent Test**: Create a directory with a junction and a file symlink. Run `tcdir` and verify the `→ target` suffix appears after each link entry.

**Acceptance Scenarios**:

1. **Given** a directory containing a junction `Projects → C:\Dev\Projects`, **When** the user runs `tcdir`, **Then** the junction entry shows the filename, a `→` arrow, and the target path `C:\Dev\Projects`
2. **Given** a directory containing a file symlink `config.yml → ..\shared\config.yml`, **When** the user runs `tcdir`, **Then** the symlink entry shows the filename, a `→` arrow, and the target path as stored (`..\shared\config.yml`)
3. **Given** a directory containing a directory symlink `docs → D:\Documentation`, **When** the user runs `tcdir`, **Then** the symlink entry shows the filename, a `→` arrow, and the target path `D:\Documentation`
4. **Given** a directory with no junctions or symlinks, **When** the user runs `tcdir`, **Then** output is identical to current behavior — no arrows, no changes

---

### User Story 2 — Tree Mode Target Display (Priority: P2)

A user runs `tcdir --Tree` in a directory tree that contains junctions and symlinks. Each link entry in the tree shows the `→ target` suffix, just like in normal mode. Junctions and symlinks are displayed but not recursed into (existing behavior preserved).

**Why this priority**: Tree mode is a heavily used display mode and already has partial reparse-point awareness (cycle guard). Extending it with target display is a natural complement.

**Independent Test**: Create a directory tree with a junction at depth 2. Run `tcdir --Tree` and verify the junction shows its target and is not expanded.

**Acceptance Scenarios**:

1. **Given** a tree containing a junction `node_modules → C:\cache\node_modules`, **When** the user runs `tcdir --Tree`, **Then** the junction shows `→ C:\cache\node_modules` and its subtree is not expanded
2. **Given** a tree containing a file symlink, **When** the user runs `tcdir --Tree`, **Then** the file symlink shows `→ target` with the stored path
3. **Given** a tree with nested junctions (junction inside a regular directory), **When** the user runs `tcdir --Tree --Depth=3`, **Then** each junction at every level shows its target path

---

### User Story 3 — Color-Coded Target Paths (Priority: P3)

Link target paths are color-coded so users can distinguish the arrow from the target name. The arrow uses the existing `Information` color attribute. The target path uses the directory color for directory targets (junctions and directory symlinks) or the target file's extension-based color for file symlinks.

**Why this priority**: Color coding enhances readability and keeps the output consistent with TCDir's existing color conventions. However, the feature is functional without it.

**Independent Test**: Create a junction pointing to a directory and a file symlink pointing to a `.cpp` file. Run `tcdir` and verify the arrow color differs from the target path color, and that the target colors match existing directory/extension colors.

**Acceptance Scenarios**:

1. **Given** a junction `build → C:\Output\build`, **When** the user runs `tcdir`, **Then** the `→` arrow uses the `Information` color attribute and the target path `C:\Output\build` uses the same color as the junction entry name
2. **Given** a file symlink `main.cpp → ..\src\main.cpp`, **When** the user runs `tcdir`, **Then** the `→` arrow uses the `Information` color attribute and the target path uses the same color as the symlink entry name
3. **Given** a file symlink to a file with no recognized extension, **When** the user runs `tcdir`, **Then** the target path uses the same color as the symlink entry name
4. **Given** an AppExecLink entry (e.g., `python.exe` in `WindowsApps`), **When** the user runs `tcdir`, **Then** the `→` arrow uses the `Information` color attribute and the target path uses the same color as the AppExecLink entry name

---

### User Story 4 — Internal Path Prefix Stripping (Priority: P3)

Junction target paths stored internally with the `\??\` device prefix are displayed with that prefix stripped, showing clean user-readable paths.

**Why this priority**: Cosmetic but important for usability — raw device paths are confusing to users.

**Independent Test**: Create a junction (which stores targets with `\??\` internally). Run `tcdir` and verify the target path does not begin with `\??\`.

**Acceptance Scenarios**:

1. **Given** a junction whose internal target is `\??\C:\Users\Dev\Projects`, **When** the user runs `tcdir`, **Then** the displayed target is `C:\Users\Dev\Projects`

---

### Edge Cases

- **Access denied on link**: If the reparse data cannot be read (e.g., permission-restricted link), display the filename without a target — graceful degradation, no error shown
- **Very long target paths**: Target paths near MAX_PATH or using extended-length (`\\?\`) syntax are displayed in full (no truncation)
- **Recursive mode (`-S`)**: Junctions and symlinks show their targets but are not recursed into (existing behavior preserved)
- **Wide mode**: No target display — wide mode is explicitly out of scope
- **Bare mode**: No target display — bare mode output stays clean for scripting
- **File vs directory symlinks**: Both are supported; icons already distinguish the type
- **Non-reparse files**: Regular files and directories are completely unaffected
- **AppExecLink entries**: Store app aliases (e.g., `python.exe` in `WindowsApps`) show the resolved target executable path
- **Other reparse tags** (WCI, DEDUP, AF_UNIX, LX_SYMLINK, LX_FIFO): Silently ignored — no arrow, no indicator. Cloud reparse points are already handled by existing cloud status display

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST display `→ target_path` after the filename for all junctions in normal mode and tree mode
- **FR-002**: System MUST display `→ target_path` after the filename for all symlinks (file and directory) in normal mode and tree mode
- **FR-002a**: System MUST display `→ target_exe_path` after the filename for AppExecLink reparse points (`IO_REPARSE_TAG_APPEXECLINK`) in normal mode and tree mode
- **FR-003**: System MUST use the Unicode arrow character `→` (U+2192) as the link indicator, formatted as `filename → target` with exactly one space before and after the arrow
- **FR-004**: System MUST display target paths as-stored in the reparse data (relative paths stay relative, absolute paths stay absolute)
- **FR-005**: System MUST strip the `\??\` device prefix from junction target paths before display
- **FR-006**: System MUST render the `→` arrow using the existing `Information` color attribute
- **FR-007**: System MUST render the target path using the same color as the source filename (the link entry's resolved color attribute)
- ~~**FR-008**: System MUST render file target paths (file symlinks) using the color associated with the target file's extension~~ — superseded by FR-007
- **FR-009**: System MUST NOT display target paths in wide mode or bare mode
- **FR-010**: System MUST NOT recurse into junctions or symlinks (preserve existing reparse-point cycle guard behavior)
- **FR-011**: System MUST gracefully handle cases where reparse data cannot be read — display the filename without a target, no error message
- **FR-012**: System MUST NOT introduce any new command-line switches or configuration keys for this feature
- **FR-013**: System MUST NOT resolve or display hardlink information
- **FR-014**: System MUST silently ignore all reparse tags other than `IO_REPARSE_TAG_MOUNT_POINT`, `IO_REPARSE_TAG_SYMLINK`, and `IO_REPARSE_TAG_APPEXECLINK` — no arrow, no indicator, no error

### Key Entities

- **Reparse Point**: A file system entry with the `FILE_ATTRIBUTE_REPARSE_POINT` attribute set and a reparse tag in `dwReserved0` indicating junction (`IO_REPARSE_TAG_MOUNT_POINT`), symlink (`IO_REPARSE_TAG_SYMLINK`), or AppExecLink (`IO_REPARSE_TAG_APPEXECLINK`)
- **Target Path**: The destination path stored in the reparse data buffer — may be relative or absolute, may contain the `\??\` device prefix for junctions
- **AppExecLink**: A Windows Store app execution alias — a 0-byte placeholder file that redirects execution to a Store app's real executable. The reparse buffer contains the target executable path, package family name, and app user model ID

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Users can identify where every junction and symlink points by reading a single `tcdir` listing — no need to run secondary commands
- **SC-002**: Target paths are displayed correctly for 100% of accessible junctions and symlinks in a directory
- **SC-003**: Listing performance in directories with no reparse points is unchanged — zero overhead when no links are present
- **SC-004**: Listing performance in directories with reparse points adds no user-perceptible delay (reparse data reads are per-link, not per-file)
- **SC-005**: All existing unit tests continue to pass with no modifications
- **SC-006**: New unit tests cover target resolution, path prefix stripping, color selection, and graceful error handling

## Assumptions

- The `→` (U+2192) character renders correctly in all target terminal environments (Windows Terminal, ConHost, etc.) — this is consistent with existing Unicode symbol usage in TCDir
- Reparse points are uncommon in typical directories (0–5 per listing), so the per-link cost of reading reparse data is negligible
- Hardlink display is explicitly out of scope and may be addressed in a future feature with an opt-in switch
- Wide mode and bare mode exclusion is a permanent design decision, not a deferral
- This feature does not require changes to the config file schema or the `TCDIR` environment variable syntax

## Release Checklist

The following items MUST be completed before this feature is considered shipped:

- Bump the minor version number (5.3 → 5.4)
- Update `CHANGELOG.md` with the new version entry and feature description
- Update `README.md` "What's New" table with a v5.4 row
- Update `README.md` feature comparison table if applicable (symlink target display column)
- Update `specs/sync-status.md` with spec 007 row (TCDir status, version)
- Associate the implementation branch/PR with GitHub issue #6 (use `Closes #6` in PR description)
- Close GitHub issue #6 upon merge to master
