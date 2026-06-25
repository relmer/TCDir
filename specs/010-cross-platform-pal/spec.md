# Feature Specification: Cross-Platform Support (Linux & macOS)

**Feature Branch**: `010-cross-platform-pal`
**Created**: 2026-06-24
**Status**: Draft
**Input**: User description: "Bring TCDir's colorized directory-listing experience to Linux (and macOS soon after), so the same tool gives the same listing across platforms. Keep the Windows product unchanged. Windows-only conveniences (terminal-font auto-config, shell aliases) should be supported on the new platforms too, but after the core listing ships." (GH Issue #8)

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Colorized directory listing on Linux (Priority: P1)

A developer on Linux runs `tcdir` in a directory and gets the same colorized,
sorted, icon-annotated listing they would get on Windows: file and directory
names, sizes, modification dates, type/permission indicators, and color coding —
in any of the existing display modes (default, bare, wide, tree).

**Why this priority**: This is the core value of the tool and the minimum viable
cross-platform product. Everything else builds on a working listing.

**Independent Test**: On a Linux machine, list several directories (including
nested trees and large directories) and confirm names, sizes, dates, sorting,
colors, icons, and all four display modes render correctly and match the Windows
output for the same content (excluding platform-specific columns).

**Acceptance Scenarios**:

1. **Given** a directory of mixed files and subdirectories on Linux, **When** the
   user runs the tool with no arguments, **Then** entries are listed with
   colorized names, sizes, dates, and type indicators, sorted per the default order.
2. **Given** the same directory, **When** the user selects bare, wide, or tree
   mode, **Then** that mode renders correctly and consistently with Windows.
3. **Given** a directory containing a file mask (e.g. `*.cpp`), **When** the user
   lists with that mask, **Then** only matching entries appear.
4. **Given** a large directory, **When** the user lists it, **Then** the listing
   completes promptly and without error.

---

### User Story 2 - Platform-appropriate file metadata on Linux (Priority: P2)

The Linux listing reflects the platform's own metadata: owner (user and group),
symlink targets, "hidden" by dotfile convention, and read-only/permission-derived
indicators — rather than Windows-only concepts.

**Why this priority**: Makes the listing genuinely useful and idiomatic on Linux,
but the basic colorized listing (P1) already delivers value without it.

**Independent Test**: List files owned by different users, symlinks (valid and
broken), dotfiles, and read-only files; confirm owner, link target, hidden
treatment, and read-only indicator are correct.

**Acceptance Scenarios**:

1. **Given** files owned by different users/groups, **When** the user lists with
   owner display enabled, **Then** the correct owner is shown for each.
2. **Given** a symbolic link, **When** the user lists it, **Then** it is marked as
   a link and its target is shown.
3. **Given** a dotfile, **When** the user lists with and without "show hidden",
   **Then** it is hidden or shown accordingly.

---

### User Story 3 - macOS support (Priority: P2)

A macOS user gets the same listing experience as Linux, shipping shortly after
Linux. Where macOS exposes cloud/online-only status for a file, the listing shows
it.

**Why this priority**: macOS is a primary target and shares most behavior with
Linux, so it should follow closely — but Linux ships first.

**Independent Test**: Run the same listing scenarios as User Stories 1-2 on macOS
and confirm parity; for a cloud-provider folder with online-only files, confirm
the online-only status is indicated.

**Acceptance Scenarios**:

1. **Given** a directory on macOS, **When** the user lists it, **Then** the result
   matches the Linux experience for the same content.
2. **Given** a cloud folder with online-only (not-yet-downloaded) files on macOS,
   **When** the user lists it, **Then** those files are indicated as online-only.

---

### User Story 4 - Auto-configure popular terminals for the Nerd Font (Priority: P3)

On Linux and macOS, the tool can configure popular terminal emulators to use the
Nerd Font, so icons render — the cross-platform counterpart to the existing
Windows Terminal auto-configuration.

**Why this priority**: A convenience that improves first-run experience, but not
required for a correct listing; explicitly after the MVP.

**Independent Test**: On a supported terminal emulator, run the configure action
and confirm the emulator's font is set to the Nerd Font without disturbing other
settings; confirm a clear message on unsupported emulators.

**Acceptance Scenarios**:

1. **Given** a supported terminal emulator, **When** the user runs the configure
   action, **Then** the emulator is set to use the Nerd Font and other settings
   are preserved.
2. **Given** an unsupported or undetected emulator, **When** the user runs the
   action, **Then** the tool reports clearly that auto-configuration is unavailable
   and how to do it manually.

---

### User Story 5 - Auto-configure aliases for popular shells (Priority: P3)

On Linux and macOS, the tool can install its command aliases into popular shells
(e.g. bash, zsh, fish, PowerShell), mirroring the existing Windows PowerShell
alias feature, with a managed, clearly-delimited block that is safe to re-run and
remove.

**Why this priority**: Convenience parity with Windows; valuable but post-MVP.

**Independent Test**: For each supported shell, run set-aliases, open a new
session of that shell, and confirm the aliases work; run remove-aliases and
confirm clean removal; confirm a manual-instructions fallback for unsupported shells.

**Acceptance Scenarios**:

1. **Given** a supported shell, **When** the user runs set-aliases, **Then** a new
   session of that shell has the aliases available.
2. **Given** previously installed aliases, **When** the user runs remove-aliases,
   **Then** the managed block is removed and nothing else is changed.
3. **Given** an unsupported shell, **When** the user runs set-aliases, **Then** the
   tool prints the alias block and instructions to add it manually.

---

### Edge Cases

- **Windows-only features requested off-Windows**: NTFS alternate data streams and
  Windows font installation have no Linux/macOS equivalent. The tool must report
  clearly that the feature is unavailable on this platform rather than erroring or
  crashing.
- **Windows-only columns/indicators**: attribute letters and the cloud column that
  represent Windows-only concepts must be presented sensibly on platforms where
  they don't apply [NEEDS CLARIFICATION: on Linux/macOS, hide such columns
  entirely, or show them blank/neutral?].
- **Name case sensitivity**: Linux (and case-sensitive macOS volumes) can hold two
  names differing only in case in one directory. Sorting and mask-matching must
  behave predictably [NEEDS CLARIFICATION: match the platform (case-sensitive) or
  preserve Windows-like case-insensitive ordering/matching?].
- **Volume/header line**: the Windows "Volume in drive X" header has no drive-letter
  equivalent on Linux/macOS [NEEDS CLARIFICATION: suppress the header, or replace it
  with mountpoint/filesystem information?].
- **Filenames the platform permits but Windows would not**: unusual characters and
  long names must display without corruption or data loss.
- **Symbolic links**: broken links and link loops must be handled gracefully
  (shown, not fatal).
- **Wide/non-BMP characters**: emoji and wide (East-Asian) characters must not
  break column alignment.
- **Non-graphical/redirected output**: piping the listing to a file or another
  program must produce clean, color-free (or appropriately encoded) output.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST list directory contents on Linux in all existing display
  modes (default, bare, wide, tree).
- **FR-002**: System MUST colorize entries by type and extension consistently with
  the Windows behavior.
- **FR-003**: System MUST display each entry's name, size, and modification date.
- **FR-004**: System MUST map the platform's file metadata to the existing
  type/attribute indicators (directory, symlink, hidden, read-only) using
  platform-appropriate conventions (e.g. dotfile = hidden).
- **FR-005**: System MUST provide the same sorting options available on Windows.
- **FR-006**: System MUST support file-mask filtering equivalent to the Windows
  behavior.
- **FR-007**: System MUST display file owner (user and group) on Linux and macOS.
- **FR-008**: System MUST resolve and display symbolic-link targets.
- **FR-009**: System MUST support macOS in addition to Linux, delivering the same
  listing experience.
- **FR-010**: Where the platform exposes cloud/online-only file status, the system
  MUST indicate it; where it does not, the system MUST omit the cloud indicator.
- **FR-011**: System MUST clearly report when a requested feature is unavailable on
  the current platform (e.g. NTFS streams), without failing silently or crashing.
- **FR-012**: System MUST detect the terminal width on Linux and macOS for
  width-sensitive layouts (wide and variable-column modes).
- **FR-013**: System MUST produce correct output when redirected to a file or pipe.
- **FR-014**: System MUST preserve all existing Windows behavior unchanged.
- **FR-015**: System SHOULD offer to configure popular terminal emulators on Linux
  and macOS to use the Nerd Font (post-MVP).
- **FR-016**: System SHOULD offer to install and remove command aliases for popular
  shells on Linux and macOS, using a managed, re-runnable, clearly-delimited block
  (post-MVP).
- **FR-017**: System MUST display names containing characters the platform permits
  without corruption, including wide and non-BMP characters, preserving column
  alignment.

### Key Entities

- **Directory entry**: a listed file or directory and its displayable metadata —
  name, size, modification (and other) timestamps, type, ownership, link target,
  and platform-appropriate indicators.
- **Platform profile**: the set of capabilities available on the current operating
  system (which indicators, columns, and convenience features apply).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A Linux user can list any readable directory and receive a colorized
  result visually equivalent to Windows for the same contents, excluding columns
  that represent Windows-only concepts.
- **SC-002**: All four display modes (default, bare, wide, tree) function on Linux.
- **SC-003**: 100% of existing Windows behavior is preserved — the existing test
  suite continues to pass unchanged.
- **SC-004**: macOS delivers the same listing experience within one release after
  the Linux release.
- **SC-005**: When a Windows-only feature is requested on Linux or macOS, the user
  receives a clear, non-error message in 100% of such invocations, with zero crashes.
- **SC-006**: Owner, size, date, and symlink-target information render correctly for
  at least 99% of typical files in a representative directory.
- **SC-007**: Listing a large directory (thousands of entries) on Linux completes
  in a time comparable to the Windows build for the same directory (no worse than
  ~1.5x), with correct output.

## Assumptions

- Target terminals support standard ANSI/VT escape sequences (true of modern Linux
  and macOS terminals).
- A UTF-8 locale/encoding is the common runtime environment.
- The existing Windows build is the reference for behavior and visual output.
- Phasing: the Linux colorized listing (P1) ships first; macOS (P2) follows
  shortly after; terminal-font and shell-alias auto-config (P3) follow the MVP.
- Cloud/online-only status is shown only where the operating system exposes it
  (Windows and macOS); on Linux it is omitted.
- Nerd-Font availability for icon display is handled by the existing detection/flag
  behavior, adapted per platform.
- "Popular" terminals and shells for the P3 stories will be a defined, bounded set
  (e.g. the most common emulators and bash/zsh/fish/PowerShell), with a
  manual-instructions fallback for the rest.
