# Application Specification: TCDir (Technicolor Directory)

**Spec Branch**: `002-full-app-spec`  
**Created**: 2026-02-07  
**Status**: Reverse-Engineered from Source  
**Version**: 4.2.x  
**Input**: Comprehensive specification reverse-engineered from TCDir source code

## Overview

TCDir ("Technicolor Directory") is a fast, colorized directory listing tool for Windows consoles. It provides a practical `dir`-style command with useful defaults including color-coded output by extension and attributes, sorting, recursion, wide output, and multi-threaded directory enumeration.

### Target Platforms

- Windows 10/11
- Architectures: x64, ARM64

### Design Philosophy

- **Familiar**: CMD `dir` compatibility with similar switches and syntax
- **Visual**: Color-coded output for rapid file identification
- **Fast**: Multi-threaded enumeration for large directory trees
- **Cloud-aware**: Native support for OneDrive/iCloud sync status visualization
- **Configurable**: Environment variable customization of colors and defaults

---

## User Scenarios & Testing *(mandatory)*

### User Story 1 - View Directory Contents with Visual Differentiation (Priority: P1)

As a developer or power user, I want to view directory contents with visual color coding so I can quickly identify file types, attributes, and directories without reading each entry.

**Why this priority**: This is the core value proposition of TCDir - colorized directory listings that improve file identification speed.

**Independent Test**: Run `tcdir` in any directory with mixed file types and verify color differentiation.

**Acceptance Scenarios**:

1. **Given** a directory with files of various extensions, **When** user runs `tcdir`, **Then** each file type is displayed in a distinct configured color
2. **Given** a directory with subdirectories, **When** user runs `tcdir`, **Then** directories are visually distinguished from files (different color)
3. **Given** system/hidden files in a directory, **When** user runs `tcdir /a:hs`, **Then** hidden and system files are displayed with their attribute-specific colors
4. **Given** the default console, **When** user runs `tcdir`, **Then** output includes: file date/time, file size, attributes, and colorized filename
5. **Given** any directory, **When** user runs `tcdir`, **Then** directory header shows volume label, drive info, and total bytes available

---

### User Story 2 - Sort Directory Listings (Priority: P1)

As a user managing files, I want to sort directory listings by various criteria (name, extension, size, date) so I can quickly find files matching my needs.

**Why this priority**: Sorting is fundamental to directory management and file discovery.

**Independent Test**: Run `tcdir /o:s` to sort by size, `tcdir /o:d` to sort by date, verify correct ordering.

**Acceptance Scenarios**:

1. **Given** files of various sizes, **When** user runs `tcdir /o:s`, **Then** files are sorted smallest to largest
2. **Given** files with various names, **When** user runs `tcdir /o:n`, **Then** files are sorted alphabetically by name
3. **Given** files with various extensions, **When** user runs `tcdir /o:e`, **Then** files are sorted alphabetically by extension
4. **Given** files with various timestamps, **When** user runs `tcdir /o:d`, **Then** files are sorted oldest to newest
5. **Given** any sort option, **When** user prepends `-` (e.g., `/o:-d`), **Then** sort order is reversed
6. **Given** both `/o-s` and `/o:-s` syntax, **When** either is used, **Then** both forms are accepted and produce identical results

---

### User Story 3 - Filter Files by Attributes (Priority: P1)

As a user searching for specific files, I want to filter directory listings by file attributes so I can focus on relevant files without visual clutter.

**Why this priority**: Attribute filtering enables targeted file discovery and is a core CMD `dir` compatibility feature.

**Independent Test**: Run `tcdir /a:d` to show only directories, `tcdir /a:-d` to exclude directories.

**Acceptance Scenarios**:

1. **Given** a directory with mixed files and directories, **When** user runs `tcdir /a:d`, **Then** only directories are displayed
2. **Given** hidden files exist, **When** user runs `tcdir /a:h`, **Then** only hidden files are displayed
3. **Given** system files exist, **When** user runs `tcdir /a:s`, **Then** only system files are displayed
4. **Given** read-only files exist, **When** user runs `tcdir /a:r`, **Then** only read-only files are displayed
5. **Given** any attribute filter, **When** user prefixes with `-` (e.g., `/a:-h`), **Then** files with that attribute are excluded
6. **Given** multiple attributes specified (e.g., `/a:hs`), **When** user runs the command, **Then** files matching ANY specified attribute are included

---

### User Story 4 - Recursive Directory Listing (Priority: P1)

As a user exploring file hierarchies, I want to recursively list all subdirectories so I can see the complete directory tree structure.

**Why this priority**: Recursive listing is essential for understanding project structures and finding files across nested directories.

**Independent Test**: Run `tcdir /s` in a directory with subdirectories and verify all levels are displayed.

**Acceptance Scenarios**:

1. **Given** a directory with nested subdirectories, **When** user runs `tcdir /s`, **Then** all subdirectory contents are listed hierarchically
2. **Given** recursive mode, **When** each subdirectory is processed, **Then** a header line shows the current directory path
3. **Given** recursive mode completes, **When** all directories are processed, **Then** a summary shows total files found, total bytes, and directories traversed
4. **Given** file masks (e.g., `*.cpp`), **When** combined with `/s`, **Then** only matching files are shown across all subdirectories
5. **Given** multi-threading is enabled (default), **When** recursive listing runs, **Then** multiple directories are enumerated in parallel for faster completion

---

### User Story 5 - Wide Listing Format (Priority: P2)

As a user viewing large directories, I want a compact wide listing format so I can see more files on screen without scrolling.

**Why this priority**: Wide format improves information density for directories with many files.

**Independent Test**: Run `tcdir /w` and verify multi-column output.

**Acceptance Scenarios**:

1. **Given** wide mode is enabled, **When** user runs `tcdir /w`, **Then** files are displayed in multiple columns fitting console width
2. **Given** wide mode, **When** displaying directories, **Then** directory names are enclosed in brackets `[dirname]`
3. **Given** wide mode, **When** column count is calculated, **Then** it adapts to the current console window width
4. **Given** wide mode, **When** files have varying name lengths, **Then** column width accommodates the longest filename in the directory

---

### User Story 6 - Bare Listing Format (Priority: P2)

As a user scripting file operations, I want a bare listing format (filenames only) so I can pipe output to other commands or scripts.

**Why this priority**: Bare format enables integration with shell pipelines and automation scripts.

**Independent Test**: Run `tcdir /b` and verify output contains only file paths with no headers/footers.

**Acceptance Scenarios**:

1. **Given** bare mode is enabled, **When** user runs `tcdir /b`, **Then** only file/directory paths are displayed (one per line)
2. **Given** bare mode, **When** output is generated, **Then** no headers, footers, volume info, or summaries are included
3. **Given** bare mode with recursion (`/s`), **When** listing occurs, **Then** full paths are displayed for each file
4. **Given** bare mode, **When** cloud-synced files are listed, **Then** no cloud status symbols are displayed

---

### User Story 7 - Time Field Selection (Priority: P2)

As a user analyzing file activity, I want to choose which timestamp to display (creation, access, modified) so I can sort and view files by the relevant time metric.

**Why this priority**: Different timestamps serve different analytical needs (when created vs. when last used vs. when changed).

**Independent Test**: Run `tcdir /t:c` to show creation times, `tcdir /t:a` to show access times.

**Acceptance Scenarios**:

1. **Given** default behavior, **When** user runs `tcdir`, **Then** last modified time (W) is displayed
2. **Given** `/t:c` switch, **When** user runs the command, **Then** file creation time is displayed
3. **Given** `/t:a` switch, **When** user runs the command, **Then** last access time is displayed
4. **Given** `/t:w` switch, **When** user runs the command, **Then** last modified time is displayed
5. **Given** time field combined with date sort (`/t:c /o:d`), **When** sorting occurs, **Then** files are sorted by the selected time field

---

### User Story 8 - Cloud Sync Status Visualization (Priority: P2)

As a user with cloud-synced folders (OneDrive, iCloud), I want to see visual indicators of sync status so I can understand which files are available offline.

**Why this priority**: Cloud storage is ubiquitous and users need to know file availability before going offline.

**Independent Test**: Run `tcdir` in a OneDrive folder and observe cloud status symbols.

**Acceptance Scenarios**:

1. **Given** a cloud-only placeholder file, **When** displayed, **Then** a hollow circle symbol (○) appears in the configured color
2. **Given** a locally available file (can be dehydrated), **When** displayed, **Then** a half-filled circle symbol (◐) appears
3. **Given** a pinned (always available) file, **When** displayed, **Then** a solid circle symbol (●) appears
4. **Given** a file not in a cloud sync root, **When** displayed, **Then** no cloud status symbol appears
5. **Given** cloud files in a folder, **When** user runs `tcdir /a:o`, **Then** only cloud-only placeholder files are shown
6. **Given** cloud files in a folder, **When** user runs `tcdir /a:v`, **Then** only pinned (always available) files are shown

---

### User Story 9 - Color Configuration via Environment Variable (Priority: P2)

As a power user, I want to customize colors and default switches via the TCDIR environment variable so I can personalize the display to my preferences.

**Why this priority**: Customization enables users to optimize TCDir for their visual preferences and workflow.

**Independent Test**: Set `TCDIR=W;D=LightGreen` and verify wide mode is default and date color is green.

**Acceptance Scenarios**:

1. **Given** TCDIR env var includes switch names (e.g., `W`), **When** tcdir runs, **Then** those switches are enabled by default
2. **Given** TCDIR env var includes item colors (e.g., `D=LightGreen`), **When** tcdir runs, **Then** that display item uses the specified color
3. **Given** TCDIR env var includes extension colors (e.g., `.cpp=Cyan`), **When** tcdir runs, **Then** files with that extension use the specified color
4. **Given** TCDIR env var includes attribute colors (e.g., `Attr:H=DarkGray`), **When** tcdir runs, **Then** hidden files use the specified color
5. **Given** TCDIR env var has syntax errors, **When** tcdir runs, **Then** errors are displayed at the end of output with specific details
6. **Given** user runs `tcdir --env`, **When** displayed, **Then** complete environment variable syntax help is shown
7. **Given** TCDIR env var entry has an invalid background color (e.g., `.png=Black on Chartreuse`), **When** tcdir runs, **Then** the entire entry is ignored and the extension keeps its default color
8. **Given** TCDIR env var entry has the same foreground and background color (e.g., `.cpp=Blue on Blue`), **When** tcdir runs, **Then** the entry is rejected as unreadable and the extension keeps its default color

---

### User Story 10 - Configuration Display (Priority: P3)

As a user troubleshooting colors, I want to see my current color configuration so I can verify settings and identify the source of each setting.

**Why this priority**: Configuration visibility aids troubleshooting and understanding current behavior.

**Independent Test**: Run `tcdir --config` and verify all settings are displayed with their sources.

**Acceptance Scenarios**:

1. **Given** user runs `tcdir --config`, **When** output is displayed, **Then** all display item colors are shown with their current values
2. **Given** user runs `tcdir --config`, **When** output is displayed, **Then** all extension color overrides are shown
3. **Given** each configuration item, **When** displayed, **Then** the source is indicated (Default vs Environment)
4. **Given** TCDIR env var has validation errors, **When** user runs `tcdir --config`, **Then** errors are shown after the configuration table (matching `--env` convention)

---

### User Story 11 - File Ownership Display (Priority: P3)

As a system administrator, I want to see file ownership information so I can audit permissions without using separate tools.

**Why this priority**: Ownership info is useful for administration but requires slow security API calls per file.

**Independent Test**: Run `tcdir --owner` and verify owner usernames appear for each file.

**Acceptance Scenarios**:

1. **Given** `--owner` switch, **When** user runs the command, **Then** file owner (DOMAIN\User format) is displayed for each file
2. **Given** a file owned by SYSTEM, **When** displayed with `--owner`, **Then** appropriate system account name is shown
3. **Given** TCDIR env var includes `Owner`, **When** tcdir runs, **Then** ownership is enabled by default

---

### User Story 12 - Alternate Data Streams Display (Priority: P3)

As a security analyst or power user, I want to see NTFS alternate data streams so I can identify hidden data attached to files.

**Why this priority**: ADS are rarely needed but important for security analysis and forensics.

**Independent Test**: Run `tcdir --streams` on files with alternate streams and verify streams are listed.

**Acceptance Scenarios**:

1. **Given** `--streams` switch, **When** user runs the command, **Then** alternate data streams are displayed below each file
2. **Given** a file with multiple streams, **When** displayed with `--streams`, **Then** each stream name and size is shown
3. **Given** a file with no alternate streams, **When** displayed with `--streams`, **Then** no additional output appears for that file
4. **Given** TCDIR env var includes `Streams`, **When** tcdir runs, **Then** streams display is enabled by default

---

### User Story 13 - Performance Timing Information (Priority: P3)

As a developer or performance-conscious user, I want to see elapsed time for directory operations so I can benchmark and optimize file system access.

**Why this priority**: Performance timing is a diagnostic/debugging feature for power users.

**Independent Test**: Run `tcdir /p` and verify elapsed time is displayed.

**Acceptance Scenarios**:

1. **Given** `/p` switch, **When** tcdir completes, **Then** elapsed time is displayed in milliseconds
2. **Given** large recursive listing with `/p`, **When** completes, **Then** time reflects total enumeration and display time

---

### User Story 14 - Multi-Threading Control (Priority: P3)

As a user on constrained systems, I want to control multi-threaded enumeration so I can balance performance vs. resource usage.

**Why this priority**: Multi-threading is enabled by default; control is needed for edge cases.

**Independent Test**: Run `tcdir /s /m-` and verify single-threaded operation.

**Acceptance Scenarios**:

1. **Given** default behavior, **When** recursive listing runs, **Then** multi-threading is enabled for parallel enumeration
2. **Given** `/m-` switch, **When** recursive listing runs, **Then** single-threaded enumeration is used
3. **Given** non-recursive listing, **When** tcdir runs, **Then** multi-threading has no effect (single directory)

---

### User Story 15 - Help Display (Priority: P1)

As a new user, I want to see usage help so I can learn available switches and syntax.

**Why this priority**: Help is essential for discoverability and user onboarding.

**Independent Test**: Run `tcdir -?` or `tcdir /?` and verify comprehensive help is displayed.

**Acceptance Scenarios**:

1. **Given** `-?` or `/?` switch, **When** user runs the command, **Then** complete usage help is displayed
2. **Given** help display, **When** rendered, **Then** all switches are listed with descriptions
3. **Given** help display, **When** rendered, **Then** attribute codes are explained with examples
4. **Given** help display, **When** rendered, **Then** cloud status symbols are shown with color and meaning
5. **Given** help display, **When** rendered, **Then** version number and architecture are shown

---

### User Story 16 - Debug Attribute Display (Priority: P4)

As a developer debugging file attribute issues, I want to see raw hexadecimal attribute values so I can diagnose cloud status and other attribute flags.

**Why this priority**: Developer diagnostic feature, not needed for normal usage.

**Independent Test**: Run `tcdir --debug` (debug builds only) and verify hex attributes appear before filenames.

**Acceptance Scenarios**:

1. **Given** `--debug` switch, **When** displaying normal listing, **Then** raw hex attribute value (e.g., `0x00000020`) appears before each filename
2. **Given** `--debug` switch, **When** file has cloud attributes, **Then** hex value shows the actual attribute bits (e.g., `0x00480000` for RECALL_ON_DATA_ACCESS)

**Note**: This switch is only available in debug builds (compiled out of release builds).

---

### Edge Cases

- What happens when a path does not exist? Error message indicating the path does not exist.
- What happens when access is denied to a directory? Error is displayed but enumeration continues for accessible items.
- What happens when a filename contains Unicode characters? Full Unicode support via wide character APIs.
- What happens when TCDIR env var has invalid syntax? Errors are displayed at the end of output with specific details.
- What happens when TCDIR env var has an invalid background color (e.g., `Chartreuse`)? The entire entry is rejected and the extension keeps its default color.
- What happens when TCDIR env var specifies the same foreground and background color? The entry is rejected as unreadable (e.g., `Blue on Blue`, `Black on Black`).
- What happens when both `-` and `/` prefixes are mixed? Both are supported; the last-used prefix affects help display format.
- What happens when long switches use single dash (e.g., `-env`)? Error is returned; long switches require `--` prefix with `-` style.

---

## Requirements *(mandatory)*

### Functional Requirements

#### Core Listing

- **FR-001**: System MUST enumerate directory contents using Windows FindFirstFile/FindNextFile APIs
- **FR-002**: System MUST display file date/time, size, attributes, and filename in normal listing mode
- **FR-003**: System MUST support multiple file masks on a single command line (e.g., `tcdir *.cpp *.h`)
- **FR-004**: System MUST group file masks by target directory and process each group
- **FR-005**: System MUST display volume label, drive type, and available space in directory headers
- **FR-006**: System MUST display total files, total bytes, and directories count in summary footer

#### Listing Modes

- **FR-010**: System MUST support three listing modes: Normal (detailed), Wide (multi-column), and Bare (names only)
- **FR-011**: System MUST select listing mode based on command-line switches (`/w`, `/b`)
- **FR-012**: Bare mode MUST suppress all headers, footers, volume info, and decorations
- **FR-013**: Wide mode MUST calculate column count based on console width and longest filename

#### Sorting

- **FR-020**: System MUST support sorting by name (N), extension (E), size (S), and date (D)
- **FR-021**: System MUST support ascending (default) and descending (with `-` prefix) sort directions
- **FR-022**: System MUST support both `/on` and `/o:n` switch syntax forms
- **FR-023**: System MUST use secondary sort criteria when primary sort values are equal

#### Attribute Filtering

- **FR-030**: System MUST support filtering by standard attributes: D, H, S, R, A, T, E, C, P, 0 (sparse)
- **FR-031**: System MUST support filtering by extended attributes: X (not indexed), I (integrity), B (no scrub)
- **FR-032**: System MUST support filtering by cloud attributes: O (cloud-only), L (locally available), V (pinned)
- **FR-033**: System MUST support attribute exclusion with `-` prefix
- **FR-034**: System MUST support both `/ah` and `/a:h` switch syntax forms

#### Recursion

- **FR-040**: System MUST support recursive directory traversal with `/s` switch
- **FR-041**: System MUST display directory path header before each subdirectory's contents
- **FR-042**: System MUST accumulate totals across all visited directories for summary
- **FR-043**: System MUST support multi-threaded enumeration for recursive listings (default: enabled)

#### Time Field Selection

- **FR-050**: System MUST support time field selection with `/t:` switch (C=creation, A=access, W=written)
- **FR-051**: System MUST use selected time field for both display and sorting
- **FR-052**: System MUST default to last write time (W) when no `/t:` switch specified

#### Cloud File Support

- **FR-060**: System MUST detect cloud sync root folders (OneDrive, iCloud, etc.)
- **FR-061**: System MUST display cloud status symbols: ○ (cloud-only), ◐ (local), ● (pinned)
- **FR-062**: System MUST support configurable colors for each cloud status symbol
- **FR-063**: System MUST suppress cloud status symbols in bare mode

#### Colorization

- **FR-070**: System MUST apply distinct colors to different display items (date, time, size, attributes, name)
- **FR-071**: System MUST apply colors based on file extension
- **FR-072**: System MUST apply colors based on file attributes (hidden, system, etc.)
- **FR-073**: System MUST support 16 standard console colors (foreground and background)
- **FR-074**: System MUST fall back to default colors when no specific color is configured

#### Configuration

- **FR-080**: System MUST read configuration from TCDIR environment variable
- **FR-081**: System MUST support switch defaults in TCDIR (e.g., `W` enables wide mode by default)
- **FR-082**: System MUST support display item colors in TCDIR (e.g., `D=LightGreen`)
- **FR-083**: System MUST support extension colors in TCDIR (e.g., `.cpp=Cyan`)
- **FR-084**: System MUST support file attribute colors in TCDIR (e.g., `Attr:H=DarkGray`)
- **FR-085**: System MUST validate TCDIR syntax and report specific errors at end of output
- **FR-086**: System MUST reject TCDIR entries with invalid background color names (entire entry ignored, default preserved)
- **FR-087**: System MUST reject TCDIR entries where foreground and background colors are the same (unreadable)
- **FR-088**: `--config` MUST display validation errors after the configuration table, matching `--env` convention

#### Extended Features

- **FR-090**: System MUST support file owner display with `--owner` switch
- **FR-091**: System MUST support alternate data stream display with `--streams` switch (NTFS only)
- **FR-092**: System MUST support performance timing display with `/p` switch
- **FR-093**: System MUST support multi-threading toggle with `/m` (enable) and `/m-` (disable)

#### Help and Information

- **FR-100**: System MUST display usage help with `-?` or `/?` switch
- **FR-101**: System MUST display environment variable help with `--env` switch
- **FR-102**: System MUST display current configuration with `--config` switch
- **FR-103**: Help MUST adapt switch prefix display based on user's chosen style (- vs /)

#### Command-Line Compatibility

- **FR-110**: System MUST support both `-` and `/` switch prefixes (CMD `dir` compatibility)
- **FR-110a**: Switch prefix style is auto-detected from the first switch used (default: `-`)
- **FR-111**: System MUST require `--` prefix for long switches when using `-` style (e.g., `--env` not `-env`)
- **FR-112**: System MUST support both `/a:hs` and `/ahs` syntax for attribute switches
- **FR-113**: System MUST support both `/o:d` and `/od` syntax for sort switches

### Key Entities

- **FileInfo**: Extended file information including WIN32_FIND_DATA plus optional alternate data streams
- **DirectoryInfo**: Directory state including path, file specs, child directories, and file lists for multi-threaded processing
- **DriveInfo**: Volume information including name, type (local/network/removable), filesystem type, and UNC path for network drives
- **Config**: Color configuration including display item colors, extension colors, attribute colors, and switch defaults
- **CommandLine**: Parsed command-line state including switches, masks, sort order, and attribute filters

---

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Users can list directory contents with color differentiation in under 1 second for typical directories (< 1000 files)
- **SC-002**: Recursive listing of 10,000+ files completes within 10 seconds on SSD storage
- **SC-003**: Users can identify file types by color without reading filenames or extensions
- **SC-004**: All CMD `dir` attribute filter codes are supported (`/a:` switch)
- **SC-005**: All CMD `dir` sort options are supported (`/o:` switch)
- **SC-006**: Users familiar with CMD `dir` can use TCDir with minimal learning curve
- **SC-007**: Color configuration allows personalization without code changes
- **SC-008**: Cloud sync status is visible at a glance for OneDrive/iCloud folders
- **SC-009**: Help text is comprehensive enough for users to learn all features independently
- **SC-010**: Multi-threaded enumeration provides measurable speedup (2x+) for large recursive listings

---

## Assumptions

- Windows console supports basic 16-color text attributes (FOREGROUND_*/BACKGROUND_*)
- Users have standard Windows file system permissions for directories they browse
- Cloud sync roots are detectable via Windows Cloud Files API
- NTFS alternate data streams use standard Windows backup APIs for enumeration
- Console width is detectable via GetConsoleScreenBufferInfo

---

# Appendix A: Implementation Details

This appendix contains exact implementation details required to build a pixel-perfect clone of TCDir.

## A.1 Unicode Symbols

| Symbol | Name | Unicode Codepoint | Usage |
|--------|------|-------------------|-------|
| ○ | CircleHollow | U+25CB | Cloud-only file (not locally available) |
| ◐ | CircleHalfFilled | U+25D0 | Locally available (can be dehydrated) |
| ● | CircleFilled | U+25CF | Pinned/always locally available |
| ─ | LineHorizontal | U+2500 | Separator line drawn across console width |
| © | Copyright | U+00A9 | Copyright symbol in help text |
| ‾ | Overline | U+203E | Underline character for error highlighting |

---

## A.2 Console Color System

### A.2.1 Windows Console Color Values (WORD)

TCDir uses Windows console text attributes, which are 16-bit values combining foreground and background colors:

**Foreground Colors (bits 0-3):**

| Name | Value (Hex) | Bit Pattern |
|------|-------------|-------------|
| FC_Black | 0x0000 | 0000 |
| FC_Blue | 0x0001 | 0001 (FOREGROUND_BLUE) |
| FC_Green | 0x0002 | 0010 (FOREGROUND_GREEN) |
| FC_Cyan | 0x0003 | 0011 |
| FC_Red | 0x0004 | 0100 (FOREGROUND_RED) |
| FC_Magenta | 0x0005 | 0101 |
| FC_Brown | 0x0006 | 0110 (dark yellow) |
| FC_LightGrey | 0x0007 | 0111 |
| FC_DarkGrey | 0x0008 | 1000 (FOREGROUND_INTENSITY) |
| FC_LightBlue | 0x0009 | 1001 |
| FC_LightGreen | 0x000A | 1010 |
| FC_LightCyan | 0x000B | 1011 |
| FC_LightRed | 0x000C | 1100 |
| FC_LightMagenta | 0x000D | 1101 |
| FC_Yellow | 0x000E | 1110 |
| FC_White | 0x000F | 1111 |

**Background Colors (bits 4-7):** Same pattern shifted left 4 bits (multiply by 0x10).

### A.2.2 Color Name Mapping (Environment Variable)

Valid color names for TCDIR environment variable (case-insensitive):

| Color Name | Foreground Value | Background Value |
|------------|------------------|------------------|
| Black | FC_Black (0x00) | BC_Black (0x00) |
| Blue | FC_Blue (0x01) | BC_Blue (0x10) |
| Green | FC_Green (0x02) | BC_Green (0x20) |
| Cyan | FC_Cyan (0x03) | BC_Cyan (0x30) |
| Red | FC_Red (0x04) | BC_Red (0x40) |
| Magenta | FC_Magenta (0x05) | BC_Magenta (0x50) |
| Brown | FC_Brown (0x06) | BC_Brown (0x60) |
| LightGrey | FC_LightGrey (0x07) | BC_LightGrey (0x70) |
| DarkGrey | FC_DarkGrey (0x08) | BC_DarkGrey (0x80) |
| LightBlue | FC_LightBlue (0x09) | BC_LightBlue (0x90) |
| LightGreen | FC_LightGreen (0x0A) | BC_LightGreen (0xA0) |
| LightCyan | FC_LightCyan (0x0B) | BC_LightCyan (0xB0) |
| LightRed | FC_LightRed (0x0C) | BC_LightRed (0xC0) |
| LightMagenta | FC_LightMagenta (0x0D) | BC_LightMagenta (0xD0) |
| Yellow | FC_Yellow (0x0E) | BC_Yellow (0xE0) |
| White | FC_White (0x0F) | BC_White (0xF0) |

### A.2.3 ANSI Escape Sequence Translation

When outputting to console with virtual terminal processing enabled, Windows console attributes are translated to ANSI SGR codes:

| Console Index | ANSI Foreground | ANSI Background |
|---------------|-----------------|-----------------|
| 0 (Black) | 30 | 40 |
| 1 (Blue) | 34 | 44 |
| 2 (Green) | 32 | 42 |
| 3 (Cyan) | 36 | 46 |
| 4 (Red) | 31 | 41 |
| 5 (Magenta) | 35 | 45 |
| 6 (Yellow) | 33 | 43 |
| 7 (White) | 37 | 47 |

For bright/intensity colors (bit 3 set), add 60 to the base code (e.g., bright red = 91).

**SGR Format:** `ESC[{foreground};{background}m` (e.g., `\x1b[91;40m` for bright red on black)

### A.2.4 Color Marker Format Strings

Output functions like `ColorPrintf` and `ColorPuts` accept embedded color markers in the format `{AttributeName}`.

**Syntax:**
```
{AttributeName}
```

**Behavior:**
1. Text before the first marker is emitted in the current (default) color
2. When a marker is encountered, subsequent text uses that color
3. Colors are "sticky" - they remain until the next marker
4. Unknown markers cause an assertion failure (developer error)
5. A literal `{` that doesn't form a valid marker is emitted as text

**Valid Marker Names:**
| Marker | Purpose |
|--------|---------|
| `{Default}` | Console default foreground/background |
| `{Date}` | Date column |
| `{Time}` | Time column |
| `{FileAttributePresent}` | Attribute letter when set |
| `{FileAttributeNotPresent}` | Attribute letter when not set |
| `{Size}` | File size column |
| `{Directory}` | Directory names |
| `{Information}` | Informational text |
| `{InformationHighlight}` | Highlighted values in info text |
| `{SeparatorLine}` | Separator lines (currently disabled) |
| `{Error}` | Error messages |
| `{Owner}` | File owner column |
| `{Stream}` | Alternate data stream names |
| `{CloudStatusCloudOnly}` | Cloud-only (○) symbol |
| `{CloudStatusLocallyAvailable}` | Locally available (◐) symbol |
| `{CloudStatusAlwaysLocallyAvailable}` | Always local (●) symbol |

**Example:**
```
ColorPrintf(L"{Error}Error: {InformationHighlight}%s{Error} not found", path);
```
Outputs: "Error: " in error color, path in highlight color, " not found" in error color.

---

## A.3 Default Color Scheme

### A.3.1 Display Item Colors

| Item | Color | Value |
|------|-------|-------|
| Default | (console default) | From GetConsoleScreenBufferInfo |
| Date | FC_Red | 0x04 |
| Time | FC_Brown | 0x06 |
| FileAttributePresent | FC_Cyan | 0x03 |
| FileAttributeNotPresent | FC_DarkGrey | 0x08 |
| Information | FC_Cyan | 0x03 |
| InformationHighlight | FC_White | 0x0F |
| Size | FC_Yellow | 0x0E |
| Directory | FC_LightBlue | 0x09 |
| SeparatorLine | FC_LightBlue | 0x09 |
| Error | FC_LightRed | 0x0C |
| Owner | FC_Green | 0x02 |
| Stream | FC_DarkGrey | 0x08 |
| CloudStatusCloudOnly | FC_LightBlue | 0x09 |
| CloudStatusLocallyAvailable | FC_LightGreen | 0x0A |
| CloudStatusAlwaysLocallyAvailable | FC_LightGreen | 0x0A |

### A.3.2 File Attribute Colors

| Attribute | Color | Value |
|-----------|-------|-------|
| Hidden (FILE_ATTRIBUTE_HIDDEN) | FC_DarkGrey | 0x08 |
| Encrypted (FILE_ATTRIBUTE_ENCRYPTED) | FC_LightGreen | 0x0A |

### A.3.3 File Extension Colors (Default Mapping)

**Source Code (FC_LightGreen = 0x0A):**
- `.asm`, `.c`, `.cpp`, `.cxx`, `.h`, `.hpp`, `.hxx`, `.rc`
- `.cs`, `.resx`, `.rcml`
- `.js`, `.jsx`, `.ts`, `.tsx`
- `.html`, `.htm`, `.css`, `.scss`, `.less`
- `.py`, `.pyw`
- `.jar`, `.java`, `.class`

**Intermediate Files (FC_Green = 0x02):**
- `.cod`, `.i`, `.obj`, `.lib`, `.res`, `.pch`

**Data/Config Files (FC_Brown = 0x06):**
- `.xml`, `.json`, `.yml`, `.yaml`

**Build Output/Errors (FC_LightRed = 0x0C):**
- `.wrn`, `.err`
- `.bash`, `.bat`, `.cmd`, `.ps1`, `.psd1`, `.psm1`, `.sh`

**Log Files (FC_White = 0x0F):**
- `.log`

**Executables (FC_LightCyan = 0x0B):**
- `.dll`, `.exe`, `.sys`

**Visual Studio Files (FC_Magenta = 0x05):**
- `.sln`, `.vcproj`, `.vcxproj`

**Visual Studio Generated (FC_DarkGrey = 0x08):**
- `.csproj`, `.csxproj`, `.user`, `.ncb`

**Documents (FC_White = 0x0F):**
- `.!!!`, `.1st`, `.doc`, `.docx`, `.eml`, `.md`, `.me`, `.now`
- `.ppt`, `.pptx`, `.text`, `.txt`, `.xls`, `.xlsx`

**Archives (FC_Magenta = 0x05):**
- `.7z`, `.arj`, `.gz`, `.rar`, `.tar`, `.zip`

---

## A.4 Output Formats

### A.4.1 Normal Listing Format

Each file line format:
```
{Date}MM/DD/YYYY  {Time}HH:MM AM{Default} {Attrs}RHSATEC P0{Default} {Size}%*s {Cloud}X {Owner?}DOMAIN\user {Filename}filename.ext
```

**Column breakdown:**
- Date: Locale-specific short date format via `GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, DATE_SHORTDATE, ...)`
  - US example: `02/07/2026`
  - UK example: `07/02/2026`
  - ISO example: `2026-02-07`
- Separator: 2 spaces
- Time: Locale-specific short time format via `GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, TIME_NOSECONDS, ...)`
  - 12-hour example: `09:38 PM`
  - 24-hour example: `21:38`
- Separator: 1 space
- Attributes: 9 chars (R H S A T E C P 0), each position shows letter if set, `-` if not
- Separator: 1 space
- Size: Right-aligned with locale-aware thousands separators, minimum 5 chars (for `<DIR>`)
- Separator: 1 space
- Cloud status: 2 chars (symbol + space) - only if in sync root
- Owner (optional): Variable width column + 1 space padding
- Filename: Left-aligned, no width limit

**Directory size display:** `<DIR>` centered within the size column

**Note on date/time width:** Since locale formats vary in length, the date and time columns should be measured dynamically or use reasonable maximums. The current C++ implementation uses fixed format strings (`MM/dd/yyyy` and `hh:mm tt`), but a proper implementation should respect user locale settings.

### A.4.2 Attribute Display Order

Attributes displayed left-to-right in this fixed order:
1. R - Read-only (FILE_ATTRIBUTE_READONLY)
2. H - Hidden (FILE_ATTRIBUTE_HIDDEN)
3. S - System (FILE_ATTRIBUTE_SYSTEM)
4. A - Archive (FILE_ATTRIBUTE_ARCHIVE)
5. T - Temporary (FILE_ATTRIBUTE_TEMPORARY)
6. E - Encrypted (FILE_ATTRIBUTE_ENCRYPTED)
7. C - Compressed (FILE_ATTRIBUTE_COMPRESSED)
8. P - Reparse point (FILE_ATTRIBUTE_REPARSE_POINT)
9. 0 - Sparse file (FILE_ATTRIBUTE_SPARSE_FILE)

### A.4.3 Wide Listing Format

- Column width = console_width / num_columns
- Num columns = console_width / (longest_filename + 1)
- Items displayed in **column-major order** (top to bottom, then left to right)
- Directories displayed as `[dirname]` (square brackets)
- No date, time, size, or attributes shown

### A.4.4 Bare Listing Format

- One entry per line
- Non-recursive: filename only
- Recursive (`/s`): full path
- No headers, footers, volume info, or summaries
- No cloud status symbols

### A.4.5 Headers and Footers

**Drive Header:**
```
{separator line}
Volume in drive {highlight}C{info} is {highlight}a hard drive{info} ({highlight}NTFS{info})
Volume name is "{highlight}Volume Label{info}"

Directory of {highlight}C:\Path\To\Directory{info}
```

For UNC paths:
```
Volume {highlight}\\server\share{info} is {highlight}a network drive{info} mapped to {highlight}Z:{info} ({highlight}NTFS{info})
```

**Directory Summary:**
```
{highlight}4{info} dirs, {highlight}27{info} files using {highlight}284,475{info} bytes
```

If streams found:
```
{highlight}4{info} dirs, {highlight}27{info} files using {highlight}284,475{info} bytes, {highlight}3{info} streams using {highlight}1,234{info} bytes
```

**Volume Footer:**
```
{highlight}123,456,789{info} bytes free on volume
{highlight}100,000,000{info} bytes available to {highlight}username{info} due to quota
```
(Second line only shown if quota differs from total free)

**Recursive Summary:**
```
Total files listed:

    {highlight}143{info} files using {highlight}123,456{info} bytes
      {highlight}7{info} subdirectories
```

If streams found during recursive listing, add after subdirectories:
```
      {highlight}3{info} streams using {highlight}1,234{info} bytes
```

- Numbers are right-aligned to match width of largest count (files vs directories)
- Width includes commas from number formatting

### A.4.6 Number Formatting

All numbers use locale-aware thousands separators via `std::format(locale(""), L"{:L}", number)`.

### A.4.7 Stream Output Format

```
{30 spaces}{Size} %*s {Default}  {owner_padding}{Stream}filename:streamname
```
- 30 character indent before size
- Size column matches file size column width
- Owner padding if `--owner` is enabled

---

## A.5 TCDIR Environment Variable Grammar

```ebnf
tcdir_value     = entry { ";" entry }
entry           = switch | color_override
switch          = switch_name [ "-" ]
switch_name     = "W" | "S" | "P" | "M" | "B" | "Owner" | "Streams"
color_override  = key "=" color_spec
key             = display_attr | file_ext | file_attr
display_attr    = "D" | "T" | "A" | "-" | "S" | "R" | "I" | "H" | "E" | "F" | "O" | "M"
file_ext        = "." identifier
file_attr       = "Attr:" attr_char
attr_char       = "R" | "H" | "S" | "A" | "T" | "E" | "C" | "P" | "0"
color_spec      = color_name [ " on " color_name ]
color_name      = "Black" | "Blue" | "Green" | "Cyan" | "Red" | "Magenta" | "Brown"
                | "LightGrey" | "DarkGrey" | "LightBlue" | "LightGreen" | "LightCyan"
                | "LightRed" | "LightMagenta" | "Yellow" | "White"
```

**Display Attribute Key Mapping:**
| Key | Attribute |
|-----|-----------|
| D | Date |
| T | Time |
| A | FileAttributePresent |
| - | FileAttributeNotPresent |
| S | Size |
| R | Directory (mnemonic: "diRectory") |
| I | Information |
| H | InformationHighlight |
| E | Error |
| F | Default (mnemonic: "deFault") |
| O | Owner |
| M | Stream (mnemonic: "streaM") |

**Switch prefixes (`/`, `-`, `--`) are NOT allowed** in the environment variable. Errors are reported with the specific invalid prefix underlined.

---

## A.6 Sorting Rules

### A.6.1 Primary Sort Behavior

- Directories **always** sort before files, regardless of sort order
- Within directories and within files, the selected criterion applies

### A.6.2 Sort Tiebreaker Fallback Order

When primary sort values are equal, fallback to next criterion in this order:
1. **Requested sort attribute** (first in list)
2. Name (alphabetic, case-insensitive)
3. Date (by selected time field)
4. Extension (alphabetic, case-insensitive)
5. Size

### A.6.3 Reverse Sort Behavior

- `-` prefix (e.g., `/o:-d`) reverses **only** the primary sort criterion
- Tiebreaker sorts are **not** reversed (always use natural order)

### A.6.4 Comparison Functions

- **Name:** `lstrcmpiW` (case-insensitive, locale-aware)
- **Extension:** Extract extension via `filesystem::path::extension()`, then `lstrcmpiW`
- **Date:** `CompareFileTime` on selected time field
- **Size:** Direct 64-bit comparison (avoid signed overflow)

---

## A.7 Attribute Filter System

### A.7.1 Attribute Character Codes

| Char | Attribute Constant | Description |
|------|-------------------|-------------|
| D | FILE_ATTRIBUTE_DIRECTORY | Directory |
| H | FILE_ATTRIBUTE_HIDDEN | Hidden |
| S | FILE_ATTRIBUTE_SYSTEM | System |
| R | FILE_ATTRIBUTE_READONLY | Read-only |
| A | FILE_ATTRIBUTE_ARCHIVE | Archive |
| T | FILE_ATTRIBUTE_TEMPORARY | Temporary |
| E | FILE_ATTRIBUTE_ENCRYPTED | Encrypted |
| C | FILE_ATTRIBUTE_COMPRESSED | Compressed |
| P | FILE_ATTRIBUTE_REPARSE_POINT | Reparse point |
| 0 | FILE_ATTRIBUTE_SPARSE_FILE | Sparse file |
| X | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED | Not indexed |
| I | FILE_ATTRIBUTE_INTEGRITY_STREAM | Integrity stream (ReFS) |
| B | FILE_ATTRIBUTE_NO_SCRUB_DATA | No scrub data (ReFS) |
| U | FILE_ATTRIBUTE_UNPINNED | Unpinned (can be dehydrated) |
| O | (composite) | Cloud-only: OFFLINE \| RECALL_ON_OPEN \| RECALL_ON_DATA_ACCESS |
| L | FILE_ATTRIBUTE_UNPINNED | Locally available |
| V | FILE_ATTRIBUTE_PINNED | Always locally available (pinned) |

### A.7.2 Filter Logic

- **Required mask** (from `/a:xyz`): File must have ALL specified attributes
- **Excluded mask** (from `/a:-xyz`): File must NOT have ANY excluded attributes
- File passes filter if: `(attrs & required) == required && (attrs & excluded) == 0`

---

## A.8 Cloud Status Detection

### A.8.1 Sync Root Detection

Use `CfGetSyncRootInfoByPath` to determine if path is in a cloud sync root:
- Returns success: path is under a cloud sync root
- Returns failure: not a cloud folder

### A.8.2 Cloud Status Logic (per file)

Algorithm (in priority order):
```
if not in_sync_root:
    return CS_NONE
if FILE_ATTRIBUTE_PINNED:
    return CS_PINNED (●)
if FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS | RECALL_ON_OPEN | OFFLINE:
    return CS_CLOUD_ONLY (○)
if FILE_ATTRIBUTE_UNPINNED:
    return CS_LOCAL (◐)
else:
    return CS_LOCAL (◐)  // Fully hydrated files lose placeholder metadata
```

---

## A.9 Alternate Data Streams

### A.9.1 Stream Enumeration

Use `FindFirstStreamW` / `FindNextStreamW` with `FindStreamInfoStandard`:
- Skip the default `::$DATA` stream
- Strip `:$DATA` suffix from stream names for display
- Store stream name and size

### A.9.2 Stream Display

Streams displayed immediately after their parent file, indented to align with filename column.

---

## A.10 Console Initialization

1. Get stdout handle via `GetStdHandle(STD_OUTPUT_HANDLE)`
2. Call `GetConsoleMode` - if it fails, console is redirected
3. If not redirected:
   - Enable `ENABLE_VIRTUAL_TERMINAL_PROCESSING` for ANSI escape support
   - Get console width via `GetConsoleScreenBufferInfo`
   - Default width if redirected: 80 columns
4. Get default text attribute from `csbi.wAttributes`
5. Allocate 10MB output buffer for batched writes

---

## A.11 Performance Timer

When `/p` is specified:
- Create `PerfTimer` before directory enumeration begins
- Timer uses `std::chrono::high_resolution_clock` for timing
- Display format: `{name}:  {value:.2f} msec\n`
  - Note: TWO spaces after the colon
  - Value formatted with exactly 2 decimal places
  - Example: `TCDir time elapsed:  123.45 msec`
- Timer auto-displays on destruction (Automatic mode)

---

## A.12 Separator Lines

**Note:** Separator lines are currently disabled in the codebase. The `WriteSeparatorLine` function exists but is empty/commented out. Headers and footers are displayed without horizontal rule separators.

---

## A.13 Drive Information

### A.12.1 Volume Type Detection

Use `GetDriveTypeW` to get volume type:
| Type | Description |
|------|-------------|
| DRIVE_UNKNOWN (0) | "an unknown type" |
| DRIVE_NO_ROOT_DIR (1) | "an unknown type" |
| DRIVE_REMOVABLE (2) | "a removable disk" |
| DRIVE_FIXED (3) | "a hard drive" |
| DRIVE_REMOTE (4) | "a network drive" |
| DRIVE_CDROM (5) | "a CD/DVD" |
| DRIVE_RAMDISK (6) | "a RAM disk" |

### A.12.2 UNC Path Handling

For paths starting with `\\`:
- Store UNC path separately
- Use `WNetGetConnection` to get remote name for mapped drives
- Check for NTFS/ReFS via `GetVolumeInformationW`

---

## A.14 Error Handling

### A.14.1 Exit Codes

- 0: Success
- 1: Error (HRESULT failure)

### A.14.2 Error Messages

Path not found:
```
{Error}Error:   {InformationHighlight}path{Error} does not exist
```

TCDIR env var errors displayed at end of output:
```
{Error}There are some problems with your TCDIR environment variable (see --env for help):
{Error}  Invalid foreground color in ".cpp=FakeColor"
{Default}                                  ‾‾‾‾‾‾‾‾‾
```
(Error text with underline showing invalid portion)

---

## A.15 Multi-Threading Architecture

### A.15.1 Work Queue

- Thread-safe work queue using `mutex` and `condition_variable`
- Worker threads dequeue directory work items
- Main thread enqueues root directory, workers enqueue children

### A.15.2 Directory Tree Processing

1. Enumerate root directory, enqueue child directories
2. Worker threads enumerate children in parallel
3. Results stored in `CDirectoryInfo` tree structure with status tracking
4. Main thread waits for completion, then prints in depth-first order
5. Totals accumulated during print traversal

### A.15.3 Status States

| Status | Meaning |
|--------|---------|
| Waiting | Queued, not yet started |
| InProgress | Currently being enumerated |
| Done | Enumeration complete |
| Error | Enumeration failed |

---

## A.16 Mask Grouping Behavior

When multiple file masks are specified on the command line, they are grouped by target directory:

### A.16.1 Pure Masks vs Directory-Qualified Masks

- **Pure mask**: No path separator or drive letter (e.g., `*.cpp`, `foo.txt`)
  - Uses current working directory
- **Directory-qualified mask**: Contains path (e.g., `src\*.cpp`, `C:\temp\*.log`)
  - Made absolute and split into directory + filespec

### A.16.2 Grouping Algorithm

```
Input:  tcdir *.cpp *.h foo\*.txt bar\

Output groups:
  [
    { "C:\cwd\",      [ "*.cpp", "*.h" ] },   // Pure masks combined
    { "C:\cwd\foo\",  [ "*.txt" ] },          // Separate directory
    { "C:\cwd\bar\",  [ "*" ] }               // Directory-only gets "*"
  ]
```

**Rules:**
1. Pure masks with the same CWD are combined into one group
2. Directory-qualified masks with the same directory are combined
3. A trailing slash on a mask means "directory only" → filespec becomes `*`
4. Directory comparison is case-insensitive
5. Each group is processed separately (but in parallel if multi-threaded recursive)

---

## A.17 Debug Attribute Display (`--debug`)

**Note**: Only available in debug builds (compiled out of release builds).

### A.17.1 Output Format

```
[XXXXXXXX:YY] filename
```

Where:
- `XXXXXXXX` = File attributes (8 hex digits, zero-padded)
- `YY` = Cloud Files API placeholder state (2 hex digits), from `CfGetPlaceholderStateFromFindData`

**Example:**
```
[00000020:00] readme.txt        // Archive attribute, no cloud state
[00480020:07] cloud-file.docx   // RECALL_ON_DATA_ACCESS | Archive, valid placeholder
```

### A.17.2 Position in Output

Appears after cloud status symbol, before owner (if enabled), immediately before filename.

### A.16.3 Special Cases

- **No masks provided**: Defaults to `[ cwd, ["*"] ]`
- **Directory-only mask** (e.g., `foo\`): Filespec becomes `*`
- **Drive letter without path** (e.g., `C:file.txt`): Treated as directory-qualified

---

# Appendix B: Windows API Reference

This appendix lists all Windows APIs used, organized by functionality. For Rust implementation, use the `windows` crate.

## B.1 Console APIs

| API | Purpose | Rust Crate |
|-----|---------|------------|
| `GetStdHandle(STD_OUTPUT_HANDLE)` | Get stdout handle | `windows::Win32::System::Console` |
| `GetConsoleMode` | Check if redirected, get current mode | `windows::Win32::System::Console` |
| `SetConsoleMode(ENABLE_VIRTUAL_TERMINAL_PROCESSING)` | Enable ANSI escape sequences | `windows::Win32::System::Console` |
| `GetConsoleScreenBufferInfo` | Get console width, default attributes | `windows::Win32::System::Console` |
| `WriteConsoleW` | Write wide string to console | `windows::Win32::System::Console` |

## B.2 File System APIs

| API | Purpose | Rust Crate |
|-----|---------|------------|
| `FindFirstFileW` | Start directory enumeration | `windows::Win32::Storage::FileSystem` |
| `FindNextFileW` | Continue enumeration | `windows::Win32::Storage::FileSystem` |
| `FindClose` | End enumeration | `windows::Win32::Storage::FileSystem` |
| `FindFirstStreamW` | Enumerate alternate data streams | `windows::Win32::Storage::FileSystem` |
| `FindNextStreamW` | Continue stream enumeration | `windows::Win32::Storage::FileSystem` |
| `GetDiskFreeSpaceExW` | Get free space on volume | `windows::Win32::Storage::FileSystem` |
| `GetVolumeInformationW` | Get volume name, filesystem type | `windows::Win32::Storage::FileSystem` |
| `GetDriveTypeW` | Get drive type (fixed, removable, etc.) | `windows::Win32::Storage::FileSystem` |

## B.3 Cloud Files APIs

| API | Purpose | Rust Crate |
|-----|---------|------------|
| `CfGetSyncRootInfoByPath` | Detect if path is in cloud sync root | `windows::Win32::Storage::CloudFilters` |
| `CfGetPlaceholderStateFromFindData` | Get cloud placeholder state | `windows::Win32::Storage::CloudFilters` |

## B.4 Security APIs

| API | Purpose | Rust Crate |
|-----|---------|------------|
| `GetNamedSecurityInfoW` | Get file owner SID | `windows::Win32::Security::Authorization` |
| `LookupAccountSidW` | Convert SID to username | `windows::Win32::Security` |
| `GetUserNameW` | Get current username | `windows::Win32::System::WindowsProgramming` |

## B.5 Time/Date APIs

| API | Purpose | Rust Crate |
|-----|---------|------------|
| `FileTimeToSystemTime` | Convert FILETIME to SYSTEMTIME | `windows::Win32::Foundation` |
| `SystemTimeToTzSpecificLocalTime` | Convert UTC to local time | `windows::Win32::System::Time` |
| `GetDateFormatEx` | Format date with locale | `windows::Win32::Globalization` |
| `GetTimeFormatEx` | Format time with locale | `windows::Win32::Globalization` |
| `CompareFileTime` | Compare two FILETIMEs | `windows::Win32::Foundation` |
| `QueryPerformanceCounter` | High-resolution timing | `windows::Win32::System::Performance` |
| `QueryPerformanceFrequency` | Timer frequency | `windows::Win32::System::Performance` |

## B.6 Network APIs

| API | Purpose | Rust Crate |
|-----|---------|------------|
| `WNetGetConnectionW` | Get remote name for mapped drive | `windows::Win32::NetworkManagement::WNet` |

## B.7 Environment APIs

| API | Purpose | Rust Crate |
|-----|---------|------------|
| `GetEnvironmentVariableW` | Read TCDIR env var | `windows::Win32::System::Environment` |

## B.8 Key Data Structures

### WIN32_FIND_DATAW
```rust
pub struct WIN32_FIND_DATAW {
    pub dwFileAttributes: u32,
    pub ftCreationTime: FILETIME,
    pub ftLastAccessTime: FILETIME,
    pub ftLastWriteTime: FILETIME,
    pub nFileSizeHigh: u32,
    pub nFileSizeLow: u32,
    pub dwReserved0: u32,
    pub dwReserved1: u32,
    pub cFileName: [u16; 260],
    pub cAlternateFileName: [u16; 14],
}
```

### File Attribute Constants (FILE_ATTRIBUTE_*)
```rust
const FILE_ATTRIBUTE_READONLY: u32 = 0x1;
const FILE_ATTRIBUTE_HIDDEN: u32 = 0x2;
const FILE_ATTRIBUTE_SYSTEM: u32 = 0x4;
const FILE_ATTRIBUTE_DIRECTORY: u32 = 0x10;
const FILE_ATTRIBUTE_ARCHIVE: u32 = 0x20;
const FILE_ATTRIBUTE_DEVICE: u32 = 0x40;
const FILE_ATTRIBUTE_NORMAL: u32 = 0x80;
const FILE_ATTRIBUTE_TEMPORARY: u32 = 0x100;
const FILE_ATTRIBUTE_SPARSE_FILE: u32 = 0x200;
const FILE_ATTRIBUTE_REPARSE_POINT: u32 = 0x400;
const FILE_ATTRIBUTE_COMPRESSED: u32 = 0x800;
const FILE_ATTRIBUTE_OFFLINE: u32 = 0x1000;
const FILE_ATTRIBUTE_NOT_CONTENT_INDEXED: u32 = 0x2000;
const FILE_ATTRIBUTE_ENCRYPTED: u32 = 0x4000;
const FILE_ATTRIBUTE_INTEGRITY_STREAM: u32 = 0x8000;
const FILE_ATTRIBUTE_NO_SCRUB_DATA: u32 = 0x20000;
const FILE_ATTRIBUTE_RECALL_ON_OPEN: u32 = 0x40000;
const FILE_ATTRIBUTE_PINNED: u32 = 0x80000;
const FILE_ATTRIBUTE_UNPINNED: u32 = 0x100000;
const FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS: u32 = 0x400000;
```

---

# Appendix C: Rust Implementation Notes

## C.1 Recommended Crates

| Purpose | Crate | Notes |
|---------|-------|-------|
| Windows APIs | `windows` | Official Microsoft crate |
| Console output | `crossterm` or raw Win32 | For ANSI sequences |
| Argument parsing | `clap` | Command-line parsing |
| Threading | `rayon` or `std::thread` | Parallel directory enumeration |
| Number formatting | `num-format` | Locale-aware thousands separators |

## C.2 Architecture Mapping

| C++ Class | Rust Equivalent |
|-----------|-----------------|
| `CCommandLine` | `Args` struct with `clap` derive |
| `CConfig` | `Config` struct with `Default` impl |
| `CConsole` | `Console` struct wrapping stdout |
| `CDirectoryLister` | `DirectoryLister` trait + impls |
| `IResultsDisplayer` | `ResultsDisplayer` trait |
| `CResultsDisplayerNormal` | `NormalDisplayer` struct |
| `CResultsDisplayerWide` | `WideDisplayer` struct |
| `CResultsDisplayerBare` | `BareDisplayer` struct |
| `FileComparator` | `Ord` impl or `sort_by` closure |
| `CDirectoryInfo` | `DirectoryInfo` struct with `Arc<Mutex<>>` for threading |
| `CDriveInfo` | `DriveInfo` struct |

## C.3 String Handling

- Use `OsString` / `OsStr` for paths
- Use `widestring` crate for `*W` API interop
- Convert to/from UTF-16 for Windows APIs

## C.4 Error Handling

- Map `HRESULT` to `Result<T, E>` pattern
- Use `windows::core::Error` for API errors
- Exit code 1 on any error

---

# Appendix D: Exact Help Screen Text

This appendix contains the exact text and formatting for all help screens, with color markers shown in `{ColorName}` format (see [A.2.4 Color Marker Format Strings](spec.md#a24-color-marker-format-strings) for full documentation).

## D.1 Help Screen (`-?` or `/?`)

### D.1.1 "Technicolor" Rainbow Header

The word "Technicolor" is printed with each character cycling through all 16 colors in order:

**Color Cycle Order:**
```
Black(0) → Blue(1) → Green(2) → Cyan(3) → Red(4) → Magenta(5) → Brown(6) → LightGrey(7) →
DarkGrey(8) → LightBlue(9) → LightGreen(10) → LightCyan(11) → LightRed(12) → 
LightMagenta(13) → Yellow(14) → White(15) → [repeat]
```

**Algorithm:**
1. For each character in the string, select the next color in the cycle
2. If the selected color matches the console background color, skip to the next color
3. Print the character in that color
4. Advance to next color index (wrapping at 16)

For "Technicolor" (11 chars), starting from index 0:
- T = Black (0)
- e = Blue (1)
- c = Green (2)
- h = Cyan (3)
- n = Red (4)
- i = Magenta (5)
- c = Brown (6)
- o = LightGrey (7)
- l = DarkGrey (8)
- o = LightBlue (9)
- r = LightGreen (10)

### D.1.2 Help Header Text

Switch prefix character determines formatting:
- `-` style: short = `-`, long = `--`, disable = ` -M-`
- `/` style: short = `/`, long = `/`, disable = ` /M-`, extra padding on long switches

```
{rainbow}Technicolor{Information} Directory version {VERSION} {ARCH} ({BUILD_DATE})
Copyright © 2004-{YEAR} by Robert Elmer

{InformationHighlight}TCDIR{Information} [{InformationHighlight}drive:{Information}][{InformationHighlight}path{Information}][{InformationHighlight}filename{Information}] [{InformationHighlight}{short}A{Information}[[:{InformationHighlight}attributes{Information}}]] [{InformationHighlight}{short}O{Information}[[:{InformationHighlight}sortorder{Information}]]] [{InformationHighlight}{short}T{Information}[[:{InformationHighlight}timefield{Information}]]] [{InformationHighlight}{short}S{Information}] [{InformationHighlight}{short}W{Information}] [{InformationHighlight}{short}B{Information}] [{InformationHighlight}{short}P{Information}] [{InformationHighlight}{short}M{Information}] [{InformationHighlight}{long}Env{Information}] [{InformationHighlight}{long}Config{Information}] [{InformationHighlight}{long}Owner{Information}] [{InformationHighlight}{long}Streams{Information}]
```

Where:
- `{VERSION}` = e.g., "4.2.1423"
- `{ARCH}` = "x64" or "ARM64"
- `{BUILD_DATE}` = e.g., "Feb  7 2026 21:38" (without seconds, from `__DATE__ __TIME__`)
- `{YEAR}` = e.g., "2026"
- `{short}` = `-` or `/`
- `{long}` = `--` or `/`

### D.1.3 Help Body Text

```
{Information}
  [drive:][path][filename]
              Specifies drive, directory, and/or files to list.

  {InformationHighlight}{short}A{Information}          Displays files with specified attributes.
  attributes   {InformationHighlight}D{Information}  Directories                {InformationHighlight}R{Information}  Read-only files
               {InformationHighlight}H{Information}  Hidden files               {InformationHighlight}A{Information}  Files ready for archiving
               {InformationHighlight}S{Information}  System files               {InformationHighlight}T{Information}  Temporary files
               {InformationHighlight}E{Information}  Encrypted files            {InformationHighlight}C{Information}  Compressed files
               {InformationHighlight}P{Information}  Reparse points             {InformationHighlight}0{Information}  Sparse files
               {InformationHighlight}X{Information}  Not content indexed        {InformationHighlight}I{Information}  Integrity stream (ReFS)
               {InformationHighlight}B{Information}  No scrub data (ReFS)       {InformationHighlight}O{Information}  Cloud-only (not local)
               {InformationHighlight}L{Information}  Locally available          {InformationHighlight}V{Information}  Always locally available
               {InformationHighlight}-{Information}  Prefix meaning not

  Cloud status symbols shown between file size and name:
               {CloudStatusCloudOnly}○{Information}  Cloud-only (not locally available)
               {CloudStatusLocallyAvailable}◐{Information}  Locally available (can be freed)
               {CloudStatusAlwaysLocallyAvailable}●{Information}  Always locally available (pinned)

  {InformationHighlight}{short}O{Information}          List by files in sorted order.
  sortorder    {InformationHighlight}N{Information}  By name (alphabetic)       {InformationHighlight}S{Information}  By size (smallest first)
               {InformationHighlight}E{Information}  By extension (alphabetic)  {InformationHighlight}D{Information}  By date/time (oldest first)
               {InformationHighlight}-{Information}  Prefix to reverse order

  {InformationHighlight}{short}T{Information}          Selects the time field for display and sorting.
  timefield    {InformationHighlight}C{Information}  Creation time              {InformationHighlight}A{Information}  Last access time
               {InformationHighlight}W{Information}  Last write time (default)

  {InformationHighlight}{short}S{Information}          Displays files in specified directory and all subdirectories.
  {InformationHighlight}{short}W{Information}          Displays results in a wide listing format.
  {InformationHighlight}{short}B{Information}          Displays bare file names only (no headers, footers, or details).
  {InformationHighlight}{short}P{Information}          Displays performance timing information.
  {InformationHighlight}{short}M{Information}          Enables multi-threaded enumeration (default). Use{InformationHighlight}{disable}{Information} to disable.
  {InformationHighlight}{long}Env{Information}       {pad}Displays TCDIR environment variable help, syntax, and current value.
  {InformationHighlight}{long}Config{Information}    {pad}Displays current color configuration for all items and extensions.
  {InformationHighlight}{long}Owner{Information}     {pad}Displays file owner (DOMAIN\User) for each file.
  {InformationHighlight}{long}Streams{Information}   {pad}Displays alternate data streams (NTFS only).
```

Where `{pad}` = empty for `-` style, single space for `/` style (to align descriptions)

---

## D.2 Environment Variable Help (`--env`)

### D.2.1 Env Help Body

Different syntax shown based on detected shell (checks for `PSModulePath` env var):

**PowerShell:**
```
{Information}Set the {InformationHighlight}TCDIR{Information} environment variable to override default colors for display items, file attributes, or file extensions:
  {InformationHighlight}$env:TCDIR{Information} = "[{InformationHighlight}<Switch>{Information}] | [{InformationHighlight}<Item>{Information} | {InformationHighlight}Attr:<fileattr>{Information} | {InformationHighlight}<.ext>{Information}] = {InformationHighlight}<Fore>{Information} [on {InformationHighlight}<Back>{Information}][;...]"

  {InformationHighlight}<Switch>{Information}    A command-line switch:
                  {InformationHighlight}W{Information}        Wide listing format
                  {InformationHighlight}P{Information}        Display performance timing information
                  {InformationHighlight}S{Information}        Recurse into subdirectories
                  {InformationHighlight}M{Information}        Enables multi-threaded enumeration (default); use {InformationHighlight}M-{Information} to disable
                  {InformationHighlight}Owner{Information}    Display file ownership
                  {InformationHighlight}Streams{Information}  Display alternate data streams (NTFS)

  {InformationHighlight}<Item>{Information}      A display item:
                  {InformationHighlight}D{Information}  Date                     {InformationHighlight}T{Information}  Time
                  {InformationHighlight}S{Information}  Size                     {InformationHighlight}R{Information}  Directory name
                  {InformationHighlight}I{Information}  Information              {InformationHighlight}H{Information}  Information highlight
                  {InformationHighlight}E{Information}  Error                    {InformationHighlight}F{Information}  File (default)
                  {InformationHighlight}O{Information}  Owner                    {InformationHighlight}M{Information}  Stream

              Cloud status (use full name, e.g., {InformationHighlight}CloudOnly=Blue{Information}):
                  {InformationHighlight}CloudOnly{Information}                   {InformationHighlight}LocallyAvailable{Information}
                  {InformationHighlight}AlwaysLocallyAvailable{Information}

  {InformationHighlight}<.ext>{Information}      A file extension, including the leading period.

  {InformationHighlight}<FileAttr>{Information}  A file attribute (see file attributes below)
                  {InformationHighlight}R{Information}  Read-only                {InformationHighlight}H{Information}  Hidden
                  {InformationHighlight}S{Information}  System                   {InformationHighlight}A{Information}  Archive
                  {InformationHighlight}T{Information}  Temporary                {InformationHighlight}E{Information}  Encrypted
                  {InformationHighlight}C{Information}  Compressed               {InformationHighlight}P{Information}  Reparse point
                  {InformationHighlight}0{Information}  Sparse file

  {InformationHighlight}<Fore>{Information}      Foreground color
  {InformationHighlight}<Back>{Information}      Background color
```

**CMD:**
```
  set {InformationHighlight}TCDIR{Information} =[{InformationHighlight}<Switch>{Information}] | ...
```
(rest of body is identical)

### D.2.2 Color Chart

Displayed as two columns showing all 16 colors with sample text:

```
                  {Black}Black{Default}             {DarkGrey}DarkGrey{Default}
                  {Blue}Blue{Default}              {LightBlue}LightBlue{Default}
                  {Green}Green{Default}             {LightGreen}LightGreen{Default}
                  {Cyan}Cyan{Default}              {LightCyan}LightCyan{Default}
                  {Red}Red{Default}               {LightRed}LightRed{Default}
                  {Magenta}Magenta{Default}           {LightMagenta}LightMagenta{Default}
                  {Brown}Brown{Default}             {Yellow}Yellow{Default}
                  {LightGrey}LightGrey{Default}         {White}White{Default}
```

Column widths:
- Left column: 18 characters total (color name + padding)
- Spacing: 18 spaces before first column

**Visibility handling:** If a color name would be invisible (foreground matches background), display on contrasting background.

### D.2.3 Example Line

**PowerShell:**
```
{Information}  Example: {InformationHighlight}$env:TCDIR{Information} = "W;D=LightGreen;S=Yellow;Attr:H=DarkGrey;.cpp=White on Blue"
```

**CMD:**
```
{Information}  Example: {InformationHighlight}set TCDIR{Information} = W;D=LightGreen;S=Yellow;Attr:H=DarkGrey;.cpp=White on Blue
```

### D.2.4 Current Value Display (if TCDIR is set)

```
{Information}Your settings:{Default}

  {Information}TCDIR{Default} = "{segment1};{segment2};..."
```

Each segment in the value is colorized:
- For `key=colorspec` entries: display both key and value in the parsed color
- For invalid entries: display in default color
- Semicolons displayed in default color

### D.2.5 Decoded Settings Display

If TCDIR has values from environment, show them grouped:

```
    {Information}Switches:
      {Default}W        Wide listing format
      {Default}S        Recurse into subdirectories

    {Information}Display item colors:
      {ColoredItem}Date
      {ColoredItem}Size

    {Information}File attribute colors:
      {ColoredAttr}H Hidden

    {Information}File extension colors:
      {ColoredExt}.cpp
      {ColoredExt}.h
```

### D.2.6 Error Display

```
{Default}
{Error}There are some problems with your TCDIR environment variable (see --env for help):
{Error}  Invalid foreground color in ".cpp=FakeColor"
{Default}                                  ‾‾‾‾‾‾‾‾‾
```

Error format:
- Error description + ` in "` + full entry + `"`
- Underline using overline character (U+203E) positioned under the invalid portion
- Underline length matches invalid text length
- Extra blank line after each error

---

## D.3 Configuration Display (`--config`)

### D.3.1 Display Item Configuration

```
{Information}
Current display item configuration:

  {ColoredDefault}Default                    {Source}
  {ColoredDate}Date                       {Source}
  {ColoredTime}Time                       {Source}
  {ColoredAttrPresent}File attribute present     {Source}
  {ColoredAttrAbsent}File attribute absent      {Source}
  {ColoredSize}Size                       {Source}
  {ColoredDirectory}Directory                  {Source}
  {ColoredInfo}Information                {Source}
  {ColoredInfoHighlight}Info highlight             {Source}
  {ColoredSeparator}Separator line             {Source}
  {ColoredError}Error                      {Source}
  {ColoredOwner}Owner                      {Source}
  {ColoredStream}Stream                     {Source}
  {ColoredCloudOnly}CloudOnly (○)              {Source}
  {ColoredLocallyAvail}LocallyAvailable (◐)       {Source}
  {ColoredAlwaysLocal}AlwaysLocallyAvailable (●) {Source}
```

Where `{Source}` is:
- `{Cyan}Environment` if from TCDIR env var
- `{DarkGrey}Default` if built-in default

Column widths:
- Item name: 27 characters
- Source: 15 characters

### D.3.2 File Attribute Configuration

Only shows attributes that have explicit color overrides:

```
{Information}
File attribute color configuration:

  {ColoredAttr}H Hidden                   {Source}
  {ColoredAttr}E Encrypted                {Source}
```

Format: `{letter} {name}` in the attribute's color

### D.3.3 File Extension Configuration

```
{Information}
File extension color configuration:
```

If console width allows multiple columns (minimum column width = extension + "  " + "Environment" + padding):
- Extensions displayed in column-major order (like wide listing)
- Each extension shown in its configured color
- Source shown next to each extension

Single column format:
```
  {ColoredExt}.7z                        {Source}
  {ColoredExt}.asm                       {Source}
  ...
```

Multi-column format (example with 3 columns):
```
  {ext1} {src}    {ext2} {src}    {ext3} {src}
  {ext4} {src}    {ext5} {src}    {ext6} {src}
```

Extensions sorted alphabetically.

---

## D.4 Error Message Formats

### D.4.1 Path Not Found

```
{Error}Error:   {InformationHighlight}{path}{Error} does not exist
```

### D.4.2 Empty Directory

If all file specs are `*`:
```
{Default}Directory is empty.
```

If specific specs:
```
{Default}No files matching '{spec}' found.
```

For multiple specs:
```
{Default}No files matching '{spec1}, {spec2}' found.
```

### D.4.3 TCDIR Parse Errors (shown at end of run)

```
{Error}There are some problems with your TCDIR environment variable (see {long}env for help):
{Error}  {error_description} in "{full_entry}"
{Default}{padding}‾‾‾...
```

Where:
- `{long}` = `--` or `/` based on switch style used
- `{padding}` = spaces to position underline under invalid portion
- Underline uses U+203E (overline) character, length matches invalid text

Error descriptions:
- `Invalid foreground color`
- `Invalid background color`  
- `Invalid entry format (expected key = value)`
- `Invalid key (expected single character, .extension, or attr:x)`
- `Invalid display attribute character (valid: D,T,A,-,S,R,I,H,E,F,O)`
- `Invalid file attribute key (expected attr:<x>)`
- `Invalid file attribute character (expected R, H, S, A, T, E, C, P or 0)`
- `Invalid switch (expected W, S, P, M, B, Owner, or Streams)`
- `Switch prefixes (/, -, --) are not allowed in env var`