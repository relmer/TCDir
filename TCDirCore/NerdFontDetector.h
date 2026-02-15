#pragma once

#include "EnvironmentProviderBase.h"



// ============================================================================
//
//  EDetectionResult
//
//  Outcome of the Nerd Font auto-detection chain.
//
// ============================================================================

enum class EDetectionResult
{
    Detected,        // Nerd Font confirmed (canary hit or WezTerm)
    NotDetected,     // No Nerd Font found
    Inconclusive     // Detection failed — default to OFF
};



// ============================================================================
//
//  CNerdFontDetector
//
//  Layered detection chain for Nerd Font availability:
//    1. WezTerm env var short-circuit (always has NF fallback)
//    2. ConPTY check (cannot use classic GDI probe)
//    3. Classic conhost canary probe via GetGlyphIndicesW
//    4. System font enumeration via EnumFontFamiliesExW
//    5. Fallback OFF
//
//  GDI-dependent methods are protected virtual so tests can derive
//  and override them (same pattern as ConfigProbe : public CConfig).
//
// ============================================================================

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
