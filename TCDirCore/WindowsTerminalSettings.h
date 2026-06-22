#pragma once

#include "JsonValue.h"
#include "NerdFontConstants.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CWindowsTerminalSettings
//
//  Editor for Windows Terminal's settings.json.  Locates profiles.defaults with
//  the JSONC parser and splices just the font face (and the calt/liga ligature
//  features) in the original text, so user comments, key order, and formatting
//  are preserved.  Manages the modern "font" object: Windows Terminal ignores the
//  legacy "fontFace" once a "font" object is present.
//
////////////////////////////////////////////////////////////////////////////////

class CWindowsTerminalSettings
{
public:
    // Pure string transforms (unit-tested with synthetic JSON).
    static bool    IsConfigured  (const string & strJson);
    static HRESULT ApplyFontFace (string & strJson, ENerdFontOperation op);

    // File-backed operations on the live settings.json.
    static HRESULT GetSettingsPath (wstring & strPath);
    static HRESULT ReadSettings    (string & strJson, bool & fExists);
    static HRESULT WriteSettings   (const string & strJson);
    static HRESULT Apply           (ENerdFontOperation op, bool & fSkipped);

private:
    static const JsonValue * FindDefaults    (const JsonValue & root);
    static const JsonValue * FindFontObject  (const JsonValue & root);
    static const JsonValue * FindManagedFace (const JsonValue & root);

    static HRESULT EnsureManagedFace         (string & strJson);
    static HRESULT EnsureFontFeatureDisabled (string & strJson, const char * pszFeature);
    static HRESULT ClearManagedFace          (string & strJson);
};





