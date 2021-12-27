#include "StdAfx.h"
#include "ConsoleBuffer.h"

#include "Config.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CConsoleBuffer::CConsoleBuffer
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CConsoleBuffer::CConsoleBuffer(void)
{
    m_hStdOut = GetStdHandle (STD_OUTPUT_HANDLE);   

    ZeroMemory (&m_consoleScreenBufferInfoEx, sizeof (m_consoleScreenBufferInfoEx));
    m_consoleScreenBufferInfoEx.cbSize = sizeof (m_consoleScreenBufferInfoEx);

    GetConsoleScreenBufferInfoEx (m_hStdOut, &m_consoleScreenBufferInfoEx);

    m_coord = m_consoleScreenBufferInfoEx.dwCursorPosition;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsoleBuffer::~CConsoleBuffer
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CConsoleBuffer::~CConsoleBuffer(void)
{
    //
    // Update the console cursor position with our final internal cursor position
    //

    SetConsoleCursorPosition (m_hStdOut, m_coord);

    HeapFree (GetProcessHeap (), 0, m_prgScreenBuffer);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsoleBuffer::InitializeBuffer
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConsoleBuffer::InitializeBuffer (void)
{
    HRESULT hr = S_OK;
    COORD      coordDestination = { 0, 0 };
    SMALL_RECT srRead = { 0 };

    //
    // Allocate an off-screen buffer
    //

    m_cScreenBuffer = m_consoleScreenBufferInfoEx.dwSize.X * m_consoleScreenBufferInfoEx.dwSize.Y;
    m_prgScreenBuffer = (CHAR_INFO *) HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, m_cScreenBuffer * sizeof (*m_prgScreenBuffer));
    CPR (m_prgScreenBuffer);

    //
    // Initialize it with the current screen buffer's contents
    //

    srRead.Top    = 0;
    srRead.Left   = 0;
    srRead.Bottom = m_consoleScreenBufferInfoEx.dwSize.Y;
    srRead.Right  = m_consoleScreenBufferInfoEx.dwSize.X;

    ReadConsoleOutput (m_hStdOut, m_prgScreenBuffer, m_consoleScreenBufferInfoEx.dwSize, coordDestination, &srRead);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsoleBuffer::FlushBuffer
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConsoleBuffer::FlushBuffer (void)
{
    HRESULT hr = S_OK;
    COORD      coordDestination = { 0, 0 };
    SMALL_RECT srWrite = { 0 };

    //
    // Write the screen buffer to the console
    //

    srWrite.Top    = 0;
    srWrite.Left   = 0;
    srWrite.Bottom = m_consoleScreenBufferInfoEx.dwSize.Y;
    srWrite.Right  = m_consoleScreenBufferInfoEx.dwSize.X;

    WriteConsoleOutput (m_hStdOut, m_prgScreenBuffer, m_consoleScreenBufferInfoEx.dwSize, coordDestination, &srWrite);

    HeapFree (GetProcessHeap (), 0, m_prgScreenBuffer);
    m_prgScreenBuffer = NULL;
    m_cScreenBuffer = 0;


//Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsoleBuffer::Puts
//
//  Write a simple string to the console
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConsoleBuffer::Puts (WORD attr, LPCWSTR psz)
{
    HRESULT       hr = S_OK;
    const WCHAR * pStart = psz;
    const WCHAR * pEnd = psz + wcslen(psz);
    const WCHAR * p = pStart;



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
//  CConsoleBuffer::Printf
//
//  
//
////////////////////////////////////////////////////////////////////////////////

int CConsoleBuffer::Printf (WORD attr, LPCWSTR pszFormat, ...)
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
//  CConsoleBuffer::WriteString
//
//  Writes a string to the console using the given attributes at the location 
//  specified by the internal m_coord member and updates the interal cursor 
//  position.  This low-level function writes directly to the console buffer 
//  and does not interpret newline characters or scroll the window.
// 
////////////////////////////////////////////////////////////////////////////////

HRESULT CConsoleBuffer::WriteString (WORD attr, __in_ecount(cch) WCHAR * p, size_t cch)
{
    HRESULT     hr              = S_OK;
    WCHAR     * pEnd            = p + cch;
    CHAR_INFO * pciBuffer       = m_prgScreenBuffer + ((size_t) m_coord.Y * m_consoleScreenBufferInfoEx.dwSize.X) + m_coord.X;
    CHAR_INFO * pciBufferEnd    = m_prgScreenBuffer + m_cScreenBuffer;
    size_t      idx             = 0;



    while (p < pEnd)
    {
        pciBuffer->Attributes       = attr;
        pciBuffer->Char.UnicodeChar = *p;

        ++p;
        ++pciBuffer;

        CBRAEx (pciBuffer < pciBufferEnd, E_NOT_SUFFICIENT_BUFFER)
    }

    idx = pciBuffer - m_prgScreenBuffer;

    m_coord.Y = (short) (idx / m_consoleScreenBufferInfoEx.dwSize.X);
    m_coord.X = (short) (idx % m_consoleScreenBufferInfoEx.dwSize.X);


Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsoleBuffer::WriteSeparatorLine
//
//  Draws a line across the console at the current internal cursor position
//  and moves to the next line
// 
////////////////////////////////////////////////////////////////////////////////

HRESULT CConsoleBuffer::WriteSeparatorLine (void)
{
    HRESULT hr     = S_OK;
    size_t  idx    = (size_t) m_coord.Y * (size_t) m_consoleScreenBufferInfoEx.dwSize.X;
    size_t  idxEnd = idx + m_consoleScreenBufferInfoEx.dwSize.X;



    while (idx < idxEnd)
    {
        m_prgScreenBuffer[idx].Attributes       = m_rgAttributes[CConfig::EAttribute::SeparatorLine];
        m_prgScreenBuffer[idx].Char.UnicodeChar = L'═';

        ++idx;
    }

    ++m_coord.Y;

//Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsoleBuffer::ScrollBuffer
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConsoleBuffer::ScrollBuffer (UINT cLinesToScroll)
{
    HRESULT     hr                  = S_OK;
    CHAR_INFO   ciFill              = { 0 };
    size_t      cbScreenBuffer      = sizeof(*m_prgScreenBuffer) * (size_t)m_consoleScreenBufferInfoEx.dwSize.Y * (size_t)m_consoleScreenBufferInfoEx.dwSize.X;
    size_t      idxNewTopOfBuffer   = cLinesToScroll * (size_t) m_consoleScreenBufferInfoEx.dwSize.X;
    size_t      cUsedBufferLines    = (size_t)m_consoleScreenBufferInfoEx.dwSize.Y - cLinesToScroll;
    size_t      cci                 = cUsedBufferLines * (size_t) m_consoleScreenBufferInfoEx.dwSize.X;
    CHAR_INFO * pNewEmptyArea       = m_prgScreenBuffer + (cUsedBufferLines * (size_t) m_consoleScreenBufferInfoEx.dwSize.X);
    CHAR_INFO * pEnd                = m_prgScreenBuffer + ((size_t) m_consoleScreenBufferInfoEx.dwSize.Y * (size_t) m_consoleScreenBufferInfoEx.dwSize.X);


    ciFill.Attributes = m_rgAttributes[CConfig::EAttribute::Default];
    ciFill.Char.UnicodeChar = L'-';

    memmove_s (m_prgScreenBuffer, cbScreenBuffer, m_prgScreenBuffer + idxNewTopOfBuffer, cci * sizeof(*m_prgScreenBuffer));
    
    while (pNewEmptyArea < pEnd)
    {
        *pNewEmptyArea = ciFill;
        ++pNewEmptyArea;
    }

    m_coord.Y = (short) (idxNewTopOfBuffer / m_consoleScreenBufferInfoEx.dwSize.X);
    m_coord.X = (short) (idxNewTopOfBuffer % m_consoleScreenBufferInfoEx.dwSize.X);

    m_coord.Y -= (short) cLinesToScroll;

//Error:
    return hr;
}
