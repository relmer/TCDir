#pragma once

#include "NerdFontConstants.h"

class CConsole;

//
// Installs and removes the Nerd Font files in the system Fonts folder, tracks
// what was installed in an HKCU manifest, and reports whether the fonts are
// present.  (Permanent font installation has no single Win32 API: it is a file
// copy plus an HKLM Fonts registry entry plus a WM_FONTCHANGE broadcast.)
//





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontRegistrar
//
////////////////////////////////////////////////////////////////////////////////

class CNerdFontRegistrar
{
public:
    // Copy every .ttf from pszFontDir into the system Fonts folder, register each
    // with GDI and the system font registry, and record them in the manifest.
    static HRESULT Install (LPCWSTR pszFontDir, CConsole & console);

    // Remove the fonts recorded in the manifest (or matched by pszFontDir / the
    // Nerd Font glob) from the system Fonts folder and the system font registry.
    static HRESULT Remove (LPCWSTR pszFontDir, CConsole & console);

    // True if every source .ttf in pszFontDir is already in the system Fonts folder.
    static bool AreFontFilesPresent (LPCWSTR pszFontDir);

    // True if any TCDir-installed Nerd Font remains in the system Fonts folder.
    static bool HasSystemNerdFontFilesInstalled();

    // Uninstall manifest (records the files TCDir installed) under HKCU.
    static HRESULT SaveManifest   (const vector<wstring> & rgFontFiles);
    static HRESULT LoadManifest   (vector<wstring> & rgFontFiles);
    static void    ClearManifest();

private:
    // Read the TrueType "full name" (name ID 4) from a font file's name table.
    static bool GetTtfFullName (LPCWSTR pszPath, wstring & strFullName);

    // Add / remove a system font registry entry so DirectWrite and Settings can
    // discover the font.
    static void RegisterFontInSystem     (LPCWSTR pszDestPath, LPCWSTR pszFileName);
    static void UnregisterFontFromSystem (LPCWSTR pszFileName);
};





