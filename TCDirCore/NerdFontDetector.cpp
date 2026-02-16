#include "pch.h"
#include "NerdFontDetector.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontDetector::Detect
//
//  Layered detection chain:
//    1. WezTerm short-circuit → Detected (always has NF fallback fonts)
//    2. ConPTY check → cannot use GDI probe, skip to font enumeration
//    3. Classic conhost canary probe via GetGlyphIndicesW
//    4. System font enumeration via EnumFontFamiliesExW
//    5. Fallback → NotDetected
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontDetector::Detect (
    HANDLE                       hConsole,
    const IEnvironmentProvider & envProvider,
    EDetectionResult &           result)
{
    HRESULT hr         = S_OK;
    bool    fHasGlyph  = false;
    bool    fFound     = false;



    result = EDetectionResult::NotDetected;

    //
    // Step 1: WezTerm always bundles Nerd Font Symbols as a fallback font.
    // If TERM_PROGRAM=WezTerm, short-circuit to Detected.
    //

    if (IsWezTerm (envProvider))
    {
        result = EDetectionResult::Detected;
        BAIL_OUT_IF (TRUE, S_OK);
    }

    //
    // Step 2: If running under ConPTY (Windows Terminal, VS Code, etc.),
    // we cannot use the classic conhost GDI probe because the console
    // handle doesn't have the real font.  Skip to font enumeration.
    //

    if (IsConPtyTerminal (envProvider))
    {
        hr = IsNerdFontInstalled (fFound);
        CHR (hr);

        result = fFound ? EDetectionResult::Detected : EDetectionResult::NotDetected;
        BAIL_OUT_IF (TRUE, S_OK);
    }

    //
    // Step 3: Classic conhost — try the canary glyph probe.
    // Use a BMP Nerd Font glyph (nf-custom-folder, U+E5FF) as canary.
    //

    hr = ProbeConsoleFontForGlyph (hConsole, static_cast<WCHAR>(0xE5FF), fHasGlyph);

    if (SUCCEEDED (hr))
    {
        result = fHasGlyph ? EDetectionResult::Detected : EDetectionResult::NotDetected;
        BAIL_OUT_IF (TRUE, S_OK);
    }

    //
    // Step 4: Canary probe failed (e.g., invalid handle in test).
    // Fall back to system font enumeration.
    //

    hr = IsNerdFontInstalled (fFound);
    if (SUCCEEDED (hr))
    {
        result = fFound ? EDetectionResult::Detected : EDetectionResult::NotDetected;
        BAIL_OUT_IF (TRUE, S_OK);
    }

    //
    // Step 5: Everything failed.  Default to Inconclusive (caller treats as OFF).
    //

    result = EDetectionResult::Inconclusive;
    hr = S_OK;



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontDetector::ProbeConsoleFontForGlyph
//
//  Classic conhost canary probe.  Gets the console font via
//  GetCurrentConsoleFontEx, creates a DC + font, and uses
//  GetGlyphIndicesW to check if the canary glyph is present.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontDetector::ProbeConsoleFontForGlyph (HANDLE hConsole, WCHAR wchCanary, bool & fHasGlyph)
{
    HRESULT              hr        = S_OK;
    CONSOLE_FONT_INFOEX  fontInfo  = { 0 };
    HDC                  hdc       = nullptr;
    HFONT                hFont     = nullptr;
    HFONT                hFontOld  = nullptr;
    WORD                 glyphIdx  = 0;
    DWORD                dwResult  = 0;



    fHasGlyph = false;

    //
    // Get the current console font face name
    //

    fontInfo.cbSize = sizeof (fontInfo);
    CBREx (GetCurrentConsoleFontEx (hConsole, FALSE, &fontInfo), HRESULT_FROM_WIN32 (GetLastError()));

    //
    // Create a memory DC and matching font
    //

    hdc = CreateCompatibleDC (nullptr);
    CBREx (hdc != nullptr, E_FAIL);

    hFont = CreateFontW (-static_cast<int>(fontInfo.dwFontSize.Y),
                         0, 0, 0,
                         FW_NORMAL,
                         FALSE, FALSE, FALSE,
                         DEFAULT_CHARSET,
                         OUT_DEFAULT_PRECIS,
                         CLIP_DEFAULT_PRECIS,
                         DEFAULT_QUALITY,
                         FIXED_PITCH | FF_MODERN,
                         fontInfo.FaceName);

    CBREx (hFont != nullptr, E_FAIL);

    hFontOld = static_cast<HFONT>(SelectObject (hdc, hFont));

    //
    // Probe for the canary glyph
    //

    dwResult = GetGlyphIndicesW (hdc, &wchCanary, 1, &glyphIdx, GGI_MARK_NONEXISTING_GLYPHS);
    CBREx (dwResult != GDI_ERROR, HRESULT_FROM_WIN32 (GetLastError()));

    fHasGlyph = (glyphIdx != 0xFFFF);



Error:
    if (hdc != nullptr)
    {
        if (hFontOld != nullptr)
        {
            SelectObject (hdc, hFontOld);
        }
        
        if (hFont != nullptr)
        {
            DeleteObject (hFont);
        }

        DeleteDC (hdc);
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  EnumFontCallback
//
//  Callback for EnumFontFamiliesExW.  Checks if the font family name
//  contains "Nerd Font" or ends with the abbreviated suffixes
//  " NF", " NFM", or " NFP" (case-insensitive).
//
////////////////////////////////////////////////////////////////////////////////

static int CALLBACK EnumFontCallback (
    const LOGFONTW *    plf,
    const TEXTMETRICW *,
    DWORD,
    LPARAM              lParam)
{
    auto pfResult = reinterpret_cast<bool *>(lParam);

    wstring fontName (plf->lfFaceName);
    std::ranges::transform (fontName, fontName.begin(), towlower);

    if (fontName.find (L"nerd font") != wstring::npos)
    {
        *pfResult = true;
        return 0;  // Stop enumeration
    }

    if (fontName.ends_with (L" nf")  ||
        fontName.ends_with (L" nfm") ||
        fontName.ends_with (L" nfp"))
    {
        *pfResult = true;
        return 0;  // Stop enumeration
    }

    return 1;  // Continue enumeration
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontDetector::IsNerdFontInstalled
//
//  Enumerate system fonts and check if any font family name contains
//  "Nerd Font" (case-insensitive).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontDetector::IsNerdFontInstalled (bool & fFound)
{
    HRESULT  hr   = S_OK;
    HDC      hdc  = nullptr;
    LOGFONTW lf   = { 0 };



    fFound = false;

    hdc = CreateCompatibleDC (nullptr);
    CBREx (hdc != nullptr, E_FAIL);

    lf.lfCharSet = DEFAULT_CHARSET;

    EnumFontFamiliesExW (hdc,
                         &lf,
                         EnumFontCallback,
                         reinterpret_cast<LPARAM> (&fFound),
                         0);



Error:
    if (hdc != nullptr)
    {
        DeleteDC (hdc);
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontDetector::IsWezTerm
//
//  Check if TERM_PROGRAM environment variable is set to "WezTerm".
//  WezTerm always bundles Nerd Font Symbols as a fallback font, so
//  icons are always available.
//
////////////////////////////////////////////////////////////////////////////////

bool CNerdFontDetector::IsWezTerm (const IEnvironmentProvider & envProvider)
{
    wstring value;

    if (!envProvider.TryGetEnvironmentVariable (L"TERM_PROGRAM", value))
    {
        return false;
    }

    return _wcsicmp (value.c_str(), L"WezTerm") == 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontDetector::IsConPtyTerminal
//
//  Check if running under ConPTY (Windows Terminal, VS Code terminal, etc.).
//  ConPTY terminals set WT_SESSION or TERM_PROGRAM env vars.  In a ConPTY
//  environment, the console handle doesn't expose the real font, so we
//  cannot use GetCurrentConsoleFontEx for glyph probing.
//
////////////////////////////////////////////////////////////////////////////////

bool CNerdFontDetector::IsConPtyTerminal (const IEnvironmentProvider & envProvider)
{
    wstring value;

    //
    // Windows Terminal sets WT_SESSION
    //

    if (envProvider.TryGetEnvironmentVariable (L"WT_SESSION", value) && !value.empty())
    {
        return true;
    }

    //
    // VS Code terminal, Hyper, etc. set TERM_PROGRAM
    //

    if (envProvider.TryGetEnvironmentVariable (L"TERM_PROGRAM", value) && !value.empty())
    {
        return true;
    }

    //
    // ConEmu sets ConEmuPID
    //

    if (envProvider.TryGetEnvironmentVariable (L"ConEmuPID", value) && !value.empty())
    {
        return true;
    }

    //
    // Alacritty sets ALACRITTY_WINDOW_ID
    //

    if (envProvider.TryGetEnvironmentVariable (L"ALACRITTY_WINDOW_ID", value) && !value.empty())
    {
        return true;
    }

    return false;
}
