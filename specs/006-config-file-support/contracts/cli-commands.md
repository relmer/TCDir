# Contract: CLI Commands

## Modified Commands

### `/config` (repurposed)

**Previous behavior**: Display merged configuration tables with Default/Environment sources.

**New behavior**: Config file diagnostic command (parallel to `/env`).

**Output sections** (in order):
1. Config file syntax reference (format description, entry types, comment rules, switch list)
2. Available color reference table
3. Icon specification format (U+XXXX, literal glyph, or empty for suppression)
4. Example `.tcdirconfig` file
5. Note: "Environment variable settings override config file settings."
6. Config file location and status:
   - `Config file: <path> found` — file was loaded
   - `Config file: <path> not found` — file does not exist
   - `Config file: (not resolved — USERPROFILE not set)` — USERPROFILE env var is missing
7. Config file parse errors (if any), with line numbers

### `/env` (unchanged)

No changes. Continues to show env var syntax help, current value decoded, and env var errors.

## New Commands

### `/settings`

**Behavior**: Display merged configuration tables — the output that `/config` produces today.

**Output sections** (in order):
1. "No config file or TCDIR environment variable set; showing defaults." (shown only when neither source is set)
2. Icon status (Nerd Font detection state, icon activation ON/OFF, and source: CLI /Icons, TCDIR=Icons, Config file, or Auto-detection)
3. Merged configuration tables:
   - Switches & parameters (W, S, P, M, B, Owner, Streams, Icons, Tree, Depth, TreeIndent, Size)
   - Display attributes (with source column)
   - Cloud status items (with symbols)
   - File attributes (with source column)
   - Extensions (with source column)
   - Well-known directory icons (when icons active)
4. Config file errors (if any)
5. Environment variable errors (if any)

**Source column values**: `Default`, `Config file`, `Environment`

### `/help` (updated)

The help output must list `/settings` as a new command and update the `/config` description.

## Error Display (on normal runs)

After directory listing output, errors are displayed in this order:
1. Config file errors (header: "There are some problems with your config file (see /config for help):") — each error includes line number in format `Line N: <message> in "<entry>"` with underline indicator
2. Environment variable errors (header: "There are some problems with your TCDIR environment variable (see /env for help):") — same underline format without line numbers

When errors are displayed within `/settings`, the `(see ... for help)` hint is omitted from both headers.

Either group is omitted if it has no errors.
