#include "pch.h"

#include "IconMapping.h"



// ============================================================================
//
//  Compile-time verification of CodePointToWideChars
//
//  Verify BMP code points produce count=1 and supplementary code points
//  produce correct surrogate pairs with count=2.
//
// ============================================================================

// BMP code point (Nerd Font Private Use Area)
static_assert(CodePointToWideChars (NfIcon::CustomCpp).count == 1,
              "BMP code point must produce 1 wchar_t");
static_assert(CodePointToWideChars (NfIcon::CustomCpp).chars[0] == 0xE61D,
              "BMP code point chars[0] must equal the code point");

// Supplementary code point (Material Design — surrogate pair range)
static_assert(CodePointToWideChars (NfIcon::MdApplication).count == 2,
              "Supplementary code point must produce 2 wchar_t");
static_assert(CodePointToWideChars (NfIcon::MdApplication).chars[0] == 0xDB82,
              "Supplementary code point high surrogate incorrect");
static_assert(CodePointToWideChars (NfIcon::MdApplication).chars[1] == 0xDCC6,
              "Supplementary code point low surrogate incorrect");

// Invalid code points
static_assert(CodePointToWideChars (0).count == 0,
              "Zero code point must produce count=0");
static_assert(CodePointToWideChars (0xD800).count == 0,
              "Surrogate code point must produce count=0");
static_assert(CodePointToWideChars (0x110000).count == 0,
              "Out-of-range code point must produce count=0");



// ============================================================================
//
//  g_rgDefaultExtensionIcons
//
//  Default extension-to-icon mapping. Seeded into CConfig's
//  m_mapExtensionToIcon during Initialize.
//
// ============================================================================

const SIconMappingEntry g_rgDefaultExtensionIcons[] =
{
    // C/C++
    { L".c",       NfIcon::CustomC },
    { L".cpp",     NfIcon::CustomCpp },
    { L".cxx",     NfIcon::CustomCpp },
    { L".h",       NfIcon::CustomC },
    { L".hpp",     NfIcon::CustomCpp },
    { L".hxx",     NfIcon::CustomCpp },
    { L".asm",     NfIcon::CustomAsm },
    { L".cod",     NfIcon::CustomAsm },
    { L".i",       NfIcon::CustomAsm },

    // C# / .NET
    { L".cs",      NfIcon::SetiCSharp },
    { L".csx",     NfIcon::SetiCSharp },
    { L".resx",    NfIcon::DevVisualStudio },
    { L".rcml",    NfIcon::DevVisualStudio },

    // JavaScript / TypeScript
    { L".js",      NfIcon::SetiJavascript },
    { L".jsx",     NfIcon::DevReact },
    { L".ts",      NfIcon::SetiTypescript },
    { L".tsx",     NfIcon::DevReact },

    // Web
    { L".html",    NfIcon::SetiHtml },
    { L".htm",     NfIcon::SetiHtml },
    { L".css",     NfIcon::SetiCss },
    { L".scss",    NfIcon::DevSass },
    { L".less",    NfIcon::DevLess },

    // Python / Java
    { L".py",      NfIcon::SetiPython },
    { L".pyw",     NfIcon::SetiPython },
    { L".jar",     NfIcon::SetiJava },

    // Rust
    { L".rs",      NfIcon::DevRust },
    { L".java",    NfIcon::SetiJava },
    { L".class",   NfIcon::SetiJava },

    // Data formats
    { L".xml",     NfIcon::SetiXml },
    { L".json",    NfIcon::SetiJson },
    { L".toml",    NfIcon::SetiConfig },
    { L".yml",     NfIcon::SetiYml },
    { L".yaml",    NfIcon::SetiYml },

    // Build artifacts
    { L".obj",     NfIcon::OctFileBinary },
    { L".lib",     NfIcon::OctFileBinary },
    { L".res",     NfIcon::OctFileBinary },
    { L".pch",     NfIcon::OctFileBinary },

    // Logs
    { L".wrn",     NfIcon::FaList },
    { L".err",     NfIcon::FaList },
    { L".log",     NfIcon::FaList },

    // Shell
    { L".bash",    NfIcon::SetiShell },
    { L".sh",      NfIcon::SetiShell },
    { L".bat",     NfIcon::CustomMsdos },
    { L".cmd",     NfIcon::CustomMsdos },
    { L".ps1",     NfIcon::SetiPowershell },
    { L".psd1",    NfIcon::SetiPowershell },
    { L".psm1",    NfIcon::SetiPowershell },

    // Executables
    { L".exe",     NfIcon::MdApplication },
    { L".sys",     NfIcon::MdApplication },
    { L".dll",     NfIcon::FaArchive },

    // Visual Studio
    { L".sln",     NfIcon::DevVisualStudio },
    { L".vcproj",  NfIcon::DevVisualStudio },
    { L".vcxproj", NfIcon::DevVisualStudio },
    { L".csproj",  NfIcon::DevVisualStudio },
    { L".csxproj", NfIcon::DevVisualStudio },
    { L".user",    NfIcon::DevVisualStudio },
    { L".ncb",     NfIcon::DevVisualStudio },

    // Documents
    { L".doc",     NfIcon::SetiWord },
    { L".docx",    NfIcon::SetiWord },
    { L".ppt",     NfIcon::MdFilePowerpoint },
    { L".pptx",    NfIcon::MdFilePowerpoint },
    { L".xls",     NfIcon::SetiXls },
    { L".xlsx",    NfIcon::SetiXls },
    { L".md",      NfIcon::SetiMarkdown },

    // Text
    { L".txt",     NfIcon::MdFileDocument },
    { L".text",    NfIcon::MdFileDocument },
    { L".!!!",     NfIcon::MdFileDocument },
    { L".1st",     NfIcon::MdFileDocument },
    { L".me",      NfIcon::MdFileDocument },
    { L".now",     NfIcon::MdFileDocument },

    // Email
    { L".eml",     NfIcon::FaEnvelope },

    // Archives
    { L".7z",      NfIcon::OctFileZip },
    { L".arj",     NfIcon::OctFileZip },
    { L".gz",      NfIcon::OctFileZip },
    { L".rar",     NfIcon::OctFileZip },
    { L".tar",     NfIcon::OctFileZip },
    { L".zip",     NfIcon::OctFileZip },

    // Resource
    { L".rc",      NfIcon::SetiConfig },
};

const size_t g_cDefaultExtensionIcons = _countof(g_rgDefaultExtensionIcons);



// ============================================================================
//
//  g_rgDefaultWellKnownDirIcons
//
//  Default well-known directory name to icon mapping.
//
// ============================================================================

const SIconMappingEntry g_rgDefaultWellKnownDirIcons[] =
{
    { L".git",         NfIcon::CustomFolderGit },
    { L".github",      NfIcon::CustomFolderGithub },
    { L".vscode",      NfIcon::CustomFolderConfig },
    { L".cargo",       NfIcon::CustomFolderConfig },
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

const size_t g_cDefaultWellKnownDirIcons = _countof(g_rgDefaultWellKnownDirIcons);



// ============================================================================
//
//  g_rgAttributePrecedenceOrder
//
//  Attribute precedence order for color+icon resolution. This is separate
//  from the display column order (k_rgFileAttributeMap in FileAttributeMap.h
//  retains the existing RHSATECP0 order).
//
//  Order: PSHERC0TA (reparse highest, archive lowest).
//
// ============================================================================

const SFileAttributeMap g_rgAttributePrecedenceOrder[] =
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

const size_t g_cAttributePrecedenceOrder = _countof(g_rgAttributePrecedenceOrder);
