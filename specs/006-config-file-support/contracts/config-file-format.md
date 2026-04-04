# Contract: Config File Format

## File Location

- Path: `%USERPROFILE%\.tcdirconfig`
- Encoding: UTF-8, with or without BOM (BOM is skipped if present; UTF-16 BOM is rejected with error)

## Syntax

Each line is one of:
- **Setting entry**: Same syntax as a single TCDIR environment variable entry
- **Comment line**: Line beginning with `#` (after trimming whitespace)
- **Inline comment**: Text after `#` on a setting line is ignored
- **Blank line**: Ignored

## Processing Rules

1. File is read as raw bytes via `std::ifstream` (binary mode), BOM-checked, converted to UTF-16 via `MultiByteToWideChar`
2. Split into lines on `\r\n`, `\n`, or `\r`
3. For each line:
   a. Trim leading/trailing whitespace
   b. Skip if empty
   c. Skip if first character is `#`
   d. Strip inline comment: find first `#`, truncate, re-trim
   e. Pass to entry processor (same as env var entry parsing)
4. Duplicate keys: last occurrence wins
5. Invalid entries: error recorded with line number, remaining lines continue processing

## Entry Types (identical to env var)

| Type | Example | Description |
|------|---------|-------------|
| Switch | `w` | Toggle a switch on |
| Switch (off) | `w-` | Toggle a switch off |
| Extension color | `.cpp = LightGreen` | Set color for file extension |
| Display attribute | `D = LightBlue` | Set color for display item |
| File attribute | `attr:h = DarkGrey` | Set color for file attribute |
| Icon override | `.cpp = LightGreen, e795` | Set color + icon for extension |
| Parameterized | `Depth = 3` | Set parameterized value |
| Well-known dir | `dir:.git = DarkGrey` | Set color for known directory |

## Precedence

```
Built-in defaults  <  .tcdirconfig  <  TCDIR env var  <  CLI flags
```
