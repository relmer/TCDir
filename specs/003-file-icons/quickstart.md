# Quickstart: Implementing Nerd Font File & Folder Icons

**Feature**: 003-file-icons | **Date**: 2026-02-14

This guide walks through the implementation in dependency order. Each section can be implemented and tested independently.

---

## Prerequisites

- Branch: `003-file-icons`
- All existing tests pass (`Build + Test Debug`)
- Visual Studio 2026 / MSBuild + VS Code with existing tasks
- **Coding conventions**: [.github/copilot-instructions.md](../../.github/copilot-instructions.md) — this is the authoritative reference for formatting, EHM patterns, indentation, column alignment, include rules, and all other coding standards

---

## Implementation Order

```
Phase 1: Foundation (no visible output yet)
  1. IconMapping.h/cpp    — Code point table, surrogate helper, lookup functions
  2. NerdFontDetector.h/cpp — Detection interface + implementation
  3. Config.h/cpp         — Icon maps, TCDIR parsing, unified precedence resolver
  4. CommandLine.h/cpp    — /Icons, /Icons- switches
  5. pch.h                — gdi32.lib link

Phase 2: Display Integration
  6. ResultsDisplayerNormal.cpp  — Icon column in normal mode
  7. ResultsDisplayerWide.cpp    — Icon + bracket suppression + cloud status
  8. ResultsDisplayerBare.cpp    — Icon in bare mode
  9. TCDir.cpp                   — Detection chain orchestration

Phase 3: Diagnostics & Polish
  10. Usage.cpp  — /? help, /env docs, /config display
  11. Version.h  — Major version bump

Phase 4: Tests
  12. IconMappingTests.cpp         — Table coverage, surrogate encoding, precedence
  13. NerdFontDetectorTests.cpp    — Detection chain with mocked env provider
  14. ConfigTests.cpp (extended)   — TCDIR icon syntax, duplicates, precedence
  15. CommandLineTests.cpp (extended) — /Icons, /Icons- parsing
  16. ResultsDisplayerTests.cpp (extended) — Icon display in all modes
  17. DirectoryListerScenarioTests.cpp (extended) — End-to-end with icons
```

---

## Phase 1: Foundation

### 1. IconMapping.h/cpp

**New files** in `TCDirCore/`.

#### IconMapping.h

```cpp
#pragma once

// Forward declarations only — no Windows headers needed in the header.
// All Windows types come from pch.h.

////////////////////////////////////////////////////////////////////////////////
//
//  WideCharPair
//
//  UTF-16 encoding of a Unicode code point. BMP code points produce 1 wchar_t;
//  supplementary plane code points produce a surrogate pair (2 wchar_t).
//
////////////////////////////////////////////////////////////////////////////////

struct WideCharPair
{
    wchar_t  chars[2];
    unsigned count;      // 1 = BMP, 2 = surrogate pair, 0 = invalid
};

constexpr WideCharPair CodePointToWideChars (char32_t cp);


////////////////////////////////////////////////////////////////////////////////
//
//  SIconMappingEntry
//
//  Static default icon mapping entry. Parallels CConfig::STextAttr for colors.
//
////////////////////////////////////////////////////////////////////////////////

struct SIconMappingEntry
{
    LPCWSTR   m_pszKey;
    char32_t  m_codePoint;
};


// Default tables (defined in IconMapping.cpp)
extern const SIconMappingEntry g_rgDefaultExtensionIcons[];
extern const size_t            g_cDefaultExtensionIcons;

extern const SIconMappingEntry g_rgDefaultWellKnownDirIcons[];
extern const size_t            g_cDefaultWellKnownDirIcons;


// Attribute precedence order for color+icon resolution (PSHERC0TA).
// Distinct from k_rgFileAttributeMap (display column order RHSATECP0).
extern const SFileAttributeMap k_rgAttributePrecedenceOrder[];
extern const size_t            k_cAttributePrecedenceOrder;
```

#### IconMapping.cpp

```cpp
#include "pch.h"
#include "IconMapping.h"
#include "FileAttributeMap.h"

// constexpr implementation
constexpr WideCharPair CodePointToWideChars (char32_t cp)
{
    if (cp <= 0xFFFF)
    {
        if (cp >= 0xD800 && cp <= 0xDFFF)
            return { { 0, 0 }, 0 };                      // Surrogate code points invalid

        return { { static_cast<wchar_t>(cp), 0 }, 1 };
    }

    if (cp <= 0x10FFFF)
    {
        char32_t adj = cp - 0x10000;
        return { { static_cast<wchar_t>(0xD800 + (adj >> 10)),
                    static_cast<wchar_t>(0xDC00 + (adj & 0x3FF)) }, 2 };
    }

    return { { 0, 0 }, 0 };
}

// Compile-time verification of key code points
static_assert (CodePointToWideChars(NfIcon::DevCss3).count == 1);        // BMP (0xE749)
static_assert (CodePointToWideChars(NfIcon::SetiTypescript).count == 1);  // BMP (0xE628)
static_assert (CodePointToWideChars(0xF08C6).count == 2);
static_assert (CodePointToWideChars(0xF0219).count == 2);
static_assert (CodePointToWideChars(0xD800).count == 0);
static_assert (CodePointToWideChars(0x110000).count == 0);

// ... default tables as specified in data-model.md ...
```

**Key conventions**:
- `pch.h` is always the first include
- All tables are `const` (not `constexpr`) because `LPCWSTR` members aren't constexpr-friendly in MSVC
- Use `static_assert` for compile-time code point verification

### 2. NerdFontDetector.h/cpp

**New files** in `TCDirCore/`.

#### NerdFontDetector.h

```cpp
#pragma once

#include "EnvironmentProviderBase.h"

enum class EDetectionResult { Detected, NotDetected, Inconclusive };

class CNerdFontDetector
{
public:
    HRESULT Detect (
        HANDLE                       hConsole,
        const IEnvironmentProvider & envProvider,
        _Out_ EDetectionResult *     pResult);

protected:
    // GDI-dependent methods — virtual so tests can derive and override
    virtual HRESULT ProbeConsoleFontForGlyph (HANDLE hConsole, WCHAR wchCanary, _Out_ bool * pfHasGlyph);
    virtual HRESULT IsNerdFontInstalled      (_Out_ bool * pfFound);

private:
    static bool  IsWezTerm        (const IEnvironmentProvider & envProvider);
    static bool  IsConPtyTerminal (const IEnvironmentProvider & envProvider);
};
```

**Testing approach**: Derive a test probe (same pattern as `ConfigProbe`):
```cpp
struct NerdFontDetectorProbe : public CNerdFontDetector
{
    bool    m_fProbeResult    = false;   // What ProbeConsoleFontForGlyph returns
    bool    m_fInstalledResult = false;  // What IsNerdFontInstalled returns
    HRESULT m_hrProbe         = S_OK;
    HRESULT m_hrInstalled     = S_OK;

    HRESULT ProbeConsoleFontForGlyph (HANDLE, WCHAR, _Out_ bool * pf) override
    {
        *pf = m_fProbeResult;
        return m_hrProbe;
    }

    HRESULT IsNerdFontInstalled (_Out_ bool * pf) override
    {
        *pf = m_fInstalledResult;
        return m_hrInstalled;
    }
};
```

### 3. Config.h/cpp Extensions

**Add to Config.h** (new members shown in data-model.md):

Key changes:
- Add `SFileDisplayStyle` struct
- Add icon mapping members (`m_mapExtensionToIcon`, `m_mapWellKnownDirToIcon`, etc.)
- Add `m_fIcons` optional
- Add `GetDisplayStyleForFile()` — the unified resolver
- Add `ParseIconValue()` — parse `U+XXXX`, literal glyph, or empty
- Extend `ProcessColorOverrideEntry()` — detect comma, split, parse icon

**Backward compatibility**: `GetTextAttrForFile()` remains, delegating to `GetDisplayStyleForFile().m_wTextAttr`. Existing callers unchanged.

**TCDIR parsing extension** in `ProcessColorOverrideEntry()`:
```
Before: entry → ParseKeyAndValue → ParseColorValue → dispatch
After:  entry → ParseKeyAndValue → SplitOnComma → ParseColorValue + ParseIconValue → dispatch
```

The comma split is performed on the value part only (after `=`). If no comma, icon parsing is skipped entirely — zero behavior change for existing entries.

### 4. CommandLine.h/cpp

Add `/Icons` and `/Icons-` as a long switch:

```cpp
// In CommandLine.h:
optional<bool> m_fIcons;      // /Icons = true, /Icons- = false, not present = nullopt

// In CommandLine.cpp, s_krgLongSwitches[]:
{ L"icons",  &CCommandLine::m_fIcons },
```

The existing long switch infrastructure handles the `-` suffix for negation automatically.

### 5. pch.h

Add the gdi32 library link:
```cpp
#pragma comment (lib, "gdi32.lib")
```

Place alongside the existing `#pragma comment (lib, "cldapi.lib")`.

---

## Phase 2: Display Integration

### 6. ResultsDisplayerNormal.cpp

In `DisplayFileResults()`, after the owner column and before filename output:

```cpp
// Existing: owner column
// NEW: icon column
if (fIconsActive)
{
    SFileDisplayStyle style = m_configPtr->GetDisplayStyleForFile (fileInfo);
    if (style.m_iconCodePoint != 0 && !style.m_fIconSuppressed)
    {
        wchar_t szIcon[3] = {};
        auto pair = CodePointToWideChars (style.m_iconCodePoint);
        szIcon[0] = pair.chars[0];
        if (pair.count > 1)
            szIcon[1] = pair.chars[1];

        m_consolePtr->Printf (style.m_wTextAttr, L"%s ", szIcon);
    }
}
// Existing: filename output
```

Note: `GetDisplayStyleForFile()` resolves both color and icon. The `m_wTextAttr` from the style is used for both the icon and the filename, ensuring they always match.

### 7. ResultsDisplayerWide.cpp

Three changes:
1. **Icon before filename**: Same pattern as normal mode
2. **Bracket suppression**: When `fIconsActive`, directory names shown without `[brackets]`
3. **Cloud status per entry** (FR-033): Add cloud status symbol before icon in wide mode

In `GetWideFormattedName()` or `DisplayFile()`:
```cpp
if (fIconsActive && (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
{
    // No brackets — folder icon provides distinction
    return wfd.cFileName;
}
else if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
{
    // Brackets as before
    return L"[" + name + L"]";
}
```

Column width calculation must account for icon width (+2 chars: icon + space).

### 8. ResultsDisplayerBare.cpp

Simple: prepend `icon + space` before each filename when icons active. Same `GetDisplayStyleForFile()` + `CodePointToWideChars()` + `Printf()` pattern.

### 9. TCDir.cpp — Detection Orchestration

In the main flow, after command-line parsing and config initialization:

```cpp
// Resolve icon activation state
bool fIconsActive = false;

if (commandLine.m_fIcons.has_value())
{
    fIconsActive = commandLine.m_fIcons.value();   // CLI wins
}
else if (config.m_fIcons.has_value())
{
    fIconsActive = config.m_fIcons.value();        // Env var next
}
else
{
    // Auto-detect
    CNerdFontDetector detector;
    EDetectionResult result = EDetectionResult::Inconclusive;
    HRESULT hr = detector.Detect (hStdOut, config.m_environmentProviderDefault, &result);

    if (SUCCEEDED(hr) && result == EDetectionResult::Detected)
        fIconsActive = true;
    // else: NotDetected or Inconclusive → OFF
}

// Pass fIconsActive to displayer construction
```

---

## Phase 3: Diagnostics & Polish

### 10. Usage.cpp

Extend three existing display functions:

- **`DisplayUsage()`** (`/?`): Add `/Icons` and `/Icons-` to the switch list with descriptions
- **`DisplayEnvVarHelp()`** (`/env`): Document `Icons`/`Icons-` switch, `[color][,icon]` comma syntax, `U+XXXX` format, `dir:` prefix, and literal glyph syntax
- **`DisplayCurrentConfiguration()`** (`/config`): Add icon status section showing:
  - Detection result and reason (e.g., "NF detected via font enumeration", "Icons forced via /Icons")
  - Resolved icon state (ON/OFF)
  - Icon mapping table with source indicators (Built-in / TCDIR)

### 11. Version.h

```cpp
#define VERSION_MAJOR 5    // Was 4 — major version bump for icons feature
```

---

## Phase 4: Tests

### Testing Strategy

| Component | Test File | Mock Dependencies | Key Scenarios |
|-----------|-----------|-------------------|---------------|
| IconMapping | IconMappingTests.cpp | None | Surrogate encoding, table completeness, lookup |
| NerdFontDetector | NerdFontDetectorTests.cpp | CTestEnvironmentProvider + NerdFontDetectorProbe (derivation) | WezTerm detection, ConPTY detection, detection chain |
| Config icon parsing | ConfigTests.cpp | CTestEnvironmentProvider | Comma syntax, U+XXXX, literal glyph, suppression, duplicates |
| Config precedence | ConfigTests.cpp | CTestEnvironmentProvider | Color+icon unified walk, attribute override, fallthrough |
| CommandLine | CommandLineTests.cpp | None | /Icons, /Icons-, precedence over env var |
| Normal displayer | ResultsDisplayerTests.cpp | FileSystemMock | Icon in output, correct color, spacing |
| Wide displayer | ResultsDisplayerTests.cpp | FileSystemMock | Brackets, cloud status, column width |
| Bare displayer | ResultsDisplayerTests.cpp | FileSystemMock | Icon + filename only |
| End-to-end | DirectoryListerScenarioTests.cpp | FileSystemMock | Full listing with mixed file types |

### Mock Pattern

The existing `ConfigProbe` pattern (test struct deriving from `CConfig`, exposing protected methods) is reused for icon tests:

```cpp
struct IconConfigProbe : public CConfig
{
    CTestEnvironmentProvider m_testEnvProvider;

    IconConfigProbe ()
    {
        SetEnvironmentProvider (&m_testEnvProvider);
    }

    void SetEnvVar (const wstring & value)
    {
        m_testEnvProvider.SetVariable (TCDIR_ENV_VAR_NAME, value);
    }

    using CConfig::GetDisplayStyleForFile;
    using CConfig::ParseIconValue;
};
```

### Key Test Scenarios (minimum coverage)

**IconMappingTests.cpp:**
1. `CodePointToWideChars` — BMP produces count=1, supplementary produces count=2, surrogates rejected
2. `CodePointToWideChars` — verify exact surrogate values for all Material Design code points in spec
3. Default extension table — every extension in `g_rgDefaultExtensionIcons` has non-zero code point
4. Default well-known dir table — every entry has non-zero code point, no duplicates
5. Table completeness — every extension in `s_rgTextAttrs[]` (color table) has a matching icon entry

**NerdFontDetectorTests.cpp:**
6. WezTerm detected → `Detected` (TERM_PROGRAM=WezTerm)
7. VS Code detected but no NF → delegates to font enumeration (overridden via NerdFontDetectorProbe)
8. No env vars → classic conhost path (glyph probe overridden via NerdFontDetectorProbe)
9. Multiple ConPTY vars set → earliest match wins, no crash
10. Empty env vars → each variable individually empty/missing combinations

**ConfigTests.cpp (icon extensions):**
11. `TCDIR=.cpp=Green,U+F0672` → color=Green, icon=MdLanguageCpp
12. `TCDIR=.cpp=,U+F0672` → default color, icon=MdLanguageCpp
13. `TCDIR=.cpp=Green` → color=Green, icon=default built-in (backward compat)
14. `TCDIR=.obj=,` → icon suppressed (m_fIconSuppressed=true)
15. `TCDIR=dir:.git=,U+F1D3` → well-known dir icon override
16. `TCDIR=attr:H=DarkGrey,U+F21B` → attribute icon override
17. `TCDIR=.cpp=Green,U+F0672;.cpp=Red` → first wins, second flagged as error
18. `TCDIR=Icons;Icons-` → first wins (Icons=ON), second flagged
19. `TCDIR=.cpp=Green,U+ZZZZ` → invalid hex → ErrorInfo generated
20. `TCDIR=.cpp=Green,U+D800` → surrogate code point → ErrorInfo
21. `TCDIR=Icons` → config.m_fIcons = true
22. `TCDIR=Icons-` → config.m_fIcons = false
23. Precedence: hidden .cpp file → DarkGrey color (attribute) + C++ icon (extension fallthrough)
24. Precedence: hidden .git dir → DarkGrey color + Git icon (well-known dir)
25. Precedence: user sets `attr:H=DarkGrey,U+F21B` → ghost icon locks at attribute level
26. Precedence: plain .cpp file → extension color + extension icon
27. Precedence: unknown extension → default file icon

**CommandLineTests.cpp (icon extensions):**
28. `/Icons` → m_fIcons = true
29. `/Icons-` → m_fIcons = false
30. `--Icons` → m_fIcons = true (long switch)
31. No icons flag → m_fIcons = nullopt
32. `/Icons` with `TCDIR=Icons-` → CLI wins (verify via integration with config)

**ResultsDisplayerTests.cpp (icon extensions):**
33. Normal mode: icon glyph + space before filename when icons active
34. Normal mode: no icon when icons inactive (backward compat)
35. Normal mode: icon color matches filename color
36. Wide mode: no brackets on directory when icons active
37. Wide mode: brackets on directory when icons inactive
38. Wide mode: cloud status shows NF glyph when icons active
39. Wide mode: cloud status shows Unicode circles when icons inactive
40. Bare mode: icon + space + filename when icons active
41. Bare mode: filename only when icons inactive
42. Column alignment correct with icons (normal and wide modes)

---

## Build & Verify

After each phase, run the build + test task:

```
VS Code Task: Build + Test Debug (current arch)
```

Or for both architectures:

```
VS Code Task: Build Debug x64
VS Code Task: Build Debug ARM64
```

Use `get_errors` to verify no compiler warnings or errors after each file change.

---

## Files Checklist

| File | Action | Phase |
|------|--------|-------|
| `TCDirCore/IconMapping.h` | NEW | 1 |
| `TCDirCore/IconMapping.cpp` | NEW | 1 |
| `TCDirCore/NerdFontDetector.h` | NEW | 1 |
| `TCDirCore/NerdFontDetector.cpp` | NEW | 1 |
| `TCDirCore/Config.h` | MODIFY | 1 |
| `TCDirCore/Config.cpp` | MODIFY | 1 |
| `TCDirCore/CommandLine.h` | MODIFY | 1 |
| `TCDirCore/CommandLine.cpp` | MODIFY | 1 |
| `TCDirCore/pch.h` | MODIFY | 1 |
| `TCDirCore/ResultsDisplayerNormal.cpp` | MODIFY | 2 |
| `TCDirCore/ResultsDisplayerWide.cpp` | MODIFY | 2 |
| `TCDirCore/ResultsDisplayerBare.cpp` | MODIFY | 2 |
| `TCDirCore/ResultsDisplayerWithHeaderAndFooter.h` | MODIFY | 2 |
| `TCDirCore/TCDir.cpp` | MODIFY | 2 |
| `TCDirCore/Usage.cpp` | MODIFY | 3 |
| `TCDirCore/Usage.h` | MODIFY | 3 |
| `TCDirCore/Version.h` | MODIFY | 3 |
| `UnitTest/IconMappingTests.cpp` | NEW | 4 |
| `UnitTest/NerdFontDetectorTests.cpp` | NEW | 4 |

| `UnitTest/ConfigTests.cpp` | MODIFY | 4 |
| `UnitTest/CommandLineTests.cpp` | MODIFY | 4 |
| `UnitTest/ResultsDisplayerTests.cpp` | MODIFY | 4 |
| `UnitTest/DirectoryListerScenarioTests.cpp` | MODIFY | 4 |
