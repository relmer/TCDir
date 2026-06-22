#pragma once

#include "NerdFontConstants.h"

//
// Acquires the Cascadia Code Nerd Font package from the ryanoasis/nerd-fonts
// GitHub releases: resolves the latest tag, downloads the zip, and extracts it.
//





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontPackage
//
////////////////////////////////////////////////////////////////////////////////

class CNerdFontPackage
{
public:
    // Resolves the latest release tag from the GitHub API.  On HTTP 403/429
    // (rate limiting) the call fails and fRateLimited is set.
    static HRESULT ResolveLatestTag (wstring & strTag, bool & fRateLimited);

    // Downloads the font zip for a tag into pszDestDir; returns its full path.
    static HRESULT Download         (LPCWSTR pszDestDir, LPCWSTR pszReleaseTag, wstring & strZipPath);

    // Extracts a zip into a directory using PowerShell's ZipFile API.
    static HRESULT Extract          (LPCWSTR pszZipPath, LPCWSTR pszDestDir);
};





