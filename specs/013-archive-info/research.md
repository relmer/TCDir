# Research: Archive Info Mode

## R1. Where format details come from

**Decision**: Extend the three readers to fill an optional details record
while they read the entry list. A caller requests details by setting
`SArchiveReadResult::m_detailsPtr` before the read. For listings the pointer is
null, so the listing path does no extra work.

**Rationale**: Every value comes from structures the readers already parse (the
ZIP central directory, the ISO volume descriptors, libarchive headers).
Collecting them in a second pass would read and parse the archive twice.

**Alternatives considered**:
- A separate details reader per format. Not chosen, because it duplicates the
  record parsing and the bounds checks.
- Adding fields to `SArchiveEntry`. Not chosen, because ZIP method and
  compressed size per entry would grow every listing entry for a value only
  `--Info` uses. The ZIP reader adds each entry to the method groups instead.

## R2. ZIP fields

All offsets are from APPNOTE.TXT 4.3 and 4.5.

| Value | Source |
|-------|--------|
| Method | Central header offset 10 (16-bit). 0 Store, 8 Deflate, 9 Deflate64, 12 BZip2, 14 LZMA, 93 Zstd, 95 XZ, 98 PPMd, 99 AES wrapper |
| Compressed size | Central header offset 20, or the second ZIP64 extra value when offset 20 is 0xFFFFFFFF |
| Encryption | Flag bit 0. Method 99 with extra 0x9901: strength byte 1/2/3 means AES-128/192/256, and the real method follows in the extra. Bit 0 without 0x9901 means ZipCrypto. Bit 6 means PKWARE strong encryption, reported as "PKWARE strong" |
| UTF-8 paths | Flag bit 11 |
| Creator | "Version made by": high byte host (0 MS-DOS, 3 Unix, 7 Macintosh, 10 NTFS, 14 VFAT, 19 OS X), low byte version times 10 |
| ZIP64 | The ZIP64 locator was found |
| Multi-disk | End record offsets 4 and 6 (disk numbers) are not zero |
| Prefix | The smallest local header offset (central header offset 42, or the ZIP64 extra) when it is not zero |
| Comment | End record comment. Decoded as UTF-8 when valid, otherwise code page 437 |

The creator line uses the most common host and version across entries.

## R3. ISO and UDF fields

| Value | Source |
|-------|--------|
| Labels and IDs | Primary volume descriptor: system ID 8, volume ID 40, publisher 318, preparer 446, application 574. Joliet supplementary descriptor values replace them when present (UCS-2 big-endian) |
| Dates | PVD offsets 813 (creation), 830 (modification), 847 (expiration). 17-byte decimal date with a 15-minute UTC offset. All zero digits means not set |
| Sector size and count | PVD offset 128 (logical block size) and 80 (volume space size) |
| Rock Ridge | The root "." record's system use area starts with SUSP "SP", and an "RR", "PX" or "NM" entry follows |
| El Torito | Boot record descriptor (type 0) with "EL TORITO SPECIFICATION" at offset 7. The boot catalog sector is at offset 71. The validation entry platform byte (offset 1) and each section header (0x90/0x91) platform byte: 0 BIOS, 0xEF UEFI, 1 PowerPC, 2 Mac |
| UDF revision | Logical volume descriptor domain ID suffix (offset 216 + 24, 16-bit BCD revision such as 0x0250) |
| UDF label | Logical volume identifier, LVD offset 84 (128-byte dstring) |
| Listed tree | The tree `CIsoReader::Read` chooses: UDF, Joliet or primary |

A pure UDF image without an ISO 9660 bridge gets its label from the UDF
logical volume and its sector count from the image size.

## R4. libarchive fields

`archiveint.dll` exports every function listed here (checked against the
export table of the Windows 11 copy).

| Value | Function |
|-------|----------|
| Format and version | `archive_format_name` after the first header |
| Filters | `archive_filter_count` and `archive_filter_name` for index 0 up to count minus 2 (the last filter is the raw read) |
| Byte totals | After the last header: `archive_filter_bytes (a, -1)` read from the file, `archive_filter_bytes (a, 0)` produced by the filters. Reported only when there is at least one filter, because ZIP and 7z seek past entry data |
| Encryption | `archive_entry_is_data_encrypted`, `archive_entry_is_metadata_encrypted`, `archive_read_has_encrypted_entries` |
| Links | `archive_entry_filetype` equal to AE_IFLNK; `archive_entry_hardlink_w` not null |
| Owners | `archive_entry_uname_w` and `archive_entry_gname_w` |
| Permissions | `archive_entry_perm` when `archive_entry_perm_is_set` |
| Times | `archive_entry_birthtime_is_set`, `archive_entry_atime_is_set` |
| Sparse, xattrs, ACLs | `archive_entry_sparse_count`, `archive_entry_xattr_count`, `archive_entry_acl_count (e, ARCHIVE_ENTRY_ACL_TYPE_ACCESS | DEFAULT | NFS4)` |

The new functions are loaded with the others in `CLibArchiveReader::LoadApi`.
Each is optional: when an export is missing, its value is unknown and its line is
omitted from the block.

## R5. Verification

**Decision**: Verify through libarchive for every format it reads, ZIP and
ISO 9660 included. Read each entry with `archive_read_data` and treat
ARCHIVE_WARN or ARCHIVE_FAILED during data reads as damage, with the
`archive_error_string` text as the reason. Encrypted entries are skipped with
`archive_read_data_skip`.

**Rationale**: libarchive checks the CRC-32 for ZIP, 7z, RAR, CAB and LZH
entries and the stream check for gzip, bzip2, xz, zstd and lzma while
decompressing. tcdir has no decompressors, and adding them is out of scope.

**Checksum coverage**, reported in the verify line:

| Format | Coverage |
|--------|----------|
| ZIP, 7z, RAR, CAB, LZH | Each entry |
| tar.gz, tar.bz2, tar.xz, tar.zst, tar.lzma, tlz | The whole stream, so a damaged byte is reported for the entry being read |
| tar, cpio, XAR | Headers only. Reported as "no data checksums", and verify checks that every entry reads |
| ISO 9660 | No checksums. Verify checks that every entry reads |
| UDF without an ISO 9660 tree | libarchive can't read it. The verify line reports that verification isn't available for this image |

Without `archiveint.dll` (Windows 10), `--Verify` prints the error
`--Archives` prints when libarchive is missing, and `--Info` works without it
for ZIP and ISO.

## R6. Command-line conflicts and configured defaults

**Problem**: `ApplyConfigDefaults` copies `TCDIR` switches such as `W` or
`Tree` into `CCommandLine` before parsing, so the parsed values don't
distinguish a typed `-w` from a configured one. A user with `TCDIR=W` must be
able to run `--Info`.

**Decision**: While parsing, record the first listing switch typed on the
command line (`m_strInfoConflictSwitch`). `ValidateSwitchCombinations`
reports that switch when `--Info` is set. When there is no conflict, the
listing members are reset to their defaults, which discards configured
listing defaults. Configured `S`, `P`, `M` and `Archives` apply to `--Info`.

**Listing switches**: `-w`, `-b`, `--Tree`, `--Depth`, `--TreeIndent`, `-o`,
`-a`, `-t`, `-q` (owner), `-r` (streams), `--Icons`, `--Ellipsize`, `--Size`.

## R7. Target resolution and recursion

**Decision**: Resolve targets in a new `CArchiveInfoFinder`:

1. Split each target with `CMaskGrouper`, as listings do, with the archive
   file system attached so `outer.zip\inner.7z` resolves.
2. A target whose full path is an archive (on disk or nested) yields that
   archive.
3. Otherwise, enumerate the folder (on disk with `FindFirstFileEx`, inside an
   archive with `CArchiveTree::EnumerateChildren`). Each match recognized by
   `CArchiveFileSystem::IsArchiveFile` yields an archive.
4. With `-s`, each subfolder and each archive found during the walk is
   searched the same way, depth first. Archives that don't match the mask are
   searched but not reported.

Results are streamed: each archive's block prints as soon as its report is
complete, in the order of the spec's Assumptions.

**Alternatives considered**: Reusing `CMultiThreadedLister`. Not chosen, because
its output is tied to the listing displayers, and reports for different
archives are independent. Parallel report building is left for later; SC-002
is met by reading only the index.

## R8. Nested archive size limit and errors

Nested archives use `CArchiveFileSystem` extraction, so the 256 MB limit and
`FormatError` messages apply. A nested archive that fails to open is
counted in its parent's block.
