#include "pch.h"

#include "NerdFontRegistrar.h"
#include "AutoHandle.h"
#include "Console.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontRegistrar::SaveManifest
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontRegistrar::SaveManifest (const vector<wstring> & rgFontFiles)
{
    HRESULT         hr      = S_OK;
    CAutoRegKey     hkey;
    LONG            lResult = ERROR_SUCCESS;
    vector<wchar_t> rgData;



    if (rgFontFiles.empty())
    {
        ClearManifest();
        BAIL_OUT_IF (true, S_OK);
    }

    lResult = RegCreateKeyExW (HKEY_CURRENT_USER, NerdFontConst::kpszManifestRegKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL, hkey.GetRef(), NULL);
    CBRAEx (lResult == ERROR_SUCCESS, HRESULT_FROM_WIN32 (lResult));

    for (const wstring & strFile : rgFontFiles)
    {
        rgData.insert (rgData.end(), strFile.begin(), strFile.end());
        rgData.push_back (L'\0');
    }
    rgData.push_back (L'\0');

    lResult = RegSetValueExW (hkey, NerdFontConst::kpszManifestRegValue, 0, REG_MULTI_SZ,
                              reinterpret_cast<const BYTE *> (rgData.data()),
                              static_cast<DWORD> (rgData.size() * sizeof (wchar_t)));
    CBRAEx (lResult == ERROR_SUCCESS, HRESULT_FROM_WIN32 (lResult));

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontRegistrar::LoadManifest
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontRegistrar::LoadManifest (vector<wstring> & rgFontFiles)
{
    HRESULT         hr      = S_OK;
    CAutoRegKey     hkey;
    LONG            lResult = ERROR_SUCCESS;
    DWORD           dwType  = 0;
    DWORD           cbData  = 0;
    vector<wchar_t> rgData;
    const wchar_t * p       = nullptr;



    rgFontFiles.clear();

    lResult = RegOpenKeyExW (HKEY_CURRENT_USER, NerdFontConst::kpszManifestRegKey, 0, KEY_QUERY_VALUE, hkey.GetRef());
    BAIL_OUT_IF (lResult == ERROR_FILE_NOT_FOUND, S_OK);
    CBRAEx (lResult == ERROR_SUCCESS, HRESULT_FROM_WIN32 (lResult));

    lResult = RegQueryValueExW (hkey, NerdFontConst::kpszManifestRegValue, NULL, &dwType, NULL, &cbData);
    BAIL_OUT_IF (lResult == ERROR_FILE_NOT_FOUND, S_OK);
    CBRAEx (lResult == ERROR_SUCCESS, HRESULT_FROM_WIN32 (lResult));
    CBRAEx (dwType == REG_MULTI_SZ && cbData >= sizeof (wchar_t), E_FAIL);

    rgData.assign ((cbData / sizeof (wchar_t)) + 1, L'\0');

    lResult = RegQueryValueExW (hkey, NerdFontConst::kpszManifestRegValue, NULL, &dwType,
                                reinterpret_cast<LPBYTE> (rgData.data()), &cbData);
    CBRAEx (lResult == ERROR_SUCCESS, HRESULT_FROM_WIN32 (lResult));

    p = rgData.data();
    while (*p != L'\0')
    {
        rgFontFiles.push_back (p);
        p += wcslen (p) + 1;
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontRegistrar::ClearManifest
//
////////////////////////////////////////////////////////////////////////////////

void CNerdFontRegistrar::ClearManifest()
{
    CAutoRegKey hkey;



    if (RegOpenKeyExW (HKEY_CURRENT_USER, NerdFontConst::kpszManifestRegKey, 0, KEY_SET_VALUE, hkey.GetRef()) == ERROR_SUCCESS)
    {
        RegDeleteValueW (hkey, NerdFontConst::kpszManifestRegValue);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontRegistrar::AreFontFilesPresent
//
////////////////////////////////////////////////////////////////////////////////

bool CNerdFontRegistrar::AreFontFilesPresent (LPCWSTR pszFontDir)
{
    HRESULT          hr                        = S_OK;
    wchar_t          szWinFontsDir[MAX_PATH]   = { };
    wchar_t          szSearchPattern[MAX_PATH] = { };
    WIN32_FIND_DATAW findData                  = { };
    AutoFindHandle   hFind;
    DWORD            dwLen                      = 0;
    bool             fAllPresent                = false;



    dwLen = GetWindowsDirectoryW (szWinFontsDir, ARRAYSIZE (szWinFontsDir));
    CBR (dwLen > 0 && dwLen < ARRAYSIZE (szWinFontsDir));

    hr = PathCchAppend (szWinFontsDir, ARRAYSIZE (szWinFontsDir), L"Fonts");
    CHRA (hr);

    hr = PathCchCombine (szSearchPattern, ARRAYSIZE (szSearchPattern), pszFontDir, L"*.ttf");
    CHRA (hr);

    hFind = FindFirstFileW (szSearchPattern, &findData);
    CBR (hFind != INVALID_HANDLE_VALUE);

    fAllPresent = true;

    do
    {
        wchar_t szDestPath[MAX_PATH] = { };

        hr = PathCchCombine (szDestPath, ARRAYSIZE (szDestPath), szWinFontsDir, findData.cFileName);
        CHRA (hr);

        if (!PathFileExistsW (szDestPath))
        {
            fAllPresent = false;
            break;
        }
    } while (FindNextFileW (hFind, &findData));

Error:
    return fAllPresent;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontRegistrar::HasSystemNerdFontFilesInstalled
//
////////////////////////////////////////////////////////////////////////////////

bool CNerdFontRegistrar::HasSystemNerdFontFilesInstalled()
{
    HRESULT          hr                      = S_OK;
    wchar_t          szWinFontsDir[MAX_PATH] = { };
    wchar_t          szPattern[MAX_PATH]     = { };
    WIN32_FIND_DATAW findData                = { };
    AutoFindHandle   hFind;
    DWORD            dwLen                   = 0;
    vector<wstring>  rgManifest;
    bool             fInstalled              = false;



    dwLen = GetWindowsDirectoryW (szWinFontsDir, ARRAYSIZE (szWinFontsDir));
    CBR (dwLen > 0 && dwLen < ARRAYSIZE (szWinFontsDir));

    hr = PathCchAppend (szWinFontsDir, ARRAYSIZE (szWinFontsDir), L"Fonts");
    CHRA (hr);

    //
    // Manifest first — it tracks exactly what TCDir installed, including
    // files whose names overlap pre-existing Windows fonts.
    //

    if (SUCCEEDED (LoadManifest (rgManifest)) && !rgManifest.empty())
    {
        for (const wstring & strFile : rgManifest)
        {
            wchar_t szFontPath[MAX_PATH] = { };

            hr = PathCchCombine (szFontPath, ARRAYSIZE (szFontPath), szWinFontsDir, strFile.c_str());
            CHRA (hr);

            if (PathFileExistsW (szFontPath))
            {
                fInstalled = true;
                break;
            }
        }
    }

    //
    // Fallback: pattern-match for installs that predate the manifest.
    //

    if (!fInstalled)
    {
        hr = PathCchCombine (szPattern, ARRAYSIZE (szPattern), szWinFontsDir, NerdFontConst::kpszInstalledFontGlob);
        CHRA (hr);

        hFind      = FindFirstFileW (szPattern, &findData);
        fInstalled = (hFind != INVALID_HANDLE_VALUE);
    }

Error:
    return fInstalled;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontRegistrar::Install
//
//  Copy every .ttf in pszFontDir into the system Fonts folder, register each with
//  GDI (AddFontResource) and the system font registry, and record the installed
//  files in the uninstall manifest.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontRegistrar::Install (LPCWSTR pszFontDir, CConsole & console)
{
    HRESULT          hr                        = S_OK;
    WIN32_FIND_DATAW findData                  = { };
    AutoFindHandle   hFind;
    wchar_t          szSearchPattern[MAX_PATH] = { };
    wchar_t          szSourcePath[MAX_PATH]    = { };
    wchar_t          szDestPath[MAX_PATH]      = { };
    wchar_t          szWinFontsDir[MAX_PATH]   = { };
    DWORD            dwLen                     = 0;
    vector<wstring>  rgInstalledFontFiles;



    dwLen = GetWindowsDirectoryW (szWinFontsDir, ARRAYSIZE (szWinFontsDir));
    CBREx (dwLen != 0 && dwLen < ARRAYSIZE (szWinFontsDir), E_UNEXPECTED);

    hr = PathCchAppend (szWinFontsDir, ARRAYSIZE (szWinFontsDir), NerdFontConst::kpszFontsSubdir);
    CHRA (hr);

    hr = PathCchCombine (szSearchPattern, ARRAYSIZE (szSearchPattern), pszFontDir, NerdFontConst::kpszTtfGlob);
    CHRA (hr);

    hFind = FindFirstFileW (szSearchPattern, &findData);
    CBR (hFind != INVALID_HANDLE_VALUE);

    do
    {
        hr = PathCchCombine (szSourcePath, ARRAYSIZE (szSourcePath), pszFontDir, findData.cFileName);
        CHRA (hr);

        hr = PathCchCombine (szDestPath, ARRAYSIZE (szDestPath), szWinFontsDir, findData.cFileName);
        CHRA (hr);

        if (CopyFileW (szSourcePath, szDestPath, FALSE))
        {
            //
            // Register with GDI, add the system font registry entry (needed by
            // DirectWrite / Windows Settings), and record it in the manifest.
            //

            AddFontResourceW (szDestPath);
            RegisterFontInSystem (szDestPath, findData.cFileName);
            rgInstalledFontFiles.push_back (findData.cFileName);
        }
    } while (FindNextFileW (hFind, &findData));

    hr = SaveManifest (rgInstalledFontFiles);
    if (FAILED (hr))
    {
        console.Printf (CConfig::Information, L"    Warning: unable to persist Nerd Font uninstall manifest.\n");
        hr = S_OK;
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontRegistrar::Remove
//
//  Remove the Nerd Font files (from the supplied source directory, the manifest,
//  or a glob of the system Fonts folder) from the Fonts folder and the system
//  font registry, then update the manifest with anything that could not be
//  removed.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontRegistrar::Remove (LPCWSTR pszFontDir, CConsole & console)
{
    HRESULT          hr                      = S_OK;
    WIN32_FIND_DATAW findData                = { };
    AutoFindHandle   hFind;
    wchar_t          szDestPath[MAX_PATH]    = { };
    wchar_t          szWinFontsDir[MAX_PATH] = { };
    int              cRemovedCount           = 0;
    DWORD            dwLen                   = 0;
    vector<wstring>  rgFontFiles;
    vector<wstring>  rgRemaining;

    static constexpr DWORD kdwFontChangeTimeoutMs = 1000;



    dwLen = GetWindowsDirectoryW (szWinFontsDir, ARRAYSIZE (szWinFontsDir));
    CBREx (dwLen != 0 && dwLen < ARRAYSIZE (szWinFontsDir), E_UNEXPECTED);

    hr = PathCchAppend (szWinFontsDir, ARRAYSIZE (szWinFontsDir), NerdFontConst::kpszFontsSubdir);
    CHRA (hr);

    //
    // Determine which files to remove: prefer the supplied source directory, then
    // the manifest, then a glob of the system Fonts folder.
    //

    if (pszFontDir != nullptr
        && _wcsicmp (pszFontDir, NerdFontConst::kpszManifestSentinel) != 0
        && PathFileExistsW (pszFontDir))
    {
        wchar_t szSearchPattern[MAX_PATH] = { };

        hr = PathCchCombine (szSearchPattern, ARRAYSIZE (szSearchPattern), pszFontDir, NerdFontConst::kpszTtfGlob);
        CHRA (hr);

        hFind = FindFirstFileW (szSearchPattern, &findData);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                rgFontFiles.push_back (findData.cFileName);
            } while (FindNextFileW (hFind, &findData));
        }
    }

    if (rgFontFiles.empty())
    {
        hr = LoadManifest (rgFontFiles);
        if (FAILED (hr))
        {
            rgFontFiles.clear();
            hr = S_OK;
        }
    }

    if (rgFontFiles.empty())
    {
        wchar_t szFallbackPattern[MAX_PATH] = { };

        hr = PathCchCombine (szFallbackPattern, ARRAYSIZE (szFallbackPattern), szWinFontsDir, NerdFontConst::kpszInstalledFontGlob);
        CHRA (hr);

        hFind = FindFirstFileW (szFallbackPattern, &findData);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                rgFontFiles.push_back (findData.cFileName);
            } while (FindNextFileW (hFind, &findData));
        }
    }

    for (const wstring & strFileName : rgFontFiles)
    {
        hr = PathCchCombine (szDestPath, ARRAYSIZE (szDestPath), szWinFontsDir, strFileName.c_str());
        CHRA (hr);

        if (PathFileExistsW (szDestPath))
        {
            RemoveFontResourceW (szDestPath);
            if (DeleteFileW (szDestPath))
            {
                UnregisterFontFromSystem (strFileName.c_str());
                ++cRemovedCount;
            }
        }

        if (PathFileExistsW (szDestPath))
        {
            rgRemaining.push_back (strFileName);
        }
    }

    if (rgRemaining.empty())
    {
        ClearManifest();
    }
    else
    {
        hr = SaveManifest (rgRemaining);
        if (FAILED (hr))
        {
            hr = S_OK;
        }
    }

    SendMessageTimeoutW (HWND_BROADCAST, WM_FONTCHANGE, 0, 0, SMTO_ABORTIFHUNG, kdwFontChangeTimeoutMs, NULL);
    console.Printf (CConfig::Information, L"    Removed %d font files\n", cRemovedCount);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontRegistrar::GetTtfFullName
//
//  Read the "full name" (name ID 4) from a TrueType font file's name table.
//  Prefers platform 3 / encoding 1 / language 0x0409 (Windows Unicode English),
//  and falls back to the first platform-3 entry found.  Returns false if the
//  table cannot be parsed.
//
////////////////////////////////////////////////////////////////////////////////

bool CNerdFontRegistrar::GetTtfFullName (LPCWSTR pszPath, wstring & strFullName)
{
    AutoHandle hFile      = CreateFileW (pszPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD      dwSize     = 0;
    bool       fSucceeded = false;



    if (hFile != INVALID_HANDLE_VALUE)
    {
        dwSize = GetFileSize (hFile, NULL);

        if (dwSize >= 12 && dwSize < 16 * 1024 * 1024)
        {
            vector<BYTE> buf (dwSize);
            DWORD        dwRead = 0;



            if (ReadFile (hFile, buf.data(), dwSize, &dwRead, NULL) && dwRead == dwSize)
            {
                const BYTE * p        = buf.data();
                auto         ReadBE16 = [](const BYTE * q) -> UINT16 { return (UINT16)((q[0] << 8) | q[1]); };
                auto         ReadBE32 = [](const BYTE * q) -> UINT32 { return (UINT32)((q[0] << 24) | (q[1] << 16) | (q[2] << 8) | q[3]); };



                UINT16 numTables = ReadBE16 (p + 4);

                //
                // Scan the table directory (starts at offset 12) for the "name" table.
                //

                UINT32 nameOffset = 0;



                for (UINT16 i = 0; i < numTables && (12u + (UINT32)i * 16u + 16u) <= dwSize; ++i)
                {
                    const BYTE * entry = p + 12 + (UINT32)i * 16;

                    if (entry[0] == 'n' && entry[1] == 'a' && entry[2] == 'm' && entry[3] == 'e')
                    {
                        nameOffset = ReadBE32 (entry + 8);
                        break;
                    }
                }

                if (nameOffset > 0 && nameOffset + 6 <= dwSize)
                {
                    const BYTE * nameTable    = p + nameOffset;
                    UINT16       count        = ReadBE16 (nameTable + 2);
                    UINT16       stringOffset = ReadBE16 (nameTable + 4);
                    const BYTE * records      = nameTable + 6;

                    wstring strBest;
                    bool    fHaveEnUs = false;



                    for (UINT16 i = 0; i < count; ++i)
                    {
                        const BYTE * rec        = records + (UINT32)i * 12;
                        UINT16       platformID = ReadBE16 (rec + 0);
                        UINT16       encodingID = ReadBE16 (rec + 2);
                        UINT16       languageID = ReadBE16 (rec + 4);
                        UINT16       nameID     = ReadBE16 (rec + 6);
                        UINT16       length     = ReadBE16 (rec + 8);
                        UINT16       strOff     = ReadBE16 (rec + 10);



                        if (nameID != 4 || platformID != 3 || encodingID != 1)
                        {
                            continue;
                        }

                        UINT32 absOff = nameOffset + stringOffset + strOff;



                        if (absOff + length > dwSize || (length % 2) != 0)
                        {
                            continue;
                        }

                        //
                        // Convert big-endian UTF-16 to wstring.
                        //

                        wstring strCandidate;
                        UINT16  cWords = length / 2;



                        strCandidate.reserve (cWords);

                        for (UINT16 w = 0; w < cWords; ++w)
                        {
                            const BYTE * pb = p + absOff + (UINT32)w * 2;



                            strCandidate += static_cast<wchar_t>((pb[0] << 8) | pb[1]);
                        }

                        if (languageID == 0x0409)
                        {
                            strFullName = move (strCandidate);
                            fHaveEnUs   = true;
                            fSucceeded  = true;
                            break;
                        }

                        if (!fHaveEnUs && strBest.empty())
                        {
                            strBest = strCandidate;
                        }
                    }

                    if (!fSucceeded && !strBest.empty())
                    {
                        strFullName = move (strBest);
                        fSucceeded  = true;
                    }
                }
            }
        }
    }

    return fSucceeded;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontRegistrar::RegisterFontInSystem
//
//  Add a value to the system font registry key so DirectWrite (and Settings >
//  Fonts) can discover the installed font.
//
////////////////////////////////////////////////////////////////////////////////

void CNerdFontRegistrar::RegisterFontInSystem (LPCWSTR pszDestPath, LPCWSTR pszFileName)
{
    CAutoRegKey hkey;
    wstring     strFullName;
    wstring     strValueName;



    if (GetTtfFullName (pszDestPath, strFullName)
        && !strFullName.empty()
        && RegOpenKeyExW (HKEY_LOCAL_MACHINE, NerdFontConst::kpszFontsRegKey, 0, KEY_SET_VALUE, hkey.GetRef()) == ERROR_SUCCESS)
    {
        strValueName = strFullName + NerdFontConst::kpszTrueTypeSuffix;

        RegSetValueExW (hkey,
                        strValueName.c_str(),
                        0,
                        REG_SZ,
                        reinterpret_cast<const BYTE *> (pszFileName),
                        static_cast<DWORD> ((wcslen (pszFileName) + 1) * sizeof (wchar_t)));
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontRegistrar::UnregisterFontFromSystem
//
//  Remove every system font registry value whose data matches pszFileName.
//
////////////////////////////////////////////////////////////////////////////////

void CNerdFontRegistrar::UnregisterFontFromSystem (LPCWSTR pszFileName)
{
    CAutoRegKey     hkey;
    vector<wstring> rgToDelete;



    if (RegOpenKeyExW (HKEY_LOCAL_MACHINE, NerdFontConst::kpszFontsRegKey, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, hkey.GetRef()) == ERROR_SUCCESS)
    {
        for (DWORD dwIndex = 0; ; ++dwIndex)
        {
            wchar_t szName[256]          = { };
            BYTE    szData[MAX_PATH * 2] = { };
            DWORD   cchName              = ARRAYSIZE (szName);
            DWORD   cbData               = sizeof (szData);
            DWORD   dwType               = 0;
            LONG    lResult              = RegEnumValueW (hkey, dwIndex, szName, &cchName, NULL, &dwType, szData, &cbData);



            if (lResult == ERROR_NO_MORE_ITEMS)
            {
                break;
            }

            if (lResult == ERROR_SUCCESS
                && dwType == REG_SZ
                && _wcsicmp (reinterpret_cast<LPCWSTR> (szData), pszFileName) == 0)
            {
                rgToDelete.push_back (szName);
            }
        }

        for (const wstring & strName : rgToDelete)
        {
            RegDeleteValueW (hkey, strName.c_str());
        }
    }
}





