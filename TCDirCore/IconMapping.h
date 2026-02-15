#pragma once

#include "FileAttributeMap.h"



// ============================================================================
//
//  WideCharPair
//
//  Converts a Unicode code point (char32_t) to its UTF-16 representation.
//  Constexpr enables compile-time verification via static_assert.
//
// ============================================================================

struct WideCharPair
{
    wchar_t  chars[2];   // UTF-16 representation (1 or 2 wchar_t)
    unsigned count;      // 1 for BMP, 2 for supplementary, 0 for invalid
};

constexpr WideCharPair CodePointToWideChars (char32_t cp)
{
    if (cp > 0 && cp <= 0xFFFF && (cp < 0xD800 || cp > 0xDFFF))
    {
        // BMP code point (not a surrogate)
        return { { static_cast<wchar_t>(cp), L'\0' }, 1 };
    }
    else if (cp >= 0x10000 && cp <= 0x10FFFF)
    {
        // Supplementary code point — encode as surrogate pair
        char32_t offset = cp - 0x10000;
        return { { static_cast<wchar_t>(0xD800 + (offset >> 10)),
                   static_cast<wchar_t>(0xDC00 + (offset & 0x3FF)) }, 2 };
    }
    else
    {
        // Invalid (0 or surrogate range)
        return { { L'\0', L'\0' }, 0 };
    }
}



// ============================================================================
//
//  SIconMappingEntry
//
//  Static default icon mapping table entry. Parallels CConfig::STextAttr
//  for colors.
//
// ============================================================================

struct SIconMappingEntry
{
    LPCWSTR   m_pszKey;        // Extension (L".cpp"), dir name (L".git"), etc.
    char32_t  m_codePoint;     // Nerd Font code point (e.g., 0xE61D)
};



// ============================================================================
//
//  NfIcon Namespace
//
//  Single source of truth for all Nerd Font code points. Eliminates magic
//  hex literals — every usage site references a named constant. Grouped by
//  Nerd Font prefix, alphabetical within each group.
//
// ============================================================================

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



// ============================================================================
//
//  Default Tables (extern declarations)
//
//  Defined in IconMapping.cpp. Seeded into CConfig's runtime maps during
//  Initialize.
//
// ============================================================================

extern const SIconMappingEntry    g_rgDefaultExtensionIcons[];
extern const size_t               g_cDefaultExtensionIcons;

extern const SIconMappingEntry    g_rgDefaultWellKnownDirIcons[];
extern const size_t               g_cDefaultWellKnownDirIcons;

extern const SFileAttributeMap    g_rgAttributePrecedenceOrder[];
extern const size_t               g_cAttributePrecedenceOrder;
