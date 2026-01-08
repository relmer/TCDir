#include "pch.h"
#include "Ehm.h"





////////////////////////////////////////////////////////////////////////////////
//
//  DEBUGMSG
//
////////////////////////////////////////////////////////////////////////////////

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





////////////////////////////////////////////////////////////////////////////////
//
//  RELEASEMSG
//
////////////////////////////////////////////////////////////////////////////////

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





EHM_BREAKPOINT_FUNC g_pfnBreakpoint = nullptr;





////////////////////////////////////////////////////////////////////////////////
//
//  SetBreakpointFunction
//
////////////////////////////////////////////////////////////////////////////////

void SetBreakpointFunction (EHM_BREAKPOINT_FUNC func)
{
    g_pfnBreakpoint = func;
}





////////////////////////////////////////////////////////////////////////////////
//
//  EhmBreakpoint
//
////////////////////////////////////////////////////////////////////////////////

void EhmBreakpoint (void)
{
    g_pfnBreakpoint ? g_pfnBreakpoint() : __debugbreak();
}
