# Feature Specification: Tree View Display Mode

**Feature Branch**: `004-tree-view`  
**Created**: 2026-02-19  
**Status**: Draft  
**Input**: User description: "Add tree view display mode with hierarchical directory listing, depth control, and tree connectors for subdirectory contents"

## Clarifications

### Session 2026-02-19

- Q: What should the footer totals aggregate in tree mode? → A: Both — each directory level shows its own per-directory summary (file/directory counts, bytes used), and a grand total for the entire tree is shown at the end. This matches the existing `-S` recursive mode pattern.
- Q: Should directories and files be interleaved or grouped within each tree level? → A: Interleaved — directories and files are sorted together as a single list by the chosen sort order. This differs from current non-tree behavior (which groups directories first) but is more natural for tree display.
- Q: How wide should each tree indentation level be, and is it configurable? → A: Default is 4 characters per level (matching eza/lsd convention). Configurable via `--TreeIndent=N` (1 ≤ N ≤ 8). Long switches that take values use `=` as the separator (e.g., `--Depth=3`, `--TreeIndent=4`), with space-separated form also accepted (`--Depth 3`).
- Q: What is explicitly out of scope? → A: (1) ASCII fallback for non-Unicode terminals, (2) tree connectors in wide or bare modes, (3) collapsible/interactive tree, (4) JSON/machine-readable tree output. Colorized tree connectors (configurable color) ARE in scope.
- Q: How should empty directories be pruned when file masks are active? → A: Shallow pruning — only leaf directories with zero matching files are pruned. Intermediate directories are shown even if they have no direct matches, since they may have matching descendants. No two-pass enumeration required.

## Out of Scope

- ASCII fallback for tree connectors on non-Unicode terminals
- Tree connectors in wide (`-W`) or bare (`-B`) display modes
- Collapsible or interactive tree (this is a static console output tool)
- JSON or machine-readable tree output format

## Assumptions

- Tree connectors will use Unicode box-drawing characters (`├──`, `└──`, `│`) which are available in all modern Windows console fonts. No fallback to ASCII is planned.
- The `--Tree` switch follows the existing long-switch convention (case-insensitive matching, `--` prefix).
- The `--Depth` switch accepts a positive integer value. Zero or negative values produce an error.
- Long switches that take values use `=` as the separator (e.g., `--Depth=3`, `--TreeIndent=4`). A space separator is also accepted (e.g., `--Depth 3`). This is the first use of parameterized long switches in TCDir and establishes the convention for future switches.
- When `--Depth` is omitted alongside `--Tree`, recursion is unlimited (enumerate the full directory tree).
- The existing multi-threaded producer/consumer enumeration model (worker threads enumerate directories, main thread walks the tree sequentially for display) is reused without architectural changes.
- File masks apply at every level of the tree — a mask like `*.cpp` shows only matching files at each directory level, but still shows all directories to preserve tree structure.
- Sort order applies independently at each directory level (siblings are sorted, but parent-child ordering is always parent-first). In tree mode, directories and files are interleaved (sorted together as a single list), unlike non-tree mode which groups directories before files.
- The existing header (volume label, directory path) and footer (file/directory counts, bytes used, free space) apply to the root directory listing. Subdirectories in tree mode do not repeat headers/footers.
- Performance timing (`-P`) is compatible with tree mode and measures the entire tree enumeration + display.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Basic Tree Listing (Priority: P1)

A user runs `tcdir --Tree` in a project directory. The top-level files and directories are listed in the standard normal-mode format (date, time, attributes, size, filename). Subdirectory contents are displayed below their parent directory with tree connectors (`├──`, `└──`, `│`) prepended to each line, visually showing the hierarchy. The tree connectors indent to reflect nesting depth.

**Why this priority**: This is the core feature — displaying directory contents hierarchically is the entire value proposition. Without this, the feature doesn't exist.

**Independent Test**: Run `tcdir --Tree` in a directory with at least two levels of nested subdirectories. Verify that tree connectors are displayed correctly and all metadata columns are present.

**Acceptance Scenarios**:

1. **Given** a directory containing files and one subdirectory with files, **When** running `tcdir --Tree`, **Then** top-level entries appear in normal format and subdirectory entries appear below their parent with tree connectors.
2. **Given** a directory with three levels of nesting (parent → child → grandchild), **When** running `tcdir --Tree`, **Then** each level is indented further with appropriate connector characters showing the hierarchy.
3. **Given** a subdirectory with multiple files, **When** running `tcdir --Tree`, **Then** the last file uses `└──` and all preceding files use `├──` as their connector.
4. **Given** a sibling directory follows a directory with children, **When** running `tcdir --Tree`, **Then** a `│` continuation line connects through the children back to the sibling at the same level.
5. **Given** a directory containing only files (no subdirectories), **When** running `tcdir --Tree`, **Then** the output looks identical to a normal `tcdir` listing (no tree connectors at the top level).

---

### User Story 2 - Depth-Limited Tree Listing (Priority: P1)

A user runs `tcdir --Tree --Depth 2` on a deep directory structure. The listing shows the root directory and its subdirectories recursively, but stops at a depth of 2 levels. Directories at the depth limit are listed as entries but their contents are not expanded.

**Why this priority**: Equally critical as basic tree — without depth control, large directory trees become unwieldy. Users need to limit output to stay productive.

**Independent Test**: Run `tcdir --Tree --Depth 1` in a directory with deeply nested subdirectories. Verify that only the immediate subdirectory contents are shown, not deeper levels.

**Acceptance Scenarios**:

1. **Given** a three-level directory structure, **When** running `tcdir --Tree --Depth 1`, **Then** only the root directory and its immediate subdirectory contents are shown; second-level subdirectories appear as entries but their contents are not expanded.
2. **Given** a deep directory tree, **When** running `tcdir --Tree --Depth 3`, **Then** exactly three levels of subdirectory contents are displayed.
3. **Given** `--Depth` specified without `--Tree`, **When** running the command, **Then** an error is displayed indicating that `--Depth` requires `--Tree`.
4. **Given** `--Depth 0` or `--Depth -1`, **When** running the command, **Then** an error is displayed indicating the depth must be a positive integer.
5. **Given** `--Tree` without `--Depth`, **When** running the command, **Then** all subdirectory levels are listed with no depth limit.

---

### User Story 3 - Tree View with Metadata Columns (Priority: P1)

A user runs `tcdir --Tree` and sees the same metadata columns they see in normal mode — date, time, file attributes, size, cloud status, and filename — at every level of the tree. The tree connector characters are prepended before the filename column, so the metadata remains in consistent columns.

**Why this priority**: The tree view must show full metadata to be useful as a directory listing tool (not just a structural overview). Without consistent columns, the output is unreadable.

**Independent Test**: Run `tcdir --Tree` and verify that date/time, attributes, and size columns are present and aligned at every nesting level.

**Acceptance Scenarios**:

1. **Given** `tcdir --Tree` output with nested files, **When** examining any entry at any depth, **Then** the date, time, attributes, and size columns are present and formatted identically to normal mode.
2. **Given** tree output spanning multiple nesting levels, **When** examining the metadata columns, **Then** columns are aligned consistently (tree connectors do not shift metadata column positions).
3. **Given** a tree listing with `--Icons` active, **When** examining entries, **Then** the icon appears in its normal position before the filename, with tree connectors prepended before the icon.
4. **Given** a tree listing with `--Owner` active, **When** examining entries, **Then** the owner column appears in its normal position alongside other metadata.

---

### User Story 4 - Tree View with Alternate Data Streams (Priority: P2)

A user runs `tcdir --Tree --Streams` on a directory containing files with alternate data streams. Stream entries appear immediately below their parent file, indented to the same level. The stream lines use a `│` vertical continuation line to connect to the parent directory level (not a horizontal connector) — the parent file's tree connector already establishes the link. The next real file/directory after the streams resumes normal tree connectors.

**Why this priority**: Streams support is a differentiating TCDir feature. Ensuring it works correctly in tree mode maintains feature parity with the recursive listing mode.

**Independent Test**: Run `tcdir --Tree --Streams` in a directory with files that have alternate data streams. Verify that streams appear below their parent file with correct vertical continuation lines.

**Acceptance Scenarios**:

1. **Given** a file with alternate data streams in a subdirectory, **When** running `tcdir --Tree --Streams`, **Then** streams appear immediately below the parent file with the same indentation level.
2. **Given** a stream entry within a tree listing, **When** examining the tree connectors, **Then** the stream line shows a `│` vertical continuation (not `├──` or `└──`) to indicate it belongs to the file above, not the directory tree.
3. **Given** a file with streams is followed by another file, **When** examining the output, **Then** the next file resumes normal tree connectors (`├──` or `└──`) after the stream entries.
4. **Given** `--Streams` without `--Tree` (just `-S` recursion or normal mode), **When** running the command, **Then** streams behave exactly as they do today (no change to existing behavior).

---

### User Story 5 - Incompatible Switch Detection (Priority: P2)

A user accidentally combines `--Tree` with an incompatible switch (`-W`, `-B`, or `-S`). The system reports a clear error identifying the conflicting switches and does not produce partial or confusing output.

**Why this priority**: Clean error messages prevent user confusion. These switch conflicts must be detected before any enumeration work begins.

**Independent Test**: Run `tcdir --Tree -W`, `tcdir --Tree -B`, and `tcdir --Tree -S` and verify each produces a clear, specific error.

**Acceptance Scenarios**:

1. **Given** `tcdir --Tree -W`, **When** parsing the command line, **Then** an error is displayed stating that `--Tree` and `-W` cannot be used together, and the program exits without listing.
2. **Given** `tcdir --Tree -B`, **When** parsing the command line, **Then** an error is displayed stating that `--Tree` and `-B` cannot be used together, and the program exits without listing.
3. **Given** `tcdir --Tree -S`, **When** parsing the command line, **Then** an error is displayed stating that `--Tree` and `-S` cannot be used together, and the program exits without listing.
4. **Given** `tcdir --Tree --Owner --Icons`, **When** parsing the command line, **Then** no error occurs and all three features work together.

---

### User Story 6 - Tree View Environment Variable Configuration (Priority: P3)

A user can enable tree mode and set a default depth via the `TCDIR` environment variable. This allows users who always prefer tree output to set it as their default without typing `--Tree` every time.

**Why this priority**: Configuration via environment variable follows the established TCDir pattern and is a convenience feature. Not required for the core tree experience.

**Independent Test**: Set `TCDIR=Tree` and run `tcdir` without flags. Verify tree mode activates. Set `TCDIR=Tree- Depth=3` and verify depth limiting.

**Acceptance Scenarios**:

1. **Given** `TCDIR=Tree`, **When** running `tcdir` without flags, **Then** tree mode is active with unlimited depth.
2. **Given** `TCDIR=Tree Depth=3`, **When** running `tcdir` without flags, **Then** tree mode is active with a depth limit of 3.
3. **Given** `TCDIR=Tree` but the user passes `--Tree-` on the CLI, **When** running the command, **Then** the CLI flag wins and tree mode is disabled.
4. **Given** `TCDIR=Depth=2` without `Tree`, **When** running `tcdir` without `--Tree`, **Then** `--Depth` is silently ignored (no error, since it has no effect without tree mode).

---

### Edge Cases

- What happens when tree encounters a directory it cannot access (permission denied)? → The inaccessible directory is listed as an entry with its name, but its contents are not expanded. An error message is displayed inline (matching existing `-S` behavior for access-denied directories).
- What happens when a symlink/junction creates a circular reference? → The existing cycle-detection behavior from recursive mode (`-S`) applies. If a cycle is detected, the directory is listed but not expanded, and a note is shown.
- What happens with an extremely deep directory tree (hundreds of levels) without `--Depth`? → The system enumerates to the full depth. The indentation grows with each level. No artificial limit is imposed beyond OS path-length limits.
- What happens when `--Tree` is used with file masks (e.g., `tcdir --Tree *.cpp`)? → Files are filtered by the mask at every level, but all directories are still shown to preserve tree structure. Empty directories (no matching files and no subdirectories with matching files) are pruned from the output.
- What happens with `--Tree --Depth 1` in an empty directory? → The normal "File Not Found" message is shown, same as today.
- What happens when the console window is too narrow for tree indentation + metadata columns? → The output wraps at the console width, same as normal mode. No special truncation is applied.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST provide a `--Tree` long switch that activates tree view display mode.
- **FR-002**: System MUST provide a `--Depth N` long switch that limits tree recursion to N levels, where N is a positive integer.
- **FR-003**: System MUST display the root directory's immediate contents in standard normal-mode format without tree connectors.
- **FR-004**: System MUST display subdirectory contents with Unicode tree connectors (`├──`, `└──`, `│`) prepended to the filename column to indicate hierarchy.
- **FR-005**: System MUST indent tree connectors by a configurable number of characters per nesting level, defaulting to 4 characters.
- **FR-005a**: System MUST provide a `--TreeIndent=N` long switch to override the default indent width, where N is an integer from 1 to 8.
- **FR-005b**: System MUST report an error when `--TreeIndent` is specified without `--Tree`.
- **FR-005c**: System MUST report an error when `--TreeIndent` is given a value outside the range 1–8.
- **FR-006**: System MUST display all standard metadata columns (date, time, attributes, size, cloud status) at every tree level, with consistent column alignment.
- **FR-007**: System MUST report an error and exit when `--Tree` is combined with `-W` (wide), `-B` (bare), or `-S` (recurse).
- **FR-008**: System MUST report an error when `--Depth` is specified without `--Tree`.
- **FR-008a**: Long switches that accept values MUST support both `=` separator (`--Depth=3`) and space separator (`--Depth 3`) forms.
- **FR-009**: System MUST report an error when `--Depth` is given a non-positive integer value.
- **FR-010**: System MUST enumerate the full directory tree when `--Tree` is specified without `--Depth`.
- **FR-011**: System MUST support `--Tree` alongside `--Streams`, displaying stream entries with a `│` vertical continuation line instead of horizontal tree connectors.
- **FR-012**: System MUST support `--Tree` alongside `--Owner`, `--Icons`, sort-order switches, time-field switches, and attribute filters.
- **FR-012a**: System MUST support configurable tree connector color via the existing color configuration system (e.g., `TCDIR` environment variable). A default connector color is used when no custom color is configured.
- **FR-013**: System MUST support multi-threaded enumeration (`-M`) in tree mode using the existing producer/consumer model.
- **FR-014**: System MUST apply file masks at every level of the tree, showing matching files while preserving directory structure to maintain visual hierarchy.
- **FR-015**: System MUST use shallow pruning when file masks are specified: leaf directories with zero matching files are not expanded in the tree output. Intermediate directories are always shown to preserve tree structure, even if they contain no direct matches (they may have matching descendants).
- **FR-016**: System MUST support `Tree`, `Depth=N`, and `TreeIndent=N` configuration via the `TCDIR` environment variable, following the existing convention.
- **FR-017**: CLI switches MUST override environment variable defaults for both `Tree`/`Tree-` and `Depth`.
- **FR-018**: System MUST handle inaccessible directories gracefully, listing them as entries without expanding their contents and displaying an inline error.
- **FR-019**: System MUST display a per-directory summary (file/directory counts, bytes used) at each directory level in the tree, plus a grand total summary for the entire tree at the end, matching the existing recursive mode pattern. The volume header (drive label) and volume footer (free space) appear only once for the root directory.

### Key Entities

- **Tree Connector State**: Tracks which tree connector characters to display for each entry based on its position within its parent directory (first, middle, last) and the nesting depth. At each depth level, the state records whether the parent still has remaining siblings (requiring `│` continuation) or not (requiring blank space).
- **Depth Tracker**: Maintains the current recursion depth during tree traversal, compared against the user-specified `--Depth` limit to decide whether to expand subdirectories.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Users can view a hierarchical directory listing with a single command (`tcdir --Tree`) and visually identify parent-child relationships without external tools.
- **SC-002**: Users can limit tree output to a specific depth (`--Depth N`) and receive results limited to exactly N levels of subdirectory contents.
- **SC-003**: Output at every tree level includes complete file metadata (date, time, attributes, size) matching the normal listing format.
- **SC-004**: All incompatible switch combinations (`--Tree` + `-W`/`-B`/`-S`) produce clear error messages within the first line of output without performing any enumeration work.
- **SC-005**: Tree view with multi-threaded enumeration on a directory with 1000+ files across 50+ subdirectories produces correct, deterministic output (same result on every invocation with the same sort order).
- **SC-006**: Alternate data streams in tree mode display with vertical continuation lines, maintaining visual clarity about which file owns the streams.
- **SC-007**: Existing non-tree functionality (normal, wide, bare, recursive modes) is completely unaffected by the addition of tree view — all existing tests continue to pass without modification.
