# Quickstart: Validate Archive Browsing

## Prerequisites

- Debug x64 build: `pwsh -File .\scripts\Build.ps1 -Configuration Debug -Platform x64 -Target Build`
- Unit tests: `pwsh -File .\scripts\RunTests.ps1 -Configuration Debug -Platform x64`
- For 7z/rar/tar checks: Windows 11 23H2 or later (`C:\Windows\System32\archiveint.dll` present).

## Setup (manual check only; unit tests never touch disk)

```powershell
$t = Join-Path $env:TEMP 'tcdir-archives'
New-Item -ItemType Directory -Force "$t\src\docs\api" | Out-Null
'a' | Set-Content "$t\src\readme.txt"
'b' | Set-Content "$t\src\docs\guide.md"
'c' | Set-Content "$t\src\docs\api\ref.md"
Compress-Archive -Force "$t\src\*" "$t\release.zip"
tar -a -cf "$t\release.tar.gz" -C "$t\src" .
tar -a -cf "$t\release.7z" -C "$t\src" .
```

`tar.exe` creates .7z only on Windows 11 23H2+. For .iso, use any available disc image, for example Windows installation media.

## Scenarios

| # | Command | Expected |
|---|---------|----------|
| 1 | `tcdir $t\release.zip` | One line for the file `release.zip` (current behavior). |
| 2 | `tcdir --Archives $t\release.zip` | `readme.txt` and `<DIR> docs`; header `Directory of ...\release.zip`. |
| 3 | `tcdir --Archives $t\release.zip\docs\*.md` | Only `guide.md`. |
| 4 | `tcdir --Archives -S $t` | `release.zip`, `release.tar.gz`, `release.7z` appear as directories, their folders are listed with subtotals, grand total includes archive files. |
| 5 | `tcdir --Archives --Tree $t` | Archive contents drawn under each archive entry. |
| 6 | `tcdir -S $t` | Archives listed as files, identical to a build without the feature. |
| 7 | `tcdir --Archives <image>.iso` | Contents of the disc image; `Get-Volume` shows no new volume afterward. |
| 8 | Copy `release.zip` to `bad.zip`, truncate it, `tcdir --Archives -S $t` | One `Error:` line for `bad.zip`, the rest of the listing completes. |
| 9 | `$env:TCDIR='Archives'; tcdir $t; tcdir --Archives- $t` | First lists archives as directories, second as files. |
| 10 | `tcdir --Archives -P $t\big.zip` (10,000 entries) | Under 1 s. |
| 11 | `tcdir -P -S C:\Windows\System32` with and without the build | No timing regression beyond noise. |
