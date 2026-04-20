te# Feature Specification: Variable-Width Columns in Wide Mode

**Feature Branch**: `009-variable-width-columns`  
**Created**: 2026-04-19  
**Status**: Draft  
**Input**: User description: "Implement variable-width columns for wide mode (/W) to improve space utilization when a directory contains a mix of short and long filenames. (GH Issue #10)"

## Clarifications

### Session 2026-04-19

- Q: How should leftover horizontal space be distributed after variable-width columns are computed? → A: Distribute leftover space evenly across inter-column gaps for a balanced appearance; no trailing whitespace on the last column.
- Q: Should per-column width calculation use per-entry display width (including directory brackets when icons are off)? → A: Yes, use per-entry display width including brackets.
- Q: Should outlier-length filenames be truncated to allow more columns? → A: Yes, truncate outliers with ellipsis by default; disable truncation when `--Ellipsize-` is specified.
- Q: What threshold triggers outlier truncation in wide mode? → A: `max(2× median display width, 20)`; filenames exceeding this are truncated with ellipsis. The floor of 20 prevents aggressive truncation in directories of very short names.
- Q: Should there be a minimum column count gate before truncation kicks in? → A: No; always truncate outliers exceeding 2× median regardless of column count.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Better Space Utilization in Wide Mode (Priority: P1)

As a user running `tc /W` on a directory where most filenames are short but a few are very long, I want the output to use variable-width columns so that short names don't waste horizontal space, allowing more columns and more filenames visible per screen.

**Why this priority**: This is the core value proposition of the feature. The current uniform-width approach wastes the majority of terminal width when even one long filename is present, sometimes forcing single-column output when dozens of entries would comfortably fit in multiple columns.

**Independent Test**: Can be fully tested by running `tc /W` on a directory with mixed-length filenames (e.g., a directory containing both short names like `docs` and long names like `Microsoft.WindowsPackageManagerManifestCreator_8wekyb3d8bbwe`) and verifying the output uses more columns than the current uniform-width layout.

**Acceptance Scenarios**:

1. **Given** a directory with filenames of varying lengths and a 120-character-wide terminal, **When** the user runs `tc /W`, **Then** short filenames are displayed in narrower columns and long filenames in wider columns, fitting more entries per row than uniform-width would allow.
2. **Given** a directory where one filename is 63 characters and the rest are under 30 characters in a 126-character terminal, **When** the user runs `tc /W`, **Then** the output displays at least 2 columns (instead of the current single-column result).
3. **Given** a directory where all filenames are roughly the same length, **When** the user runs `tc /W`, **Then** the output is visually identical to the current behavior (all columns equal width).

---

### User Story 2 - Column-Major Ordering Preserved (Priority: P1)

As a user, I want the variable-width column layout to continue listing entries in column-major order (top-to-bottom, then left-to-right), so the reading order remains consistent with the current behavior.

**Why this priority**: Changing the reading order would break user expectations and muscle memory. This is a hard constraint, not a nice-to-have.

**Independent Test**: Can be verified by running `tc /W` and confirming that entries fill columns top-to-bottom before moving to the next column.

**Acceptance Scenarios**:

1. **Given** a directory with 10 entries and a layout that fits 3 columns, **When** the user runs `tc /W`, **Then** entries 1–4 appear in column 1, entries 5–7 in column 2, and entries 8–10 in column 3 (top-to-bottom fill).

---

### User Story 3 - Correct Width Accounting for Icons and Cloud Status (Priority: P2)

As a user with file icons or cloud status indicators enabled, I want the column width calculation to correctly account for per-entry icon and cloud status widths, so columns are neither too narrow (truncating) nor too wide (wasting space).

**Why this priority**: Icon and cloud status indicators add visual width to each entry. If variable-width calculation ignores these, columns may overflow or entries may be clipped. This is essential for correctness but secondary to the core algorithm.

**Independent Test**: Can be tested by running `tc /W` with icons enabled (`/I`) and verifying that icon-bearing entries display without truncation or misalignment.

**Acceptance Scenarios**:

1. **Given** icons are enabled and a directory has entries with icons, **When** the user runs `tc /W`, **Then** each column's width accounts for the icon width (+2 characters) on a per-entry basis, not globally.
2. **Given** cloud status is enabled and some entries have cloud status indicators, **When** the user runs `tc /W`, **Then** each column's width accounts for the cloud status indicator (+2 characters) only for entries that display it.

---

### Edge Cases

- What happens when the terminal width is very narrow (e.g., 40 characters)? The algorithm should gracefully fall back to single-column output.
- What happens when a single filename is wider than the terminal? That entry should still be displayed (on its own line if necessary), without crashing or corrupting output.
- What happens with an empty directory? No output is produced (existing behavior preserved).
- What happens when all filenames are the exact same length? Output should match the current uniform-width behavior exactly.
- What happens when there is exactly one file? Single-column, single-row output (no change from current behavior).
- What happens when outlier truncation is active and a filename is truncated? The truncated name displays with an ellipsis character (…) indicating omission, and the full name is not recoverable from the output (users can use normal listing mode to see full names).

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST calculate per-column widths based on the actual entries assigned to each column, rather than using a single uniform width for all columns.
- **FR-002**: The system MUST try candidate column counts from the maximum feasible down to 1, selecting the highest count whose total width fits within the console width.
- **FR-003**: The system MUST preserve column-major ordering (entries fill top-to-bottom within each column before moving to the next column).
- **FR-004**: The system MUST compute each column's required width using per-entry display width, which includes directory brackets (`[name]` when icons are off), icon glyphs (+2), and cloud status indicators (+2) as applicable.
- **FR-005**: The system MUST distribute leftover horizontal space evenly across inter-column gaps (after computing each column's minimum required width). The last column MUST NOT emit trailing whitespace.
- **FR-006**: The system MUST fall back to single-column output when no multi-column layout fits within the terminal width.
- **FR-007**: The system MUST produce output identical to the current uniform-width behavior when all filenames have the same display width.
- **FR-008**: By default (ellipsis enabled), the system MUST truncate filenames whose display width exceeds the outlier threshold, replacing the truncated portion with an ellipsis character (…), to allow more columns. The outlier threshold is `max(2 × median display width, 20)` — the floor of 20 prevents aggressive truncation in directories of very short filenames. When `--Ellipsize-` is specified, no truncation occurs and the algorithm accepts whatever column count fits.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: For a directory containing 100 entries where 95% of filenames are under 30 characters and 5% are over 50 characters, wide mode displays at least 50% more columns than the current uniform-width approach at 120-character terminal width.
- **SC-002**: Users can scan wide mode output without encountering column misalignment or entries that overflow their column boundary. (Intentional outlier truncation via FR-008 is expected, not a defect.)
- **SC-003**: The column layout calculation completes imperceptibly fast — users perceive no difference in command responsiveness compared to the current uniform-width calculation.
- **SC-004**: Existing wide mode output for directories with uniform-length filenames remains visually unchanged.

## Assumptions

- The change is limited to the wide mode display path; no other display modes (normal, bare, tree) are affected.
- Column-major ordering (top-to-bottom, left-to-right) is the only supported ordering; row-major is not in scope.
- The algorithm runs once per directory listing, not per file, so performance impact is negligible.
- The maximum feasible column count is bounded by the console width and the minimum possible entry width (1 character + padding), which naturally limits the search space.
- The existing sort order of entries is preserved; the variable-width algorithm only changes how entries are arranged spatially, not their order.
