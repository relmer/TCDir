# Feature Specification: Archive Browsing

**Feature Branch**: `012-archive-browsing`

**Created**: 2026-10-05

**Status**: Draft

**Input**: User description: "Add .zip and other archive file browsing to tcdir: treat archive files (e.g. .zip and other common archive formats) like directories so their contents can be listed, as if the archive were a folder."

## Clarifications

### Session 2026-11-05

- Q: Which formats beyond .zip? → A: The formats that Windows can read with its built-in support, plus .iso. The plan phase determines which built-in readers exist and which formats they cover.
- Q: Does `tcdir foo.zip` list the archive contents? → A: No. Without the new switch, `tcdir foo.zip` lists the details of the file `foo.zip`, as it does today. The contents are listed only with the new switch.
- Q: Do `-S` and `--Tree` descend into archives? → A: Only with the new switch. With the switch, every archive is a directory for all purposes, so the normal `-S` and `--Tree` rules apply to archive contents too.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - List the Contents of an Archive (Priority: P1)

A user runs `tcdir --Archives release.zip` and gets a listing of the files and folders at the root of the archive. The listing has the same columns, colors, icons, sorting and summary footer as a normal directory listing. The user does not have to extract the archive or open another tool to see its contents.

**Why this priority**: This is the core of the feature. Every other story depends on a single archive being listed as a directory.

**Independent Test**: Create a .zip with a few files and one subfolder. Run `tcdir --Archives <archive>` and verify that the root entries appear with sizes and dates, and that the subfolder appears as a directory.

**Acceptance Scenarios**:

1. **Given** an archive `release.zip` with `readme.txt`, `app.exe` and folder `docs\`, **When** the user runs `tcdir --Archives release.zip`, **Then** the listing contains `readme.txt`, `app.exe` and the directory `docs`, with the uncompressed size and last-modified time of each file
2. **Given** the same archive, **When** the user runs `tcdir release.zip` without the switch, **Then** the output lists the file `release.zip` itself, identical to current behavior
3. **Given** the same archive, **When** the user adds any existing display switch (wide, bare, sort order, attribute filter, owner, streams), **Then** the switch applies to the archive entries as it does to a real directory, or has no visible effect where the information does not exist in an archive
4. **Given** the same archive, **When** the listing completes, **Then** the header contains the archive path as the listed directory, and the footer contains the file, directory and byte totals of the listed entries
5. **Given** a directory that contains archives, **When** the user runs `tcdir --Archives` on that directory, **Then** each archive appears in the listing as a directory instead of a file

---

### User Story 2 - Navigate Into Folders and Filter Inside an Archive (Priority: P2)

With the switch, a user runs `tcdir --Archives release.zip\docs` to list a folder inside the archive, or `tcdir --Archives release.zip\docs\*.md` to filter with a wildcard mask, the same way as with a real path.

**Why this priority**: Most large archives have nested folders. A root-only listing is of limited use without a way to go deeper and filter.

**Independent Test**: With an archive that has nested folders, run tcdir on an inner path and on an inner path with a mask. Verify that only the matching entries at that level appear.

**Acceptance Scenarios**:

1. **Given** `release.zip` with `docs\guide.md` and `docs\api\ref.md`, **When** the user runs `tcdir --Archives release.zip\docs`, **Then** the listing contains `guide.md` and the directory `api`
2. **Given** the same archive, **When** the user runs `tcdir --Archives release.zip\docs\*.md`, **Then** the listing contains only `guide.md`
3. **Given** an archive whose entries are file paths with no explicit folder entries, **When** the user lists it, **Then** the implied folders appear as directories
4. **Given** a path inside an archive that does not exist, **When** the user lists it, **Then** tcdir reports the same "not found" error it reports for a missing real path
5. **Given** a path that continues past an archive file, **When** the user runs tcdir on it without the switch, **Then** tcdir reports the path as not found, identical to current behavior

---

### User Story 3 - Recursive and Tree Listings Through Archives (Priority: P2)

A user runs `tcdir --Archives -S` or `tcdir --Archives --Tree` on a real directory. tcdir descends into every archive in the tree, the same way it descends into a subdirectory. This also applies when the target itself is an archive.

**Why this priority**: Searching a whole tree, including archive contents, for a file is a primary use of this feature. It depends only on Story 1.

**Independent Test**: Build a directory tree that contains an archive with three levels of nesting. Run `-S` and `--Tree` with and without the switch, and verify that archive contents appear only with the switch, with correct per-folder and grand totals.

**Acceptance Scenarios**:

1. **Given** a directory `src` that contains `lib.zip` with nested folders, **When** the user runs `tcdir --Archives -S src`, **Then** every folder inside `lib.zip` is listed in turn with its subtotal, and the grand total includes the archive contents
2. **Given** the same directory, **When** the user runs `tcdir --Archives --Tree src`, **Then** the tree contains the contents of `lib.zip` under its entry, with the same connectors and depth limit that apply to real directories
3. **Given** the same directory, **When** the user runs `tcdir -S src` or `tcdir --Tree src` without the switch, **Then** `lib.zip` is listed as a file and the output is identical to current behavior
4. **Given** the same directory, **When** the user runs `tcdir --Archives -S src\*.cpp`, **Then** `.cpp` files inside archives match the mask the same way as `.cpp` files in real subdirectories

---

### User Story 4 - Turn the Switch On by Default (Priority: P3)

A user who prefers archives as directories in every listing sets this option in the TCDIR environment variable or the config file, and turns it off for one command with the negated switch.

**Why this priority**: Convenience for frequent use. The command-line switch alone delivers the feature.

**Independent Test**: Set the option in the config file, run `tcdir` on a directory with an archive and verify that the archive appears as a directory. Then run with the negated switch and verify that the archive appears as a file.

**Acceptance Scenarios**:

1. **Given** the option is set in the config file or TCDIR, **When** the user runs `tcdir`, **Then** archives appear as directories
2. **Given** the option is set, **When** the user runs `tcdir --Archives-`, **Then** archives appear as files, identical to current behavior
3. **Given** the option is set, **When** the user runs `tcdir --Config` or `tcdir --Env`, **Then** the output includes the option and its value

---

### User Story 5 - Other Common Archive Formats (Priority: P3)

Beyond .zip, the user can browse the other archive formats that Windows can read, plus .iso disc images, in the same way.

**Why this priority**: .zip is the most common archive format on Windows. Other formats cover more users, but an MVP does not require them.

**Independent Test**: For each supported format, list a sample archive with the same file tree and verify that the listings are identical.

**Acceptance Scenarios**:

1. **Given** sample archives of every supported format with the same file tree, **When** the user lists each one with the switch, **Then** all listings contain the same entries, sizes and folder structure
2. **Given** an .iso disc image, **When** the user lists it with the switch, **Then** the listing contains the files and folders on the image, and tcdir does not mount the image
3. **Given** a file with a supported extension whose contents are not a valid archive, **When** the user lists it with the switch, **Then** tcdir prints a clear error for that file and continues with any other targets

---

### Edge Cases

- A corrupt or truncated archive, or a file with an archive extension that is not an archive: tcdir prints a clear error, lists nothing for that archive and continues. During `-S` or `--Tree`, the error does not stop the recursion.
- A password-protected archive or one with encrypted entries: tcdir lists the entry names, sizes and dates where the format exposes them without the password. If the format encrypts the listing itself, tcdir reports that the archive is encrypted. tcdir never prompts for a password.
- An empty archive: the output matches the output for an empty directory.
- Entry paths with forward slashes, or a mix of `/` and `\`: both are path separators.
- Entry paths that point outside the archive root (`..\`, absolute paths, drive letters): tcdir lists them under a sanitized path and never reads anything outside the archive.
- Duplicate entry names in one folder: tcdir lists every entry, as it appears in the archive.
- Entry names in a legacy code page instead of UTF-8: tcdir decodes them with the encoding that the archive declares, or with the system OEM code page if the archive does not declare an encoding.
- An archive inside an archive (`outer.zip\inner.zip`): the inner archive is listed as a file. Browsing nested archives is out of scope.
- Very large archives (100,000+ entries) or archives over 4 GB: tcdir lists them correctly, including 64-bit sizes.
- An archive that is a cloud placeholder and is not available locally: tcdir does not trigger a download. It lists the archive as a file and reports that its contents are not available offline.
- A real directory whose name has an archive extension (`foo.zip` is a directory): it is a real directory and is listed as one.
- Attributes that do not exist in archives (owner, alternate streams, cloud status, link targets): the columns are blank for archive entries. tcdir never invents values for them.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: tcdir MUST add a switch, `--Archives`, that turns on archive browsing. Without the switch, tcdir MUST treat archives as ordinary files, identical to current behavior.
- **FR-002**: With the switch, tcdir MUST treat every file of a supported archive format as a directory: in the target path, in directory listings, and in `-S` and `--Tree` recursion.
- **FR-003**: With the switch, a command-line target that resolves to an archive MUST list the archive root contents.
- **FR-004**: With the switch, tcdir MUST accept target paths that continue past an archive file into it (`archive.zip\folder\sub`) and list that folder inside the archive.
- **FR-005**: Wildcard masks MUST work on the last component of a path inside an archive, with the same matching rules as for real directories.
- **FR-006**: Each archive entry MUST include its name, uncompressed size, last-modified time and file or folder type. The read-only, hidden and system attributes MUST appear when the archive stores them.
- **FR-007**: Folders implied by entry paths MUST appear as directories even when the archive has no explicit entry for them.
- **FR-008**: Archive entries MUST use the same colors, icons, sorting, column layout and summary footer as real file system entries, based on their names and attributes.
- **FR-009**: Display switches that have no meaning in an archive (owner, streams, cloud status, link targets) MUST be accepted without error. Their columns MUST be blank for archive entries.
- **FR-010**: The listing header MUST make it clear when the listed location is inside an archive, for example with the full archive path plus the inner folder.
- **FR-011**: tcdir MUST support .zip and .iso, plus every other archive format that Windows can read with its built-in support. tcdir MUST NOT bundle a third-party archive library unless the plan phase finds that Windows cannot read a required format.
- **FR-012**: Listing an .iso image MUST NOT mount it or otherwise change system state.
- **FR-013**: With the switch, `-S` and `--Tree` MUST descend into archives with the same rules, depth limit and masks that apply to real subdirectories.
- **FR-014**: The option MUST be settable as a default in the TCDIR environment variable and the config file, MUST be turned off for one command with the negated switch (`--Archives-`), and MUST appear in the `--Env` and `--Config` output.
- **FR-015**: tcdir MUST NEVER extract archive contents to disk or modify the archive, and MUST NOT decompress entry data merely to list it, except where a format requires a full scan to enumerate its entries.
- **FR-016**: Errors that occur while tcdir opens or reads an archive MUST use the existing tcdir error output (the `Error:` line style of the "does not exist" message) and MUST NOT stop the listing of other targets or the rest of a recursion.
- **FR-017**: The switch MUST be documented in the `-?` help output.

### Key Entities

- **Archive**: A single file of a supported format, treated as the root of a read-only virtual directory tree. Attributes: path, format, total entry count.
- **Archive Entry**: One file or folder inside an archive. Attributes: path within the archive, name, type (file or folder, explicit or implied), uncompressed size, compressed size, last-modified time, and attributes when present.
- **Archive Path**: A target path split into the real path of the archive file, an inner path and an optional wildcard mask.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: With the switch, a user can list the contents of any supported archive with a single command, without extraction, in 100% of tested sample archives.
- **SC-002**: Without the switch, the output of every existing command is byte-for-byte identical to current output, including for paths that contain archives.
- **SC-003**: A root listing of a .zip archive with 10,000 entries completes in under 1 second on a typical developer machine.
- **SC-004**: After an archive listing, no new files exist on disk, no new volumes are mounted, and the archive is not modified (verified by before/after comparison).
- **SC-005**: For every supported format, the listing of a sample archive matches the entries, sizes and folder structure of the same tree extracted to disk.
- **SC-006**: The timing of non-archive listings (measured with `-P`) does not regress.
- **SC-007**: A corrupt, encrypted or unsupported archive never crashes tcdir and produces a clear error in 100% of tested cases.

## Assumptions

- tcdir recognizes archives by file extension. It does not inspect file contents to treat files with other extensions as archives.
- Archive browsing is read-only. Extraction and adding or deleting entries are out of scope.
- Nested archives (an archive inside an archive) are listed as files, not browsed.
- Times stored in archives without a time zone (as in .zip) are displayed as local time, consistent with other tools.
- The compressed size and compression ratio are not displayed by default. A future switch could add them.
- For most formats, a listing reads only the archive metadata, so its cost depends on the entry count rather than the archive size. Some formats, such as compressed tarballs, require a scan of the whole stream to enumerate entries.
- Unit tests use synthetic in-memory archive bytes and never real files on disk, per the project constitution.
