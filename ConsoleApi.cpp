#include "StdAfx.h"
#include "ConsoleApi.h"

#include "Color.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CConsoleApi::CConsoleApi
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CConsoleApi::CConsoleApi(void)
{
    m_hStdOut = GetStdHandle (STD_OUTPUT_HANDLE);   

    ZeroMemory (&m_consoleScreenBufferInfoEx, sizeof (m_consoleScreenBufferInfoEx));
    m_consoleScreenBufferInfoEx.cbSize = sizeof (m_consoleScreenBufferInfoEx);

    GetConsoleScreenBufferInfoEx (m_hStdOut, &m_consoleScreenBufferInfoEx);

    m_coord = m_consoleScreenBufferInfoEx.dwCursorPosition;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsoleApi::~CConsoleApi
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CConsoleApi::~CConsoleApi(void)
{
    //
    // Update the console cursor position with our final internal cursor position
    //

    SetConsoleCursorPosition (m_hStdOut, m_coord);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsoleApi::Puts
//
//  Write a simple string to the console
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConsoleApi::Puts (WORD attr, LPCWSTR psz)
{
    HRESULT       hr        = S_OK;
    const WCHAR * pStart    = psz;
    const WCHAR * pEnd      = psz + wcslen(psz);
    const WCHAR * p         = pStart;



    //
    // WriteConsoleOutputCharacter does not support newline characters, so
    // we'll have to handle those manually
    //
    while (p < pEnd)
    {
        if (*p == L'\n')
        {
            hr = WriteString (attr, (LPWSTR) pStart, p - pStart);
            CHR (hr);

            pStart = p + 1;

            // Handle the newline
            m_coord.X = 0;
            ++m_coord.Y;
        }

        ++p;
    }

    hr = WriteString (attr, (LPWSTR) pStart, p - pStart);
    CHR (hr);


Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsoleApi::Printf
//
//  
//
////////////////////////////////////////////////////////////////////////////////

int CConsoleApi::Printf (WORD attr, LPCWSTR pszFormat, ...)
{
    static const int   s_cchBuf          = 9999;  // Max console buffer width
    static  WCHAR      s_szBuf[s_cchBuf] = { L'\0' };

    HRESULT   hr              = S_OK;
    va_list   vaArgs          = 0;  
    int       cchFormatted    = 0; 



    va_start (vaArgs, pszFormat);
    
    cchFormatted = vswprintf_s (s_szBuf, s_cchBuf, pszFormat, vaArgs);
    CBRA (cchFormatted >= 0);
    CBRA (cchFormatted <= s_cchBuf);

    hr = Puts (attr, s_szBuf);
    CHR (hr);



Error:
    return SUCCEEDED (hr) ? cchFormatted : 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsoleApi::WriteString
//
//  Writes a string to the console using the given attributes at the location 
//  specified by the internal m_coord member and updates the interal cursor 
//  position.  This low-level function writes directly to the console buffer 
//  and does not interpret newline characters or scroll the window.
// 
////////////////////////////////////////////////////////////////////////////////

HRESULT CConsoleApi::WriteString (WORD attr, __in_ecount(cch) WCHAR * p, size_t cch)
{
    HRESULT hr       = S_OK;
    BOOL    fSuccess = FALSE;
    DWORD   cWritten = 0;



    fSuccess = FillConsoleOutputAttribute (m_hStdOut, attr, (DWORD) cch, m_coord, &cWritten);
    CWRA (fSuccess);

    fSuccess = WriteConsoleOutputCharacter (m_hStdOut, p, (DWORD) cch, m_coord, &cWritten);
    CWRA (fSuccess);

    m_coord.Y += (m_coord.X + (SHORT) cWritten) / m_consoleScreenBufferInfoEx.dwSize.X;
    m_coord.X += cWritten % m_consoleScreenBufferInfoEx.dwSize.X;



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsoleApi::WriteConsoleLine
//
//  Draws a line across the console at the current internal cursor position
//  and moves to the next line
// 
////////////////////////////////////////////////////////////////////////////////

HRESULT CConsoleApi::WriteSeparatorLine (void)
{
    HRESULT hr          = S_OK;
    BOOL    fSuccess    = FALSE;
    DWORD   bufferWidth = m_consoleScreenBufferInfoEx.dwSize.X;
    DWORD   cWritten    = 0;


    assert (m_coord.X == 0);

    fSuccess = FillConsoleOutputAttribute (m_hStdOut, m_rgAttributes[CConfig::EAttribute::SeparatorLine], bufferWidth, m_coord, &cWritten);
    CWRA (fSuccess);

    fSuccess = FillConsoleOutputCharacter (m_hStdOut, L'═', bufferWidth, m_coord, &cWritten);
    CWRA (fSuccess);

    ++m_coord.Y;

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsoleApi::ScrollBuffer
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConsoleApi::ScrollBuffer (UINT cLinesToScroll)
{
    HRESULT    hr               = S_OK;
    CHAR_INFO  ciFill           = { 0 };
    BOOL       fSuccess         = FALSE;
    SMALL_RECT srcScrollRect    = { 0 };
    COORD      coordDest        = { 0 };
    COORD      coordCursorPos   = { 0 };



    ciFill.Attributes       = m_rgAttributes[CConfig::EAttribute::Default];
    ciFill.Char.UnicodeChar = L'-';

    srcScrollRect.Top    = (short) cLinesToScroll;
    srcScrollRect.Bottom = m_consoleScreenBufferInfoEx.dwSize.Y - 1;
    srcScrollRect.Left   = 0;
    srcScrollRect.Right  = m_consoleScreenBufferInfoEx.dwSize.X - 1;

    fSuccess = ScrollConsoleScreenBuffer (m_hStdOut, &srcScrollRect, NULL, coordDest, &ciFill);
    CWRA (fSuccess);

    // this looks wrong...
    //m_coord.Y = srcScrollRect.Top;

    m_coord.Y -= (short) cLinesToScroll;

Error:
    return hr;
}
