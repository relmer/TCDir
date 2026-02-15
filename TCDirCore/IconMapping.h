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
    constexpr char32_t SetiGit                = 0xE65D;
    constexpr char32_t SetiGithub             = 0xE65B;
    constexpr char32_t SetiHtml               = 0xE60E;
    constexpr char32_t SetiJson               = 0xE60B;
    constexpr char32_t SetiJulia              = 0xE624;
    constexpr char32_t SetiLua                = 0xE620;
    constexpr char32_t SetiMakefile           = 0xE673;
    constexpr char32_t SetiNpm                = 0xE616;
    constexpr char32_t SetiProject            = 0xE601;
    constexpr char32_t SetiPython             = 0xE606;
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
    constexpr char32_t DevVscode              = 0xE8DA;

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
