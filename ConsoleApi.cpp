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

CConsoleApi::CConsoleApi (void)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsoleApi::~CConsoleApi
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CConsoleApi::~CConsoleApi (void)
{
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
    HRESULT hr        = S_OK;
    BOOL    fSuccess  = FALSE;
    BOOL    fClipped  = FALSE;
    DWORD   cWritten  = 0;



    //
    // If we're already past the buffer's width, there's no room to write anything
    //

    BAIL_OUT_IF (m_coord.X >= m_consoleScreenBufferInfoEx.dwSize.X, S_OK);

    //
    // If we're clipping text that would otherwise cause wrapping, 
    // set cch to the max count to actually write.
    //

    if (m_wrapMode == EWrapMode::Clip)
    {
        assert (m_coord.X <= m_consoleScreenBufferInfoEx.dwSize.X);
        if (m_coord.X + cch >= m_consoleScreenBufferInfoEx.dwSize.X)
        {
            cch = m_consoleScreenBufferInfoEx.dwSize.X - m_coord.X;
            assert (cch > 0);            

            fClipped = TRUE;
        }
    }

    fSuccess = FillConsoleOutputAttribute(m_hStdOut, attr, (DWORD)cch, m_coord, &cWritten);
    CWRA(fSuccess);

    fSuccess = WriteConsoleOutputCharacter(m_hStdOut, p, (DWORD)cch, m_coord, &cWritten);
    CWRA(fSuccess);

    if (!fClipped)
    {
        m_coord.Y += (m_coord.X + (SHORT)cWritten) / m_consoleScreenBufferInfoEx.dwSize.X;
    }

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

HRESULT CConsoleApi::WriteSeparatorLine (WORD attr)
{
    HRESULT hr          = S_OK;
    BOOL    fSuccess    = FALSE;
    DWORD   bufferWidth = m_consoleScreenBufferInfoEx.dwSize.X;
    DWORD   cWritten    = 0;


    assert (m_coord.X == 0);

    fSuccess = FillConsoleOutputAttribute (m_hStdOut, attr, bufferWidth, m_coord, &cWritten);
    CWRA (fSuccess);

    fSuccess = FillConsoleOutputCharacter (m_hStdOut, L'═', bufferWidth, m_coord, &cWritten);
    CWRA (fSuccess);

    ++m_coord.Y;

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsoleApi::ReserveLines
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConsoleApi::ReserveLines (UINT cLines)
{
    HRESULT hr                      = S_OK;
    BOOL    fSuccess                = FALSE;
    UINT    cWindowLinesTotal       = 0;
    UINT    cBufferLinesRemaining   = 0;


    //
    // If there are enough lines left in the current window, no need to scroll
    //

    cWindowLinesTotal = m_consoleScreenBufferInfoEx.srWindow.Bottom - m_consoleScreenBufferInfoEx.srWindow.Top + 1;
    BAIL_OUT_IF((cLines + m_coord.Y) <= cWindowLinesTotal, S_OK);

    //
    // If we need more lines than are free in the buffer, scroll the buffer up to make room
    //

    cBufferLinesRemaining = m_consoleScreenBufferInfoEx.dwSize.Y - m_coord.Y;
    if (cLines > cBufferLinesRemaining)
    {
        ScrollBuffer(cLines - cBufferLinesRemaining);
    }

    //
    // If we're going to write past the bottom of the window, reposition the window 
    // to the bottom of the area we need
    //

    if (m_coord.Y + cLines > (UINT) m_consoleScreenBufferInfoEx.srWindow.Bottom)
    {
        SHORT      cLinesToMoveWindow = 0;
        SMALL_RECT srWindow = m_consoleScreenBufferInfoEx.srWindow;



        cLinesToMoveWindow = (SHORT) m_coord.Y + (SHORT)cLines - m_consoleScreenBufferInfoEx.srWindow.Bottom;

        // relative coords
        srWindow.Top    += cLinesToMoveWindow;
        srWindow.Bottom += cLinesToMoveWindow;

        fSuccess = SetConsoleWindowInfo(m_hStdOut, TRUE, &srWindow);
        CWRA(fSuccess);

        m_consoleScreenBufferInfoEx.srWindow = srWindow;
    }



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
    SMALL_RECT srScrollRect     = { 0 };
    COORD      coordDest        = { 0 };
    COORD      coordCursorPos   = { 0 };



    ciFill.Attributes       = m_attrDefault;
    ciFill.Char.UnicodeChar = L' ';

    srScrollRect.Top    = m_consoleScreenBufferInfoEx.dwSize.Y - (short) cLinesToScroll - 1;
    srScrollRect.Bottom = m_consoleScreenBufferInfoEx.dwSize.Y - 1;
    srScrollRect.Left   = 0;
    srScrollRect.Right  = m_consoleScreenBufferInfoEx.dwSize.X - 1;

    fSuccess = ScrollConsoleScreenBuffer (m_hStdOut, &srScrollRect, NULL, coordDest, &ciFill);
    CWRA (fSuccess);

    // this looks wrong...
    //m_coord.Y = srScrollRect.Top;

    m_coord.Y -= (short) cLinesToScroll;

Error:
    return hr;
}
