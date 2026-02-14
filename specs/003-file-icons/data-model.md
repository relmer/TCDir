# Data Model: Nerd Font File & Folder Icons

**Feature**: 003-file-icons | **Date**: 2026-02-14

---

## Entity Overview

```
CConfig (extended)
├── SFileDisplayStyle          # Resolved color + icon for a file entry
├── SIconMappingEntry          # Static default table entry (extension → code point)
├── m_mapExtensionToIcon       # Runtime extension → icon overrides
├── m_mapWellKnownDirToIcon    # Runtime well-known dir → icon overrides
├── m_mapFileAttributeToIcon   # Runtime attribute → icon overrides
├── m_iconDirectoryDefault     # Default directory icon code point
├── m_iconFileDefault          # Default file icon code point
├── m_iconSymlink              # Symlink icon code point
├── m_iconJunction             # Junction point icon code point
└── m_fIcons                   # optional<bool> — icons switch from TCDIR env var

CNerdFontDetector (new)
├── EIconActivation            # Tri-state: Auto / ForceOn / ForceOff
├── EDetectionResult           # Detected / NotDetected / Inconclusive
└── Protected virtual methods  # GDI calls overridable in tests via derivation

WideCharPair (new utility)
└── CodePointToWideChars()     # constexpr char32_t → wchar_t[2] converter
```

---

## New Structs

### WideCharPair

```cpp
// Location: IconMapping.h

struct WideCharPair
{
    wchar_t  chars[2];   // UTF-16 representation (1 or 2 wchar_t)
    unsigned count;      // 1 for BMP, 2 for supplementary, 0 for invalid
};

constexpr WideCharPair CodePointToWideChars (char32_t cp);
```

Purpose: Convert Unicode code points (stored as `char32_t`) to UTF-16 for `CConsole::Printf`. Constexpr enables compile-time verification via `static_assert`.

### SIconMappingEntry

```cpp
// Location: IconMapping.h

struct SIconMappingEntry
{
    LPCWSTR   m_pszKey;        // Extension (L".cpp"), dir name (L".git"), etc.
    char32_t  m_codePoint;     // Nerd Font code point (e.g., 0xE61D)
};
```

Purpose: Static constexpr default icon mapping table entry. Parallels `CConfig::STextAttr` for colors.

### NfIcon Namespace

```cpp
// Location: IconMapping.h

namespace NfIcon
{
    // --- Custom (nf-custom-*) ---
    constexpr char32_t CustomC                = 0xE61E;
    constexpr char32_t CustomCpp              = 0xE61D;
    constexpr char32_t CustomAsm              = 0xE6AB;
    constexpr char32_t CustomFolder           = 0xE5FF;
    constexpr char32_t CustomFolderConfig     = 0xE5FC;
    constexpr char32_t CustomFolderGit        = 0xE5FB;
    constexpr char32_t CustomFolderGithub     = 0xE5FD;
    constexpr char32_t CustomFolderNpm        = 0xE5FA;
    constexpr char32_t CustomMsdos            = 0xE629;

    // --- Seti (nf-seti-*) ---
    constexpr char32_t SetiConfig             = 0xE615;
    constexpr char32_t SetiCSharp             = 0xE648;
    constexpr char32_t SetiCss                = 0xE614;
    constexpr char32_t SetiHtml               = 0xE60E;
    constexpr char32_t SetiImage              = 0xE60D;
    constexpr char32_t SetiJava               = 0xE66D;
    constexpr char32_t SetiJavascript         = 0xE60C;
    constexpr char32_t SetiJson               = 0xE60B;
    constexpr char32_t SetiMarkdown           = 0xE609;
    constexpr char32_t SetiNpm                = 0xE616;
    constexpr char32_t SetiPowershell         = 0xE683;
    constexpr char32_t SetiPython             = 0xE606;
    constexpr char32_t SetiShell              = 0xE691;
    constexpr char32_t SetiTypescript         = 0xE628;
    constexpr char32_t SetiWord               = 0xE6A5;
    constexpr char32_t SetiXls                = 0xE6A6;
    constexpr char32_t SetiXml                = 0xE619;
    constexpr char32_t SetiYml                = 0xE6A8;

    // --- Dev (nf-dev-*) ---
    constexpr char32_t DevLess                = 0xE758;
    constexpr char32_t DevReact               = 0xE7BA;
    constexpr char32_t DevSass                = 0xE74B;
    constexpr char32_t DevVisualStudio        = 0xE70C;

    // --- Octicons (nf-oct-*) ---
    constexpr char32_t OctFileBinary          = 0xF471;
    constexpr char32_t OctFileZip             = 0xF410;
    constexpr char32_t OctRepo                = 0xF401;
    constexpr char32_t OctTerminal            = 0xF489;

    // --- Font Awesome (nf-fa-*) ---
    constexpr char32_t FaArchive              = 0xF187;
    constexpr char32_t FaEnvelope             = 0xF0E0;
    constexpr char32_t FaExternalLink         = 0xF08E;
    constexpr char32_t FaFile                 = 0xF15B;
    constexpr char32_t FaList                 = 0xF03A;

    // --- Material Design (nf-md-*) — surrogate pair range ---
    constexpr char32_t MdApplication          = 0xF08C6;
    constexpr char32_t MdCloudCheck           = 0xF0160;
    constexpr char32_t MdCloudOutline         = 0xF0163;
    constexpr char32_t MdFileDocument         = 0xF0219;
    constexpr char32_t MdFilePowerpoint       = 0xF0227;
    constexpr char32_t MdPin                  = 0xF0403;
    constexpr char32_t MdTestTube             = 0xF0668;

    // --- Codicons (nf-cod-*) ---
    constexpr char32_t CodFileSymlinkDir      = 0xEAED;
    constexpr char32_t CodFolderLibrary       = 0xEBDF;
    constexpr char32_t CodOutput              = 0xEB9D;
}
```

Purpose: Single source of truth for all Nerd Font code points. Eliminates magic hex literals — every usage site references a named constant. Grouped by Nerd Font prefix, alphabetical within each group.

### SFileDisplayStyle

```cpp
// Location: Config.h

struct SFileDisplayStyle
{
    WORD      m_wTextAttr;       // Resolved color attribute (Windows console WORD)
    char32_t  m_iconCodePoint;   // Resolved icon code point (0 = no icon configured)
    bool      m_fIconSuppressed; // true if icon explicitly set to empty (user typed ",")
};
```

Purpose: Return type of the unified precedence resolver. Both color and icon resolved in a single walk. `m_fIconSuppressed` distinguishes "no icon configured at any level" (show nothing) from "icon explicitly removed" (also show nothing, but don't fall through).

### EIconActivation

```cpp
// Location: NerdFontDetector.h

enum class EIconActivation
{
    Auto,       // Determined by auto-detection
    ForceOn,    // /Icons CLI flag or TCDIR=Icons
    ForceOff    // /Icons- CLI flag or TCDIR=Icons-
};
```

### EDetectionResult

```cpp
// Location: NerdFontDetector.h

enum class EDetectionResult
{
    Detected,        // Nerd Font confirmed (canary hit or WezTerm)
    NotDetected,     // No Nerd Font found
    Inconclusive     // Detection failed — default to OFF
};
```

### CNerdFontDetector

```cpp
// Location: NerdFontDetector.h

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

Purpose: Font detection with GDI-dependent methods as `protected virtual` so tests can derive and override them (same pattern as `ConfigProbe : public CConfig`). Env var logic is testable via the existing `IEnvironmentProvider` / `CTestEnvironmentProvider`. Displayers receive a plain `bool fIconsActive` — no interface needed.

---

## Extended Structures (CConfig)

### New Members on CConfig

```cpp
// Location: Config.h — added to CConfig class

// Icon mapping tables (parallel to color tables)
unordered_map<wstring, char32_t>   m_mapExtensionToIcon;          // ".cpp" → NfIcon::CustomCpp
unordered_map<wstring, char32_t>   m_mapWellKnownDirToIcon;       // ".git" → NfIcon::CustomFolderGit
unordered_map<DWORD, char32_t>     m_mapFileAttributeToIcon;      // FILE_ATTRIBUTE_HIDDEN → 0 (none by default)

// Icon source tracking (parallel to color source tracking)
unordered_map<wstring, EAttributeSource>  m_mapExtensionIconSources;
unordered_map<wstring, EAttributeSource>  m_mapWellKnownDirIconSources;

// Type fallback icons
char32_t  m_iconDirectoryDefault    = NfIcon::CustomFolder;
char32_t  m_iconFileDefault         = NfIcon::FaFile;
char32_t  m_iconSymlink             = NfIcon::CodFileSymlinkDir;
char32_t  m_iconJunction            = NfIcon::FaExternalLink;

// Cloud status NF glyphs (used when icons active)
char32_t  m_iconCloudOnly           = NfIcon::MdCloudOutline;
char32_t  m_iconLocallyAvailable    = NfIcon::MdCloudCheck;
char32_t  m_iconAlwaysLocal         = NfIcon::MdPin;

// Icon switch from env var (parallel to m_fWideListing etc.)
optional<bool>  m_fIcons;
```

### New Methods on CConfig

```cpp
// Location: Config.h — added to CConfig class

public:
    SFileDisplayStyle  GetDisplayStyleForFile    (const WIN32_FIND_DATA & wfd);
    char32_t           GetCloudStatusIcon        (DWORD dwCloudStatus);

protected:
    void     InitializeExtensionToIconMap     (void);
    void     InitializeWellKnownDirToIconMap  (void);
    void     ProcessFileExtensionIconOverride (wstring_view extension, char32_t iconCodePoint, bool fSuppressed);
    void     ProcessWellKnownDirIconOverride  (wstring_view dirName, char32_t iconCodePoint, bool fSuppressed);
    void     ProcessFileAttributeIconOverride (DWORD dwAttribute, char32_t iconCodePoint);
    HRESULT  ParseIconValue                   (wstring_view iconSpec, char32_t & codePoint, bool & fSuppressed);
```

### Modified Methods on CConfig

```
ProcessColorOverrideEntry() — extended to detect comma, split, and parse icon part
GetTextAttrForFile()        — now delegates to GetDisplayStyleForFile().m_wTextAttr for backward compat
IsSwitchName()              — handles "Icons" and "Icons-"
ProcessSwitchOverride()     — handles Icons/Icons- switch
```

---

## Static Default Tables

### Extension Icon Table

```cpp
// Location: IconMapping.cpp

static constexpr SIconMappingEntry s_rgDefaultExtensionIcons[] =
{
    // C/C++
    { L".c",     NfIcon::CustomC },
    { L".cpp",   NfIcon::CustomCpp },
    { L".cxx",   NfIcon::CustomCpp },
    { L".h",     NfIcon::CustomC },
    { L".hpp",   NfIcon::CustomCpp },
    { L".hxx",   NfIcon::CustomCpp },
    { L".asm",   NfIcon::CustomAsm },
    { L".cod",   NfIcon::CustomAsm },
    { L".i",     NfIcon::CustomAsm },

    // C# / .NET
    { L".cs",    NfIcon::SetiCSharp },
    { L".csx",   NfIcon::SetiCSharp },
    { L".resx",  NfIcon::DevVisualStudio },
    { L".rcml",  NfIcon::DevVisualStudio },

    // JavaScript / TypeScript
    { L".js",    NfIcon::SetiJavascript },
    { L".jsx",   NfIcon::DevReact },
    { L".ts",    NfIcon::SetiTypescript },
    { L".tsx",   NfIcon::DevReact },

    // Web
    { L".html",  NfIcon::SetiHtml },
    { L".htm",   NfIcon::SetiHtml },
    { L".css",   NfIcon::SetiCss },
    { L".scss",  NfIcon::DevSass },
    { L".less",  NfIcon::DevLess },

    // Python / Java
    { L".py",    NfIcon::SetiPython },
    { L".pyw",   NfIcon::SetiPython },
    { L".jar",   NfIcon::SetiJava },
    { L".java",  NfIcon::SetiJava },
    { L".class", NfIcon::SetiJava },

    // Data formats
    { L".xml",   NfIcon::SetiXml },
    { L".json",  NfIcon::SetiJson },
    { L".yml",   NfIcon::SetiYml },
    { L".yaml",  NfIcon::SetiYml },

    // Build artifacts
    { L".obj",   NfIcon::OctFileBinary },
    { L".lib",   NfIcon::OctFileBinary },
    { L".res",   NfIcon::OctFileBinary },
    { L".pch",   NfIcon::OctFileBinary },

    // Logs
    { L".wrn",   NfIcon::FaList },
    { L".err",   NfIcon::FaList },
    { L".log",   NfIcon::FaList },

    // Shell
    { L".bash",  NfIcon::SetiShell },
    { L".sh",    NfIcon::SetiShell },
    { L".bat",   NfIcon::CustomMsdos },
    { L".cmd",   NfIcon::CustomMsdos },
    { L".ps1",   NfIcon::SetiPowershell },
    { L".psd1",  NfIcon::SetiPowershell },
    { L".psm1",  NfIcon::SetiPowershell },

    // Executables
    { L".exe",   NfIcon::MdApplication },
    { L".sys",   NfIcon::MdApplication },
    { L".dll",   NfIcon::FaArchive },

    // Visual Studio
    { L".sln",     NfIcon::DevVisualStudio },
    { L".vcproj",  NfIcon::DevVisualStudio },
    { L".vcxproj", NfIcon::DevVisualStudio },
    { L".csproj",  NfIcon::DevVisualStudio },
    { L".csxproj", NfIcon::DevVisualStudio },
    { L".user",    NfIcon::DevVisualStudio },
    { L".ncb",     NfIcon::DevVisualStudio },

    // Documents
    { L".doc",   NfIcon::SetiWord },
    { L".docx",  NfIcon::SetiWord },
    { L".ppt",   NfIcon::MdFilePowerpoint },
    { L".pptx",  NfIcon::MdFilePowerpoint },
    { L".xls",   NfIcon::SetiXls },
    { L".xlsx",  NfIcon::SetiXls },
    { L".md",    NfIcon::SetiMarkdown },

    // Text
    { L".txt",   NfIcon::MdFileDocument },
    { L".text",  NfIcon::MdFileDocument },
    { L".!!!",   NfIcon::MdFileDocument },
    { L".1st",   NfIcon::MdFileDocument },
    { L".me",    NfIcon::MdFileDocument },
    { L".now",   NfIcon::MdFileDocument },

    // Email
    { L".eml",   NfIcon::FaEnvelope },

    // Archives
    { L".7z",    NfIcon::OctFileZip },
    { L".arj",   NfIcon::OctFileZip },
    { L".gz",    NfIcon::OctFileZip },
    { L".rar",   NfIcon::OctFileZip },
    { L".tar",   NfIcon::OctFileZip },
    { L".zip",   NfIcon::OctFileZip },

    // Resource
    { L".rc",    NfIcon::SetiConfig },
};
```

### Well-Known Directory Icon Table

```cpp
// Location: IconMapping.cpp

static constexpr SIconMappingEntry s_rgDefaultWellKnownDirIcons[] =
{
    { L".git",         NfIcon::CustomFolderGit },
    { L".github",      NfIcon::CustomFolderGithub },
    { L".vscode",      NfIcon::CustomFolderConfig },
    { L".config",      NfIcon::SetiConfig },
    { L"node_modules", NfIcon::CustomFolderNpm },
    { L"src",          NfIcon::OctTerminal },
    { L"source",       NfIcon::OctTerminal },
    { L"docs",         NfIcon::OctRepo },
    { L"doc",          NfIcon::OctRepo },
    { L"documents",    NfIcon::OctRepo },
    { L"bin",          NfIcon::OctFileBinary },
    { L"build",        NfIcon::CodOutput },
    { L"dist",         NfIcon::CodOutput },
    { L"out",          NfIcon::CodOutput },
    { L"output",       NfIcon::CodOutput },
    { L"test",         NfIcon::MdTestTube },
    { L"tests",        NfIcon::MdTestTube },
    { L"__tests__",    NfIcon::MdTestTube },
    { L"spec",         NfIcon::MdTestTube },
    { L"specs",        NfIcon::MdTestTube },
    { L"lib",          NfIcon::CodFolderLibrary },
    { L"libs",         NfIcon::CodFolderLibrary },
    { L"scripts",      NfIcon::SetiShell },
    { L"images",       NfIcon::SetiImage },
    { L"img",          NfIcon::SetiImage },
    { L"assets",       NfIcon::SetiImage },
    { L"packages",     NfIcon::SetiNpm },
};
```

### Attribute Precedence Array (New Order)

```cpp
// Location: IconMapping.h
// Note: This is for color+icon precedence ONLY, NOT for display column order.
// The display column order (k_rgFileAttributeMap in FileAttributeMap.h) retains
// the existing RHSATECP0 order.

static constexpr SFileAttributeMap k_rgAttributePrecedenceOrder[] =
{
    { FILE_ATTRIBUTE_REPARSE_POINT, L'P' },   // Priority 1 (highest)
    { FILE_ATTRIBUTE_SYSTEM,        L'S' },   // Priority 2
    { FILE_ATTRIBUTE_HIDDEN,        L'H' },   // Priority 3
    { FILE_ATTRIBUTE_ENCRYPTED,     L'E' },   // Priority 4
    { FILE_ATTRIBUTE_READONLY,      L'R' },   // Priority 5
    { FILE_ATTRIBUTE_COMPRESSED,    L'C' },   // Priority 6
    { FILE_ATTRIBUTE_SPARSE_FILE,   L'0' },   // Priority 7
    { FILE_ATTRIBUTE_TEMPORARY,     L'T' },   // Priority 8
    { FILE_ATTRIBUTE_ARCHIVE,       L'A' },   // Priority 9 (lowest)
};
```

---

## Relationships

```
CCommandLine  ──parses──>  m_fIcons (optional<bool>)
                            m_fIconsFromCLI (bool, true only if /Icons or /Icons- on CLI)

CConfig       ──reads───>  TCDIR env var ──extends──> icon override maps
              ──owns───>   extension icon map, well-known dir map, attribute icon map
              ──exposes──> GetDisplayStyleForFile() → SFileDisplayStyle

CNerdFontDetector ──uses──> IEnvironmentProvider (env var detection)
                  ──uses──> Win32 GDI APIs (protected virtual — overridable in tests)
                  ──returns──> EDetectionResult

Displayers    ──receive──> bool fIconsActive (plain boolean, no interface)
              ──call───>   CConfig::GetDisplayStyleForFile()
              ──emit───>   icon glyph + space via CConsole::Printf
```

---

## State Flow

```
1. CCommandLine::Parse()
   └── Sets m_fIcons = true/false if /Icons or /Icons- present

2. CConfig::Initialize()
   ├── InitializeExtensionToTextAttrMap()   (existing — colors)
   ├── InitializeExtensionToIconMap()       (new — icons)
   ├── InitializeWellKnownDirToIconMap()    (new — icons)
   ├── InitializeFileAttributeToTextAttrMap() (existing — colors)
   └── ApplyUserColorOverrides()            (extended — parses both color AND icon from TCDIR)

3. TCDir.cpp main flow
   ├── CLI check: if m_fIcons has value → use it (ForceOn/ForceOff)
   ├── Env var check: if config.m_fIcons has value → use it
   └── Auto-detect: CNerdFontDetector::Detect() → EDetectionResult
   └── Set g_fIconsActive = resolved boolean

4. Displayers (per-file)
   ├── style = config.GetDisplayStyleForFile(wfd)
   ├── if g_fIconsActive && style.m_iconCodePoint != 0 && !style.m_fIconSuppressed:
   │   ├── pair = CodePointToWideChars(style.m_iconCodePoint)
   │   └── Printf(style.m_wTextAttr, L"%s ", szIcon)
   └── Printf(style.m_wTextAttr, L"%s", filename)
```

---

## Validation Rules

| Entity | Rule | Error Handling |
|--------|------|---------------|
| Icon code point (U+XXXX) | 4–6 hex digits after `U+`, range 0x0001–0x10FFFF, not in D800–DFFF | ErrorInfo with underline on invalid hex |
| Icon literal glyph | Single BMP char OR valid surrogate pair | ErrorInfo if invalid |
| Icon comma syntax | At most one comma per entry | ErrorInfo if multiple commas |
| Duplicate key | First-write-wins, subsequent flagged | ErrorInfo on duplicate, value preserved from first |
| Icons / Icons- switch | Mutual exclusive (first wins) | ErrorInfo on conflicting switch |

---

## No External Contracts

This feature has no external APIs, REST endpoints, or IPC interfaces. All contracts are internal C++ class interfaces:
- `IEnvironmentProvider` — existing injection interface for env var access
- `IResultsDisplayer` — existing interface for display output

No new interfaces are introduced. `CNerdFontDetector` uses protected virtual methods for testability (derivation pattern), consistent with `ConfigProbe : public CConfig` elsewhere in the test suite.
