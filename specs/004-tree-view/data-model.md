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

**Validation rules**:
- `m_fTree` + `m_fWideListing` → error
- `m_fTree` + `m_fBareListing` → error
- `m_fTree` + `m_fRecurse` → error
- `m_cMaxDepth > 0` without `m_fTree` → error
- `m_cTreeIndent` outside [1, 8] → error
- `m_cTreeIndent != 4` without `m_fTree` → error
- `m_cMaxDepth ≤ 0` when explicitly specified → error (default value of 0 means unlimited; the parser rejects user-supplied values ≤ 0 before storage)

### 2. CConfig (extended)

Existing class, extended with new optional members and a new color attribute.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `m_fTree` | `optional<bool>` | `nullopt` | `Tree` / `Tree-` in TCDIR env var |
| `m_cMaxDepth` | `optional<int>` | `nullopt` | `Depth=N` in TCDIR env var |
| `m_cTreeIndent` | `optional<int>` | `nullopt` | `TreeIndent=N` in TCDIR env var |

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
- `DisplayCloudStatusSymbol`, `DisplayRawAttributes`, `DisplayFileOwner`, `GetFileOwner`, `GetFileOwners`
- All `CResultsDisplayerWithHeaderAndFooter` base helpers (`DisplayDriveHeader`, `DisplayDirectorySummary`, `DisplayListingSummary`, `FormatNumberWithSeparators`, etc.)

| Method | Override | Description |
|--------|----------|-------------|
| `DisplayResults` | Yes (from `WithHeaderAndFooter`) | Tree-walking flow: drive header → recursive tree traversal (no per-subdir path headers); indented per-dir summaries; grand total |
| `DisplayFileResults` | Yes (from `Normal`) | Same column sequence as Normal but prepends tree connector prefix before icon/filename; modified stream continuation with `│` prefix |
| `DisplayTreeEntry` | New (private helper) | Internal helper called by `DisplayFileResults`: renders one file line — calls inherited column helpers, inserts tree prefix from `STreeConnectorState`, then icon + filename |
| `DisplayFileStreamsWithTreePrefix` | New | Like inherited `DisplayFileStreams` but prepends tree continuation prefix (`│   `) to each stream line |

**Pruning behavior**: When file masks are active, `DisplayResults` applies shallow pruning during tree traversal — leaf directories with zero matching files and no matching descendants are skipped. Intermediate directories are always rendered to preserve tree structure (FR-015).

### 5. FileComparator (extended)

The existing comparator is extended to support an interleaved sort mode where directories and files are sorted together without grouping.

| Field | Type | Description |
|-------|------|-------------|
| `m_fInterleavedSort` | `bool` | When true, directories and files sort together instead of directories first |

## Relationships

```text
CCommandLine ──parses──> m_fTree, m_cMaxDepth, m_cTreeIndent
     │
     ├─applies─> CConfig (m_fTree, m_cMaxDepth, m_cTreeIndent from TCDIR env var)
     │
     ├─validates─> Switch conflicts (Tree vs Wide/Bare/Recurse; Depth without Tree)
     │
     └─controls─> CMultiThreadedLister::PrintDirectoryTree
                       │
                       ├─creates─> STreeConnectorState
                       │                │
                       │                ├─Push/Pop per subdirectory
                       │                │
                       │                └─GetPrefix() per entry
                       │
                       ├─passes──> CResultsDisplayerTree::DisplayResults (tree flow)
                       │                │
                       │                └─> CResultsDisplayerTree::DisplayFileResults
                       │                        │
                       │                        ├─ Calls inherited column helpers (date, attrs, size, ...)
                       │                        ├─ Prepends tree prefix before icon/filename
                       │                        └─ Uses DisplayFileStreamsWithTreePrefix() for streams
                       │
                       └─checks──> m_cMaxDepth vs STreeConnectorState::Depth()
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
2026/02/19  10:30 AM  ----A---       1,234  ■ README.md
2026/02/19  10:30 AM  D-------      <DIR>   ■ src/
2026/02/19  10:30 AM  ----A---       5,678  ├── ■ main.cpp
2026/02/19  10:30 AM  D-------      <DIR>   ├── ■ utils/
2026/02/19  10:30 AM  ----A---       2,345  │   ├── ■ helpers.cpp
2026/02/19  10:30 AM  ----A---       1,111  │   └── ■ helpers.h
2026/02/19  10:30 AM  ----A---       3,456  └── ■ app.cpp
2026/02/19  10:30 AM  D-------      <DIR>   ■ tests/
2026/02/19  10:30 AM  ----A---       4,567  └── ■ test_main.cpp
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
