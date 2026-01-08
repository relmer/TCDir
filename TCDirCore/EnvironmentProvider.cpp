#include "pch.h"

#include "EnvironmentProvider.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CEnvironmentProvider::TryGetEnvironmentVariable
//
////////////////////////////////////////////////////////////////////////////////

bool CEnvironmentProvider::TryGetEnvironmentVariable (LPCWSTR pszName, wstring & value) const
{
    HRESULT hr         = S_OK;
    bool    fIsSet     = false;
    DWORD   cchNeeded  = 0;
    DWORD   cchWritten = 0;



    cchNeeded = GetEnvironmentVariableW (pszName, nullptr, 0);
    if (cchNeeded == 0)
    {
        DWORD dwLastError = GetLastError();
        BAIL_OUT_IF (dwLastError == ERROR_ENVVAR_NOT_FOUND, S_OK);
        CWRA (cchNeeded != 0);
    }

    CBRAEx (cchNeeded > 1, E_UNEXPECTED);
    value.resize (cchNeeded - 1, L'\0');

    cchWritten = GetEnvironmentVariableW (pszName, value.data(), cchNeeded);
    CWRA (cchWritten != 0);
    CWRA (cchWritten == (cchNeeded - 1));

    value.resize (cchWritten);
    fIsSet = true;



Error:
    return fIsSet;
}
