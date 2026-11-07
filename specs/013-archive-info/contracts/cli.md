# CLI Contract: Archive Info Mode

## Syntax

```text
tcdir --Info [--Verify] [-s] [-p] [-m-] [--Archives] [target ...]
```

`target` is an archive path, a mask, a folder, or a path through outer
archives (`outer.zip\inner.7z`). No target means `*`.

## Switches

| Switch | Valid with | Effect |
|--------|------------|--------|
| `--Info` | `--Verify`, `-s`, `-p`, `-m`, `--Archives`, color settings | Report archives instead of listing |
| `--Verify` | `--Info` only | Read every entry and check its checksum |

Both switches accept the `-` and `/` prefixes and are case insensitive, like
the other long switches. Neither switch can be set in `TCDIR` or the config
file.

## Errors

| Condition | Message (in `{Error}` color) | Exit code |
|-----------|------------------------------|-----------|
| A listing switch with `--Info` | `--Info and <switch> cannot be used together.` | 1, after the usage text, as for other conflicts |
| `--Verify` without `--Info` | `--Verify requires --Info.` | 1 |
| No archive matches a target | `No archives match <target>` | 1 at the end of the run |
| Archive can't be read | The `--Archives` message for that error | 1 at the end of the run |
| Damaged entry | Listed under the verify line | 1 at the end of the run |

Listing switches: `-w`, `-b`, `--Tree`, `--Depth`, `--TreeIndent`, `-o`,
`-a`, `-t`, `-q`, `-r`, `--Icons`, `--Ellipsize`, `--Size`. The check covers
switches typed on the command line. `TCDIR` and config file listing defaults
are ignored under `--Info`.

## Output

```text
 C:\TestData\Archives\zip\level1.zip

   Format         ZIP 2.0, Windows file attributes, UTF-8 paths
   Size on disk   3,104 bytes
   Contents       9 files, 4 folders, 1 nested archive
   Uncompressed   4,289 bytes (72%)
   Largest file   level0.zip (2,235 bytes)
   Deepest path   src\lib\util.cpp (3 levels)
   Dates          01/01/2026 12:00 PM to 01/01/2026 12:00 PM
   Encryption     None
   Methods        Deflate   8 files   1,102 of 2,054 bytes
                  Store     1 file    2,235 of 2,235 bytes
   Extensions     .zip      1 file    2,235 bytes
                  .cpp      1 file       42 bytes
                  ...
   Verify         10 passed, 0 damaged, 0 skipped (CRC-32 per entry)
```

- The heading line uses `{Information}`. Labels use `{Information}` and
  values use `{InformationHighlight}`. Sizes use `{Size}`, dates `{Date}` and
  times `{Time}`.
- Label width is the longest label in the block plus 3. Rows of a breakdown
  align their columns.
- Lines with no data are omitted (see the spec's FR-020 and FR-024).
- After two or more blocks, a summary follows:

```text
 3 archives, 9,210 bytes on disk, 12,867 bytes uncompressed
```

## Exit codes

0 when every target matched and every archive was read and verified. 1
otherwise. Command-line errors exit as they do today.
