# Quickstart: Variable-Width Columns in Wide Mode

**Feature**: 009-variable-width-columns  
**Date**: 2026-04-19

## What Changed

Wide mode (`tc /W`) now uses variable-width columns. Instead of making all columns the same width based on the longest filename, each column is only as wide as its widest entry. Leftover space is distributed evenly between columns.

## Behavior Summary

| Scenario | Before | After |
|----------|--------|-------|
| Mixed-length filenames (e.g., 95% short, 5% long) | Single column or very few columns (uniform width based on longest) | Multiple columns; short-name columns are narrow, long-name columns are wider |
| All filenames same length | N uniform columns | N uniform columns (identical output) |
| One extreme outlier filename | Forces 1 column for entire listing | Outlier truncated with `…`; remaining entries fit more columns |

## Outlier Truncation

Filenames exceeding **2× the median display width** are automatically truncated with an ellipsis character (`…`). This prevents a single long name from collapsing the entire listing to one column.

To disable truncation: `tc /W --Ellipsize-`

## Examples

### Before (uniform width)
```
Microsoft.WindowsPackageManagerManifestCreator_8wekyb3d8bbwe
docs
src
README.md
build
```
*One column because the longest name (63 chars) consumes the full width.*

### After (variable width)
```
Microsoft.WindowsPackag…   docs         src          build
README.md
```
*Outlier truncated; remaining entries fit into multiple columns.*

### With --Ellipsize- (no truncation)
```
Microsoft.WindowsPackageManagerManifestCreator_8wekyb3d8bbwe   docs
README.md                                                       src
build
```
*Outlier not truncated; algorithm still optimizes per-column widths for 2 columns instead of uniform's 1.*

## Column Ordering

Column-major order is preserved (top-to-bottom, then left-to-right) — same as current behavior.

## No Impact On

- Normal listing mode (default)
- Bare listing mode (`/B`)
- Tree mode (`/T`)
- Sort order
- File icons or cloud status indicators (correctly accounted for in width)
