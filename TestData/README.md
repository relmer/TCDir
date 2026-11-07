# Test data

Files and directory structures for manual testing. The unit tests don't use
them.

## Archives

`Archives\<format>\` contains three archives for each format that
`--Archives` recognizes:

| File | Contents |
|------|----------|
| `level0.<ext>` | Sample files in `data`, `docs`, `src` and `src\lib` |
| `level1.<ext>` | The sample files plus `level0.<ext>` |
| `level2.<ext>` | The sample files plus `level1.<ext>` |

The sample files are short generated text files dated 2026-01-01 12:00.

Example commands:

```powershell
tcdir --Archives TestData\Archives\zip\level0.zip
tcdir --Archives -s TestData\Archives\zip\level2.zip
tcdir --Archives --Tree TestData\Archives
```

### Rebuilding

```powershell
scripts\New-ArchiveSamples.ps1
```

The script uses `tar.exe` and `makecab.exe` from Windows, plus `rar.exe` from
WinRAR for `.rar`. Windows has no tool that writes LZH, so the script writes
LZH archives directly, with stored entries and level 2 headers. ISO, CAB, RAR, 7z and
XAR files contain the build time, so a rebuild changes their bytes.
