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
    char32_t  m_codePoint;     // Nerd Font code point (e.g., 0xE749 = DevCss3)
};
```

Purpose: Static constexpr default icon mapping table entry. Parallels `CConfig::STextAttr` for colors.

### NfIcon Namespace

```cpp
// Location: IconMapping.h

namespace NfIcon
{
    // --- Custom (nf-custom-*) ---
    constexpr char32_t CustomAsm              = 0xE6AB;
    constexpr char32_t CustomElixir           = 0xE62D;
    constexpr char32_t CustomElm              = 0xE62C;
    constexpr char32_t CustomFolder           = 0xE5FF;
    constexpr char32_t CustomFolderConfig     = 0xE5FC;
    constexpr char32_t CustomKotlin           = 0xE634;
    constexpr char32_t CustomMsdos            = 0xE629;

    // --- Seti (nf-seti-*) ---
    constexpr char32_t SetiBicep              = 0xE63B;
    constexpr char32_t SetiConfig             = 0xE615;
    constexpr char32_t SetiDb                 = 0xE64D;
    constexpr char32_t SetiGit                = 0xE65D;   // DEVIATION from TI
    constexpr char32_t SetiGithub             = 0xE65B;   // DEVIATION from TI
    constexpr char32_t SetiHtml               = 0xE60E;
    constexpr char32_t SetiJson               = 0xE60B;
    constexpr char32_t SetiJulia              = 0xE624;
    constexpr char32_t SetiLua                = 0xE620;
    constexpr char32_t SetiMakefile           = 0xE673;
    constexpr char32_t SetiNpm                = 0xE616;   // DEVIATION from TI
    constexpr char32_t SetiProject            = 0xE601;
    constexpr char32_t SetiPython             = 0xE606;   // DEVIATION from TI
    constexpr char32_t SetiShell              = 0xE691;
    constexpr char32_t SetiSvelte             = 0xE697;
    constexpr char32_t SetiSwift              = 0xE699;
    constexpr char32_t SetiTerraform          = 0xE69A;
    constexpr char32_t SetiTypescript         = 0xE628;

    // --- Dev (nf-dev-*) ---
    constexpr char32_t DevAws                 = 0xE7AD;
    constexpr char32_t DevClojure             = 0xE768;
    constexpr char32_t DevCss3                = 0xE749;
    constexpr char32_t DevDart                = 0xE798;
    constexpr char32_t DevDatabase            = 0xE706;
    constexpr char32_t DevDocker              = 0xE7B0;
    constexpr char32_t DevErlang              = 0xE7B1;
    constexpr char32_t DevFsharp              = 0xE7A7;
    constexpr char32_t DevGo                  = 0xE724;
    constexpr char32_t DevGroovy              = 0xE775;
    constexpr char32_t DevHaskell             = 0xE777;
    constexpr char32_t DevJavascriptAlt       = 0xE74E;
    constexpr char32_t DevLess                = 0xE758;
    constexpr char32_t DevMarkdown            = 0xE73E;
    constexpr char32_t DevPerl                = 0xE769;
    constexpr char32_t DevPhp                 = 0xE73D;
    constexpr char32_t DevReact               = 0xE7BA;
    constexpr char32_t DevRust                = 0xE7A8;
    constexpr char32_t DevSass                = 0xE74B;
    constexpr char32_t DevScala               = 0xE737;
    constexpr char32_t DevVisualStudio        = 0xE70C;
    constexpr char32_t DevVscode              = 0xE8DA;   // DEVIATION from TI

    // --- Font Awesome Extension (nf-fae-*) ---
    constexpr char32_t FaeJava                = 0xE256;

    // --- Octicons (nf-oct-*) ---
    constexpr char32_t OctFileBinary          = 0xF471;
    constexpr char32_t OctFileMedia           = 0xF40F;
    constexpr char32_t OctFileZip             = 0xF410;
    constexpr char32_t OctRepo                = 0xF401;
    constexpr char32_t OctRuby                = 0xF43B;
    constexpr char32_t OctTerminal            = 0xF489;

    // --- Font Awesome (nf-fa-*) ---
    constexpr char32_t FaArchive              = 0xF187;
    constexpr char32_t FaCertificate          = 0xF0A3;
    constexpr char32_t FaEnvelope             = 0xF0E0;
    constexpr char32_t FaExternalLink         = 0xF08E;
    constexpr char32_t FaFile                 = 0xF15B;
    constexpr char32_t FaFileAudioO           = 0xF1C7;
    constexpr char32_t FaFileImageO           = 0xF1C5;
    constexpr char32_t FaFilePdfO             = 0xF1C1;
    constexpr char32_t FaFileVideoO           = 0xF1C8;
    constexpr char32_t FaFont                 = 0xF031;
    constexpr char32_t FaGear                 = 0xF013;
    constexpr char32_t FaGithubAlt            = 0xF113;
    constexpr char32_t FaKey                  = 0xF084;
    constexpr char32_t FaList                 = 0xF03A;
    constexpr char32_t FaLock                 = 0xF023;
    constexpr char32_t FaUsers                = 0xF0C0;
    constexpr char32_t FaWindows              = 0xF17A;

    // --- Material Design (nf-md-*) — surrogate pair range ---
    constexpr char32_t MdApplication          = 0xF08C6;
    constexpr char32_t MdApps                 = 0xF003B;
    constexpr char32_t MdCached               = 0xF00E8;
    constexpr char32_t MdCloudCheck           = 0xF0160;
    constexpr char32_t MdCloudOutline         = 0xF0163;
    constexpr char32_t MdConsoleLine          = 0xF07B7;
    constexpr char32_t MdContacts             = 0xF06CB;
    constexpr char32_t MdDesktopClassic       = 0xF07C0;
    constexpr char32_t MdFileDocument         = 0xF0219;
    constexpr char32_t MdFileExcel            = 0xF021B;
    constexpr char32_t MdFilePowerpoint       = 0xF0227;
    constexpr char32_t MdFileWord             = 0xF022C;
    constexpr char32_t MdFolderDownload       = 0xF024D;
    constexpr char32_t MdFolderImage          = 0xF024F;
    constexpr char32_t MdFolderStar           = 0xF069D;
    constexpr char32_t MdFormatAlignLeft      = 0xF0262;
    constexpr char32_t MdLanguageC            = 0xF0671;
    constexpr char32_t MdLanguageCpp          = 0xF0672;
    constexpr char32_t MdLanguageCsharp       = 0xF031B;
    constexpr char32_t MdLanguageR            = 0xF07D4;
    constexpr char32_t MdLanguageXaml         = 0xF0673;
    constexpr char32_t MdMicrosoftAzure       = 0xF0805;
    constexpr char32_t MdMicrosoftOnedrive    = 0xF03CA;
    constexpr char32_t MdMovie                = 0xF0381;
    constexpr char32_t MdMusicBoxMultiple     = 0xF0333;
    constexpr char32_t MdNotebook             = 0xF082E;
    constexpr char32_t MdElephant             = 0xF07C6;
    constexpr char32_t MdPackageVariant       = 0xF03D6;
    constexpr char32_t MdPin                  = 0xF0403;
    constexpr char32_t MdShipWheel            = 0xF0833;
    constexpr char32_t MdSvg                  = 0xF0721;
    constexpr char32_t MdTestTube             = 0xF0668;
    constexpr char32_t MdVuejs                = 0xF0844;
    constexpr char32_t MdTimer                = 0xF13AB;
    constexpr char32_t MdXml                  = 0xF05C0;

    // --- Codicons (nf-cod-*) ---
    constexpr char32_t CodFileSymlinkDir      = 0xEAED;
    constexpr char32_t CodFolderLibrary       = 0xEBDF;
    constexpr char32_t CodOutput              = 0xEB9D;
    constexpr char32_t CodPackage             = 0xEB29;
    constexpr char32_t CodPreview             = 0xEB2F;
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
unordered_map<wstring, char32_t>   m_mapExtensionToIcon;          // ".cpp" → NfIcon::MdLanguageCpp
unordered_map<wstring, char32_t>   m_mapWellKnownDirToIcon;       // ".git" → NfIcon::SetiGit
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
// Aligned to Terminal-Icons devblackops default theme (NF v3.4.0).
// See "Deviations from Terminal-Icons" in spec.md for rationale.

const SIconMappingEntry g_rgDefaultExtensionIcons[] =
{
    // C/C++ (Terminal-Icons: nf-md-language_c / nf-md-language_cpp)
    { L".c",       NfIcon::MdLanguageC },
    { L".h",       NfIcon::MdLanguageC },
    { L".cpp",     NfIcon::MdLanguageCpp },
    { L".cxx",     NfIcon::MdLanguageCpp },
    { L".c++",     NfIcon::MdLanguageCpp },
    { L".hpp",     NfIcon::MdLanguageCpp },
    { L".hxx",     NfIcon::MdLanguageCpp },
    { L".asm",     NfIcon::CustomAsm },
    { L".cod",     NfIcon::CustomAsm },
    { L".i",       NfIcon::CustomAsm },

    // C# / .NET (Terminal-Icons: nf-md-language_csharp, nf-md-xml for resx)
    { L".cs",      NfIcon::MdLanguageCsharp },
    { L".csx",     NfIcon::MdLanguageCsharp },
    { L".resx",    NfIcon::MdXml },
    { L".xaml",    NfIcon::MdLanguageXaml },

    // JavaScript / TypeScript (Terminal-Icons: nf-dev-javascript_alt)
    { L".js",      NfIcon::DevJavascriptAlt },
    { L".mjs",     NfIcon::DevJavascriptAlt },
    { L".cjs",     NfIcon::DevJavascriptAlt },
    { L".jsx",     NfIcon::DevReact },
    { L".ts",      NfIcon::SetiTypescript },
    { L".tsx",     NfIcon::DevReact },

    // Web (Terminal-Icons: nf-dev-css3)
    { L".html",    NfIcon::SetiHtml },
    { L".htm",     NfIcon::SetiHtml },
    { L".xhtml",   NfIcon::SetiHtml },
    { L".css",     NfIcon::DevCss3 },
    { L".scss",    NfIcon::DevSass },
    { L".sass",    NfIcon::DevSass },
    { L".less",    NfIcon::DevLess },
    { L".vue",     NfIcon::MdVuejs },
    { L".svelte",  NfIcon::SetiSvelte },

    // Python (DEVIATION: nf-seti-python for legibility)
    { L".py",      NfIcon::SetiPython },
    { L".pyw",     NfIcon::SetiPython },
    { L".ipynb",   NfIcon::MdNotebook },

    // Java (Terminal-Icons: nf-fae-java)
    { L".java",    NfIcon::FaeJava },
    { L".jar",     NfIcon::FaeJava },
    { L".class",   NfIcon::FaeJava },
    { L".gradle",  NfIcon::MdElephant },

    // Rust
    { L".rs",      NfIcon::DevRust },

    // Go (Terminal-Icons: nf-dev-go)
    { L".go",      NfIcon::DevGo },

    // Ruby (Terminal-Icons: nf-oct-ruby)
    { L".rb",      NfIcon::OctRuby },
    { L".erb",     NfIcon::OctRuby },

    // F# (Terminal-Icons: nf-dev-fsharp)
    { L".fs",      NfIcon::DevFsharp },
    { L".fsx",     NfIcon::DevFsharp },
    { L".fsi",     NfIcon::DevFsharp },

    // Lua
    { L".lua",     NfIcon::SetiLua },

    // Perl (Terminal-Icons: nf-dev-perl)
    { L".pl",      NfIcon::DevPerl },
    { L".pm",      NfIcon::DevPerl },

    // PHP
    { L".php",     NfIcon::DevPhp },

    // Haskell (Terminal-Icons: nf-dev-haskell)
    { L".hs",      NfIcon::DevHaskell },

    // Dart (Terminal-Icons: nf-dev-dart)
    { L".dart",    NfIcon::DevDart },

    // Kotlin
    { L".kt",      NfIcon::CustomKotlin },
    { L".kts",     NfIcon::CustomKotlin },

    // Swift (not in TI; keep Seti)
    { L".swift",   NfIcon::SetiSwift },

    // Scala (Terminal-Icons: nf-dev-scala)
    { L".scala",   NfIcon::DevScala },
    { L".sc",      NfIcon::DevScala },
    { L".sbt",     NfIcon::DevScala },

    // Clojure (Terminal-Icons: nf-dev-clojure)
    { L".clj",     NfIcon::DevClojure },
    { L".cljs",    NfIcon::DevClojure },
    { L".cljc",    NfIcon::DevClojure },

    // Elixir / Erlang
    { L".ex",      NfIcon::CustomElixir },
    { L".exs",     NfIcon::CustomElixir },
    { L".erl",     NfIcon::DevErlang },

    // Groovy
    { L".groovy",  NfIcon::DevGroovy },

    // Julia
    { L".jl",      NfIcon::SetiJulia },

    // R
    { L".r",       NfIcon::MdLanguageR },
    { L".rmd",     NfIcon::MdLanguageR },

    // Elm
    { L".elm",     NfIcon::CustomElm },

    // Data formats (Terminal-Icons: nf-md-xml, nf-md-format_align_left)
    { L".xml",     NfIcon::MdXml },
    { L".xsd",     NfIcon::MdXml },
    { L".xsl",     NfIcon::MdXml },
    { L".xslt",    NfIcon::MdXml },
    { L".dtd",     NfIcon::MdXml },
    { L".plist",   NfIcon::MdXml },
    { L".manifest",NfIcon::MdXml },
    { L".json",    NfIcon::SetiJson },
    { L".toml",    NfIcon::FaGear },
    { L".yml",     NfIcon::MdFormatAlignLeft },
    { L".yaml",    NfIcon::MdFormatAlignLeft },

    // Config / Settings (Terminal-Icons: nf-fa-gear)
    { L".ini",     NfIcon::FaGear },
    { L".cfg",     NfIcon::FaGear },
    { L".conf",    NfIcon::FaGear },
    { L".config",  NfIcon::FaGear },
    { L".properties", NfIcon::FaGear },
    { L".settings",NfIcon::FaGear },
    { L".reg",     NfIcon::FaGear },

    // Database / SQL (Terminal-Icons: nf-dev-database, nf-seti-db)
    { L".sql",     NfIcon::DevDatabase },
    { L".sqlite",  NfIcon::DevDatabase },
    { L".mdb",     NfIcon::DevDatabase },
    { L".accdb",   NfIcon::DevDatabase },
    { L".pgsql",   NfIcon::DevDatabase },
    { L".db",      NfIcon::SetiDb },
    { L".csv",     NfIcon::MdFileExcel },
    { L".tsv",     NfIcon::MdFileExcel },

    // Build artifacts
    { L".obj",     NfIcon::OctFileBinary },
    { L".lib",     NfIcon::OctFileBinary },
    { L".res",     NfIcon::OctFileBinary },
    { L".pch",     NfIcon::OctFileBinary },
    { L".pdb",     NfIcon::DevDatabase },

    // Logs
    { L".wrn",     NfIcon::FaList },
    { L".err",     NfIcon::FaList },
    { L".log",     NfIcon::FaList },

    // Shell (Terminal-Icons: nf-oct-terminal for sh)
    { L".bash",    NfIcon::OctTerminal },
    { L".sh",      NfIcon::OctTerminal },
    { L".zsh",     NfIcon::OctTerminal },
    { L".fish",    NfIcon::OctTerminal },
    { L".bat",     NfIcon::CustomMsdos },
    { L".cmd",     NfIcon::CustomMsdos },

    // PowerShell (Terminal-Icons: nf-md-console_line)
    { L".ps1",     NfIcon::MdConsoleLine },
    { L".psd1",    NfIcon::MdConsoleLine },
    { L".psm1",    NfIcon::MdConsoleLine },
    { L".ps1xml",  NfIcon::MdConsoleLine },

    // Executables
    { L".exe",     NfIcon::MdApplication },
    { L".sys",     NfIcon::MdApplication },
    { L".dll",     NfIcon::FaArchive },

    // Installers (Terminal-Icons: nf-md-package_variant)
    { L".msi",     NfIcon::MdPackageVariant },
    { L".msix",    NfIcon::MdPackageVariant },
    { L".deb",     NfIcon::MdPackageVariant },
    { L".rpm",     NfIcon::MdPackageVariant },

    // Visual Studio
    { L".sln",     NfIcon::DevVisualStudio },
    { L".vcproj",  NfIcon::DevVisualStudio },
    { L".vcxproj", NfIcon::DevVisualStudio },
    { L".csproj",  NfIcon::DevVisualStudio },
    { L".csxproj", NfIcon::DevVisualStudio },
    { L".fsproj",  NfIcon::DevFsharp },
    { L".user",    NfIcon::DevVisualStudio },
    { L".ncb",     NfIcon::DevVisualStudio },
    { L".suo",     NfIcon::DevVisualStudio },
    { L".code-workspace", NfIcon::DevVisualStudio },

    // Documents (Terminal-Icons: nf-md-file_word, nf-md-file_excel)
    { L".doc",     NfIcon::MdFileWord },
    { L".docx",    NfIcon::MdFileWord },
    { L".rtf",     NfIcon::MdFileWord },
    { L".ppt",     NfIcon::MdFilePowerpoint },
    { L".pptx",    NfIcon::MdFilePowerpoint },
    { L".xls",     NfIcon::MdFileExcel },
    { L".xlsx",    NfIcon::MdFileExcel },
    { L".pdf",     NfIcon::FaFilePdfO },

    // Markdown (Terminal-Icons: nf-dev-markdown)
    { L".md",      NfIcon::DevMarkdown },
    { L".markdown",NfIcon::DevMarkdown },
    { L".rst",     NfIcon::DevMarkdown },

    // Text
    { L".txt",     NfIcon::MdFileDocument },
    { L".text",    NfIcon::MdFileDocument },
    { L".!!!",     NfIcon::MdFileDocument },
    { L".1st",     NfIcon::MdFileDocument },
    { L".me",      NfIcon::MdFileDocument },
    { L".now",     NfIcon::MdFileDocument },

    // Email
    { L".eml",     NfIcon::FaEnvelope },

    // Images (Terminal-Icons: nf-fa-file_image_o)
    { L".png",     NfIcon::FaFileImageO },
    { L".jpg",     NfIcon::FaFileImageO },
    { L".jpeg",    NfIcon::FaFileImageO },
    { L".gif",     NfIcon::FaFileImageO },
    { L".bmp",     NfIcon::FaFileImageO },
    { L".ico",     NfIcon::FaFileImageO },
    { L".tif",     NfIcon::FaFileImageO },
    { L".tiff",    NfIcon::FaFileImageO },
    { L".webp",    NfIcon::FaFileImageO },
    { L".psd",     NfIcon::FaFileImageO },
    { L".cur",     NfIcon::FaFileImageO },
    { L".raw",     NfIcon::FaFileImageO },
    { L".svg",     NfIcon::MdSvg },

    // Audio (Terminal-Icons: nf-fa-file_audio_o)
    { L".mp3",     NfIcon::FaFileAudioO },
    { L".wav",     NfIcon::FaFileAudioO },
    { L".flac",    NfIcon::FaFileAudioO },
    { L".m4a",     NfIcon::FaFileAudioO },
    { L".wma",     NfIcon::FaFileAudioO },
    { L".aac",     NfIcon::FaFileAudioO },
    { L".ogg",     NfIcon::FaFileAudioO },
    { L".opus",    NfIcon::FaFileAudioO },
    { L".aiff",    NfIcon::FaFileAudioO },

    // Video (Terminal-Icons: nf-fa-file_video_o)
    { L".mp4",     NfIcon::FaFileVideoO },
    { L".avi",     NfIcon::FaFileVideoO },
    { L".mkv",     NfIcon::FaFileVideoO },
    { L".mov",     NfIcon::FaFileVideoO },
    { L".wmv",     NfIcon::FaFileVideoO },
    { L".webm",    NfIcon::FaFileVideoO },
    { L".flv",     NfIcon::FaFileVideoO },
    { L".mpg",     NfIcon::FaFileVideoO },
    { L".mpeg",    NfIcon::FaFileVideoO },

    // Fonts (Terminal-Icons: nf-fa-font)
    { L".ttf",     NfIcon::FaFont },
    { L".otf",     NfIcon::FaFont },
    { L".woff",    NfIcon::FaFont },
    { L".woff2",   NfIcon::FaFont },
    { L".eot",     NfIcon::FaFont },
    { L".ttc",     NfIcon::FaFont },

    // Archives
    { L".7z",      NfIcon::OctFileZip },
    { L".arj",     NfIcon::OctFileZip },
    { L".gz",      NfIcon::OctFileZip },
    { L".rar",     NfIcon::OctFileZip },
    { L".tar",     NfIcon::OctFileZip },
    { L".zip",     NfIcon::OctFileZip },
    { L".xz",      NfIcon::OctFileZip },
    { L".bz2",     NfIcon::OctFileZip },
    { L".tgz",     NfIcon::OctFileZip },
    { L".cab",     NfIcon::OctFileZip },
    { L".zst",     NfIcon::OctFileZip },

    // Certificates / Keys (Terminal-Icons: nf-fa-certificate, nf-fa-key)
    { L".cer",     NfIcon::FaCertificate },
    { L".cert",    NfIcon::FaCertificate },
    { L".crt",     NfIcon::FaCertificate },
    { L".pfx",     NfIcon::FaCertificate },
    { L".pem",     NfIcon::FaKey },
    { L".pub",     NfIcon::FaKey },
    { L".key",     NfIcon::FaKey },
    { L".asc",     NfIcon::FaKey },
    { L".gpg",     NfIcon::FaKey },

    // Docker (Terminal-Icons: nf-dev-docker)
    { L".dockerfile", NfIcon::DevDocker },
    { L".dockerignore", NfIcon::DevDocker },

    // Terraform (using seti-terraform for NF v3 compat)
    { L".tf",      NfIcon::SetiTerraform },
    { L".tfvars",  NfIcon::SetiTerraform },
    { L".bicep",   NfIcon::SetiBicep },

    // Lock files (Terminal-Icons: nf-fa-lock)
    { L".lock",    NfIcon::FaLock },

    // Resource (.rc)
    { L".rc",      NfIcon::SetiConfig },
};
```

### Well-Known Directory Icon Table

```cpp
// Location: IconMapping.cpp
// Aligned to Terminal-Icons devblackops default theme (NF v3.4.0).
// DEVIATION entries noted — see spec.md for rationale.

const SIconMappingEntry g_rgDefaultWellKnownDirIcons[] =
{
    // Version control / IDEs (DEVIATIONS for legibility)
    { L".git",         NfIcon::SetiGit },            // DEVIATION: nf-seti-git (not TI's nf-custom-folder_git)
    { L".github",      NfIcon::SetiGithub },          // DEVIATION: nf-seti-github
    { L".vscode",      NfIcon::DevVscode },            // DEVIATION: nf-dev-vscode
    { L".vscode-insiders", NfIcon::DevVscode },
    { L"node_modules", NfIcon::SetiNpm },              // DEVIATION: nf-seti-npm (not TI's nf-custom-folder_npm)

    // Config / Cloud provider directories (Terminal-Icons)
    { L".config",      NfIcon::SetiConfig },
    { L".cargo",       NfIcon::CustomFolderConfig },
    { L".cache",       NfIcon::MdCached },
    { L".docker",      NfIcon::DevDocker },
    { L".aws",         NfIcon::DevAws },
    { L".azure",       NfIcon::MdMicrosoftAzure },
    { L".kube",        NfIcon::MdShipWheel },

    // Source / Development (Terminal-Icons)
    { L"src",          NfIcon::OctTerminal },
    { L"source",       NfIcon::OctTerminal },
    { L"development",  NfIcon::OctTerminal },
    { L"projects",     NfIcon::SetiProject },

    // Documentation (Terminal-Icons)
    { L"docs",         NfIcon::OctRepo },
    { L"doc",          NfIcon::OctRepo },
    { L"documents",    NfIcon::OctRepo },

    // Build outputs (Terminal-Icons)
    { L"bin",          NfIcon::OctFileBinary },
    { L"build",        NfIcon::CodOutput },
    { L"dist",         NfIcon::CodOutput },
    { L"out",          NfIcon::CodOutput },
    { L"output",       NfIcon::CodOutput },
    { L"artifacts",    NfIcon::CodPackage },

    // Testing (Terminal-Icons)
    { L"test",         NfIcon::MdTestTube },
    { L"tests",        NfIcon::MdTestTube },
    { L"__tests__",    NfIcon::MdTestTube },
    { L"spec",         NfIcon::MdTestTube },
    { L"specs",        NfIcon::MdTestTube },
    { L"benchmark",    NfIcon::MdTimer },

    // Libraries / Packages
    { L"lib",          NfIcon::CodFolderLibrary },
    { L"libs",         NfIcon::CodFolderLibrary },
    { L"packages",     NfIcon::SetiNpm },

    // Scripts
    { L"scripts",      NfIcon::SetiShell },

    // Media / Images (Terminal-Icons)
    { L"images",       NfIcon::MdFolderImage },
    { L"img",          NfIcon::MdFolderImage },
    { L"photos",       NfIcon::MdFolderImage },
    { L"pictures",     NfIcon::MdFolderImage },
    { L"assets",       NfIcon::MdFolderImage },
    { L"videos",       NfIcon::MdMovie },
    { L"movies",       NfIcon::MdMovie },
    { L"media",        NfIcon::OctFileMedia },
    { L"music",        NfIcon::MdMusicBoxMultiple },
    { L"songs",        NfIcon::MdMusicBoxMultiple },
    { L"fonts",        NfIcon::FaFont },

    // User directories (Terminal-Icons)
    { L"downloads",    NfIcon::MdFolderDownload },
    { L"desktop",      NfIcon::MdDesktopClassic },
    { L"favorites",    NfIcon::MdFolderStar },
    { L"contacts",     NfIcon::MdContacts },
    { L"onedrive",     NfIcon::MdMicrosoftOnedrive },
    { L"users",        NfIcon::FaUsers },
    { L"windows",      NfIcon::FaWindows },

    // Other (Terminal-Icons)
    { L"apps",         NfIcon::MdApps },
    { L"applications", NfIcon::MdApps },
    { L"demo",         NfIcon::CodPreview },
    { L"samples",      NfIcon::CodPreview },
    { L"shortcuts",    NfIcon::CodFileSymlinkDir },
    { L"links",        NfIcon::CodFileSymlinkDir },
    { L"github",       NfIcon::FaGithubAlt },
};
```

### Attribute Precedence Array (New Order)

```cpp
// Location: IconMapping.cpp
// Note: This is for color+icon precedence ONLY, NOT for display column order.
// The display column order (k_rgFileAttributeMap in FileAttributeMap.h) retains
// the existing RHSATECP0 order.

const SFileAttributeMap k_rgAttributePrecedenceOrder[] =
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
