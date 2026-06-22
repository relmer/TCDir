#include "pch.h"

#include "WindowsTerminalSettings.h"
#include "AutoHandle.h"
#include "JsonParser.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CWindowsTerminalSettings::FindDefaults
//
//  Returns the profiles.defaults object, or nullptr if the document has no
//  such object.
//
////////////////////////////////////////////////////////////////////////////////

const JsonValue * CWindowsTerminalSettings::FindDefaults (const JsonValue & root)
{
    const JsonValue * profiles = root.FindObject ("profiles");



    if (profiles == nullptr)
    {
        return nullptr;
    }

    return profiles->FindObject ("defaults");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CWindowsTerminalSettings::FindFontObject
//
//  Returns the profiles.defaults.font object we manage (falling back to a
//  top-level font object), or nullptr if there is none.
//
////////////////////////////////////////////////////////////////////////////////

const JsonValue * CWindowsTerminalSettings::FindFontObject (const JsonValue & root)
{
    const JsonValue * defaults = FindDefaults (root);
    const JsonValue * fontObj  = nullptr;



    if (defaults != nullptr)
    {
        fontObj = defaults->FindObject ("font");
    }

    if (fontObj == nullptr)
    {
        fontObj = root.FindObject ("font");
    }

    return fontObj;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CWindowsTerminalSettings::FindManagedFace
//
//  Returns the font face value node we own.  Windows Terminal uses the modern
//  font object OR the legacy fontFace, never both: once a font object exists only
//  its face matters; otherwise the legacy fontFace applies.  Never returns a
//  profile-specific font (those belong to the user).
//
////////////////////////////////////////////////////////////////////////////////

const JsonValue * CWindowsTerminalSettings::FindManagedFace (const JsonValue & root)
{
    const JsonValue * fontObj  = FindFontObject (root);
    const JsonValue * defaults = nullptr;
    const JsonValue * face     = nullptr;



    if (fontObj != nullptr)
    {
        return fontObj->FindMember ("face");
    }

    defaults = FindDefaults (root);

    if (defaults != nullptr)
    {
        face = defaults->FindMember ("fontFace");
    }

    if (face == nullptr)
    {
        face = root.FindMember ("fontFace");
    }

    return face;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CWindowsTerminalSettings::IsConfigured
//
//  True if the font face we manage is already set to the Nerd Font.  Parses
//  the document so a profile-specific fontFace cannot produce a false positive;
//  falls back to a substring probe only if the document will not parse.
//
////////////////////////////////////////////////////////////////////////////////

bool CWindowsTerminalSettings::IsConfigured (const string & strJson)
{
    JsonValue         root;
    JsonParseError    err;
    const JsonValue * face        = nullptr;
    bool              fConfigured = false;



    if (FAILED (JsonParser::ParseJsonc (strJson, root, err)))
    {
        fConfigured = strJson.find (NerdFontConst::kpszWtFontFaceUtf8) != string::npos;
    }
    else
    {
        face        = FindManagedFace (root);
        fConfigured = face != nullptr
                   && face->GetType() == JsonType::String
                   && face->GetString() == NerdFontConst::kpszWtFontFaceUtf8;
    }

    return fConfigured;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CWindowsTerminalSettings::ApplyFontFace
//
//  Install:   set profiles.defaults.font.face to the Nerd Font and disable the
//             calt + liga ligature features, preserving any existing font size,
//             weight, and axes.
//  Uninstall: clear the managed font face value.
//
//  Each step re-parses and splices the smallest region it can, so the rest of
//  the file (comments, key order, formatting) is left intact.  The caller owns
//  the decision to assert on parse failure (it knows the file is expected valid).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CWindowsTerminalSettings::ApplyFontFace (string & strJson, ENerdFontOperation op)
{
    HRESULT hr = S_OK;



    if (op == ENerdFontOperation::Install)
    {
        hr = EnsureManagedFace (strJson);
        CHR (hr);

        hr = EnsureFontFeatureDisabled (strJson, "calt");
        CHR (hr);

        hr = EnsureFontFeatureDisabled (strJson, "liga");
        CHR (hr);
    }
    else
    {
        hr = ClearManagedFace (strJson);
        CHR (hr);
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CWindowsTerminalSettings::EnsureManagedFace
//
//  Ensures profiles.defaults.font.face is the Nerd Font: replaces an existing
//  font.face value, inserts a face into an existing font object, or inserts a
//  fresh font object.  A legacy fontFace, if any, is left alone (Windows Terminal
//  ignores it once a font object exists).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CWindowsTerminalSettings::EnsureManagedFace (string & strJson)
{
    HRESULT           hr        = S_OK;
    JsonValue         root;
    JsonParseError    err;
    const JsonValue * fontObj   = nullptr;
    const JsonValue * face      = nullptr;
    const JsonValue * defaults  = nullptr;
    size_t            posInsert = 0;



    hr = JsonParser::ParseJsonc (strJson, root, err);
    CHR (hr);

    CBR (root.GetType() == JsonType::Object);

    fontObj = FindFontObject (root);

    if (fontObj != nullptr)
    {
        face = fontObj->FindMember ("face");

        if (face != nullptr)
        {
            strJson.replace (face->SpanBegin(), face->SpanEnd() - face->SpanBegin(), string ("\"") + NerdFontConst::kpszWtFontFaceUtf8 + "\"");
        }
        else
        {
            strJson.insert (fontObj->SpanBegin() + 1, string (" \"face\": \"") + NerdFontConst::kpszWtFontFaceUtf8 + "\",");
        }

        BAIL_OUT_IF (true, S_OK);
    }

    defaults  = FindDefaults (root);
    posInsert = (defaults != nullptr) ? defaults->SpanBegin() + 1 : root.SpanBegin() + 1;

    strJson.insert (posInsert, string ("\n            \"font\": { \"face\": \"") + NerdFontConst::kpszWtFontFaceUtf8 + "\" },");

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CWindowsTerminalSettings::EnsureFontFeatureDisabled
//
//  Sets profiles.defaults.font.features.<feature> to 0, inserting the feature (or
//  the whole features object) when absent.  Assumes the font object already
//  exists (EnsureManagedFace runs first).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CWindowsTerminalSettings::EnsureFontFeatureDisabled (string & strJson, const char * pszFeature)
{
    HRESULT           hr       = S_OK;
    JsonValue         root;
    JsonParseError    err;
    const JsonValue * fontObj  = nullptr;
    const JsonValue * features = nullptr;
    const JsonValue * feature  = nullptr;



    hr = JsonParser::ParseJsonc (strJson, root, err);
    CHR (hr);

    fontObj = FindFontObject (root);
    CBR (fontObj != nullptr);

    features = fontObj->FindObject ("features");

    if (features == nullptr)
    {
        strJson.insert (fontObj->SpanBegin() + 1, string (" \"features\": { \"") + pszFeature + "\": 0 },");
        BAIL_OUT_IF (true, S_OK);
    }

    feature = features->FindMember (pszFeature);

    if (feature != nullptr)
    {
        strJson.replace (feature->SpanBegin(), feature->SpanEnd() - feature->SpanBegin(), "0");
    }
    else
    {
        strJson.insert (features->SpanBegin() + 1, string (" \"") + pszFeature + "\": 0,");
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CWindowsTerminalSettings::ClearManagedFace
//
//  Uninstall: clears the managed font face value (modern font.face or legacy
//  fontFace) when it is our Nerd Font, so the profile is no longer configured.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CWindowsTerminalSettings::ClearManagedFace (string & strJson)
{
    HRESULT           hr   = S_OK;
    JsonValue         root;
    JsonParseError    err;
    const JsonValue * face = nullptr;



    hr = JsonParser::ParseJsonc (strJson, root, err);
    CHR (hr);
    CBR (root.GetType() == JsonType::Object);

    face = FindManagedFace (root);

    if (face != nullptr && face->GetType() == JsonType::String && face->GetString() == NerdFontConst::kpszWtFontFaceUtf8)
    {
        strJson.replace (face->SpanBegin(), face->SpanEnd() - face->SpanBegin(), "\"\"");
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CWindowsTerminalSettings::GetSettingsPath
//
//  Builds %LOCALAPPDATA%\Packages\<wt package>\LocalState\settings.json.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CWindowsTerminalSettings::GetSettingsPath (wstring & strPath)
{
    HRESULT hr                = S_OK;
    wchar_t szPath[MAX_PATH]  = { };
    DWORD   dwLen             = 0;



    strPath.clear();

    dwLen = GetEnvironmentVariableW (L"LOCALAPPDATA", szPath, ARRAYSIZE (szPath));
    CBR (dwLen > 0 && dwLen < ARRAYSIZE (szPath));

    hr = PathCchAppend (szPath, ARRAYSIZE (szPath), L"Packages");
    CHRA (hr);

    hr = PathCchAppend (szPath, ARRAYSIZE (szPath), NerdFontConst::kpszWtPackageDir);
    CHRA (hr);
    
    hr = PathCchAppend (szPath, ARRAYSIZE (szPath), L"LocalState");
    CHRA (hr);
    
    hr = PathCchAppend (szPath, ARRAYSIZE (szPath), L"settings.json");
    CHRA (hr);

    strPath = szPath;

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CWindowsTerminalSettings::ReadSettings
//
//  Reads the live settings.json into strJson.  fExists is false (and the call
//  succeeds) when Windows Terminal is not installed.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CWindowsTerminalSettings::ReadSettings (string & strJson, bool & fExists)
{
    HRESULT            hr          = S_OK;
    wstring            strPath;
    AutoHandle         hFile;
    DWORD              dwFileSize  = 0;
    DWORD              dwBytesRead = 0;
    BOOL               fRead       = FALSE;
    unique_ptr<char[]> pszBuf;



    strJson.clear();
    fExists = false;

    hr = GetSettingsPath (strPath);
    CHR (hr);

    BAIL_OUT_IF (!PathFileExistsW (strPath.c_str()), S_OK);

    fExists = true;

    hFile = CreateFileW (strPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    CBRA (hFile != INVALID_HANDLE_VALUE);

    dwFileSize = GetFileSize (hFile, NULL);
    CBRA (dwFileSize > 0 && dwFileSize < 1024 * 1024);

    pszBuf.reset (new (nothrow) char[dwFileSize + 1]);
    CPRA (pszBuf.get());

    fRead = ReadFile (hFile, pszBuf.get(), dwFileSize, &dwBytesRead, NULL);
    CWRA (fRead);
    CBRA (dwBytesRead == dwFileSize);

    pszBuf[dwFileSize] = '\0';

    strJson = pszBuf.get();

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CWindowsTerminalSettings::WriteSettings
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CWindowsTerminalSettings::WriteSettings (const string & strJson)
{
    HRESULT    hr        = S_OK;
    wstring    strPath;
    AutoHandle hFile;
    DWORD      dwToWrite = 0;
    DWORD      dwWritten = 0;
    BOOL       fWritten  = FALSE;



    hr = GetSettingsPath (strPath);
    CHR (hr);

    hFile = CreateFileW (strPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    CBRA (hFile != INVALID_HANDLE_VALUE);

    dwToWrite = static_cast<DWORD> (strJson.length());

    fWritten = WriteFile (hFile, strJson.c_str(), dwToWrite, &dwWritten, NULL);
    CWRA (fWritten);
    CBRA (dwWritten == dwToWrite);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CWindowsTerminalSettings::Apply
//
//  Reads the live settings.json, splices the font face, and writes it back.
//  fSkipped is true (and the call succeeds) when Windows Terminal is not
//  installed.  Parse failures assert: the file is expected to be valid.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CWindowsTerminalSettings::Apply (ENerdFontOperation op, bool & fSkipped)
{
    HRESULT hr      = S_OK;
    string  strJson;
    bool    fExists = false;



    fSkipped = false;

    hr = ReadSettings (strJson, fExists);
    CHRA (hr);

    BAIL_OUT_IF (!fExists, S_OK);

    hr = ApplyFontFace (strJson, op);
    CHRA (hr);

    hr = WriteSettings (strJson);
    CHRA (hr);

Error:
    if (!fExists)
    {
        fSkipped = true;
    }

    return hr;
}





