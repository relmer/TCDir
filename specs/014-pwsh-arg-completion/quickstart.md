# Quickstart: PowerShell Argument Completion

## Unit tests

```powershell
scripts\RunTests.ps1 -Configuration Debug
```

Tests call the completer with synthetic requests and compare the catalog
against the parser and the help. No test starts PowerShell.

## Manual scenarios

Run each in a new PowerShell 7 session and in Windows PowerShell 5.1.

| # | Steps | Result |
|---|-------|--------|
| 1 | `tcdir --Completion pwsh \| Out-String \| Invoke-Expression`, then `tcdir --Tr<Tab>` | `--Tree` |
| 2 | `tcdir -<Ctrl+Space>` | Short switches, then long switches, each with a tooltip |
| 3 | `tcdir /Str<Tab>` | `/Streams` |
| 4 | `tcdir C:\Pro<Tab>` | Same paths as without the completer |
| 5 | `tcdir -O:<Ctrl+Space>` | Sort keys with and without `-` |
| 6 | `tcdir -A:H<Tab>` | `-A:HD`, `-A:HS`, ... |
| 7 | `tcdir --Tree -<Ctrl+Space>` | No `-W`, `-B`, `-S`, `--Owner` |
| 8 | `tcdir --Set-Aliases`, new session, `d --Tr<Tab>` | `--Tree` |
| 9 | `tcdir --Get-Aliases` | `Tab completion: installed` |
| 10 | `tcdir --Remove-Aliases`, new session, `tcdir --Tr<Tab>` | Path completion only |
| 11 | `tcdir --Completion bash` | Error listing `pwsh`, exit code 1 |
| 12 | `Measure-Command { tcdir --Complete w:--T }` | Under 100 ms |
| 13 | `tcdir -?` before and after the change | Identical output |
