# Data Model: Ellipsize Long Link Target Paths

**Date**: 2026-04-19
**Feature**: 008-ellipsize-targets

## Entities

### SEllipsizedPath (new)

Return type from `EllipsizePath()`. Allows the displayer to render prefix and suffix in the source file's color with the `…` in a different color.

| Field | Type | Description |
|-------|------|-------------|
| `prefix` | `wstring` | Path text before the ellipsis (e.g., `C:\Program Files\`). Full path if not truncated. |
| `suffix` | `wstring` | Path text after the ellipsis (e.g., `\python3.12.exe`). Empty if not truncated. |
| `fTruncated` | `bool` | `true` if the path was truncated, `false` if shown in full. |

**Rules:**
- When `fTruncated` is false: `prefix` contains the full path, `suffix` is empty
- When `fTruncated` is true: display is `prefix` + `…` + `suffix`
- The total display length is `prefix.length() + 1 + suffix.length()` when truncated

### CConfig / CCommandLine (modified)

| Field | Type | Default | New? |
|-------|------|---------|------|
| `m_fEllipsize` | `optional<bool>` | nullopt (treated as true) | Yes |

## State Transitions

None. `SEllipsizedPath` is computed per-line during display, not stored.

## Relationships

```
ResultsDisplayerNormal/Tree
    │
    │ Computes available width from metadata column widths
    │
    ▼
EllipsizePath(targetPath, availableWidth)
    │
    ▼
SEllipsizedPath { prefix, suffix, fTruncated }
    │
    ▼
Printf(textAttr, prefix) + Printf(Default, "…") + Printf(textAttr, suffix)
```
