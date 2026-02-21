# Research: Tree View Display Mode

**Feature**: 004-tree-view  
**Date**: 2026-02-19

## R1: Tree Connector Insertion Point

**Decision**: Insert tree prefix between the owner/cloud-status columns and the icon/filename in `CResultsDisplayerTree::DisplayFileResults` (new class derived from `CResultsDisplayerNormal`).

**Rationale**: The existing file line rendering in `ResultsDisplayerNormal.cpp` emits columns in this order: date/time → attributes → file size → cloud status → debug attrs → owner → icon → filename. Tree connectors logically belong with the filename (they describe the file's position in the hierarchy), so they insert just before the icon glyph (or before the filename if no icon). This preserves all metadata column alignment. The new `CResultsDisplayerTree` class inherits all column helpers from `CResultsDisplayerNormal` (date/time, attributes, size, cloud status, debug, owner) and overrides the display flow and file-results loop to add tree prefix rendering.

**Alternatives considered**:
- Prepending connectors before the date column (shifts all columns, breaks alignment)
- Modifying `CResultsDisplayerNormal` in-place with conditional branching (tangles tree flow with normal flow; tree mode fundamentally changes the `DisplayResults` flow — no per-subdir path headers, tree-indented summaries, recursive child walking)
- Refactoring column helpers into an intermediate base class between `WithHeaderAndFooter` and Normal (unnecessary churn — Normal's protected methods are already inheritable)

## R2: Parameterized Long Switch Convention

**Decision**: Long switches that take values use `=` as separator (`--Depth=3`), with space also accepted (`--Depth 3`). Parsed via `HandleLongSwitch` with `=` splitting.

**Rationale**: No existing long switch takes a value — `--owner`, `--streams`, `--icons` are all boolean. The `=` convention is the GNU standard and matches `eza`/`lsd` (`--level=3`). It distinguishes cleanly from the short-switch colon pattern (`/T:C`). Space support is added because many users expect it.

**Alternatives considered**:
- Colon separator (`--Depth:3`) — conflicts with existing short-switch convention
- Separate arguments only (`--Depth 3`) — ambiguous when value looks like a switch or file mask

## R3: Config Integer Value Parsing

**Decision**: Extend `CConfig` with new `optional<int>` members and add integer-typed entries to the switch/config parsing system alongside the existing `optional<bool>` pattern.

**Rationale**: The existing `SSwitchMapping` table only supports `optional<bool>` members. Adding `Depth=N` and `TreeIndent=N` to the env var requires a parallel table or extended struct for integer-valued settings. A new `SIntSwitchMapping` struct (or extending `ProcessColorOverrideEntry` with a new key check before `ParseKeyAndValue`) keeps the existing boolean path untouched.

**Alternatives considered**:
- String-parsing with `_wtoi` inline in the existing switch handler (ad-hoc, doesn't scale)
- Generic variant-based switch table (over-engineered for 2 integers)

## R4: Architecture for Tree Display

**Decision**: Reuse the multi-threaded `PrintDirectoryTree` → `ProcessChildren` recursive pattern on the main thread. Add tree connector state as a parameter threaded through the recursion. Display through `CResultsDisplayerTree` (derived from `CResultsDisplayerNormal`).

**Rationale**: The existing MT path already builds a tree of `CDirectoryInfo` nodes (via `m_vChildren`) and walks it depth-first on the main thread in `PrintDirectoryTree`. For tree view, the same traversal adds a `vector<bool>` (or equivalent) tracking which ancestor levels still have siblings — this determines whether each level draws `│` (more siblings) or ` ` (no more siblings). The connector character for each entry (├── vs └──) is determined by whether it's the last entry at its level. `CResultsDisplayerTree` overrides `DisplayResults` to provide the tree-walking flow (no per-subdir path headers, indented summaries) and `DisplayFileResults` to prepend tree connectors, reusing all inherited column helpers.

**Alternatives considered**:
- Building the tree in the displayer (wrong responsibility boundary)
- Post-processing approach — enumerate everything, store, then display (loses streaming benefits)

## R5: Single-Threaded Tree Path

**Decision**: Extend the single-threaded `RecurseIntoSubdirectories` path to build `m_vChildren` so it can also support tree display. Alternatively, tree mode always uses the MT lister (even with `-M-`), since the MT lister already builds the tree structure needed.

**Rationale**: The ST path currently uses ephemeral `CDirectoryInfo` objects — it doesn't build a tree. Tree view requires knowing all entries at each level to determine which is "last" (for `└──` vs `├──`). The MT path already builds this tree. The cleanest approach is: when `--Tree` is active, always use the MT code path for enumeration (which builds `m_vChildren`), regardless of `-M` flag. The MT lister handles single-worker scenarios correctly anyway.

**Alternatives considered**:
- Refactoring ST path to build `m_vChildren` (unnecessary duplication of MT logic)
- Two-pass ST: enumerate all, build tree, then display (works but slower)

## R6: Interleaved Dir/File Sorting for Tree

**Decision**: In tree mode, sort all entries (files and directories together) by the active sort order, instead of the current behavior that groups directories before files.

**Rationale**: The current `SortResults` in `MultiThreadedLister.cpp` and `FileComparator` sort `m_vMatches`. In current non-tree mode, directories are listed separately from files because the display path header/footer structure creates natural grouping. In tree mode, interleaving is more natural — users see entries in their sorted position regardless of type. A directory entry is followed by its children (indented), then the next sibling entry continues.

**Alternatives considered**:
- Keep grouping (directories first, then files) — less natural for tree view

## R7: Cycle Detection

**Decision**: There is NO existing cycle detection. Add reparse-point checking to the shared recursion path so both `-S` (recursive) and `--Tree` modes are protected from infinite recursion through junctions/symlinks.

**Rationale**: The current codebase unconditionally recurses into all directories. A junction or symlink creating a cycle causes infinite recursion — this is an existing bug in `-S` mode, not just a tree concern. The fix belongs in the common recursion point (`EnumerateDirectoryNode` in the MT lister): check `FILE_ATTRIBUTE_REPARSE_POINT` before recursing; if set, list the directory but don't expand its children, and show a `[→ target]` indicator. Fixing it once protects both modes.

**Alternatives considered**:
- Canonical path set tracking (more robust but more expensive)
- Rely on OS protections (unreliable — NTFS junctions can bypass)
- Tree-only fix (leaves existing `-S` mode vulnerable to the same bug)

## R8: Per-Directory Summary in Tree Mode

**Decision**: Each directory in the tree shows its own summary line (file/dir counts, bytes used), plus a grand total at the end.

**Rationale**: The existing `DisplayResults` already calls `DisplayDirectorySummary` for every directory. In recursive mode, `DisplayRecursiveSummary` shows aggregate totals at the end. Tree mode should preserve per-directory summaries (each directory's summary appears after its contents, within the tree indentation) and show a grand total after the entire tree.

**Alternatives considered**:
- Root summary only (loses per-directory detail)
- Grand total only (less useful)

## R9: Tree Connector Color

**Decision**: Add a new `CConfig::EAttribute` entry for tree connector color, configurable via the `TCDIR` environment variable using the existing color system.

**Rationale**: The existing color system supports per-element color attributes (Date, Time, FileAttributePresent, Size, Directory, etc.). Adding a `TreeConnector` attribute follows the same pattern. The default color can be a subtle terminal color (e.g., LightGray) to distinguish structural connectors from content.

**Alternatives considered**:
- Hardcoded color (not configurable — violates UX consistency principle)
- Inherit from Default attribute (may be too bright/distracting)

## R10: Displayer Class Architecture

**Decision**: Create a new `CResultsDisplayerTree` class that derives from `CResultsDisplayerNormal`. Override `DisplayResults` (for tree-walking flow) and `DisplayFileResults` (for tree prefix insertion). Reuse all inherited protected column helpers (date/time, attributes, size, cloud status, debug, owner).

**Rationale**: Tree mode isn't just "normal + prefix column" — it fundamentally changes the display flow: no separate path headers per subdirectory, summaries indented within the tree, directory-then-children recursion drives display order, stream continuation lines need `│` prefixes. Trying to branch all of this inside the existing `WithHeaderAndFooter::DisplayResults` and `Normal::DisplayFileResults` methods tangles two concerns. A separate class encapsulates tree-specific flow cleanly. Deriving from Normal (rather than `WithHeaderAndFooter`) is optimal because Normal's 9 protected column helper methods (`GetTimeFieldForDisplay`, `DisplayResultsNormalDateAndTime`, `DisplayResultsNormalAttributes`, `DisplayResultsNormalFileSize`, `DisplayCloudStatusSymbol`, `DisplayRawAttributes`, `DisplayFileOwner`, `GetFileOwner`/`GetFileOwners`, `DisplayFileStreams`) are exactly what Tree needs to render each file line — only the loop structure and prefix differ.

**Alternatives considered**:
- Modifying `CResultsDisplayerNormal` in-place (tangles tree and normal flow logic)
- Refactoring column helpers into intermediate base class (unnecessary churn — protected methods already inheritable)
- Deriving from `CResultsDisplayerWithHeaderAndFooter` (would duplicate column logic from Normal)
## R11: Streaming Output — Flush Before Child Recursion

**Decision**: Flush the console buffer before recursing into each child directory in `PrintDirectoryTreeMode`, and again after the entry loop completes (for trailing file entries).

**Rationale**: Without explicit flushes, all output is buffered until the entire tree traversal completes — the user sees zero output for potentially minutes on large trees, making it appear hung. The tree display must feel live. Flushing before each child directory guarantees the parent's entry line (and any preceding siblings) are visible before the (potentially slow) child subtree begins. A second flush after the entry loop ensures trailing file entries (those after the last subdirectory in sort order) are also visible promptly. Both flushes exist only in `PrintDirectoryTreeMode` — the `-S` recursive path (`PrintDirectoryTree`/`ProcessChildren`) is untouched since it already flushes per-directory via `DisplayResults`.

**Alternatives considered**:
- Flushing only at the end of `PrintDirectoryTreeMode` (user sees nothing until the entire subtree is done — defeats streaming)
- Flushing after every entry (excessive system calls, measurable perf impact)
- Reducing the CConsole buffer size (would affect all modes, not just tree)

## R12: Per-Directory Display State Preservation Across Recursion

**Decision**: Save and restore the per-directory display state (`m_cchStringLengthOfMaxFileSize`, `m_fInSyncRoot`, `m_owners`, `m_cchMaxOwnerLength`) around each recursive child call in `PrintDirectoryTreeMode`, via `SaveDirectoryState()` / `RestoreDirectoryState()` on `CResultsDisplayerTree`.

**Rationale**: `BeginDirectory()` stores per-directory computed state (field widths, sync root flag, owner vector) in member variables on the displayer. When tree mode recurses into a child directory, the child's `BeginDirectory()` call overwrites the parent's state. After returning from the child, the parent's remaining entries would render with the wrong column widths, causing misaligned output. Saving and restoring the state around each recursive descent preserves the parent's column layout. The state struct is small (a size_t, a bool, a vector, another size_t) and the copy/move is negligible relative to I/O.

**Alternatives considered**:
- Making `BeginDirectory` stack-based (would require refactoring all state management)
- Pre-computing a global max across the entire tree before display (eliminates streaming benefit, terrible latency on large trees — this was implemented and rejected)
- Passing state as a parameter instead of member variables (large refactor of the Normal base class)

## R13: Fixed-Width Abbreviated File Sizes for Tree Alignment

**Decision**: Add a `--Size=Auto` mode that formats file sizes using Explorer-style abbreviated format (1024-based, 3 significant digits) in a fixed 7-character column. Tree mode defaults to `--Size=Auto`; non-tree mode defaults to `--Size=Bytes` (existing exact-byte format).

**Rationale**: In tree mode, entries from different directories are interleaved in a single output stream. Each directory may have a different largest file, producing different `cchStringLengthOfMaxFileSize` values. Variable-width size columns cause tree connectors (│, ├──, └──) to misalign between directories at different levels. A global pre-scan to compute a uniform max was implemented but rejected because it requires traversing the entire tree before displaying any output — defeating the streaming flush strategy (R11). A fixed-width abbreviated format solves alignment permanently with zero pre-scan cost. The Explorer-style format is familiar to Windows users.

**Format**: 1024-based division with 3 significant digits:
- `0 B` to `999 B` (exact bytes with `B` suffix)
- `1 KB` to `9.99 KB` (2 decimal places)
- `10.0 KB` to `99.9 KB` (1 decimal place)
- `100 KB` to `999 KB` (integer)
- Same pattern for MB, GB, TB
- `<DIR>` centered in the same 7-char field
- Right-justified, max 7 characters

**Alternatives considered**:
- Global max file size pre-scan before display (implemented in an earlier iteration; works but eliminates streaming — terrible latency on large trees)
- Variable-width columns with per-directory max (the original approach — causes tree connector misalignment)
- Separate size and connector columns with fixed position (would break compatibility with the Normal displayer's column order)