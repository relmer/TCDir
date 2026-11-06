# CLI Contract: Archive Browsing

## Switch

| Form | Effect |
|------|--------|
| `--Archives` | Treat a file with a recognized archive extension as a directory when it is in the target path and in `-S` and `--Tree` recursion. Archive files in a listing appear as files. Case-insensitive; `/Archives` and `-Archives` follow the existing long-switch rules. |
| `--Archives-` | Turn the option off for this command, overriding TCDIR and the config file. |

Default: off.

## TCDIR environment variable and config file

`Archives` turns the option on; `Archives-` turns it off. Both use the existing switch-override syntax (for example `set TCDIR=Archives;Tree`). Invalid combinations are reported the same way as for other switches.

`--Env`, `--Config` and `--Settings` include an `Archives` row with its value and source (default, config file, environment variable).

## Help (`-?`)

One row in the primary options table:

```text
  --Archives    Lists the contents of an archive file (.zip, .iso, .7z, .rar, .tar, ...) given as a path,
                and recurses into archive files with -S and --Tree. Archive files in a listing appear as files.
```

## Target paths

| Command | Result |
|---------|--------|
| `tcdir x.zip` | Lists the file `x.zip` (current behavior). |
| `tcdir --Archives x.zip` | Lists the archive root. |
| `tcdir --Archives x.zip\docs` | Lists the folder `docs` inside the archive. |
| `tcdir --Archives x.zip\docs\*.md` | Lists matching entries in `docs`. |
| `tcdir --Archives x.zip\*.md` | Lists matching entries in the archive root. |
| `tcdir --Archives -S dir` / `--Tree dir` | Recurses through real directories and archives. |
| `tcdir --Archives x.zip\missing` | `Error:   <full path> does not exist` (current not-found text). |

## Errors (existing `{Error}` console output, same path as "does not exist")

| Condition | Message |
|-----------|---------|
| Corrupt or not an archive | `Error:   <archive path> is not a valid archive` |
| Encrypted listing | `Error:   <archive path> is encrypted; its contents cannot be listed without the password` |
| libarchive format on a system without `archiveint.dll` | `Error:   <archive path>: browsing this archive format requires Windows 11 23H2 or later` |
| Unsupported variant (for example UDF 2.50 metadata partition) | `Error:   <archive path> uses an unsupported format variant` |
| Cloud placeholder not available locally (targeted directly) | `Error:   <archive path> is not available offline` |
| I/O error | `Error:   <archive path>: <system message>` |

Errors never stop the listing of other targets or the rest of a recursion. The exit code follows the current rules for an inaccessible directory.
