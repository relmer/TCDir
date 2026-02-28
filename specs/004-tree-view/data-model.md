# Data Model: Tree View Display Mode

**Feature**: 004-tree-view  
**Date**: 2026-02-19

## Entities

### 1. CCommandLine (extended)

Existing class, extended with three new members for tree view switches.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `m_fTree` | `bool` | `false` | `--Tree` switch: activate tree view display mode |
| `m_cMaxDepth` | `int` | `0` | `--Depth=N`: max recursion depth (0 = unlimited) |
| `m_cTreeIndent` | `int` | `4` | `--TreeIndent=N`: characters per tree indent level (1–8) |
| `m_eSizeFormat` | `ESizeFormat` | `Default` | `--Size=Auto\|Bytes`: file size display format |

**`ESizeFormat` enum**:
- `Default` — not explicitly set; tree mode uses `Auto`, non-tree uses `Bytes`
- `Auto` — Explorer-style abbreviated (1024-based, 3 significant digits, fixed 7-char width)
- `Bytes` — exact byte count with comma separators (existing behavior)

**Validation rules**:
- `m_fTree` + `m_fWideListing` → error
- `m_fTree` + `m_fBareListing` → error
- `m_fTree` + `m_fRecurse` → error
- `m_cMaxDepth > 0` without `m_fTree` → error
- `m_cTreeIndent` outside [1, 8] → error
- `m_cTreeIndent != 4` without `m_fTree` → error
- `m_cMaxDepth ≤ 0` when explicitly specified → error (default value of 0 means unlimited; the parser rejects user-supplied values ≤ 0 before storage)
- `m_eSizeFormat` values other than `Auto` or `Bytes` → error

### 2. CConfig (extended)

Existing class, extended with new optional members and a new color attribute.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `m_fTree` | `optional<bool>` | `nullopt` | `Tree` / `Tree-` in TCDIR env var |
| `m_cMaxDepth` | `optional<int>` | `nullopt` | `Depth=N` in TCDIR env var |
| `m_cTreeIndent` | `optional<int>` | `nullopt` | `TreeIndent=N` in TCDIR env var |
| `m_eSizeFormat` | `optional<ESizeFormat>` | `nullopt` | `Size=Auto` / `Size=Bytes` in TCDIR env var |

**New EAttribute value**:
- `EAttribute::TreeConnector` — color attribute for tree connector characters

### 3. STreeConnectorState (new)

Lightweight struct tracking the tree drawing state as the main thread recurses through the directory tree. Passed by reference through the display call chain.

| Field | Type | Description |
|-------|------|-------------|
| `m_vAncestorHasSibling` | `vector<bool>` | One entry per nesting depth. `true` = ancestor at that level has more siblings coming (draw `│`); `false` = ancestor was last at that level (draw space) |
| `m_cTreeIndent` | `int` | Characters per indent level (same name as `CCommandLine::m_cTreeIndent`, default 4) |

**Methods**:

| Method | Returns | Description |
|--------|---------|-------------|
| `GetPrefix(bool fIsLastEntry)` | `wstring` | Generates the full tree prefix string for the current entry. Iterates `m_vAncestorHasSibling` to build continuation lines, then appends `├── ` or `└── ` based on `fIsLastEntry` |
| `GetStreamContinuation()` | `wstring` | Generates the prefix for a stream line: same as regular prefix but replaces the connector with `│` + padding (vertical continuation only) |
| `Push(bool fHasSibling)` | `void` | Push a new depth level (entering a subdirectory) |
| `Pop()` | `void` | Pop a depth level (leaving a subdirectory) |
| `Depth()` | `size_t` | Current nesting depth (size of `m_vAncestorHasSibling`) |

**State transitions**:
- Start: empty (depth 0, root directory)
- When entering a subdirectory's children: `Push(parentHasMoreSiblings)` 
- When leaving a subdirectory's children: `Pop()`
- At depth 0 (root): no prefix generated (top-level entries have no connectors)

### 4. CResultsDisplayerTree (new)

New class derived from `CResultsDisplayerNormal`. Overrides the display flow for tree mode while reusing all inherited column rendering helpers.

**Inherits from `CResultsDisplayerNormal`**:
- `GetTimeFieldForDisplay`, `DisplayResultsNormalDateAndTime`, `DisplayResultsNormalAttributes`, `DisplayResultsNormalFileSize`
- `DisplayCloudStatusSymbol`, `DisplayRawAttributes` (note: `DisplayFileOwner`, `GetFileOwner`, `GetFileOwners` are inherited but unused since `--Owner` is incompatible with `--Tree`)
- All `CResultsDisplayerWithHeaderAndFooter` base helpers (`DisplayDriveHeader`, `DisplayDirectorySummary`, `DisplayListingSummary`, `FormatNumberWithSeparators`, etc.)

| Method | Override | Description |
|--------|----------|-------------|
| `DisplayResults` | Yes (from `WithHeaderAndFooter`) | Delegates to base class; tree-walking flow is driven externally by `CMultiThreadedLister::PrintDirectoryTreeMode` for streaming output (see Design Note below) |
| `DisplayFileResults` | Yes (from `Normal`) | Delegates to base class; individual entry rendering is driven by MT lister calling `DisplaySingleEntry` directly |
| `DisplaySingleEntry` | New (public) | Renders one file line — calls inherited column helpers, inserts tree prefix from `STreeConnectorState`, then icon + filename. Public because the MT lister calls it directly rather than through `DisplayFileResults`. |
| `DisplayFileStreamsWithTreePrefix` | New | Like inherited `DisplayFileStreams` but prepends tree continuation prefix (`│   `) to each stream line |
| `SaveDirectoryState` | New | Captures per-directory display state (field widths, sync root flag) into an `SDirectoryDisplayState` struct so it can be restored after recursing into a child |
| `RestoreDirectoryState` | New | Restores previously saved per-directory display state after returning from a child directory recursion |

**`SDirectoryDisplayState` struct** (used by `SaveDirectoryState`/`RestoreDirectoryState`):

| Field | Type | Description |
|-------|------|-------------|
| `m_cchStringLengthOfMaxFileSize` | `size_t` | Cached string length of the largest file size in the current directory |
| `m_fInSyncRoot` | `bool` | Whether the current directory is under a cloud sync root |
| `m_owners` | `vector<wstring>` | Pre-computed owner strings for all files in the directory |
| `m_cchMaxOwnerLength` | `size_t` | Max length of owner strings for column sizing |

**Why save/restore is needed**: `BeginDirectory()` stores per-directory computed state in member variables. When tree mode recurses into a child, the child's `BeginDirectory()` overwrites the parent's state. Without save/restore, the parent's remaining entries after returning from the child render with incorrect column widths, causing misaligned output. Note: owner fields (`m_owners`, `m_cchMaxOwnerLength`) are retained in the struct but unused since `--Owner` is incompatible with `--Tree`.

**Design Note — Lister-Driven Architecture**: The spec originally described a displayer-driven tree walk (where `DisplayResults` would control recursion). In practice, the MT lister drives the tree walk (`PrintDirectoryTreeMode` → `DisplayTreeEntries` → `RecurseIntoChildDirectory`) and calls tree-specific public methods (`DisplaySingleEntry`, `BeginDirectory`, `DisplayTreeRootHeader`, `DisplayTreeRootSummary`) directly. This inversion is necessary because streaming output (FR-020) requires flush points between entry display and child recursion, and the MT lister already owns the `CDirectoryInfo` tree and the console flush logic. `DisplayResults` and `DisplayFileResults` delegate to the base class for non-tree paths.

**Pruning behavior**: When file masks are active (`m_fTreePruningActive` on `CMultiThreadedLister`), empty subdirectories are pruned using a thread-safe event-based approach. Each `CDirectoryInfo` node carries `m_fDescendantMatchFound` and `m_fSubtreeComplete` atomics (set by producer threads) that the display thread waits on to determine visibility. See R14 in research.md for the full design. Leaf directories with zero matching files and no matching descendants are skipped. Intermediate directories are rendered to preserve tree structure (FR-015). When a directory is pruned, the parent's `m_cSubDirectories` count is decremented so that `AccumulateTotals` only accumulates counts for directories actually shown in the output.

### 5. CDirectoryInfo (extended for tree pruning)

Existing struct, extended with three new members for thread-safe empty-subdirectory pruning in tree mode with file masks. These members are only active when `m_fTreePruningActive` is true on the lister.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `m_wpParent` | `weak_ptr<CDirectoryInfo>` | empty | Back-pointer to parent node for upward propagation; avoids reference cycles. Only set when `m_fTreePruningActive`. |
| `m_fDescendantMatchFound` | `atomic<bool>` | `false` | Set `true` by producer when this node or any descendant has `m_cFiles > 0`. Propagated upward through `m_wpParent` chain. |
| `m_fSubtreeComplete` | `atomic<bool>` | `false` | Set `true` by producer when this node AND all descendants have finished enumeration. |

**Thread safety**: `m_fDescendantMatchFound` and `m_fSubtreeComplete` are atomics — lock-free writes by producers, lock-free reads by the display thread. After setting either atomic, the producer notifies the existing `m_cvStatusChanged` condition variable to wake the display thread. The `m_wpParent` is set once during `EnqueueChildDirectory` (before the child is enqueued) and read-only thereafter — no synchronization needed.

**Invariants**:
- Once `m_fDescendantMatchFound` is `true`, it never reverts to `false`.
- Once `m_fSubtreeComplete` is `true`, it never reverts to `false`.
- A node with `m_fSubtreeComplete == true && m_fDescendantMatchFound == false` is definitively invisible (no matching descendants).
- A node with `m_fDescendantMatchFound == true` is definitively visible (regardless of `m_fSubtreeComplete`).

### 6. CMultiThreadedLister (extended for tree pruning)

Existing class, extended with one new member and several new helper methods.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `m_fTreePruningActive` | `bool` | `false` | Set `true` when `m_fTree && !fAllStar` (tree mode with a non-`*` file mask). Gates all pruning logic. |

**New methods**:

| Method | Scope | Description |
|--------|-------|-------------|
| `PropagateDescendantMatch` | private | Walks up from a node through `m_wpParent`, setting `m_fDescendantMatchFound = true` and notifying `m_cvStatusChanged` on each ancestor. Stops when parent is null or already flagged. |
| `TrySignalParentSubtreeComplete` | private | Checks if all of a parent's children have `m_fSubtreeComplete == true`. If so, sets the parent's `m_fSubtreeComplete`, notifies, and recurses to grandparent. |
| `WaitForTreeVisibility` | private | Given a child `CDirectoryInfo`, waits on `m_cvStatusChanged` until `m_fDescendantMatchFound` or `m_fSubtreeComplete` is true. Returns `true` if visible. |

**Removed**: `HasDescendantFiles` static method (replaced by the above).

### 7. FileComparator (extended)

The existing comparator is extended to support an interleaved sort mode where directories and files are sorted together without grouping.

| Field | Type | Description |
|-------|------|-------------|
| `m_fInterleavedSort` | `bool` | When true, directories and files sort together instead of directories first |

### 8. Abbreviated Size Formatter (new helper)

A new formatting function (or method on the displayer base class) that converts a byte count to Explorer-style abbreviated format with a fixed 7-character width. The numeric portion is right-justified in a 4-character field, followed by a space, followed by the unit label left-justified in a 2-character field.

**Algorithm** (1024-based division, 3 significant digits):

| Range (bytes) | Format | Example | Width |
|---------------|--------|---------|-------|
| 0 | `0 B` | `   0 B ` | 7 |
| 1–999 | `### B` | ` 426 B ` | 7 |
| 1,000–1,023 | `1 KB` | `   1 KB` | 7 |
| 1,024–10,239 | `X.XX KB` | `4.61 KB` | 7 |
| 10,240–102,399 | `XX.X KB` | `17.1 KB` | 7 |
| 102,400–1,048,575 | `### KB` | ` 976 KB` | 7 |
| 1 MB+ | Same 3-sig-digit pattern | `16.7 MB` | 7 |
| 1 GB+ | Same | `1.39 GB` | 7 |
| 1 TB+ | Same | `1.00 TB` | 7 |

**`<DIR>` formatting**: Rendered as `" <DIR>   "` (1 leading space + `<DIR>` + 3 trailing spaces) when abbreviated mode is active, matching the alignment padding used by the Bytes-mode `<DIR>` display path.

**Usage**: Called by `DisplayResultsNormalFileSize` (or a tree-mode override) when `m_eSizeFormat` resolves to `Auto`. The existing comma-separated path is used when it resolves to `Bytes`.

## Relationships

```text
CCommandLine ──parses──> m_fTree, m_cMaxDepth, m_cTreeIndent
     │
     ├─applies─> CConfig (m_fTree, m_cMaxDepth, m_cTreeIndent from TCDIR env var)
     │
     ├─validates─> Switch conflicts (Tree vs Wide/Bare/Recurse; Depth without Tree)
     │
     └─controls─> CMultiThreadedLister::PrintDirectoryTreeMode (lister-driven)
                       │
                       ├─creates─> STreeConnectorState
                       │                │
                       │                ├─Push/Pop per subdirectory
                       │                │
                       │                └─GetPrefix() per entry
                       │
                       ├─calls──> CResultsDisplayerTree (public methods directly)
                       │                │
                       │                ├─> DisplaySingleEntry (per-entry rendering)
                       │                ├─> BeginDirectory (per-directory state setup)
                       │                ├─> DisplayTreeRootHeader / DisplayTreeRootSummary
                       │                ├─> SaveDirectoryState / RestoreDirectoryState
                       │                └─> DisplayFileStreamsWithTreePrefix (streams)
                       │
                       ├─flushes─> CConsole::Flush() before child recursion (streaming output)
                       │
                       ├─checks──> m_cMaxDepth vs STreeConnectorState::Depth()
                       │
                       └─prunes──> m_fTreePruningActive (tree + file mask)
                                       │
                                       ├─ Producer: PropagateDescendantMatch (upward via m_wpParent)
                                       ├─ Producer: TrySignalParentSubtreeComplete (upward via m_wpParent)
                                       └─ Display:  WaitForTreeVisibility (blocks on m_cvStatusChanged)
```

## Tree Connector Visual Format

Each indent level occupies `m_cTreeIndent` characters (default 4). The connector characters are:

| Position | Characters (width 4) | Description |
|----------|---------------------|-------------|
| Middle entry | `├── ` | Vertical+right connector, 2 horizontal dashes, space |
| Last entry | `└── ` | Up+right connector, 2 horizontal dashes, space |
| Continuation (has more siblings) | `│   ` | Vertical line, then spaces to fill width |
| Continuation (no more siblings) | `    ` | Spaces only |
| Stream line | `│   ` | Always vertical continuation within current level |

For `m_cTreeIndent=N`, the horizontal dashes after `├`/`└` are `N-2` characters, with the final character being a space.

### Example Output (depth 2, indent 4, icons active)

Icons are 2 display columns wide (shown below as `■` placeholders). Tree connectors for a directory's children start at the same column as that directory's icon, so `├`/`└`/`│` lines sit directly below the parent folder icon.

```text
                                                       col:  0   4   8
                                                             v   v   v
2026/02/19  10:30 AM  ----A---  1.20 KB  ■ README.md
2026/02/19  10:30 AM  D-------   <DIR>   ■ src/
2026/02/19  10:30 AM  ----A---  5.55 KB  ├── ■ main.cpp
2026/02/19  10:30 AM  D-------   <DIR>   ├── ■ utils/
2026/02/19  10:30 AM  ----A---  2.29 KB  │   ├── ■ helpers.cpp
2026/02/19  10:30 AM  ----A---  1.08 KB  │   └── ■ helpers.h
2026/02/19  10:30 AM  ----A---  3.37 KB  └── ■ app.cpp
2026/02/19  10:30 AM  D-------   <DIR>   ■ tests/
2026/02/19  10:30 AM  ----A---  4.46 KB  └── ■ test_main.cpp
```

**Column alignment** (from start of tree+icon area):

| Item | Icon col | Connector col for children |
|------|----------|---------------------------|
| `src/` (depth 0) | `■` at col 0 | `├`/`└`/`│` at col 0 ✓ |
| `utils/` (depth 1) | `■` at col 4 | `├`/`└`/`│` at col 4 ✓ |

General rule: a directory's icon at depth D starts at column `D × indent_width`. Its children's connectors also start at `D × indent_width`. The vertical tree lines pass directly below the folder icons.

### Example Output (icons not active)

```text
2026/02/19  10:30 AM  ----A---       1,234  README.md
2026/02/19  10:30 AM  D-------      <DIR>   src/
2026/02/19  10:30 AM  ----A---       5,678  ├── main.cpp
2026/02/19  10:30 AM  D-------      <DIR>   ├── utils/
2026/02/19  10:30 AM  ----A---       2,345  │   ├── helpers.cpp
2026/02/19  10:30 AM  ----A---       1,111  │   └── helpers.h
2026/02/19  10:30 AM  ----A---       3,456  └── app.cpp
2026/02/19  10:30 AM  D-------      <DIR>   tests/
2026/02/19  10:30 AM  ----A---       4,567  └── test_main.cpp
```
