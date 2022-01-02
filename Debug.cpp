#include "stdafx.h"
#include "debug.h"





int DbgPrintf (LPCWSTR pszFormat, ...)
{
    int cchFormatted = 0;

#ifdef _DEBUG

    static const int   s_cchBuf             = 9999;  // Max console buffer width
    static  WCHAR      s_szBuf[s_cchBuf]    = { L'\0' };
    va_list            vaArgs               = 0;



    va_start(vaArgs, pszFormat);

    cchFormatted = vswprintf_s(s_szBuf, s_cchBuf, pszFormat, vaArgs);
    assert (cchFormatted >= 0);
    assert (cchFormatted <= s_cchBuf);

    OutputDebugString(s_szBuf);

#endif _DEBUG

    return cchFormatted;
}
