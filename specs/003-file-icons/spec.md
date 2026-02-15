# Feature Specification: Nerd Font File & Folder Icons

**Feature Branch**: `003-file-icons`  
**Created**: 2026-02-13  
**Updated**: 2026-02-14  
**Status**: Draft  
**Input**: User description: "Add Nerd Font file and folder icons to directory listings similar to Terminal-Icons PowerShell module"

## Clarifications

### Session 2026-02-14

- Q: What should the CLI flags for icon enable/disable be? → A: `/Icons` to enable, `/Icons-` to disable (verbose, matches env var switch name exactly).
- Q: What is explicitly out of scope for this feature? → A: Per-icon color independent of filename color, image/thumbnail previews, custom theme files, and Linux/macOS support (Linux/macOS may be pursued later but no need to plan for it now).
- Q: How should icon detection diagnostics be surfaced? → A: Extend existing `/config` (show detection result + resolved icon state with reason), `/env` (document icon syntax + show icon-related TCDIR overrides), and `/?` (document `/Icons` and `/Icons-` flags). No new diagnostic flag needed.
- Q: How should duplicate/conflicting TCDIR entries be handled? → A: First-write-wins (first value seen is used), and subsequent duplicates/conflicts are flagged as errors using the existing env var validation pattern (underlined error display).
- Note: This feature constitutes a new **major version** of TCDir. Version number must be incremented accordingly when implementation begins.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Display File-Type Icons in Normal Listings (Priority: P1)

A user runs `tcdir` in a directory containing a mix of file types (`.cpp`, `.h`, `.exe`, `.pdf`, `.txt`, folders). Each entry shows a small icon glyph prepended to the filename that visually indicates the file type — for example, a folder icon before directory names, a C++ icon before `.cpp` files, and a generic file icon for unrecognized extensions.

**Why this priority**: This is the core value proposition — making directory listings visually scannable at a glance. Without this, the feature doesn't exist.

**Independent Test**: Run `tcdir` on a system with a Nerd Font and verify that each entry has an appropriate icon glyph prepended to its filename with correct spacing.

**Acceptance Scenarios**:

1. **Given** a directory with `.cpp`, `.h`, `.exe`, `.txt`, and subdirectory entries, **When** icons are active, **Then** each entry displays a recognized icon glyph followed by a space before the filename.
2. **Given** a file with an unrecognized extension (e.g., `.xyz`), **When** icons are active, **Then** a generic file icon is displayed before the filename.
3. **Given** a subdirectory entry, **When** icons are active, **Then** a folder icon is displayed before the directory name.
4. **Given** a symlink or junction point, **When** icons are active, **Then** a distinct link icon is displayed before the entry name.

---

### User Story 2 - Auto-Detection of Nerd Font with Manual Override (Priority: P1)

When a user launches `tcdir`, the system automatically detects whether Nerd Font glyphs are available in the current terminal. The detection uses a layered strategy that works across both classic conhost and modern ConPTY-hosted terminals (Windows Terminal, VS Code, etc.). If Nerd Font support is detected, icons are shown automatically. If not, the classic output is preserved. The user can always override with `/Icons` (force-enable) or `/Icons-` (force-disable) on the command line, or via the `TCDIR` environment variable.

**Why this priority**: Equally critical as Story 1 — automatically doing the right thing means users get icons with zero configuration when they have a Nerd Font, and never see broken glyphs when they don't.

**Independent Test**: On a system with a Nerd Font as the console font, run `tcdir` and verify icons appear. On a system with a non-Nerd Font, run `tcdir` and verify classic output. Then test explicit overrides in both environments.

**Acceptance Scenarios**:

1. **Given** a classic conhost session using a Nerd Font, **When** running `tcdir`, **Then** icons are automatically displayed (detected via glyph canary probe).
2. **Given** a classic conhost session using Consolas, **When** running `tcdir`, **Then** the listing appears identical to the pre-feature version.
3. **Given** a WezTerm session (any font), **When** running `tcdir`, **Then** icons are automatically displayed (WezTerm bundles Nerd Font Symbols as a fallback).
4. **Given** a Windows Terminal or VS Code session on a system with a Nerd Font installed, **When** running `tcdir`, **Then** icons are automatically displayed (detected via system font enumeration heuristic).
5. **Given** a Windows Terminal session on a system with no Nerd Font installed, **When** running `tcdir`, **Then** the listing appears identical to the pre-feature version.
6. **Given** any terminal, **When** running `tcdir /Icons`, **Then** icons appear regardless of font detection.
7. **Given** any terminal, **When** running `tcdir /Icons-`, **Then** icons are suppressed regardless of font detection.
8. **Given** `TCDIR=Icons`, **When** running `tcdir` without a CLI flag, **Then** icons are always enabled (overriding auto-detect).
9. **Given** `TCDIR=Icons-`, **When** running `tcdir` without a CLI flag, **Then** icons are always disabled (overriding auto-detect).
10. **Given** `TCDIR=Icons` but the user passes `/Icons-` on the CLI, **When** running `tcdir /Icons-`, **Then** CLI flag wins and icons are suppressed.

---

### User Story 3 - Icon Colors Match File Type Colors (Priority: P1)

When icons are displayed, the icon glyph uses the same color as the filename itself. Directory icons use the directory color, `.cpp` file icons use the `.cpp` extension color, hidden file icons use the hidden-file attribute color, etc. This means the icon inherits the existing color system with no additional configuration required.

**Why this priority**: Color consistency with existing file-type coloring is integral to the icon experience. Showing all icons in one flat color would look wrong. Since the color infrastructure already exists, this should be part of the core implementation.

**Independent Test**: Run `tcdir` with icons active and verify that each icon glyph appears in the same color as its corresponding filename.

**Acceptance Scenarios**:

1. **Given** icons are active, **When** displaying a directory entry, **Then** the folder icon is shown in the same color as the directory name.
2. **Given** icons are active, **When** displaying a `.cpp` file, **Then** the icon color matches the `.cpp` extension color.
3. **Given** icons are active and a file has a special attribute (hidden, encrypted), **When** the attribute color override takes effect, **Then** the icon color matches the overridden filename color.
4. **Given** `TCDIR=.cpp=Yellow`, **When** displaying a `.cpp` file with icons, **Then** both the icon and filename appear in Yellow.

---

### User Story 4 - Environment Variable Icon Configuration (Priority: P2)

A user can customize icons through the same `TCDIR` environment variable they already use for colors. The existing `key=value` syntax is extended: a comma after the color value introduces an icon override. The icon can be a `U+XXXX` hex code point or a literal glyph character. The `Icons` / `Icons-` switch controls the global on/off state. Well-known directory names use a `dir:` prefix.

**Why this priority**: Env variable configuration extends the feature from "nice default" to "customizable for power users." It reuses the existing syntax rather than inventing a new one.

**Independent Test**: Set various `TCDIR` icon entries and verify correct behavior.

**Acceptance Scenarios**:

1. **Given** `TCDIR=Icons`, **When** running `tcdir` without flags, **Then** icons are always displayed (overriding auto-detect to ON).
2. **Given** `TCDIR=Icons-`, **When** running `tcdir` without flags, **Then** icons are always suppressed (overriding auto-detect to OFF).
3. **Given** `TCDIR=.py=Green,U+E73C`, **When** displaying a `.py` file with icons, **Then** the Python icon at U+E73C is shown in Green.
4. **Given** `TCDIR=.py=,U+E73C`, **When** displaying a `.py` file with icons, **Then** the Python icon at U+E73C is shown in the default color.
5. **Given** `TCDIR=.obj=,`, **When** displaying a `.obj` file with icons, **Then** no icon is shown for that extension (icon suppressed).
6. **Given** `TCDIR=dir:.git=,U+F1D3`, **When** displaying a `.git` directory with icons, **Then** the Git icon at U+F1D3 is used.
7. **Given** `TCDIR=attr:H=DarkGrey,U+F21B`, **When** displaying a hidden file with icons, **Then** a ghost icon at U+F21B is shown in DarkGrey.
8. **Given** `TCDIR=.cpp=Green`, **When** displaying a `.cpp` file with icons, **Then** original behavior is preserved — Green color, default built-in C++ icon (no comma = no icon change).

---

### User Story 5 - Icons in Wide and Bare Display Modes (Priority: P3)

When the user runs `tcdir` in wide mode (`/W`) or bare mode (`/B`) with icons active, icons appear before each entry, and column alignment accounts for the icon width.

**Why this priority**: Normal mode is the primary use case. Wide and bare modes are secondary but should work correctly for completeness.

**Independent Test**: Run `tcdir /W` and `tcdir /B` with icons active and verify icons appear with correct alignment in each mode.

**Acceptance Scenarios**:

1. **Given** wide display mode with icons active, **When** viewing multi-column output, **Then** columns are properly aligned accounting for the icon character width.
2. **Given** bare display mode with icons active, **When** viewing the output, **Then** each filename is preceded by its icon and a space.
3. **Given** wide display mode in a cloud sync root, **When** viewing multi-column output, **Then** each entry shows a cloud status symbol before the icon/filename, similar to File Explorer's List view.
4. **Given** wide display mode with icons active, **When** displaying a directory entry, **Then** the directory name is shown without brackets (the folder icon provides the visual distinction).
5. **Given** wide display mode with icons NOT active, **When** displaying a directory entry, **Then** directory names retain their `[brackets]` as before.

---

### User Story 6 - Well-Known Folder Icons (Priority: P3)

Certain well-known directory names (e.g., `.git`, `.github`, `node_modules`, `src`, `docs`, `bin`, `build`) display folder-specific icons rather than the generic folder icon.

**Why this priority**: This adds polish and is directly inspired by Terminal-Icons' "WellKnown" directory mappings, but generic folder icons already provide good value.

**Independent Test**: Create directories with well-known names and verify each shows a distinct icon.

**Acceptance Scenarios**:

1. **Given** a directory named `.git` with icons active, **When** viewing the listing, **Then** a Git-specific icon is displayed instead of the generic folder icon.
2. **Given** a directory named `src` with icons active, **When** viewing the listing, **Then** a source-code-specific folder icon is displayed.
3. **Given** a directory with an unrecognized name, **When** viewing the listing, **Then** the generic folder icon is still displayed.
4. **Given** `TCDIR=dir:src=,U+ABCD`, **When** viewing a `src` directory, **Then** the user's custom icon overrides the built-in well-known icon.

---

### User Story 7 - Enhanced Cloud Status Symbols (Priority: P3)

When icons are active and the listing is within a cloud sync root (OneDrive, etc.), cloud status symbols upgrade from basic Unicode geometric shapes to richer Nerd Font glyphs that more clearly communicate sync state.

**Why this priority**: Cloud status is displayed in its own column, separate from file-type icons. The upgrade is cosmetic polish that leverages the Nerd Font already being available.

**Independent Test**: Run `tcdir` with icons active in a OneDrive-synced folder and verify cloud status symbols use NF glyphs instead of circles.

**Acceptance Scenarios**:

1. **Given** icons are active in a cloud sync root, **When** displaying a cloud-only file, **Then** a cloud outline glyph replaces the hollow circle (○).
2. **Given** icons are active in a cloud sync root, **When** displaying a locally available file, **Then** a cloud-with-checkmark glyph replaces the half-filled circle (◐).
3. **Given** icons are active in a cloud sync root, **When** displaying an always-local (pinned) file, **Then** a cloud-lock or pin glyph replaces the filled circle (●).
4. **Given** icons are NOT active, **When** displaying cloud status, **Then** the original Unicode circles are used (zero regression).

---

### Edge Cases

- What happens when auto-detect returns a false positive (heuristic says NF is installed but terminal isn't using it)? The listing displays broken glyph characters but remains functional — filenames and data are correct. The user can force-disable via `/Icons-` or `TCDIR=Icons-`.
- What happens with very long filenames that would be truncated? The icon + space takes up 2 character positions; truncation logic must account for this additional width.
- What happens when output is redirected to a file or piped? Icons follow the same enable/disable logic — if the user force-enabled them or auto-detect is positive, they appear in the output as valid Unicode characters.
- What happens with files that have no extension? A generic file icon is used (falls through to the file type fallback level).
- How are Nerd Font glyphs handled when multiple characters wide? Only single-width glyphs should be used to avoid column misalignment.
- What happens when a file has multiple attributes (e.g., hidden + system + readonly)? The attribute with the highest priority in the fixed precedence order wins for both color and icon (see Precedence Chain below).
- What happens when the console font cannot be queried (e.g., SSH session)? Auto-detect defaults to OFF (no icons). The user can force-enable with `/Icons` or `TCDIR=Icons`.

## Font Dependency & Alternatives

### Nerd Fonts (Required)

Icons in this feature rely on **Nerd Fonts** — a collection of 67+ patched developer fonts that include 10,000+ glyphs from popular icon sets (Font Awesome, Material Design Icons, Devicons, Octicons, Codicons, etc.) in the Unicode Private Use Area. This is the de facto standard used by Terminal-Icons, Oh My Posh, LSD, colorls, Starship, and many other terminal tools.

Nerd Fonts is not a font itself — it's a patching project. Users choose their preferred programming font (e.g., CaskaydiaCove = Cascadia Code, FiraCode, JetBrainsMono, Hack, Meslo, etc.) and install the "Nerd Font" variant which includes all the extra glyphs.

### Are There Alternatives?

There are **no practical alternatives** that provide comparable coverage:

- **Standard Unicode** has some basic shapes (arrows, geometric forms, dingbats) but lacks file-type-specific icons (no C++ symbol, no Python snake, no Git icon, no folder-open icon, etc.).
- **Emoji** exist on most systems but are typically double-width, inconsistently rendered across platforms, and lack developer-specific icons (no `.cpp` emoji, no Git emoji).
- **Powerline fonts** only include a handful of separator/arrow glyphs — no file-type icons.
- **Nerd Fonts "Symbols Only"** variant can be installed as a **font fallback** alongside any existing font, so users don't even need to change their primary font.

**Conclusion**: Nerd Fonts is effectively required. This is consistent with how every comparable tool in the ecosystem works (Terminal-Icons, LSD, colorls, etc.).

### Auto-Detection Strategy

Detecting Nerd Font availability is complicated by the ConPTY architecture used by all modern terminals (Windows Terminal, VS Code, WezTerm, Alacritty, etc.). In ConPTY, the console application talks to a hidden `conhost.exe` shim — not the real terminal. Font-querying APIs (`GetCurrentConsoleFontEx`) return the shim's fake font name, not the terminal's actual rendering font. There is no API, escape sequence, or buffer attribute that reveals whether a glyph was successfully rendered.

The detection uses a **layered strategy** (evaluated in order, first definitive result wins):

| Step | Condition | Method | Result |
|------|-----------|--------|--------|
| 1 | CLI flag `/Icons` or `/Icons-` | User explicitly passed force-enable or force-disable flag | Definitive ON or OFF |
| 2 | `TCDIR` env var has `Icons` or `Icons-` | User configured global icon preference | Definitive ON or OFF |
| 3 | Running in WezTerm (`TERM_PROGRAM=WezTerm`) | WezTerm bundles Nerd Font Symbols as a default fallback font | ON |
| 4 | Running in classic conhost (no ConPTY env vars detected) | Glyph canary probe: select the console font, test for glyph at U+E5FA via glyph index lookup | Definitive ON or OFF |
| 5 | Running in a ConPTY-hosted terminal (`WT_SESSION`, `TERM_PROGRAM`, `ConEmuPID`, `ALACRITTY_WINDOW_ID`, etc.) | System font enumeration: check if any Nerd Font is installed on the system | Heuristic ON or OFF |
| 6 | All above inconclusive | Fallback | OFF |

**ConPTY detection**: If any of `WT_SESSION`, `TERM_PROGRAM`, `ConEmuPID`, or `ALACRITTY_WINDOW_ID` are set, the session is ConPTY-hosted and font query APIs are unreliable.

**Canary code point** (step 4): U+E5FA (`nf-seti-folder`) is in the Seti-UI range, specific to Nerd Fonts v3, and absent from all standard Windows console fonts (Consolas, Cascadia Code, Courier New, Lucida Console, Terminal raster). A glyph index lookup returning a valid index (not 0xFFFF) is definitive proof the console font has Nerd Font glyphs.

**System font enumeration heuristic** (step 5): Enumerating installed font families and checking for Nerd Font naming patterns ("Nerd Font", " NF", " NFM", " NFP" suffixes). This is a heuristic — "a Nerd Font is installed" ≠ "the terminal is using it" — but it's a reasonable bet: users don't install Nerd Fonts by accident. False positives are handled gracefully via `Icons-` override.

## TCDIR Environment Variable Syntax

### Icon Override Syntax

The existing `key=color` syntax is extended with an optional comma-separated icon component:

```
key=[color][,icon]
```

Where `icon` is one of:
- **`U+XXXX`** — a Unicode code point in standard hex notation (4–6 hex digits)
- **A literal glyph** — a single character (the actual NF glyph pasted directly)
- **Empty** — suppresses the icon for that entry type (comma present, nothing after it)
- **Absent** — no comma means no icon change (fully backward compatible)

### Entry Types

| Entry format | Applies to | Example |
|-------------|------------|---------|
| `.ext=[color][,icon]` | Files with the given extension | `.cpp=Green,U+E61D` |
| `attr:X=[color][,icon]` | Files with the given attribute | `attr:H=DarkGrey,U+F21B` |
| `dir:name=[color][,icon]` | Well-known directory names (case-insensitive) | `dir:.git=,U+F1D3` |
| `R=[color][,icon]` | Default directory color + icon | `R=Yellow,U+F115` |
| `F=[color][,icon]` | Default file color + icon | `F=White,U+F15B` |
| `Icons` | Global icon switch: force ON | `Icons` |
| `Icons-` | Global icon switch: force OFF | `Icons-` |

### Examples

```
TCDIR=Icons;S;.cpp=Green,U+E61D;R=,U+F115;attr:H=DarkGrey;dir:.git=,U+F1D3;.obj=,
       │     │  │                  │          │                │             │
       │     │  │                  │          │                │             └─ suppress .obj icon
       │     │  │                  │          │                └─ .git dir gets Git icon
       │     │  │                  │          └─ hidden files: DarkGrey, default icon
       │     │  │                  └─ directories: default color, folder icon
       │     │  └─ .cpp: Green color + C++ icon
       │     └─ existing: recurse into subdirectories
       └─ force icons ON
```

Backward compatibility: entries without a comma (e.g., `.cpp=Green`, `attr:H=DarkGrey`) behave exactly as before — color only, no icon change.

### Duplicate & Conflicting Entries

When the same key appears multiple times in the `TCDIR` variable (e.g., `.cpp=Green,U+E61D;.cpp=Red` or `Icons;Icons-`), the **first value encountered wins**. Subsequent duplicate or conflicting entries are flagged as errors using the existing env var validation pattern (underlined error display on normal runs, detailed in `/config`). This applies uniformly to color entries, icon entries, and switches.

## Icon & Color Precedence Chain

Both color and icon selection follow a single, consistent precedence chain. The same entry type that wins for color also wins for icon. This avoids a split mental model.

### Precedence Levels (first match wins)

| Priority | Level | Matches | Icon example |
|----------|-------|---------|-------------|
| 1 (highest) | File attribute | Walks the attribute precedence array (see below); first configured match wins | Ghost icon for hidden files |
| 2 | Well-known directory name | Exact directory name match (case-insensitive); directories only | Git icon for `.git` |
| 3 | File extension | Extension match (case-insensitive); files only | C++ icon for `.cpp` |
| 4 (lowest) | File type fallback | Generic directory, symlink, junction, or generic file | Folder icon, file icon |

At each level, if a color/icon is configured (built-in or user override), it wins and the chain stops. If no color/icon is configured at that level, evaluation falls through to the next level.

### File Attribute Precedence Order

When a file has multiple attributes set, the first match in this fixed priority order wins for both color and icon:

| Priority | Attribute | Key | Rationale |
|----------|-----------|-----|-----------|
| 1 (highest) | `REPARSE_POINT` | `P` | Identity-altering — symlink/junction point |
| 2 | `SYSTEM` | `S` | OS-critical — danger to modify/delete |
| 3 | `HIDDEN` | `H` | Intentionally invisible — noteworthy if seen |
| 4 | `ENCRYPTED` | `E` | Access-restricting — can't be read by others |
| 5 | `READONLY` | `R` | Access-restricting — can't be modified |
| 6 | `COMPRESSED` | `C` | Informational — disk space implication |
| 7 | `SPARSE_FILE` | `0` | Rare, niche |
| 8 | `TEMPORARY` | `T` | Rare, ephemeral |
| 9 (lowest) | `ARCHIVE` | `A` | Near-universal, effectively noise |

**Note**: This precedence order applies to color and icon selection only. The display column showing attribute letters (`RHSATECP0`) retains its existing order for visual consistency with `attrib` and `Get-ChildItem`.

**Default behavior**: No built-in attribute icons are shipped initially. Attribute entries only have default *colors*. Since no attribute-level icon is configured by default, the attribute level will match for color but fall through for icon — meaning a hidden `.cpp` file gets DarkGrey color (attribute wins) and C++ icon (extension wins, since attribute has no icon). Users can configure attribute-level icons via `TCDIR=attr:H=DarkGrey,U+F21B` if desired.

### Precedence Example: Hidden `.cpp` File

```
Level 1 (attributes): HIDDEN matches, has color=DarkGrey but no icon → color=DarkGrey, icon=fall through
Level 2 (well-known dir): N/A (it's a file)
Level 3 (extension): .cpp matches → icon=C++ icon
Result: DarkGrey C++ icon + DarkGrey filename
```

### Precedence Example: Hidden Directory Named `.git`

```
Level 1 (attributes): HIDDEN matches, has color=DarkGrey but no icon → color=DarkGrey, icon=fall through
Level 2 (well-known dir): .git matches → icon=Git icon
Result: DarkGrey Git icon + DarkGrey directory name
```

### Precedence Example: System Hidden Directory Symlink

```
Level 1 (attributes): REPARSE_POINT matches (priority 1), has color but no icon → color wins, icon=fall through
                       SYSTEM (priority 2), HIDDEN (priority 3) — skipped, REPARSE_POINT already won
Level 2 (well-known dir): no match (generic name)
Level 3 (extension): N/A (directory)
Level 4 (type fallback): directory → generic folder icon
Result: reparse-point color + folder icon
```

### Precedence Example: User Configures Attribute Icon

Given `TCDIR=attr:H=DarkGrey,U+F21B`:

```
Level 1 (attributes): HIDDEN matches, has color=DarkGrey AND icon=U+F21B → color=DarkGrey, icon=ghost
Result: DarkGrey ghost icon + DarkGrey filename (icon no longer falls through)
```

## Cloud Status Symbols

Cloud status is displayed as a **separate column**, independent of the file-type icon. When icons are active, the cloud status symbols upgrade from basic Unicode geometric shapes to richer Nerd Font glyphs:

| Status | Without Icons | With Icons | NF Class | Code Point |
|--------|--------------|------------|----------|------------|
| Cloud-only (needs download) | ○ (hollow circle) | Cloud outline | `nf-md-cloud_outline` | U+F0163 |
| Locally available (freeable) | ◐ (half-filled circle) | Cloud with checkmark | `nf-md-cloud_check` | U+F0160 |
| Always local (pinned) | ● (filled circle) | Pin | `nf-md-pin` | U+F0403 |
| Not in sync root | (space) | (space) | — | — |

Cloud status colors are unchanged — the existing `CloudStatusCloudOnly`, `CloudStatusLocallyAvailable`, and `CloudStatusAlwaysLocallyAvailable` color attributes continue to apply.

## Requirements *(mandatory)*

### Functional Requirements

#### Icon Display

- **FR-001**: System MUST display an appropriate icon glyph before each filename in the listing when icons are active.
- **FR-002**: System MUST provide default icon mappings for every file extension already in TCDir's built-in color table (`.cpp`, `.h`, `.exe`, `.dll`, `.py`, `.js`, `.ts`, `.json`, `.xml`, `.yml`, `.md`, `.txt`, `.zip`, `.sln`, etc.) plus directories and generic fallbacks.
- **FR-003**: System MUST display a folder icon before directory entries and a generic file icon before entries with unrecognized extensions.
- **FR-004**: System MUST display distinct icons for symlinks and junction points.
- **FR-005**: System MUST support well-known directory name mappings (e.g., `.git`, `node_modules`, `src`, `docs`) with folder-specific icons, configurable via the `dir:` prefix in the `TCDIR` env var.
- **FR-006**: System MUST use only single-width Nerd Font glyphs to maintain consistent column alignment.
- **FR-007**: System MUST insert exactly one space between the icon glyph and the filename. In normal mode, the icon is placed immediately before the filename, after all other columns (date, time, attributes, size, cloud status, owner). In wide and bare modes, the icon precedes each entry name directly.
- **FR-008**: Icon display MUST work correctly in all display modes (normal, wide, bare) when icons are active.
- **FR-008a**: In wide display mode, directory name brackets (`[name]`) MUST be suppressed when icons are active, since the folder icon already provides visual distinction. When icons are off, brackets MUST be retained for backward compatibility.
- **FR-009**: Icon lookup MUST follow the same precedence chain as color lookup: file attribute → well-known directory name → file extension → file type fallback.
- **FR-010**: Within the attribute level, precedence MUST follow a fixed order: REPARSE_POINT → SYSTEM → HIDDEN → ENCRYPTED → READONLY → COMPRESSED → SPARSE_FILE → TEMPORARY → ARCHIVE.
- **FR-011**: Attribute-level precedence order for color and icon selection MUST be independent of the attribute display column order (which retains existing `attrib`/`gci` conventions).

#### Icon Activation

- **FR-012**: System MUST auto-detect Nerd Font availability at startup using a layered strategy: (1) CLI flag, (2) `TCDIR` env var `Icons`/`Icons-`, (3) WezTerm detection, (4) glyph canary probe in classic conhost, (5) system font enumeration heuristic in ConPTY terminals, (6) default OFF.
- **FR-013**: When a Nerd Font is detected and no explicit override is set, icons MUST be displayed automatically.
- **FR-014**: When no Nerd Font is detected and no explicit override is set, icons MUST NOT be displayed.
- **FR-015**: System MUST provide the `/Icons` command-line flag to force-enable icons (overriding auto-detect).
- **FR-016**: System MUST provide the `/Icons-` command-line flag to force-disable icons (overriding auto-detect).
- **FR-017**: When detection is inconclusive and no override is set, icons MUST default to OFF.
- **FR-018**: CLI flags MUST take precedence over `TCDIR` environment variable, which MUST take precedence over auto-detection.

#### Icon Colors

- **FR-019**: Icon color MUST default to the same color as the filename for the same entry — determined by the same precedence chain used for icon selection.
- **FR-020**: Both color and icon MUST be resolved through the same precedence chain. The winning level for color determines the icon (unless that level has no icon configured, in which case icon evaluation continues to lower levels while color is locked).

#### Environment Variable Configuration

- **FR-021**: The `TCDIR` environment variable MUST support `Icons` as a switch name to force-enable icons.
- **FR-022**: The `TCDIR` environment variable MUST support `Icons-` to force-disable icons.
- **FR-023**: The `TCDIR` environment variable MUST support the extended syntax `key=[color][,icon]` where `icon` is a `U+XXXX` code point (4–6 hex digits), a literal single glyph character, or empty (to suppress the icon).
- **FR-024**: Entries without a comma MUST behave identically to the pre-feature format (backward compatible — color only, no icon change).
- **FR-025**: The `dir:` prefix MUST be supported for well-known directory name overrides, following the same `[color][,icon]` syntax.

#### Diagnostics & Help

- **FR-026**: The `/config` output MUST include an icon status line showing the detection result and resolved icon state (e.g., "Nerd Font detected, icons enabled", "Nerd Font not detected, icons disabled", "Icons disabled via TCDIR=Icons-"). The icon status line MUST appear before the configuration tables so the user immediately sees whether icons are active.
- **FR-027**: When icons are active, the `/config` output MUST show icon glyphs inline in the file extension color table (renamed to "file extension color and icon configuration") and in a separate "well-known directory icon configuration" table. Both tables use multi-column layout with source indicators (Default vs Environment). Well-known directories are shown in the Directory display color. When icons are not active, icon glyphs MUST be omitted from the extension table, and the well-known directory table MUST NOT be shown. There MUST NOT be a separate icon mapping table.
- **FR-028**: The `/env` output MUST document the `Icons`/`Icons-` switch, the `[color][,icon]` comma syntax, the `dir:` prefix, and the `U+XXXX` code point format. Layout requirements:
  - The `<Fore>` and `<Back>` descriptions MUST appear immediately before the color name list (not separated by other content).
  - The `<Icon>` description (U+XXXX, literal glyph, empty=suppressed) MUST appear *after* the color name list, not between `<Fore>/<Back>` and the colors.
  - The example MUST include an icon code point (e.g., `.cpp=White on Blue,U+E61D`) to demonstrate the comma syntax.
  - When running under PowerShell, a persist hint MUST be shown after the example: `[Environment]::SetEnvironmentVariable("TCDIR", $env:TCDIR, "User")`.
- **FR-029**: The `/env` output MUST show any icon-related overrides parsed from the current `TCDIR` value.
- **FR-030**: The `/?` usage display MUST document the `/Icons` and `/Icons-` command-line flags.

#### Cloud Status

- **FR-031**: When icons are active, cloud status symbols MUST upgrade from Unicode geometric shapes to Nerd Font glyphs in the existing cloud status column.
- **FR-032**: When icons are not active, cloud status symbols MUST remain unchanged (zero regression).
- **FR-033**: Wide display mode MUST show cloud status for each entry when in a cloud sync root, similar to File Explorer's List view. The cloud status symbol is placed immediately before the icon (or before the filename if icons are off).

### Key Entities

- **Icon Mapping Table**: A built-in lookup structure mapping file extensions, well-known directory names, and file type fallbacks to Nerd Font glyph code points. Parallels the existing extension-to-color table.
- **Glyph**: A single Unicode character from the Nerd Font Private Use Area that visually represents a file type, folder type, or file attribute.
- **Icon Activation State**: A tri-state value (auto / force-on / force-off) determined by the precedence chain: CLI flag → TCDIR env var → auto-detect result.
- **Attribute Precedence Array**: A fixed-order array of file attributes that determines which attribute wins when multiple are present. Used identically for both color and icon selection. Independent of the attribute display column order.

## Default Icon Mappings

The following default mappings cover every extension in TCDir's built-in color table, plus directories and well-known folders. All code points reference Nerd Font v3.

**Glyph selection criteria**: Prefer Seti-UI (`nf-seti-*` / `nf-custom-*`, BMP 4-digit code points) because they were purpose-built for file explorers and require only a single `wchar_t` on Windows. Material Design (`nf-md-*`, 5-digit supplementary plane) is used only when no suitable BMP glyph exists — these require surrogate pairs. See [nerd-font-glyphs.md](nerd-font-glyphs.md) for the full research reference including alternatives.

### File Extension Icons

| Category | Extensions | NF Class | Code Point | Set |
|----------|-----------|----------|------------|-----|
| C source | `.c` | `nf-custom-c` | U+E61E | Seti-UI |
| C++ source | `.cpp`, `.cxx` | `nf-custom-cpp` | U+E61D | Seti-UI |
| C header | `.h` | `nf-custom-c` | U+E61E | Seti-UI |
| C++ header | `.hpp`, `.hxx` | `nf-custom-cpp` | U+E61D | Seti-UI |
| Assembly | `.asm`, `.cod`, `.i` | `nf-custom-asm` | U+E6AB | Seti-UI |
| C# | `.cs`, `.csx` | `nf-seti-c_sharp` | U+E648 | Seti-UI |
| C# resource | `.resx`, `.rcml` | `nf-dev-visualstudio` | U+E70C | Devicons |
| JavaScript | `.js` | `nf-seti-javascript` | U+E60C | Seti-UI |
| JSX | `.jsx` | `nf-dev-react` | U+E7BA | Devicons |
| TypeScript | `.ts` | `nf-seti-typescript` | U+E628 | Seti-UI |
| TSX | `.tsx` | `nf-dev-react` | U+E7BA | Devicons |
| HTML | `.html`, `.htm` | `nf-seti-html` | U+E60E | Seti-UI |
| CSS | `.css` | `nf-seti-css` | U+E614 | Seti-UI |
| SCSS | `.scss` | `nf-dev-sass` | U+E74B | Devicons |
| LESS | `.less` | `nf-dev-less` | U+E758 | Devicons |
| Python | `.py`, `.pyw` | `nf-seti-python` | U+E606 | Seti-UI |
| Java | `.jar`, `.java`, `.class` | `nf-seti-java` | U+E66D | Seti-UI |
| XML | `.xml` | `nf-seti-xml` | U+E619 | Seti-UI |
| JSON | `.json` | `nf-seti-json` | U+E60B | Seti-UI |
| YAML | `.yml`, `.yaml` | `nf-seti-yml` | U+E6A8 | Seti-UI |
| Build artifacts | `.obj`, `.lib`, `.res`, `.pch` | `nf-oct-file_binary` | U+F471 | Octicons |
| Build logs | `.wrn`, `.err`, `.log` | `nf-fa-list` | U+F03A | Font Awesome |
| Shell scripts | `.bash`, `.sh` | `nf-seti-shell` | U+E691 | Seti-UI |
| Batch/CMD | `.bat`, `.cmd` | `nf-custom-msdos` | U+E629 | Seti-UI |
| PowerShell | `.ps1`, `.psd1`, `.psm1` | `nf-seti-powershell` | U+E683 | Seti-UI |
| Executables | `.exe`, `.sys` | `nf-md-application` | U+F08C6 | Material Design* |
| DLL | `.dll` | `nf-fa-archive` | U+F187 | Font Awesome |
| VS Solution | `.sln` | `nf-dev-visualstudio` | U+E70C | Devicons |
| VS Projects | `.vcproj`, `.vcxproj`, `.csproj`, `.csxproj`, `.user`, `.ncb` | `nf-dev-visualstudio` | U+E70C | Devicons |
| Word docs | `.doc`, `.docx` | `nf-seti-word` | U+E6A5 | Seti-UI |
| PowerPoint | `.ppt`, `.pptx` | `nf-md-file_powerpoint` | U+F0227 | Material Design* |
| Excel | `.xls`, `.xlsx` | `nf-seti-xls` | U+E6A6 | Seti-UI |
| Markdown | `.md` | `nf-seti-markdown` | U+E609 | Seti-UI |
| Text files | `.txt`, `.text`, `.!!!`, `.1st`, `.me`, `.now` | `nf-md-file_document` | U+F0219 | Material Design* |
| Email | `.eml` | `nf-fa-envelope` | U+F0E0 | Font Awesome |
| Archives | `.7z`, `.arj`, `.gz`, `.rar`, `.tar`, `.zip` | `nf-oct-file_zip` | U+F410 | Octicons |
| Resource | `.rc` | `nf-seti-config` | U+E615 | Seti-UI |

\* Material Design code points (5-digit, U+Fxxxx) require **surrogate pairs** on Windows (`wchar_t` is 16-bit). Conversion: `hi = 0xD800 + ((cp - 0x10000) >> 10)`, `lo = 0xDC00 + ((cp - 0x10000) & 0x3FF)`.

### Directory Icons

| Type | NF Class | Code Point | Set |
|------|----------|------------|-----|
| Default directory | `nf-custom-folder` | U+E5FF | Seti-UI |
| Symlink directory | `nf-cod-file_symlink_directory` | U+EAED | Codicons |
| Junction point | `nf-fa-external_link` | U+F08E | Font Awesome |

### Well-Known Directory Icons

| Name | NF Class | Code Point | Set |
|------|----------|------------|-----|
| `.git` | `nf-custom-folder_git` | U+E5FB | Seti-UI |
| `.github` | `nf-custom-folder_github` | U+E5FD | Seti-UI |
| `.vscode` | `nf-custom-folder_config` | U+E5FC | Seti-UI |
| `.config` | `nf-seti-config` | U+E615 | Seti-UI |
| `node_modules` | `nf-custom-folder_npm` | U+E5FA | Seti-UI |
| `src`, `source` | `nf-oct-terminal` | U+F489 | Octicons |
| `docs`, `doc`, `documents` | `nf-oct-repo` | U+F401 | Octicons |
| `bin` | `nf-oct-file_binary` | U+F471 | Octicons |
| `build`, `dist`, `out`, `output` | `nf-cod-output` | U+EB9D | Codicons |
| `test`, `tests`, `__tests__`, `spec`, `specs` | `nf-md-test_tube` | U+F0668 | Material Design* |
| `lib`, `libs` | `nf-cod-folder_library` | U+EBDF | Codicons |
| `scripts` | `nf-seti-shell` | U+E691 | Seti-UI |
| `images`, `img`, `assets` | `nf-seti-image` | U+E60D | Seti-UI |
| `packages` | `nf-seti-npm` | U+E616 | Seti-UI |

### Cloud Status Icons

| Status | NF Class | Code Point | Set | Replaces |
|--------|----------|------------|-----|----------|
| Cloud-only (needs download) | `nf-md-cloud_outline` | U+F0163 | Material Design* | ○ hollow circle |
| Locally available (freeable) | `nf-md-cloud_check` | U+F0160 | Material Design* | ◐ half-filled circle |
| Always local (pinned) | `nf-md-pin` | U+F0403 | Material Design* | ● filled circle |

### Generic Fallbacks

| Type | NF Class | Code Point | Set |
|------|----------|------------|-----|
| Unknown file extension | `nf-fa-file` | U+F15B | Font Awesome |
| Unknown directory | `nf-custom-folder` | U+E5FF | Seti-UI |

## Assumptions

- Nerd Font v3 glyph code points are used (the `nf-md-*`, `nf-dev-*`, `nf-fa-*`, `nf-seti-*`, `nf-cod-*`, `nf-oct-*` families).
- The glyph canary probe at U+E5FA is definitive for classic conhost because no standard Windows console font has glyphs in the Nerd Font PUA ranges.
- The system font enumeration heuristic (step 5) is an acceptable trade-off: false positives (NF installed but not active) are rare and gracefully handled via `Icons-`; false negatives (NF active but not detected) don't occur because if no NF is installed, it can't be active.
- Icon mappings for extensions not in the default table silently fall back to the generic file icon. Users can add custom mappings via the comma syntax.
- The `dir:` prefix entries match exact directory names, case-insensitive on Windows.
- Changing the attribute precedence order (from the historical `RHSATEC0P` to `PSHERC0TA`) is a deliberate improvement. This is a **behavioral change** for users who have configured attribute-level color overrides and rely on the old precedence. It should be documented in the changelog.
- This feature warrants a **major version bump** (v4→v5). The combination of new icon functionality, attribute precedence reordering, and env var syntax extension represents a significant milestone.

## Out of Scope

The following are explicitly **not** part of this feature:

- **Per-icon color independent of filename color**: Icons always inherit the filename's resolved color. There is no syntax for setting an icon color separately from the filename color.
- **Image or thumbnail previews**: This feature displays single-glyph icons, not bitmap/image previews.
- **Custom theme files** (e.g., `.tcdir-theme`): Icon configuration is done exclusively through the `TCDIR` environment variable and CLI flags. No external theme file format is introduced.
- **Linux/macOS support**: This feature targets Windows only (classic conhost + ConPTY). Linux/macOS may be pursued as a separate future effort, but no cross-platform abstractions are required for this feature.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Users can visually distinguish file types at a glance — a developer scanning a project directory can identify source files, config files, images, and folders without reading extensions.
- **SC-002**: Existing users experience zero regression — on systems without a Nerd Font, `tcdir` produces byte-identical output to the previous version (except for the attribute precedence reorder, which is a deliberate improvement).
- **SC-003**: On systems with a Nerd Font, icons appear automatically with no user configuration required.
- **SC-004**: The icon feature adds less than 5% overhead to listing time for directories with 1000+ files.
- **SC-005**: Every file extension in TCDir's built-in color table has a corresponding default icon mapping.
- **SC-006**: Column alignment in all display modes (normal, wide, bare) remains correct when icons are active.
- **SC-007**: Icons display correctly when output is redirected or piped (valid Unicode characters in the output stream).
- **SC-008**: Users can force-enable (`/Icons`) or force-disable (`/Icons-`) icons via CLI flag, overriding auto-detection.
- **SC-009**: Users can customize individual icon mappings via the `TCDIR` environment variable using the extended `[color][,icon]` syntax.
- **SC-010**: Cloud status symbols upgrade to Nerd Font glyphs when icons are active, with zero regression when icons are off.
- **SC-011**: Running `tcdir /config` displays the icon detection result and resolved icon state, giving users clear visibility into why icons are on or off.
