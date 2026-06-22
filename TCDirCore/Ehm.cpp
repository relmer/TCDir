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
EHM_NOTIFY_FUNC     g_pfnNotify     = nullptr;





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





////////////////////////////////////////////////////////////////////////////////
//
//  SetNotifyFunction
//
////////////////////////////////////////////////////////////////////////////////

void SetNotifyFunction (EHM_NOTIFY_FUNC func)
{
    g_pfnNotify = func;
}





////////////////////////////////////////////////////////////////////////////////
//
//  EhmNotifyUser
//
////////////////////////////////////////////////////////////////////////////////

void EhmNotifyUser (const wchar_t * message)
{
    HANDLE hConsole = NULL;



    if (g_pfnNotify != nullptr)
    {
        g_pfnNotify (message);
        return;
    }

    hConsole = GetStdHandle (STD_ERROR_HANDLE);

    if (hConsole != NULL && hConsole != INVALID_HANDLE_VALUE)
    {
        fwprintf (stderr, L"Error: %s\n", message);
    }
    else
    {
        MessageBoxW (NULL, message, L"TCDir", MB_OK | MB_ICONERROR);
    }
}
