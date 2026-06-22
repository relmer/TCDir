#pragma once



////////////////////////////////////////////////////////////////////////////////
//
//  INerdFontSystemAccess
//
//  Injectable abstraction over the registry accesses used by Nerd Font
//  profile configuration helpers.  Production code uses
//  CNerdFontSystemAccessReal; tests substitute CNfSystemAccessMock.
//
////////////////////////////////////////////////////////////////////////////////

class INerdFontSystemAccess
{
public:
    virtual ~INerdFontSystemAccess() = default;

    virtual LONG  OpenRegKey         (HKEY hRoot, LPCWSTR pszSubKey, REGSAM sam, HKEY & hKeyOut) = 0;
    virtual LONG  CreateOrOpenRegKey (HKEY hRoot, LPCWSTR pszSubKey, REGSAM sam, HKEY & hKeyOut) = 0;
    virtual LONG  QueryRegString     (HKEY hKey, LPCWSTR pszValue, wstring & strOut) = 0;
    virtual LONG  SetRegString       (HKEY hKey, LPCWSTR pszValue, LPCWSTR pszStr) = 0;
    virtual LONG  SetRegDword        (HKEY hKey, LPCWSTR pszValue, DWORD dw) = 0;
    virtual void  DeleteRegValue     (HKEY hKey, LPCWSTR pszValue) = 0;
    virtual void  CloseRegKey        (HKEY hKey) = 0;
};





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontSystemAccessReal
//
//  Production implementation — delegates directly to Win32 APIs.
//
////////////////////////////////////////////////////////////////////////////////

class CNerdFontSystemAccessReal : public INerdFontSystemAccess
{
public:
    LONG OpenRegKey         (HKEY hRoot, LPCWSTR pszSubKey, REGSAM sam, HKEY & hKeyOut) override;
    LONG CreateOrOpenRegKey (HKEY hRoot, LPCWSTR pszSubKey, REGSAM sam, HKEY & hKeyOut) override;
    LONG QueryRegString     (HKEY hKey, LPCWSTR pszValue, wstring & strOut) override;
    LONG SetRegString       (HKEY hKey, LPCWSTR pszValue, LPCWSTR pszStr) override;
    LONG SetRegDword        (HKEY hKey, LPCWSTR pszValue, DWORD dw) override;
    void DeleteRegValue     (HKEY hKey, LPCWSTR pszValue) override;
    void CloseRegKey        (HKEY hKey) override;
};





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontSystemAccessReal::OpenRegKey
//
////////////////////////////////////////////////////////////////////////////////

inline LONG CNerdFontSystemAccessReal::OpenRegKey (HKEY hRoot, LPCWSTR pszSubKey, REGSAM sam, HKEY & hKeyOut)
{
    return RegOpenKeyExW (hRoot, pszSubKey, 0, sam, &hKeyOut);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontSystemAccessReal::CreateOrOpenRegKey
//
////////////////////////////////////////////////////////////////////////////////

inline LONG CNerdFontSystemAccessReal::CreateOrOpenRegKey (HKEY hRoot, LPCWSTR pszSubKey, REGSAM sam, HKEY & hKeyOut)
{
    DWORD dwDisp = 0;
    return RegCreateKeyExW (hRoot, pszSubKey, 0, NULL, REG_OPTION_NON_VOLATILE, sam, NULL, &hKeyOut, &dwDisp);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontSystemAccessReal::QueryRegString
//
////////////////////////////////////////////////////////////////////////////////

inline LONG CNerdFontSystemAccessReal::QueryRegString (HKEY hKey, LPCWSTR pszValue, wstring & strOut)
{
    HRESULT hr      = S_OK;
    wchar_t szBuf[LF_FACESIZE] = { };
    DWORD   dwType  = 0;
    DWORD   cbData  = sizeof (szBuf);
    LONG    lResult = ERROR_SUCCESS;



    lResult = RegQueryValueExW (hKey, pszValue, NULL, &dwType, reinterpret_cast<LPBYTE>(szBuf), &cbData);
    CBR (lResult == ERROR_SUCCESS && dwType == REG_SZ);

    strOut = szBuf;

Error:
    return lResult;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontSystemAccessReal::SetRegString
//
////////////////////////////////////////////////////////////////////////////////

inline LONG CNerdFontSystemAccessReal::SetRegString (HKEY hKey, LPCWSTR pszValue, LPCWSTR pszStr)
{
    return RegSetValueExW (hKey,
                           pszValue,
                           0,
                           REG_SZ,
                           reinterpret_cast<const BYTE *>(pszStr),
                           static_cast<DWORD>((wcslen (pszStr) + 1) * sizeof (wchar_t)));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontSystemAccessReal::SetRegDword
//
////////////////////////////////////////////////////////////////////////////////

inline LONG CNerdFontSystemAccessReal::SetRegDword (HKEY hKey, LPCWSTR pszValue, DWORD dw)
{
    return RegSetValueExW (hKey, pszValue, 0, REG_DWORD, reinterpret_cast<const BYTE *>(&dw), sizeof (dw));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontSystemAccessReal::DeleteRegValue
//
////////////////////////////////////////////////////////////////////////////////

inline void CNerdFontSystemAccessReal::DeleteRegValue (HKEY hKey, LPCWSTR pszValue)
{
    RegDeleteValueW (hKey, pszValue);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontSystemAccessReal::CloseRegKey
//
////////////////////////////////////////////////////////////////////////////////

inline void CNerdFontSystemAccessReal::CloseRegKey (HKEY hKey)
{
    RegCloseKey (hKey);
}
