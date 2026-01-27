# Feature Specification: CMD Dir Compatibility & Cloud File Visualization

**Feature Branch**: `001-dir-compat-cloud`  
**Created**: 2026-01-24  
**Status**: Draft  
**Input**: User description: "Add CMD dir compatibility features and cloud file visualization to TCDir"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Visualize Cloud Sync Status (Priority: P1)

As a user browsing a cloud-synced folder, I want to see visual indicators showing each file's sync status so I can quickly understand which files are available offline without checking each one individually.

**Why this priority**: Replicates File Explorer's familiar iconography, making TCDir a first-class citizen for cloud folder browsing. High visibility feature that differentiates TCDir.

**Independent Test**: Run `tcdir` in a OneDrive folder and observe cloud status symbols next to each filename.

**Acceptance Scenarios**:

1. **Given** a cloud-only placeholder file, **When** displayed in TCDir, **Then** a hollow circle symbol (○) appears in the configured color (default: bright blue)
2. **Given** a locally available file in a sync root (can be dehydrated), **When** displayed in TCDir, **Then** a half-filled circle symbol (◐) appears in the configured color (default: bright green)
3. **Given** a pinned (always available) file, **When** displayed in TCDir, **Then** a solid circle symbol (●) appears in the configured color (default: green)
4. **Given** a file not in a cloud sync root and with no cloud attributes, **When** displayed in TCDir, **Then** no cloud status symbol is displayed
5. **Given** bare mode (`/B`) is enabled, **When** displaying cloud-synced files, **Then** cloud status symbols are suppressed

---

### User Story 2 - Filter Files by Cloud Sync Status (Priority: P1)

As a user with OneDrive or iCloud synced folders, I want to filter directory listings by cloud sync status so I can quickly find files that are only in the cloud, locally available, or pinned for offline use.

**Why this priority**: Cloud storage is ubiquitous (OneDrive, iCloud). Users frequently need to identify which files are available offline before traveling or working without internet. This delivers immediate, visible value.

**Independent Test**: Run `tcdir /A:O` in a OneDrive folder to list only cloud-only placeholder files. Run `tcdir /A:F` to list only pinned files. Each filter works independently.

**Acceptance Scenarios**:

1. **Given** a OneDrive folder with mixed cloud-only and local files, **When** user runs `tcdir /A:O`, **Then** only cloud-only placeholder files are displayed
2. **Given** a OneDrive folder with pinned files, **When** user runs `tcdir /A:F`, **Then** only pinned (always available) files are displayed
3. **Given** a OneDrive folder, **When** user runs `tcdir /A:-O`, **Then** only locally available files are displayed (excludes cloud-only)
4. **Given** a regular folder with no cloud files, **When** user runs `tcdir /A:O`, **Then** no files are displayed (filter finds nothing)

---

### User Story 3 - Select Time Field for Display and Sorting (Priority: P2)

As a user, I want to choose which timestamp to display (creation, last access, or last modified) so I can analyze files based on the time metric most relevant to my task.

**Why this priority**: CMD `dir` compatibility feature. Useful for finding recently accessed files or sorting by creation date, but less commonly needed than cloud features.

**Independent Test**: Run `tcdir /T:C` to display creation times. Run `tcdir /T:A /O:D` to sort by last access time.

**Acceptance Scenarios**:

1. **Given** default behavior, **When** user runs `tcdir`, **Then** last modified time (W) is displayed
2. **Given** `/T:C` switch, **When** user runs `tcdir /T:C`, **Then** creation time is displayed for each file
3. **Given** `/T:A` switch, **When** user runs `tcdir /T:A`, **Then** last access time is displayed for each file
4. **Given** `/T:C` combined with `/O:D`, **When** user runs `tcdir /T:C /O:D`, **Then** files are sorted by creation time (oldest first)
5. **Given** `/T:A` combined with `/O:-D`, **When** user runs `tcdir /T:A /O:-D`, **Then** files are sorted by last access time (newest first)

---

### User Story 4 - Filter by Extended File Attributes (Priority: P2)

As a power user, I want to filter files by additional Windows file attributes (not content indexed, ReFS integrity, ReFS no-scrub) so I can manage files with specific storage characteristics.

**Why this priority**: Completes CMD `dir /A:` compatibility and adds ReFS support. Useful for system administrators and power users managing storage.

**Independent Test**: Run `tcdir /A:X` to list files excluded from Windows Search indexing. Run `tcdir /A:I` on a ReFS volume to list files with integrity streams.

**Acceptance Scenarios**:

1. **Given** files with NOT_CONTENT_INDEXED attribute, **When** user runs `tcdir /A:X`, **Then** only those files are displayed
2. **Given** ReFS volume with integrity-enabled files, **When** user runs `tcdir /A:I`, **Then** only files with integrity streams are displayed
3. **Given** ReFS volume with no-scrub files, **When** user runs `tcdir /A:B`, **Then** only files with no-scrub attribute are displayed
4. **Given** any attribute filter, **When** user prefixes with `-`, **Then** files with that attribute are excluded

---

### User Story 5 - Display File Ownership (Priority: P3)

As a system administrator, I want to see which user owns each file so I can audit permissions and identify orphaned files without using separate tools.

**Why this priority**: CMD `dir /Q` compatibility. Useful but niche; requires slow security API calls per file. Opt-in only.

**Independent Test**: Run `tcdir --owner` and verify owner usernames appear for each file.

**Acceptance Scenarios**:

1. **Given** `--owner` switch, **When** user runs `tcdir --owner`, **Then** owner username (DOMAIN\User format) is displayed for each file
2. **Given** a file owned by SYSTEM, **When** displayed with `--owner`, **Then** "NT AUTHORITY\SYSTEM" or similar is shown
3. **Given** `--owner` is NOT specified and not in TCDIR env var, **When** user runs `tcdir`, **Then** no ownership information is displayed
4. **Given** TCDIR environment variable includes `--owner`, **When** user runs `tcdir`, **Then** ownership IS enabled (user explicitly opted in via env var)

---

### User Story 6 - Display Alternate Data Streams (Priority: P3)

As a power user or security analyst, I want to see NTFS alternate data streams so I can identify hidden data attached to files.

**Why this priority**: CMD `dir /R` compatibility. Niche use case for security analysis. Requires slow stream enumeration API. Opt-in only.

**Independent Test**: Run `tcdir --streams` on a file with ADS and verify streams are listed.

**Acceptance Scenarios**:

1. **Given** a file with alternate data streams, **When** user runs `tcdir --streams`, **Then** each stream name and size is displayed
2. **Given** a file with only the default `::$DATA` stream, **When** displayed with `--streams`, **Then** no additional streams are shown
3. **Given** `--streams` is NOT specified and not in TCDIR env var, **When** user runs `tcdir`, **Then** no stream information is displayed
4. **Given** TCDIR environment variable includes `--streams`, **When** user runs `tcdir`, **Then** streams ARE enabled (user explicitly opted in via env var)

---

### Edge Cases

- What happens when using `--streams` on a non-NTFS volume? (**Warning** — display message that option is ignored because filesystem doesn't support ADS, then continue normally)
- What happens when using `/A:I` or `/A:B` on a non-ReFS volume? (**Warning** — display message that option is ignored because filesystem doesn't support these attributes, then continue normally)
- What happens when file ownership lookup fails (access denied)? (Display placeholder like "Unknown" or skip owner column for that file)
- What happens when a cloud placeholder file is being hydrated during listing? (Display current state; may show cloud-only)
- What happens with `/T:A` when volume has access time updates disabled? (Display available timestamp; may show zeros or stale data)

## Requirements *(mandatory)*

### Functional Requirements

#### New Command-Line Switches

- **FR-001**: System MUST support `--owner` switch to display file ownership (DOMAIN\User format)
- **FR-002**: System MUST support `/T:C|A|W` switch to select time field (Creation, Access, Written)
- **FR-003**: System MUST support `--streams` switch to display NTFS alternate data streams
- **FR-004**: `--owner` and `--streams` MUST NOT be enabled by default; MAY be enabled via TCDIR environment variable if user explicitly configures it
- **FR-005**: `--streams` MUST display a warning that the option is ignored if target volume is not NTFS, then continue processing

#### New /A: Attribute Filters

- **FR-006**: System MUST support `/A:X` for NOT_CONTENT_INDEXED attribute filtering
- **FR-007**: System MUST support `/A:O` as composite filter matching OFFLINE OR RECALL_ON_DATA_ACCESS OR RECALL_ON_OPEN
- **FR-008**: System MUST support `/A:I` for INTEGRITY_STREAM attribute filtering (ReFS only)
- **FR-009**: System MUST support `/A:B` for NO_SCRUB_DATA attribute filtering (ReFS only)
- **FR-010**: System MUST support `/A:F` for PINNED attribute filtering (OneDrive always-available)
- **FR-011**: System MUST support `/A:U` for UNPINNED attribute filtering (can be dehydrated)
- **FR-012**: All new attribute filters MUST support negation prefix (`-`) like existing attributes
- **FR-013**: `/A:I` and `/A:B` MUST display a warning that the option is ignored if target volume is not ReFS, then continue processing

#### Cloud Status Visualization

- **FR-014**: System MUST display cloud status symbols for files with cloud-related attributes
- **FR-015**: Cloud-only files (RECALL_* attributes) MUST display ☁ symbol; default color: bright blue (`FOREGROUND_BLUE | FOREGROUND_INTENSITY`), configurable via TCDIR env var
- **FR-016**: Locally available files (UNPINNED, no RECALL) MUST display ✓ symbol; default color: bright green (`FOREGROUND_GREEN | FOREGROUND_INTENSITY`), configurable via TCDIR env var
- **FR-017**: Pinned files (PINNED attribute) MUST display ● symbol; default color: green (`FOREGROUND_GREEN`), configurable via TCDIR env var
- **FR-018**: Files with no cloud attributes MUST NOT display any cloud status symbol
- **FR-019**: Cloud status symbols MUST be suppressed in bare mode (`/B`)
- **FR-020**: Cloud status MUST be displayed in a dedicated column between file size and filename

#### Documentation

- **FR-021**: README.md MUST be updated with all new switches and attributes
- **FR-022**: Help text (`/?`) MUST document all new switches, attributes, and cloud symbols
- **FR-023**: Environment variable help (`--Env`) MUST document any applicable new options
- **FR-024**: Config display (`--Config`) MUST show cloud status colors if configurable

### Key Entities

- **Cloud Status**: Derived state from combination of OFFLINE, RECALL_ON_DATA_ACCESS, RECALL_ON_OPEN, PINNED, UNPINNED attributes
- **Time Field**: Enumeration of Creation (C), Access (A), Written (W) affecting display and sort
- **Alternate Data Stream**: Named stream within NTFS file, with name and size

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Users can filter OneDrive/iCloud folders by cloud status using `/A:O`, `/A:F`, `/A:U` switches
- **SC-002**: Cloud status symbols (☁/✓/●) are visible and correctly colored when browsing cloud-synced folders
- **SC-003**: Users can switch between creation, access, and modified times using `/T:C|A|W`
- **SC-004**: All new `/A:` filters work correctly with negation prefix
- **SC-005**: `--owner` displays correct ownership for files where user has read permissions
- **SC-006**: `--streams` correctly enumerates alternate data streams on NTFS volumes
- **SC-007**: Performance-intensive features (`--owner`, `--streams`) are not enabled by default but can be enabled via environment variable
- **SC-008**: All new features are documented in `/?` help output
- **SC-009**: README.md accurately describes all new capabilities
- **SC-010**: TCDir continues to build and pass all existing unit tests after changes
- **SC-011**: All new command-line switches and attribute filters MUST have corresponding unit tests in the `UnitTest/` project

## Assumptions

- Users have Windows 10 version 1709 or later (Cloud Files API support)
- OneDrive and iCloud use Windows Cloud Files API and set standard placeholder attributes
- `--streams` only produces output on NTFS volumes; other filesystems do not support alternate data streams
- Terminal supports UTF-8/Unicode for cloud status symbols (Windows Terminal, ConEmu, modern cmd.exe with UTF-8 codepage). No ASCII fallback provided; users with legacy terminals see raw Unicode codepoints.
- Existing TCDir color configuration infrastructure can be extended for cloud status colors
