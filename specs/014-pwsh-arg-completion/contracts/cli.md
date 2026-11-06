# CLI Contract: PowerShell Argument Completion

## `--Completion <shell>`

Prints the completer script for the shell and exits with 0.

| Value | Output |
|-------|--------|
| `pwsh` | Registration script for `tcdir` and `tcdir.exe` |
| anything else | `{Error}` `--Completion supports: pwsh` and exit code 1 |

`powershell` is accepted as a synonym for `pwsh`; one script serves both
PowerShell editions.

Use: `tcdir --Completion pwsh | Out-String | Invoke-Expression`.

## `--Complete w:<word> [<word> ...]`

Prints candidates for `<word>`, one per line, and exits with 0. Documented
in the help under `--Completion`.

```text
<completion text>\t<list text>\t<tooltip>
```

- No color, banner or summary.
- No file system access.
- No output when `<word>` isn't a switch or switch value.
- `--Complete` must be the first argument. The words after `w:<word>` are context only and aren't applied as switches.

## Registration script

```powershell
Register-ArgumentCompleter -Native -CommandName 'tcdir', 'tcdir.exe' -ScriptBlock {
    param ($wordToComplete, $commandAst, $cursorPosition)

    $others = @($commandAst.CommandElements | Select-Object -Skip 1 |
                Where-Object { $_.Extent.EndOffset -lt $cursorPosition -or $_.Extent.StartOffset -gt $cursorPosition } |
                ForEach-Object { $_.Extent.Text })

    & 'C:\path\to\tcdir.exe' --Complete "w:$wordToComplete" @others 2>$null | ForEach-Object {
        $fields = $_ -split "`t"
        [System.Management.Automation.CompletionResult]::new ($fields[0], $fields[1], 'ParameterName', $fields[2])
    }
}
```

The profile block adds one registration per alias, with the alias's flags
before `@others`.

## `--get-aliases`

Adds one line per profile block: `Tab completion: installed` or
`Tab completion: not installed (run --set-aliases to add it)`.
