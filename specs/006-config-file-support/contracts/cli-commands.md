# Contract: CLI Commands

## Modified Commands

### `/config` (repurposed)

**Previous behavior**: Display merged configuration tables with Default/Environment sources.

**New behavior**: Config file diagnostic command (parallel to `/env`).

**Output sections** (in order):
1. Config file syntax reference (format description, entry types, comment rules)
2. Config file location: `%USERPROFILE%\.tcdirconfig`
3. File status: "Loaded" / "Not found" / "Error: <message>"
4. Decoded settings from config file, grouped by type:
   - Switches
   - Display items
   - File attributes
   - Extensions
   - Icon overrides
5. Config file parse errors (if any), with line numbers

### `/env` (unchanged)

No changes. Continues to show env var syntax help, current value decoded, and env var errors.

## New Commands

### `/settings`

**Behavior**: Display merged configuration tables — the output that `/config` produces today.

**Output sections** (in order):
1. Icon status (activation source and state)
2. Merged configuration tables:
   - Display attributes (with source column)
   - File attributes (with source column)
   - Extensions (with source column)
   - Well-known directories (icons, if active)
3. Config file errors (if any)
4. Environment variable errors (if any)

**Source column values**: `Default`, `Config file`, `Environment`

### `/help` (updated)

The help output must list `/settings` as a new command and update the `/config` description.

## Error Display (on normal runs)

After directory listing output, errors are displayed in this order:
1. Config file errors (header: "Config file issues in <path>:") — each error includes line number
2. Environment variable errors (header: "Environment variable issues:") — unchanged format

Either group is omitted if it has no errors.
