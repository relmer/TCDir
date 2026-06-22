#pragma once

#include "Version.h"

//
// Shared constants and the install/uninstall mode enum for the Nerd Font
// feature.  Centralized so the GitHub URLs, font face names, file names, and
// registry paths are each defined exactly once.
//





////////////////////////////////////////////////////////////////////////////////
//
//  ENerdFontOperation
//
//  Whether a Nerd Font action is installing or uninstalling.  Replaces the
//  opaque bool fInstall parameter that used to thread through this code.
//
////////////////////////////////////////////////////////////////////////////////

enum class ENerdFontOperation
{
    Install,
    Uninstall
};




////////////////////////////////////////////////////////////////////////////////
//
//  EFontInstallOutcome
//
//  Whether the system font files were newly installed this run or were already
//  present, so the installer can detect a no-change state and adjust messaging.
//
////////////////////////////////////////////////////////////////////////////////

enum class EFontInstallOutcome
{
    Installed,       // Font files were installed during this run
    AlreadyPresent   // Font files were already installed
};





////////////////////////////////////////////////////////////////////////////////
//
//  NerdFontConst
//
//  Constants for the Nerd Font installer.  Two distinct font face names matter:
//  the console/GDI host wants "CaskaydiaCove NF" while Windows Terminal and
//  DirectWrite want "CaskaydiaCove Nerd Font".
//
////////////////////////////////////////////////////////////////////////////////

struct NerdFontConst
{
    // Font face names.  Console/GDI uses "CaskaydiaCove NF"; Windows Terminal and
    // DirectWrite use "CaskaydiaCove Nerd Font" (also spliced into settings.json
    // as UTF-8).
    static constexpr LPCWSTR      kpszConsoleFontFace = L"CaskaydiaCove NF";
    static constexpr LPCWSTR      kpszWtFontFace      = L"CaskaydiaCove Nerd Font";
    static constexpr const char * kpszWtFontFaceUtf8  = "CaskaydiaCove Nerd Font";

    // Font package (GitHub releases of ryanoasis/nerd-fonts).
    static constexpr LPCWSTR kpszZipName           = L"CascadiaCode.zip";
    static constexpr LPCWSTR kpszExtractSubdir     = L"TCDir-NerdFonts";
    static constexpr LPCWSTR kpszInstalledFontGlob = L"CaskaydiaCoveNerdFont*.ttf";
    static constexpr LPCWSTR kpszReleaseUrlFormat  = L"https://github.com/ryanoasis/nerd-fonts/releases/download/{}/{}";
    static constexpr LPCWSTR kpszGithubApiHost     = L"api.github.com";
    static constexpr LPCWSTR kpszLatestReleasePath = L"/repos/ryanoasis/nerd-fonts/releases/latest";

    // HTTP user agent: "TCDir/<version>" (e.g. TCDir/5.5.1742).
    static constexpr LPCWSTR kpszUserAgent = L"TCDir/" VERSION_WSTRING;

    // System font registry (so DirectWrite / Settings can discover the fonts).
    static constexpr LPCWSTR kpszFontsRegKey    = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
    static constexpr LPCWSTR kpszFontsSubdir    = L"Fonts";        // %WINDIR%\Fonts
    static constexpr LPCWSTR kpszTtfGlob        = L"*.ttf";
    static constexpr LPCWSTR kpszTrueTypeSuffix = L" (TrueType)";  // system Fonts registry value-name suffix

    // Uninstall manifest (records the font files we installed).
    static constexpr LPCWSTR kpszManifestRegKey   = L"Software\\TCDir\\NerdFonts";
    static constexpr LPCWSTR kpszManifestRegValue = L"InstalledFontFiles";
    static constexpr LPCWSTR kpszManifestSentinel = L"__TCDIR_NF_MANIFEST__";

    // Elevated-process (UAC) IPC: the parent passes these switches to the elevated
    // child process that performs the system-wide font install/remove.
    static constexpr LPCWSTR kpszElevatedInstallSwitch = L"--nf-elevated-install";
    static constexpr LPCWSTR kpszElevatedRemoveSwitch  = L"--nf-elevated-remove";
    static constexpr LPCWSTR kpszElevatedDirSwitch     = L"--nf-elevated-dir";

    // Windows Terminal settings location (under %LOCALAPPDATA%\Packages\...).
    static constexpr LPCWSTR kpszWtPackageDir = L"Microsoft.WindowsTerminal_8wekyb3d8bbwe";
};





