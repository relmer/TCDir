# Terminal Host Detection & Font Configuration Research

## Purpose

This document provides a comprehensive reference for detecting which terminal host a
Windows console application is running inside, and how to read the configured font from
that host's settings. This supports the file-icons feature (#3) which needs to determine
whether the user's terminal font supports Nerd Font glyphs.

---

## Detection Priority Order

Check environment variables in this order (first match wins):

| Priority | Env Variable            | Terminal          |
|----------|-------------------------|-------------------|
| 1        | `WT_SESSION`            | Windows Terminal  |
| 2        | `TERM_PROGRAM=vscode`   | VS Code           |
| 3        | `TERM_PROGRAM=WezTerm`  | WezTerm           |
| 4        | `TERM_PROGRAM=Hyper`    | Hyper             |
| 5        | `TERM_PROGRAM=mintty`   | Mintty / Git Bash |
| 6        | `ConEmuPID`             | ConEmu / Cmder    |
| 7        | `ALACRITTY_WINDOW_ID`   | Alacritty         |
| 8        | *(fallback)*            | Classic Conhost   |

---

## 1. Windows Terminal

### Detection
- **Environment variable:** `WT_SESSION` (contains a GUID for the current session)
- **Also available:** `WT_PROFILE_ID` (GUID of the active profile)

### Settings File Location
Multiple paths depending on installation method:

| Install Method            | Path                                                                                              |
|---------------------------|---------------------------------------------------------------------------------------------------|
| Microsoft Store (stable)  | `%LOCALAPPDATA%\Packages\Microsoft.WindowsTerminal_8wekyb3d8bbwe\LocalState\settings.json`        |
| Microsoft Store (preview) | `%LOCALAPPDATA%\Packages\Microsoft.WindowsTerminalPreview_8wekyb3d8bbwe\LocalState\settings.json` |
| Unpackaged (Scoop, Choco) | `%LOCALAPPDATA%\Microsoft\Windows Terminal\settings.json`                                         |

### Format
JSON (with comments support, aka JSONC)

### Font Config Keys
```jsonc
{
  "profiles": {
    "defaults": {
      "font": {
        "face": "Cascadia Mono",   // font family name
        "size": 12,                // font size in points
        "weight": "normal"         // font weight
      }
    },
    "list": [
      {
        "guid": "{...}",
        "font": {
          "face": "JetBrainsMono Nerd Font"  // per-profile override
        }
      }
    ]
  }
}
```

**Reading strategy:**
1. Find the profile matching `WT_PROFILE_ID` in `profiles.list[]`
2. Check if that profile has `font.face` set
3. If not, fall back to `profiles.defaults.font.face`
4. **Legacy keys** (pre v1.10): `fontFace`, `fontSize` (flat strings on profiles)

### Default Font
`Cascadia Mono`, size 12

### Complications
- Profile inheritance: per-profile overrides `defaults`
- Three different install paths to search
- Legacy key names (`fontFace`) in older configs
- JSON may contain trailing commas and comments (JSONC)
- Must find all three store paths before falling back

---

## 2. VS Code Integrated Terminal

### Detection
- **Environment variable:** `TERM_PROGRAM=vscode`
- **Also available:** `TERM_PROGRAM_VERSION` (VS Code version string)

### Settings File Location
```
%APPDATA%\Code\User\settings.json
```
VS Code Insiders: `%APPDATA%\Code - Insiders\User\settings.json`

### Format
JSON (with comments, JSONC)

### Font Config Keys
```jsonc
{
  "terminal.integrated.fontFamily": "JetBrainsMono Nerd Font",  // CSS font-family format
  "terminal.integrated.fontSize": 14,
  "terminal.integrated.fontWeight": "normal"
}
```

**Reading strategy:**
1. Look for `terminal.integrated.fontFamily` in settings.json
2. If not set, VS Code falls back to `editor.fontFamily`, then system monospace default
3. The value is CSS format: may contain comma-separated fallbacks with optional quotes
   (e.g., `"'JetBrains Mono', 'Courier New', monospace"`)
4. Extract the **first** font family name from the CSS list

### Default Font
When `terminal.integrated.fontFamily` is not set, VS Code uses the value of
`editor.fontFamily` (itself defaulting to platform monospace). On Windows this
typically resolves to `Consolas`.

### Complications
- CSS font-family format requires parsing (strip quotes, split on commas)
- Settings may have `//` comments
- Workspace settings can override user settings
  (`<workspace>/.vscode/settings.json`)
- VS Code Insiders has a separate settings path
- Remote SSH/WSL sessions: settings are on the remote, not the local machine

---

## 3. ConEmu / Cmder

### Detection
- **Environment variables:** `ConEmuDir`, `ConEmuPID`, `ConEmuBuild`,
  `ConEmuBaseDir`, `ConEmuArgs`
- **Best check:** `ConEmuPID` is always set when running inside ConEmu
- **Cmder:** Also sets `CMDER_ROOT`; Cmder wraps ConEmu so ConEmu env vars are present

### Settings File Location
Search sequence (first found wins):

| Priority | Location                       | Notes                          |
|----------|--------------------------------|--------------------------------|
| 1        | CLI: `-loadcfgfile` argument   | Explicit path                  |
| 2        | `%ConEmuDir%\ConEmu.xml`       | Same folder as ConEmu.exe      |
| 3        | `%ConEmuBaseDir%\ConEmu.xml`   | Same folder as ConEmuC.exe     |
| 4        | `%APPDATA%\ConEmu.xml`         | Roaming profile                |
| 5        | Windows Registry               | `HKCU\Software\ConEmu\.Vanilla`|

Files without a leading dot (`.`) have priority over dotted variants (`.ConEmu.xml`).

### Format
XML (custom schema)

### Font Config Keys
In ConEmu.xml, font settings are stored under the main settings node. Key elements:

```xml
<key name=".Vanilla" modified="...">
  <value name="FontName" type="string" data="Consolas"/>
  <value name="FontSize" type="ulong" data="14"/>
  <value name="FontBold" type="hex" data="00"/>
  <value name="FontItalic" type="hex" data="00"/>
  <value name="FontCharSet" type="ulong" data="0"/>
  <!-- Alternative font for pseudographics / CJK -->
  <value name="FontName2" type="string" data=""/>
  <value name="FontSize2" type="ulong" data="0"/>
</key>
```

**Reading strategy:**
1. Parse XML, locate the active configuration key (usually `.Vanilla` or named config)
2. Read `FontName` value for the primary font family
3. Read `FontSize` for the font size

### Default Font
`Consolas`, size 14

### Complications
- XML parsing required; custom schema with `<value>` elements
- May fall back to Windows Registry if no XML file exists
- Named configurations: `-config "name"` switch selects a sub-key
- Cmder may have its own ConEmu.xml in a non-standard location
  (`%CMDER_ROOT%\vendor\conemu-maximus5\ConEmu.xml`)

---

## 4. Alacritty

### Detection
- **Environment variable:** `ALACRITTY_WINDOW_ID` (set since Alacritty 0.4.0)
- **Also available:** `ALACRITTY_LOG` (path to log file), `ALACRITTY_SOCKET` (IPC socket, Unix)
- **Note:** Alacritty does NOT set `TERM_PROGRAM` by default. Users can configure it
  via `[env]` in config. `ALACRITTY_WINDOW_ID` is the reliable detection method.

### Settings File Location
On Windows:
```
%APPDATA%\alacritty\alacritty.toml
```

Full search sequence on Windows:
1. `%APPDATA%\alacritty\alacritty.toml`

(On Unix: `$XDG_CONFIG_HOME/alacritty/alacritty.toml`, `$HOME/.config/alacritty/alacritty.toml`, etc.)

### Format
TOML (since Alacritty 0.13; previously YAML in `alacritty.yml`)

### Font Config Keys
```toml
[font]
size = 11.25

[font.normal]
family = "JetBrainsMono Nerd Font"
style = "Regular"

[font.bold]
style = "Bold"

[font.italic]
style = "Italic"

[font.bold_italic]
style = "Bold Italic"
```

**Reading strategy:**
1. Parse TOML file
2. Read `font.normal.family` for the font family name
3. Read `font.size` for the font size
4. If `font.normal.family` is absent, use the platform default

### Default Font
- **Windows:** `Consolas`, Regular
- **Linux/BSD:** `monospace`, Regular
- **macOS:** `Menlo`, Regular
- **Default size:** 11.25

### Complications
- Pre-0.13 configs used YAML (`alacritty.yml`) instead of TOML
- Legacy YAML files may still be present; Alacritty stopped reading them in 0.14
- TOML inline table syntax: `normal = { family = "Consolas", style = "Regular" }`
- The `[general]` table supports `import` for loading additional TOML files
- No `TERM_PROGRAM` set by default; must use `ALACRITTY_WINDOW_ID`

---

## 5. WezTerm

### Detection
- **Environment variable:** `TERM_PROGRAM=WezTerm`
- **Also available:** `WEZTERM_EXECUTABLE` (path to wezterm binary),
  `WEZTERM_PANE` (pane ID), `WEZTERM_UNIX_SOCKET` (mux socket)

### Settings File Location
Search sequence on Windows:

| Priority | Location                                          |
|----------|---------------------------------------------------|
| 1        | `--config-file` CLI argument                      |
| 2        | `$WEZTERM_CONFIG_FILE` environment variable        |
| 3        | Same directory as `wezterm.exe` (thumb drive mode) |
| 4        | `%USERPROFILE%\.wezterm.lua`                       |
| 5        | `%USERPROFILE%\.config\wezterm\wezterm.lua`        |

### Format
Lua script (returns a config table)

### Font Config Keys
```lua
local wezterm = require 'wezterm'
local config = wezterm.config_builder()

-- Simple single font:
config.font = wezterm.font('JetBrains Mono')

-- With attributes:
config.font = wezterm.font('JetBrains Mono', { weight = 'Bold', italic = true })

-- With fallback chain:
config.font = wezterm.font_with_fallback {
  'Fira Code',
  'DengXian',
}

-- Font size:
config.font_size = 12.0

return config
```

**Reading strategy:**
Since the config is Lua code, you **cannot** simply parse it as data.
Best approaches:
1. **Regex extraction:** Look for patterns like:
   - `config.font\s*=\s*wezterm\.font\s*[({]\s*['"]([^'"]+)['"]`
   - `config.font_size\s*=\s*([0-9.]+)`
   - `config.font\s*=\s*wezterm\.font_with_fallback\s*{[^}]*['"]([^'"]+)['"]`
2. **Run `wezterm ls-fonts`:** This CLI command outputs the effective font
   configuration, including the resolved primary font family.

### Default Font
`JetBrains Mono` (bundled with WezTerm), with `Nerd Font Symbols` as automatic
fallback. Default font size: 12.0.

**Important for Nerd Font detection:** WezTerm bundles Nerd Font Symbols as a
default fallback font, so Nerd Font glyphs are available even when the primary
font isn't a Nerd Font.

### Complications
- Lua config cannot be reliably parsed without Lua execution
- `wezterm.font()` and `wezterm.font_with_fallback()` are function calls
- Config can span multiple files via `require` modules
- `wezterm ls-fonts` provides runtime info but requires wezterm to be installed
- WezTerm auto-includes Nerd Font Symbols fallback, so icon support is likely
  available regardless of the user's primary font choice

---

## 6. Mintty / Git Bash

### Detection
- **Environment variables:** `TERM_PROGRAM=mintty` (since mintty 3.1.5),
  `TERM_PROGRAM_VERSION`, `MINTTY_PID`, `MINTTY_SHORTCUT`
- **Other env vars:** `MINTTY_PROG`, `MINTTY_CWD`
- **Fallback:** Secondary Device Attributes escape sequence query

### Settings File Location
Config files are read in order (later files override earlier):

| Priority | Location                           | Notes                        |
|----------|------------------------------------|------------------------------|
| 1        | `/etc/minttyrc`                    | System-wide                  |
| 2        | `$APPDATA/mintty/config`           | User config (typical for Git Bash) |
| 3        | `~/.config/mintty/config`          | XDG-style user config        |
| 4        | `~/.minttyrc`                      | Legacy user config           |

Additional: `-c`/`--config` CLI flag, `--configdir` for directory override.

**For Git Bash specifically:** The config is almost always at:
```
%APPDATA%\mintty\config
```

### Format
Simple `Name=Value` pairs, one per line (INI-like, no section headers).

### Font Config Keys
```ini
Font=Cascadia Code NF
FontHeight=11
FontWeight=400
FontIsBold=no

# Alternative fonts for specific Unicode ranges:
Font1=Segoe UI Symbol
Font2=Noto Color Emoji
# Nerd Font private use area fallback:
FontChoice=Private:3
Font3=MesloLGS NF
```

**Reading strategy:**
1. Read the config file (simple line-by-line key=value parsing)
2. Look for `Font=` for the primary font family name
3. Look for `FontHeight=` for the font size
4. Note: `FontChoice=Private:N` maps Private Use Area glyphs to `FontN`

### Default Font
`Lucida Console`, size 9, weight 400

### Complications
- Cascading config files: settings in later files override earlier ones
- Git Bash may place config at `$APPDATA/mintty/config` while MSYS2 uses `~/.minttyrc`
- `FontChoice` with `Private` keyword maps PUA ranges to alternate fonts,
  which is directly relevant for Nerd Font glyph detection
- The `Font1`..`Font10` alternate font system means multiple fonts may provide
  different glyph ranges

---

## 7. Hyper

### Detection
- **Environment variable:** `TERM_PROGRAM=Hyper`

### Settings File Location
```
%APPDATA%\Hyper\.hyper.js
```

### Format
JavaScript module (`module.exports = { config: {...}, plugins: [...] }`)

### Font Config Keys
```javascript
module.exports = {
  config: {
    fontSize: 14,
    fontFamily: '"JetBrainsMono Nerd Font", "DejaVu Sans Mono", "Lucida Console", monospace',
    fontWeight: "normal",
    fontWeightBold: "bold",
  },
  plugins: [],
};
```

**Reading strategy:**
1. The file is JavaScript, not JSON; requires regex extraction
2. Look for `fontSize:\s*(\d+)` for font size
3. Look for `fontFamily:\s*['"](.*?)['"]` for font family (CSS format)
4. The `fontFamily` value is CSS font-family format with comma-separated fallbacks
5. Extract the first font family from the CSS list

### Default Font
Typically `"Menlo, DejaVu Sans Mono, Lucida Console, monospace"` (varies by platform).

### Complications
- JavaScript file cannot be parsed as JSON (has `module.exports`, trailing commas, comments)
- Font family is CSS format with quoted font names and comma-separated fallbacks
- Hyper plugins can modify config via `decorateConfig` hook (impossible to detect
  without running the JavaScript)
- Hyper is Electron-based, so it uses xterm.js for rendering (similar to VS Code)

---

## 8. Classic Windows Console Host (conhost.exe)

### Detection
- **Fallback:** When none of the above environment variables are detected,
  the application is likely running in classic conhost.exe
- **Process tree:** Parent process is `conhost.exe` (can verify via
  process tree walking, but environment variable absence is simpler)
- **Note:** Windows Terminal wraps conhost.exe processes, so always check for
  `WT_SESSION` first

### Settings File Location
No settings file. Font configuration is per-console-window via:
- **Win32 API:** `GetCurrentConsoleFontEx()` (runtime query)
- **Registry:** Per-shortcut settings at `HKCU\Console\<title>`
- **Default:** `HKCU\Console` (default values)

### Format
Win32 API / Registry

### Font Config Keys
**Runtime API (preferred):**
```cpp
CONSOLE_FONT_INFOEX cfi = {};
cfi.cbSize = sizeof(cfi);
GetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);
// cfi.FaceName   = L"Consolas"  (font family, WCHAR[LF_FACESIZE])
// cfi.dwFontSize = {8, 16}      (width, height in pixels)
// cfi.FontWeight = 400           (FW_NORMAL = 400, FW_BOLD = 700)
```

**Registry (for reading defaults without a console):**
```
HKCU\Console
  FaceName    REG_SZ      "Consolas"
  FontSize    REG_DWORD   0x00100000  (HIWORD=height, LOWORD=width)
  FontWeight  REG_DWORD   400
  FontFamily  REG_DWORD   54          (FF_MODERN | FIXED_PITCH)
```

### Default Font
`Consolas` on modern Windows (Vista+), `Lucida Console` on older systems.
Default size: 16 pixels high (approximately 12pt).

### Complications
- `GetCurrentConsoleFontEx()` only works when the process has an attached console
- Registry stores per-shortcut overrides under `HKCU\Console\<window title>`
- The API returns pixel sizes, not point sizes
- `conhost.exe` is also the backend for Windows Terminal (but WT_SESSION
  distinguishes the two)
- Windows 11 may default to Windows Terminal as the default console host

---

## Summary Table

| Terminal         | Detection Method                | Config Format | Config Path (Windows)                                    | Font Key                           | Default Font         |
|------------------|---------------------------------|---------------|----------------------------------------------------------|------------------------------------|----------------------|
| Windows Terminal | `WT_SESSION` env var            | JSON(C)       | `%LOCALAPPDATA%\Packages\...\settings.json`              | `font.face`                        | Cascadia Mono 12     |
| VS Code          | `TERM_PROGRAM=vscode`           | JSON(C)       | `%APPDATA%\Code\User\settings.json`                      | `terminal.integrated.fontFamily`   | (inherits) Consolas  |
| WezTerm          | `TERM_PROGRAM=WezTerm`          | Lua           | `%USERPROFILE%\.wezterm.lua`                              | `config.font = wezterm.font("x")`  | JetBrains Mono 12    |
| Hyper            | `TERM_PROGRAM=Hyper`            | JavaScript    | `%APPDATA%\Hyper\.hyper.js`                               | `fontFamily`                       | (CSS fallback list)  |
| Mintty/Git Bash  | `TERM_PROGRAM=mintty`           | Key=Value     | `%APPDATA%\mintty\config`                                 | `Font=`                            | Lucida Console 9     |
| ConEmu/Cmder     | `ConEmuPID` env var             | XML           | `%ConEmuDir%\ConEmu.xml` or `%APPDATA%\ConEmu.xml`       | `<value name="FontName">`          | Consolas 14          |
| Alacritty        | `ALACRITTY_WINDOW_ID` env var   | TOML          | `%APPDATA%\alacritty\alacritty.toml`                     | `font.normal.family`               | Consolas 11.25       |
| Classic Conhost  | *(fallback)*                    | Win32 API     | N/A (runtime API or registry)                             | `GetCurrentConsoleFontEx()`        | Consolas             |

---

## Implementation Notes

### Nerd Font Detection Strategy
Once the font family name is obtained, determine if it's a Nerd Font by:
1. **Name matching:** Check if the font name contains "Nerd Font", "NF", or known
   Nerd Font family names (e.g., "MesloLGS NF", "FiraCode Nerd Font")
2. **Glyph probing:** Attempt to render a known Nerd Font glyph (e.g., U+E0A0,
   Powerline branch symbol) and check if it renders. This requires terminal
   cooperation and is less reliable.
3. **WezTerm special case:** WezTerm bundles Nerd Font Symbols as a default fallback,
   so Nerd Font glyphs are likely available regardless of primary font.

### Config Parsing Libraries Needed
- **JSON/JSONC:** For Windows Terminal, VS Code (need comment-stripping)
- **TOML:** For Alacritty
- **XML:** For ConEmu
- **INI/Key-Value:** For Mintty (simple line parsing)
- **Regex:** For WezTerm (Lua), Hyper (JavaScript)
- **Win32 API:** For Classic Conhost (`GetCurrentConsoleFontEx`)

### Error Handling
- Config file may not exist (user hasn't customized fonts)
- Config file may be malformed
- Font key may be absent (use default for that terminal)
- Multiple config files may need to be searched (Windows Terminal, Mintty)
- Always have a graceful fallback: if font detection fails, assume icons are
  NOT supported and use text-only mode

---

*Last Updated: 2025-07*
*Source: Official documentation for each terminal emulator*
