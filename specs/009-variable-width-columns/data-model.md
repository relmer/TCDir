# Data Model: Variable-Width Columns in Wide Mode

**Feature**: 009-variable-width-columns  
**Date**: 2026-04-19

## Entities

### SColumnLayout (new)

Result of the column-fitting algorithm. Produced once per directory listing.

| Field | Type | Description |
|-------|------|-------------|
| `cColumns` | `size_t` | Number of columns in the layout |
| `cRows` | `size_t` | Number of rows |
| `vColumnWidths` | `vector<size_t>` | Per-column display width (includes entry width + gap for all except last column) |

### Per-Entry Display Width (computed, not stored)

Each entry's effective display width is computed during the column-fitting pass. This is **not** a new stored field — it is derived on-the-fly from `WIN32_FIND_DATA` attributes and display context.

| Component | Width | Condition |
|-----------|-------|-----------|
| Filename (`cFileName`) | `wcslen(cFileName)` | Always |
| Directory brackets | +2 | `FILE_ATTRIBUTE_DIRECTORY` and icons off |
| Icon glyph + space | +2 | Icons active and `!style.m_fIconSuppressed` |
| Cloud status + space | +2 | In a sync root |

**Note**: The `m_fIconSuppressed` field on `CConfig::SFileDisplayStyle` controls whether a file type's icon is hidden. When suppressed, the +2 icon width MUST NOT be added.

### Outlier Truncation State (computed, not stored)

When ellipsis is enabled and an entry's display width exceeds 2× median:

| Field | Type | Description |
|-------|------|-------------|
| `medianDisplayWidth` | `size_t` | Median of all entry display widths |
| `truncCap` | `size_t` | `max(2 × medianDisplayWidth, 20)` |

Truncation is applied at render time in `DisplayFile`, not pre-computed on the data model.

## Relationships

```text
CDirectoryInfo
  └─ m_vMatches: vector<FileInfo>    (existing, read-only input)
       └─ derives from WIN32_FIND_DATA (cFileName, dwFileAttributes)

CResultsDisplayerWide
  └─ ComputeColumnLayout()  → SColumnLayout  (new pure function)
  └─ ComputeDisplayWidth()  → size_t          (new helper)
  └─ DisplayFileResults()   → uses SColumnLayout instead of uniform cxColumnWidth
  └─ DisplayFile()          → truncates outlier names at render time
```

## State Transitions

None — this feature is stateless display logic. The column layout is computed fresh for each directory listing and discarded after rendering.
