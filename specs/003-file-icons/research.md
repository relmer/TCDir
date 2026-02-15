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

---

# Phase 0: Implementation Research

*Added: 2026-02-14 during /speckit.plan*

This section consolidates implementation-specific research for the file-icons feature. The terminal host detection section above provides context; this section covers the specific Win32 APIs, design decisions, and coding patterns needed for implementation.

---

## R1: Glyph Canary Probe via GetGlyphIndicesW

### Decision
Use `GetGlyphIndicesW` with `GGI_MARK_NONEXISTING_GLYPHS` to definitively test whether the current console font contains a Nerd Font glyph at U+E5FA. This is the step 4 detection method for classic conhost.

### Rationale
`GetGlyphIndicesW` is the only Win32 API that can definitively answer "does font X contain glyph Y?" without rendering. It works in a memory DC and doesn't require a window. The canary code point U+E5FA (`nf-custom-folder_npm`) is in the Seti-UI BMP range, specific to Nerd Fonts v3, and absent from all standard Windows console fonts.

### Alternatives Considered
- **`GetCharacterPlacementW`**: Returns glyph indices but with more overhead (kerning, reordering). Overkill for a single-glyph probe.
- **`ScriptShape`** (Uniscribe): Full complex script shaping — far too heavy for a boolean glyph check.
- **Render and read back**: Write the glyph, read the screen buffer, compare. Fragile, slow, and distorts output.

### API Flow
```
GetCurrentConsoleFontEx → fontInfo.FaceName
CreateCompatibleDC(nullptr) → hdc
CreateFontW(fontInfo.FaceName) → hFont
SelectObject(hdc, hFont)
GetGlyphIndicesW(hdc, &canary, 1, &glyphIndex, GGI_MARK_NONEXISTING_GLYPHS)
glyphIndex != 0xFFFF → glyph exists
Cleanup: SelectObject back, DeleteDC, DeleteObject
```

### Key Details
- **Library**: `gdi32.lib` (add `#pragma comment(lib, "gdi32.lib")` to `pch.h`)
- **Works for BMP only**: U+E5FA is a single `wchar_t`. `GetGlyphIndicesW` does not handle surrogate pairs — but the canary is BMP, so this is fine.
- **Error vs missing**: `GDI_ERROR` (0xFFFFFFFF as DWORD) = API failure. Glyph index 0xFFFF = glyph not in font. Must check both.
- **Raster fonts**: If console uses Terminal raster font, `FaceName` may be empty/unusual. The probe correctly returns false — raster fonts don't have NF glyphs.
- **ConPTY caveat**: Under ConPTY, `GetCurrentConsoleFontEx` returns the conhost shim's font (often "Consolas"), NOT the terminal's rendering font. This probe is ONLY valid in classic conhost. The layered detection strategy ensures this probe isn't called under ConPTY.

### EHM Pattern
Single exit via `Error:` label. GDI handle cleanup in the Error block. Use `CWRA` for `GetCurrentConsoleFontEx` (sets last error), `CPRAEx` for `CreateCompatibleDC`/`CreateFontW` (return null on failure), `CWRAEx` for `GetGlyphIndicesW`.

---

## R2: System Font Enumeration via EnumFontFamiliesExW

### Decision
Use `EnumFontFamiliesExW` with `DEFAULT_CHARSET` and empty `lfFaceName` to enumerate all installed font families, checking each name for Nerd Font naming patterns. This is the step 5 heuristic for ConPTY terminals.

### Rationale
When running under ConPTY (Windows Terminal, VS Code, etc.), there is no API to query the real rendering font. Enumerating installed system fonts and checking for Nerd Font naming conventions ("Nerd Font", " NF", " NFM", " NFP" suffixes) is a reasonable heuristic — users don't install Nerd Fonts by accident.

### Alternatives Considered
- **Read terminal config files** (Windows Terminal settings.json, VS Code settings.json, etc.): Possible but requires parsing JSONC/TOML/Lua/XML for 7+ terminal types. Fragile, high maintenance, and the font cascade may be overridden by profiles. The research.md above documents all config formats for future reference, but the simpler enumeration heuristic is preferred for v1.
- **`TERM_PROGRAM`-based font inference**: Knowing the terminal type but not the font doesn't help — only WezTerm guarantees NF fallback.

### Name Matching Patterns
```
faceName.find(L"Nerd Font") != npos   — "FiraCode Nerd Font", "FiraCode Nerd Font Mono"
faceName.ends_with(L" NF")            — "FiraCode NF"
faceName.ends_with(L" NFM")           — "FiraCode NFM" (mono)
faceName.ends_with(L" NFP")           — "FiraCode NFP" (proportional)
```

### Key Details
- **Library**: `gdi32.lib` (same as R1)
- **`DEFAULT_CHARSET`**: Critical — enumerates fonts across all character sets.
- **Callback returns 0 to stop**: Once a Nerd Font is found, return 0 to terminate enumeration early.
- **Performance**: Sub-millisecond on typical systems (200–500 fonts). Runs once at startup; no caching needed.
- **C++20**: `std::wstring_view::ends_with` available with `/std:c++latest`.
- **False positive handling**: If a NF is installed but the terminal isn't using it, icons appear as broken glyphs. User can fix with `/Icons-` or `TCDIR=Icons-`. This is acceptable per spec assumptions.

---

## R3: Surrogate Pair Encoding for Material Design Icons

### Decision
Store all icon code points as `char32_t` in mapping tables. Convert to UTF-16 at display time using a constexpr helper. Use `%s` (not `%c`) with a null-terminated `wchar_t[3]` buffer for Printf output.

### Rationale
Material Design icons (`nf-md-*`) use 5-digit supplementary plane code points (e.g., U+F08C6) that require surrogate pairs in UTF-16 (`wchar_t` is 16-bit on Windows). Storing as `char32_t` is clean, unambiguous, and can be validated at compile time. The conversion to `wchar_t[3]` (max 2 surrogates + null) is trivial.

### Formula
```
For cp > 0xFFFF:
  hi = 0xD800 + ((cp - 0x10000) >> 10)
  lo = 0xDC00 + ((cp - 0x10000) & 0x3FF)
```

### Compile-Time Verification
Key code points from the spec verified at compile time via `static_assert`:

| Code Point | Glyph | High | Low |
|-----------|-------|------|-----|
| U+F08C6 (`nf-md-application`) | 0xDB82 | 0xDCC6 | Surrogate pair |
| U+F0219 (`nf-md-file_document`) | 0xDB80 | 0xDE19 | Surrogate pair |
| U+F0227 (`nf-md-file_powerpoint`) | 0xDB80 | 0xDE27 | Surrogate pair |
| U+F0163 (`nf-md-cloud_outline`) | 0xDB80 | 0xDD63 | Surrogate pair |
| U+F0160 (`nf-md-cloud_check`) | 0xDB80 | 0xDD60 | Surrogate pair |
| U+F0403 (`nf-md-pin`) | 0xDB81 | 0xDC03 | Surrogate pair |
| U+F0668 (`nf-md-test_tube`) | 0xDB81 | 0xDE68 | Surrogate pair |
| U+E749 (`nf-dev-css3`) | 0xE749 | — | Single wchar_t |
| U+E628 (`nf-seti-typescript`) | 0xE628 | — | Single wchar_t |

### Printf Pattern
```cpp
// Unified pattern for both BMP and supplementary plane:
wchar_t szIcon[3] = {};
auto pair = CodePointToWideChars(iconCodePoint);
szIcon[0] = pair.chars[0];
szIcon[1] = (pair.count > 1) ? pair.chars[1] : L'\0';
pConsole->Printf(textAttr, L"%s %s", szIcon, wfd.cFileName);
```

### Alternatives Considered
- **Store as `wchar_t[3]` directly**: Wastes space for BMP glyphs and obscures the actual Unicode code point. Hard to validate.
- **Store as `wstring`**: Heap allocation for every icon mapping. Unnecessary since max length is 2 `wchar_t`.
- **BMP-only (reject Material Design)**: Would lose `.exe`, `.ppt`, `.txt`, cloud status, and test dir icons. Too many important glyphs are in the supplementary plane.

---

## R4: ConPTY Detection Strategy

### Decision
Probe a fixed list of environment variables in priority order. If any are set, the process is under ConPTY and `GetCurrentConsoleFontEx` is unreliable for font detection.

### Variables (in check order)
```
WT_SESSION         → Windows Terminal
TERM_PROGRAM       → VS Code, WezTerm, Hyper, mintty
ConEmuPID          → ConEmu / Cmder
ALACRITTY_WINDOW_ID → Alacritty
```

### WezTerm Special Case
WezTerm (`TERM_PROGRAM=WezTerm`) bundles Nerd Font Symbols as a default fallback font. The detection chain short-circuits to "icons ON" for WezTerm before checking ConPTY.

### Key Details
- **Uses existing `IEnvironmentProvider`**: All env var access goes through the injected provider, enabling full unit test coverage with `CTestEnvironmentProvider`.
- **No additional variables needed**: The four listed cover all major ConPTY terminals on Windows. Rare terminals that don't set any of these will fall through to "no env vars set → assume classic conhost → probe glyph canary." If the probe fails (the font doesn't have NF glyphs), icons default OFF. Safe.
- **SSH sessions**: Env vars may not be forwarded. Falls through to classic conhost probe — correct behavior since the SSH terminal's font is the local terminal's font, and the remote conhost has no knowledge of it.

---

## R5: Extended TCDIR Parsing Design

### Decision
Extend the existing `ProcessColorOverrideEntry()` flow to detect and parse the optional `,icon` suffix after the color value. The comma is the delimiter. Icon parsing happens after color parsing in the same entry.

### Parsing Flow
```
Entry: ".cpp=Green,U+E61D"
  1. ParseKeyAndValue → key=".cpp", value="Green,U+E61D"
  2. Split value on first comma → colorPart="Green", iconPart="U+E61D"
  3. ParseColorValue(colorPart) → WORD colorAttr
  4. ParseIconValue(iconPart) → char32_t codePoint
  5. Dispatch: ProcessFileExtensionOverride(key, colorAttr, codePoint)
```

### Icon Value Formats
| Input | Interpretation |
|-------|---------------|
| `U+E61D` | Hex code point (4–6 digits after `U+`) |
| `U+F0163` | Hex code point (supplementary plane) |
| Literal glyph (1 char or surrogate pair) | Direct code point from character |
| Empty (comma present, nothing after) | Icon suppressed (explicit removal) |
| No comma | No icon change (backward compatible) |

### Validation
- `U+` prefix followed by 4–6 hex digits → valid code point
- Code point must be in range U+0001–U+10FFFF (reject 0, reject surrogates D800–DFFF, reject >10FFFF)
- Literal glyph: 1 character (BMP) or 2 characters forming a valid surrogate pair
- Invalid icon values → `ErrorInfo` with underline at the icon portion

### Duplicate Handling
First-write-wins for both color and icon components of an extension/attribute/dir entry. If `.cpp=Green,U+E61D` appears first and `.cpp=Red` appears later, the second entry is flagged as a duplicate error. The color stays Green, the icon stays U+E61D.

### Backward Compatibility
Entries without a comma are parsed exactly as before — the icon parsing path is not entered. Zero regression for existing TCDIR configurations.

---

## R6: Unified Precedence Resolution Design

### Decision
Extend `GetTextAttrForFile()` to return a `SFileDisplayStyle` struct containing both resolved color (`WORD`) and resolved icon (`char32_t`). The precedence walk resolves both in a single pass, with independent locking semantics for color and icon.

### Precedence Walk
```
for each level in [attributes, well-known-dir, extension, type-fallback]:
    if level matches the file:
        if !colorLocked and level has color configured:
            resolvedColor = level's color
            colorLocked = true
        if !iconLocked and level has icon configured:
            resolvedIcon = level's icon
            iconLocked = true
        if colorLocked and iconLocked:
            break
```

### Key Insight
Color and icon lock independently. A hidden `.cpp` file gets DarkGrey color from the attribute level (color locks) but falls through to extension level for the C++ icon (icon locks there). This matches the spec's precedence examples exactly.

### Return Type
```cpp
struct SFileDisplayStyle
{
    WORD      m_wTextAttr;      // Resolved color attribute
    char32_t  m_iconCodePoint;  // Resolved icon (0 = no icon)
    bool      m_fIconSuppressed; // true if icon was explicitly set to empty
};
```

### Attribute Precedence Order Change
The spec reorders attribute precedence from `RHSATEC0P` to `PSHERC0TA`. This is a behavioral change for users with attribute-level color overrides relying on the old order. A new `k_rgIconAttributePrecedence[]` array defines the new order, separate from the existing `k_rgFileAttributeMap[]` (which retains display-column order `RHSATECP0`).

---

## R7: New gdi32.lib Dependency

### Decision
Add `#pragma comment(lib, "gdi32.lib")` to `pch.h` alongside the existing `#pragma comment(lib, "cldapi.lib")`.

### Rationale
`gdi32.lib` is required for `GetGlyphIndicesW`, `CreateCompatibleDC`, `CreateFontW`, `SelectObject`, `DeleteDC`, `DeleteObject`, and `EnumFontFamiliesExW`. All are standard Win32 APIs present in every Windows SDK version. The pragma approach matches the existing pattern in the codebase.

### Alternatives Considered
- **vcxproj AdditionalDependencies**: Works but less visible than the pragma in `pch.h`. The project already uses pragmas for `cldapi.lib`.

---

## Research Summary

| ID | Topic | Decision | Status |
|----|-------|----------|--------|
| R1 | Canary probe API | `GetGlyphIndicesW` + `GGI_MARK_NONEXISTING_GLYPHS` | Resolved |
| R2 | Font enumeration API | `EnumFontFamiliesExW` + name matching | Resolved |
| R3 | Surrogate pairs | `char32_t` storage + constexpr converter | Resolved |
| R4 | ConPTY detection | 4 env vars (WT_SESSION, TERM_PROGRAM, ConEmuPID, ALACRITTY_WINDOW_ID) | Resolved |
| R5 | TCDIR icon parsing | Comma delimiter, U+XXXX hex, literal glyph, empty = suppress | Resolved |
| R6 | Precedence resolver | Single walk, independent color/icon locking | Resolved |
| R7 | gdi32.lib link | `#pragma comment(lib, "gdi32.lib")` in pch.h | Resolved |

All NEEDS CLARIFICATION items resolved. No blockers for Phase 1 design.
