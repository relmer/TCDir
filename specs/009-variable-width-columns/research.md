# Research: Variable-Width Columns in Wide Mode

**Feature**: 009-variable-width-columns  
**Date**: 2026-04-19

## 1. Column Layout Algorithm

### Decision: GNU ls-style incremental column fitting

**Rationale**: GNU ls has solved this problem for decades with a proven O(files × max_cols) algorithm that naturally handles variable-width entries. The algorithm is well-understood, efficient, and matches user expectations from Unix tooling.

**Alternatives considered**:
- **Brute-force per-column recalculation**: Try each column count, compute column widths from scratch each time. Simpler but O(files × max_cols) anyway — same complexity, less elegant.
- **Sorting-based bin packing**: Sort entries by width and pack optimally. More complex, changes entry order (violates FR-003), and provides marginal improvement.

### Algorithm (adapted for TCDir)

```text
function ComputeColumnLayout(entries[], consoleWidth, fEllipsize):
    # Step 1: Compute per-entry display widths
    for each entry:
        displayWidth = strlen(entry.cFileName)
        if (entry is directory && !iconsActive): displayWidth += 2  # [brackets]
        if iconsActive && !iconSuppressed: displayWidth += 2  # icon + space
        if inSyncRoot: displayWidth += 2  # cloud status + space
        entry.displayWidth = displayWidth

    # Step 2: Outlier truncation (when ellipsis enabled)
    if fEllipsize:
        medianWidth = median(all displayWidths)
        truncCap = max(2 × medianWidth, 20)
        for each entry where displayWidth > truncCap:
            entry.truncatedWidth = truncCap
        # Use truncatedWidth for column fitting; display with ellipsis at render time

    # Step 3: Column fitting (GNU ls approach)
    maxCols = min(entryCount, consoleWidth / 2)  # At minimum 2 chars per column
    
    for nCols from maxCols down to 2:
        nRows = ceil(entryCount / nCols)
        colWidths[0..nCols-1] = 0
        
        for each entry at index i:
            col = i / nRows  # column-major mapping
            w = entry.effectiveWidth + (col < nCols-1 ? 1 : 0)  # +1 base gap except last col
            colWidths[col] = max(colWidths[col], w)
        
        totalWidth = sum(colWidths)  # includes base 1-space gaps in non-last columns
        if totalWidth <= consoleWidth:
            # Distribute leftover space evenly across the C-1 inter-column gaps (FR-005)
            # Each column width already includes a base 1-space gap (except last).
            # Leftover = consoleWidth - totalWidth, spread across non-last columns.
            leftover = consoleWidth - totalWidth
            extraPerGap = leftover / (nCols - 1)
            remainder = leftover % (nCols - 1)  # first 'remainder' gaps get +1 more
            return { nCols, nRows, colWidths (adjusted with distributed gaps) }
    
    return { 1 column, entryCount rows, [consoleWidth] }
```

### Column-Major Index Mapping

For column-major order with variable-width columns, the file-to-column mapping is:

```text
Given N entries, C columns, R = ceil(N/C) rows:
  Entry i → column = i / R, row = i % R
  
  When last columns have fewer rows (N not divisible by C):
    fullColumns = N % C (or C if divisible)
    Columns 0..fullColumns-1 have R rows
    Columns fullColumns..C-1 have R-1 rows
```

This matches TCDir's existing column-major ordering logic in `DisplayFileResults`.

## 2. Outlier Detection via Median

### Decision: max(2× median display width, 20) threshold

**Rationale**: The median is robust against skewed distributions. A 2× multiplier means only filenames that are truly extreme outliers get truncated. A floor of 20 prevents aggressive truncation when the median is very small (e.g., a directory of 3-character filenames would otherwise truncate anything over 6 characters).

**Alternatives considered**:
- **Mean + 1σ**: Statistically sound but sensitive to the outliers themselves inflating the mean. A single 100-char name in a directory of 10-char names would raise the threshold unnecessarily.
- **1.5× median**: Too aggressive — would truncate names that are only moderately longer than typical.
- **Fixed cap (40 chars)**: Not adaptive — fails on directories where the median is already 35 chars.

### Median Computation

For N display widths, sort (or use nth_element for O(N)) and take the middle value. For even N, use the lower of the two middle values (simpler, and width is discrete).

## 3. Filename Truncation for Wide Mode

### Decision: Middle-truncate using existing PathEllipsis infrastructure where applicable; simple right-truncate for plain filenames

**Rationale**: `PathEllipsis.h` provides `EllipsizePath()` which middle-truncates paths (keeping prefix + suffix with `…` in the middle). However, in wide mode we're truncating *filenames*, not full paths. For filenames, right-truncation (keeping the beginning, replacing end with `…`) is more natural and simpler.

**Alternatives considered**:
- **Reuse PathEllipsis directly**: The existing function is path-oriented (splits on `\` separators). Filenames don't have separators, so it would degenerate to keeping the full name or just the leaf — not useful.
- **New middle-truncate for filenames**: Over-engineered for filenames where the beginning is most informative.

### Truncation approach

```text
if entry.displayWidth > truncCap:
    visibleChars = truncCap - 1  # Reserve 1 char for ellipsis (…)
    truncatedName = entry.name[0..visibleChars-1] + "…"
```

The `…` (U+2026 HORIZONTAL ELLIPSIS) is already available in `UnicodeSymbols.h`.

## 4. Gap Distribution

### Decision: Distribute leftover space evenly across inter-column gaps; no trailing whitespace on last column

**Rationale**: Produces a balanced, readable layout. With C columns there are C-1 inter-column gaps. Leftover pixels are distributed round-robin (first gaps get +1 extra if not evenly divisible).

**Implementation**:
```text
baseGap = 1 + (leftover / (nCols - 1))     # Each gap gets at least 1 + even share
bonusGaps = leftover % (nCols - 1)           # First 'bonusGaps' gaps get +1 more
```
