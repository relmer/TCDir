# Feature Specification: Archive Info Mode

**Feature Branch**: `013-archive-info`

**Created**: 2026-11-06

**Status**: Draft

**Input**: User description: "Add an --Info mode for archives to tcdir. `tcdir --Info <target>` prints statistics about each archive matching the target instead of a directory listing. The target is one archive, a mask, or a path to an archive stored inside another archive. Under -s, archives are treated as folders, the same rule --Archives -s uses. --Verify checks stored checksums. Each block contains common statistics plus format-specific details for ZIP, ISO/UDF and the libarchive formats."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Statistics for one archive (Priority: P1)

A user runs `tcdir --Info foo.zip` and gets one block of statistics about
`foo.zip` instead of a directory listing: its size on disk, how many files,
folders and nested archives it contains, the total uncompressed size and
compression ratio, the largest file, the deepest path, the oldest and newest
timestamps, encryption status and a breakdown by file extension.

**Why this priority**: This is the core of the mode. Every later story adds
targets or details to the same block.

**Independent Test**: Run `tcdir --Info TestData\Archives\zip\level1.zip` and
compare each value against the known contents of the sample.

**Acceptance Scenarios**:

1. **Given** a ZIP archive with nine files in four folders and one nested archive,
   **When** the user runs `tcdir --Info archive.zip`, **Then** one block
   appears with the archive path as its heading and lines for size on disk,
   contents (nine files, four folders, one nested archive), uncompressed size, ratio,
   largest file, deepest path, date range, encryption and extension breakdown.
2. **Given** the same archive, **When** the user runs `tcdir --Archives --Info
   archive.zip`, **Then** the output is identical to `tcdir --Info archive.zip`.
3. **Given** an archive with no entries, **When** the user runs `--Info` on it,
   **Then** the block reports zero files and zero folders, and the lines for
   largest file, deepest path, date range and ratio contain "none" instead
   of a value.
4. **Given** a file that is damaged or isn't a valid archive, **When** the
   user runs `--Info` on it, **Then** tcdir prints the error that
   `--Archives` prints for the same file, and continues with the next target.

---

### User Story 2 - Masks and several targets (Priority: P1)

A user runs `tcdir --Info foo\*.zip` or `tcdir --Info *.7z *.rar` and gets one
block for each matching archive, in listing sort order.

**Why this priority**: Comparing a set of archives is the second most common
use, and it shares the target handling with User Story 1.

**Independent Test**: Run `tcdir --Info TestData\Archives\zip\*` and check
that three blocks appear, one for each `level` file.

**Acceptance Scenarios**:

1. **Given** a folder with `a.zip`, `b.zip` and `readme.txt`, **When** the
   user runs `tcdir --Info folder\*`, **Then** blocks appear for `a.zip` and
   `b.zip`, and `readme.txt` is skipped without a message.
2. **Given** a folder without archives, **When** the user runs `tcdir --Info
   folder\*.zip`, **Then** tcdir prints an error that no archives match
   `folder\*.zip` and exits with a failure code.
3. **Given** no target, **When** the user runs `tcdir --Info`, **Then** every
   archive in the current folder is reported, as if the target were `*`.
4. **Given** several archives, **When** the blocks finish, **Then** a summary
   line reports how many archives were reported and their combined size on
   disk and uncompressed size.

---

### User Story 3 - Nested archives and recursion (Priority: P2)

A user targets an archive stored inside another archive with
`tcdir --Info outer.zip\inner.7z`, or adds `-s` to report every matching
archive below a folder, including archives nested inside other archives.

**Why this priority**: Nesting is supported by `--Archives`, and the mode is
incomplete without a way to target nested archives. It depends on User
Stories 1 and 2.

**Independent Test**: Run `tcdir --Info -s TestData\Archives\zip\level2.zip`
and check that three blocks appear: `level2.zip`, `level2.zip\level1.zip` and
`level2.zip\level1.zip\level0.zip`.

**Acceptance Scenarios**:

1. **Given** `outer.zip` that contains `inner.7z`, **When** the user runs
   `tcdir --Info outer.zip\inner.7z`, **Then** one block appears for the
   nested archive, with `outer.zip\inner.7z` as its heading.
2. **Given** the same archive, **When** the user runs `tcdir --Info
   outer.zip` without `-s`, **Then** one block appears for `outer.zip`, and
   its contents line counts `inner.7z` as a nested archive.
3. **Given** a folder tree whose subfolders contain ZIP files, some of which
   contain ZIP files, **When** the user runs `tcdir --Info -s folder\*.zip`,
   **Then** a block appears for every ZIP file on disk below `folder` and for
   every ZIP file inside them at any depth, each identified by its full path.
4. **Given** the same tree, **When** the user runs `tcdir --Info -s
   folder\*.7z`, **Then** blocks appear only for 7z archives, whether they are
   on disk or inside a ZIP file. Archives that don't match the mask are
   searched for nested matches but don't get a block.

---

### User Story 4 - ZIP details (Priority: P2)

For a ZIP archive, the block adds details that only ZIP stores: compressed
size and ratio by compression method, encryption type, ZIP64, the archive
comment, the tool and host OS that created the archive, whether paths are
stored as UTF-8, whether the archive spans several disks, and whether it is
self-extracting.

**Why this priority**: ZIP is the most common format, and these details
answer questions that the common statistics can't.

**Independent Test**: Run `--Info` on ZIP files built with known methods,
encryption and comments, and compare each line against the build settings.

**Acceptance Scenarios**:

1. **Given** a ZIP file with three Deflate entries and one stored entry, **When**
   the user runs `--Info` on it, **Then** a methods line reports Deflate with
   three files and their compressed and uncompressed sizes, and Store with
   one file.
2. **Given** a ZIP file with AES-256 entries, **When** the user runs `--Info`
   on it, **Then** the encryption line reports AES-256 and the number of
   encrypted entries.
3. **Given** a ZIP file with a comment, **When** the user runs `--Info` on it,
   **Then** the comment appears in the block. A comment with several lines is
   indented to line up under its first line.
4. **Given** a self-extracting ZIP file, **When** the user runs `--Info` on it,
   **Then** the block reports the size of the program stored before the
   archive data.

---

### User Story 5 - Disc image details (Priority: P3)

For an ISO or UDF image, the block adds the file systems present and which
one tcdir lists, the volume label, the system, publisher, preparer and
application IDs, the volume creation, modification and expiration dates, the
sector size and count, and whether the image is bootable and for which
platforms.

**Why this priority**: Disc images are less common than ZIP files, but their
volume details are the main reason to inspect one.

**Independent Test**: Run `--Info` on an ISO built with a known label and
IDs, and on a bootable image, and compare each line.

**Acceptance Scenarios**:

1. **Given** an ISO with Joliet and a volume label, **When** the user runs
   `--Info` on it, **Then** the block reports ISO 9660 and Joliet, notes that
   the Joliet tree is listed, and reports the label.
2. **Given** a UDF 2.50 image with an ISO 9660 bridge, **When** the user runs
   `--Info` on it, **Then** the block reports UDF 2.50 and ISO 9660, and notes
   that the UDF tree is listed.
3. **Given** a bootable image with BIOS and UEFI boot entries, **When** the
   user runs `--Info` on it, **Then** the block reports it as bootable for
   BIOS and UEFI.
4. **Given** an ID field that is blank in the image, **When** the user runs
   `--Info` on it, **Then** that line is omitted from the block.

---

### User Story 6 - Details for other formats (Priority: P3)

For 7z, RAR, tar and its compressed variants, CAB, cpio, XAR and LZH, the
block adds the specific format and version, the compression filters, the
compressed and uncompressed byte totals, whether entries or headers are
encrypted, symbolic and hard link counts, a summary of Unix owners, groups and
permissions, and counts of sparse files, extended attributes and ACLs.

**Why this priority**: These formats share one set of details, and the values
come from the same source for all of them.

**Independent Test**: Run `--Info` on each sample in `TestData\Archives` and
check that the format line matches the folder the sample is in.

**Acceptance Scenarios**:

1. **Given** a `.tar.gz` file, **When** the user runs `--Info` on it, **Then**
   the format line reports the tar variant and the gzip filter, and the ratio
   compares the file size to the uncompressed tar stream.
2. **Given** a RAR 5 archive, **When** the user runs `--Info` on it, **Then**
   the format line reports RAR 5.
3. **Given** a tar file with entries owned by two users, **When** the user
   runs `--Info` on it, **Then** an owners line lists both users with their
   entry counts.
4. **Given** a format that doesn't store owners, links or attributes, **When** the
   user runs `--Info` on it, **Then** those lines are omitted.

---

### User Story 7 - Verification (Priority: P3)

A user adds `--Verify` to extract every entry and compare it with the
checksum the archive stores, to find damaged archives.

**Why this priority**: Verification is slow and used less often, but it is
the only way to detect damaged entry data.

**Independent Test**: Corrupt one byte inside an entry's data in a copy of a
sample ZIP file, run `--Info --Verify` on it, and check that the damaged
entry is reported.

**Acceptance Scenarios**:

1. **Given** an intact archive, **When** the user runs `tcdir --Info --Verify
   archive.zip`, **Then** the block adds a verify line that reports all
   entries as passed.
2. **Given** an archive with one damaged entry, **When** the user runs the
   same command, **Then** the verify line reports 1 damaged entry, the
   damaged entry's path appears below it in the `{Error}` color, and tcdir
   exits with a failure code.
3. **Given** a format that doesn't store checksums, such as an ISO image,
   **When** the user runs `--Verify`, **Then** the verify line reports that
   the format has no checksums and every entry was read without errors.
4. **Given** an encrypted entry, **When** the user runs `--Verify`, **Then**
   the entry is counted as skipped because it is encrypted.

---

### Edge Cases

- `--Info` with a listing switch such as `-w`, `-b`, `--Tree`, `-o`, `-a`,
  `-q`, `-r` or `--Icons` is a command-line error, and the message identifies the
  conflicting switch.
- `--Verify` without `--Info` is a command-line error.
- A target that is a folder without a mask is treated as `folder\*`.
- An archive nested deeper than the nested-archive size limit is reported
  with the error `--Archives` prints, and its parent's block counts it as a
  nested archive.
- An encrypted listing (for example a 7z archive with encrypted headers)
  produces a block with the size on disk and the format, and reports that the
  listing is encrypted. Entry statistics are omitted.
- Entries without timestamps are left out of the date range. When no entry
  has a timestamp, the date range contains "none".
- Extensions are compared without regard to case. Files without an extension
  are grouped under "(none)".
- An archive larger than 4 GB, or with more than 65,535 entries, reports
  exact sizes and counts.
- `--Info` combined with `-p` reports the time taken, as it does for a
  listing.
- Ctrl+C stops the run between archives and during `--Verify`.

## Requirements *(mandatory)*

### Functional Requirements

**Mode and command line**

- **FR-001**: `--Info` MUST print a block of statistics for each archive that
  matches the targets, and MUST NOT print a directory listing.
- **FR-002**: `--Info` MUST recognize archives whether or not `--Archives` is
  given. `--Archives` with `--Info` MUST NOT change the output.
- **FR-003**: Each target MUST be one of: an archive path, a mask, a folder
  (treated as `folder\*`), or a path to an archive stored inside another
  archive. With no target, `--Info` MUST use `*` in the current folder.
- **FR-004**: Files that match a mask but aren't recognized archives MUST be
  skipped without a message.
- **FR-005**: When no archive matches a target, tcdir MUST print an error for
  that target and exit with a failure code after processing the other
  targets.
- **FR-006**: `--Info` combined with any switch that only affects listing
  output (wide, bare, tree, sort order, attribute filters, owner, streams,
  icons, ellipsize, size format, time field) MUST be a command-line error that
  identifies the conflicting switch. `-s`, `-p`, `-m`, `--Archives` and the
  color settings remain valid.
- **FR-007**: `--Verify` MUST be valid only with `--Info`.
- **FR-008**: `--Info` and `--Verify` MUST be documented in the `-?` help, and
  in `--Env` and `--Config` where switches can be set there.

**Recursion**

- **FR-009**: Without `-s`, only archives that match the targets directly MUST
  be reported. Nested archives MUST be counted in their parent's block.
- **FR-010**: With `-s`, archives MUST be searched as folders, the rule
  `--Archives -s` uses. tcdir MUST report every archive that matches the mask
  in the target folder, in its subfolders, and inside any archive found there
  at any depth. Each block MUST use the archive's full path, including the
  outer archives.
- **FR-011**: Under `-s`, archives that don't match the mask MUST be searched
  for nested matches but MUST NOT get a block.

**Common statistics** (all formats)

- **FR-012**: Each block MUST report: size on disk (for a nested archive, its
  size inside the parent); counts of files, folders and nested archives;
  total uncompressed size; overall ratio of size on disk to uncompressed size;
  the largest file with its size; the deepest path with its depth; and the
  oldest and newest entry timestamps.
- **FR-013**: Each block MUST report the number of encrypted entries and
  whether the listing itself is encrypted.
- **FR-014**: Each block MUST include a breakdown by file extension with the
  count and total uncompressed size of each extension, sorted by size,
  largest first. Extensions MUST be compared without regard to case.
- **FR-015**: When more than one archive is reported, a summary MUST follow
  the blocks with the archive count, combined size on disk and combined
  uncompressed size.

**ZIP details**

- **FR-016**: ZIP blocks MUST add a breakdown by compression method (Store,
  Deflate, Deflate64, BZip2, LZMA, Zstd, XZ, PPMd, and the method number for
  any other method) with the file count and the compressed and uncompressed
  size of each.
- **FR-017**: ZIP blocks MUST report the encryption type in use: none,
  ZipCrypto, AES-128, AES-192 or AES-256. When several types are used, each
  MUST be listed with its entry count.
- **FR-018**: ZIP blocks MUST report whether the archive uses ZIP64,
  whether paths are stored as UTF-8, whether the archive spans several disks,
  the host OS and version of the tool that created it, the size of any data
  before the archive (self-extracting program), and the archive comment.

**Disc image details**

- **FR-019**: ISO and UDF blocks MUST report the file systems present (ISO
  9660, Joliet, Rock Ridge, UDF with its revision) and which tree tcdir
  lists.
- **FR-020**: ISO and UDF blocks MUST report the volume label; the system,
  publisher, preparer and application IDs; and the volume creation,
  modification and expiration dates. Blank fields MUST be omitted.
- **FR-021**: ISO and UDF blocks MUST report the sector size and sector
  count, and whether the image is bootable with the platform of each boot
  entry (BIOS, UEFI, or the platform number for others).

**Other formats** (7z, RAR, tar variants, CAB, cpio, XAR, LZH)

- **FR-022**: These blocks MUST report the specific format and version, and
  the compression filters applied to the archive stream.
- **FR-023**: These blocks MUST report the compressed and uncompressed byte
  totals of the archive stream when available.
- **FR-024**: These blocks MUST report whether entry data or headers are
  encrypted; symbolic and hard link counts; owners and groups with entry
  counts; the distinct permission sets with entry counts; and counts of
  sparse files, entries with extended attributes and entries with ACLs.
  Lines with no data MUST be omitted.
- **FR-025**: Compressed size per entry and whether the archive is solid are
  out of scope for these formats.

**Verification**

- **FR-026**: `--Verify` MUST extract every entry and compare it with the
  checksum the format stores. The block MUST add a line with the counts of
  passed, damaged and skipped entries.
- **FR-027**: Each damaged entry MUST be listed under the verify line in the
  `{Error}` color with the reason, and tcdir MUST exit with a failure code
  when any entry is damaged.
- **FR-028**: Encrypted entries MUST be counted as skipped. For formats
  without checksums, the verify line MUST state that and report whether every
  entry was read without errors.

**Output**

- **FR-029**: Labels and values MUST use the configured colors. Errors MUST
  print through the console in the `{Error}` color, inline with the blocks.
- **FR-030**: Sizes MUST use the digit grouping and units of the listing.
  Dates MUST use the listing's date and time format.

### Key Entities

- **Archive report**: The statistics for one archive. It has the archive's
  full path, size on disk, format, common statistics, format-specific
  details and, with `--Verify`, the verification result.
- **Extension group**: One extension (or "(none)") with its file count and
  total uncompressed size.
- **Method group**: One ZIP compression method with its file count and
  compressed and uncompressed sizes.
- **Format details**: The values specific to ZIP, disc images, or the other
  formats. Only the set for the archive's format applies.
- **Verification result**: Counts of passed, damaged and skipped entries,
  plus the path and reason for each damaged entry.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: For every sample in `TestData\Archives`, `--Info` reports file,
  folder and nested-archive counts and uncompressed sizes that match the
  sample's known contents exactly.
- **SC-002**: `--Info` on a ZIP or ISO file under 1 GB completes in under 1
  second, because only the index is read.
- **SC-003**: `--Info -s` on `TestData\Archives` reports every archive, nested
  ones included, at the depth each one is stored.
- **SC-004**: `--Verify` detects a single corrupted byte in the data of any
  checksummed entry in every format that stores checksums.
- **SC-005**: Every listing switch combined with `--Info` produces a
  command-line error rather than silently changing or ignoring output.
- **SC-006**: A listing without `--Info` produces output and timing identical
  to the current release.

## Assumptions

- The archive formats and suffixes are the ones `--Archives` recognizes. No
  new formats are added.
- Nested archives use the size limit and error messages of `--Archives`.
- The ratio is size on disk divided by uncompressed size, printed as a
  percentage. A lower value means better compression.
- Without `-s`, blocks appear in the order the targets are given, and within a
  mask in name order. Under `-s`, each folder's archives come before its
  subfolders', and an archive's nested archives follow it.
- Disc images rarely store checksums, so `--Verify` on them checks only that
  each entry can be read.
- The manual test data in `TestData\Archives` covers every format. Unit tests
  use in-memory archives, as the constitution requires.
