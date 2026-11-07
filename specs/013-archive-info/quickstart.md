# Quickstart: Archive Info Mode

## Prerequisites

- Debug or Release build of tcdir (`scripts\Build.ps1`).
- Windows 11 23H2 or later for the libarchive formats and `--Verify`.
- Sample archives in `TestData\Archives` (`scripts\New-ArchiveSamples.ps1`
  rebuilds them).

## Unit tests

```powershell
scripts\RunTests.ps1 -Configuration Debug
```

The tests use in-memory archives and the archive access mock. No test reads
`TestData`.

## Manual scenarios

| # | Command | Result |
|---|---------|----------|
| 1 | `tcdir --Info TestData\Archives\zip\level1.zip` | One block. Contents line: `9 files, 4 folders, 1 nested archive` |
| 2 | `tcdir --Archives --Info TestData\Archives\zip\level1.zip` | Same output as scenario 1 |
| 3 | `tcdir --Info TestData\Archives\zip\*` | Three blocks and a summary |
| 4 | `tcdir --Info TestData\Archives\zip\level2.zip\level1.zip` | One block headed with the nested path |
| 5 | `tcdir --Info -s TestData\Archives\zip\level2.zip` | Blocks for level2, level1 and level0 |
| 6 | `tcdir --Info -s TestData\Archives\*.7z` | Blocks for the three 7z samples and their nested 7z archives only |
| 7 | `tcdir --Info TestData\Archives\iso\level0.iso` | ISO 9660, Joliet and Rock Ridge lines, sector count, no boot line |
| 8 | `tcdir --Info TestData\Archives\tar.gz\level0.tar.gz` | Format line with the tar variant and gzip, stream byte totals |
| 9 | `tcdir --Info --Verify TestData\Archives\zip\level2.zip` | Verify line: all passed |
| 10 | Copy `level0.zip`, change one byte inside `readme.txt` data, run `--Info --Verify` on the copy | One damaged entry listed in red, exit code 1 |
| 11 | `tcdir --Info -w x.zip` | `--Info and -W cannot be used together.` |
| 12 | `tcdir --Verify x.zip` | `--Verify requires --Info.` |
| 13 | `$env:TCDIR='W'; tcdir --Info TestData\Archives\zip\level0.zip` | Block as in scenario 1, no error |
| 14 | `tcdir --Info TestData\README.md` | `No archives match TestData\README.md`, exit code 1 |
| 15 | `tcdir --Info -p TestData\Archives\iso\level2.iso` | Block, then the elapsed time. Under 1 second |
| 16 | `tcdir TestData\Archives\zip` before and after the change | Identical output |
