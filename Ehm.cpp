#include "stdafx.h"
#include "Ehm.h"





void DEBUGMSG (LPCWSTR pszFormat, ...)
{
#ifdef _DEBUG

    HRESULT hr          = S_OK;
    va_list vlArgs;
    WCHAR   szMsg[1024];



    va_start (vlArgs, pszFormat);
    hr = StringCchVPrintf (szMsg, ARRAYSIZE (szMsg), pszFormat, vlArgs);
    if (FAILED (hr))
    {
        EHM_BREAKPOINT;
    }
    else
    {
        OutputDebugString (szMsg);
    }

    va_end (vlArgs);

#else

    UNREFERENCED_PARAMETER (pszFormat);

#endif
}





void RELEASEMSG (LPCWSTR pszFormat, ...)
{
    HRESULT hr          = S_OK;
    va_list vlArgs;
    WCHAR   szMsg[1024];



    va_start (vlArgs, pszFormat);
    hr = StringCchVPrintf (szMsg, ARRAYSIZE (szMsg), pszFormat, vlArgs);
    if (FAILED (hr))
    {
        EHM_BREAKPOINT;
    }
    else
    {
        OutputDebugString (szMsg);
    }

    va_end (vlArgs);
}
