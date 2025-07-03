#include "pch.h"
#include "Console.h"





////////////////////////////////////////////////////////////////////////////////  
//  
//  CConsole::CConsole  
//  
//  
////////////////////////////////////////////////////////////////////////////////  

CConsole::CConsole(void) :  
    m_consoleScreenBufferInfoEx (sizeof (m_consoleScreenBufferInfoEx)),  
    m_attrDefault               (0),
    m_coord                     ({0, 0}),
    m_hStdOut                   (nullptr)
{  
    // Initialization of m_consoleScreenBufferInfoEx above assumes cbSize is the first member  
    static_assert (offsetof (CONSOLE_SCREEN_BUFFER_INFOEX, cbSize) == 0);  
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::~CConsole
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CConsole::~CConsole (void)
{
    Flush();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::Initialize
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConsole::Initialize (void)
{
    HRESULT hr       = S_OK;
    BOOL    fSuccess = FALSE;
    DWORD   mode     = 0;



    m_hStdOut = GetStdHandle (STD_OUTPUT_HANDLE);

    fSuccess = GetConsoleScreenBufferInfoEx (m_hStdOut, &m_consoleScreenBufferInfoEx);
    CWRA (fSuccess);

    m_attrDefault = m_consoleScreenBufferInfoEx.wAttributes;
    m_coord       = m_consoleScreenBufferInfoEx.dwCursorPosition;

    m_strBuffer.reserve (s_kcchInitialBufferSize);

    fSuccess = GetConsoleMode (m_hStdOut, &mode);
    CWRA (fSuccess);

    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    fSuccess = SetConsoleMode (m_hStdOut, mode);
    CWRA (fSuccess);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::Puts
//
//  Write a single line to the console (no format string)
//
////////////////////////////////////////////////////////////////////////////////

void CConsole::Puts (WORD attr, LPCWSTR psz)
{
    SetColor (attr);
    
    m_strBuffer.append (psz);
    m_strBuffer.append (L"\n");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::Printf
//
//  
//
////////////////////////////////////////////////////////////////////////////////

int CConsole::Printf (WORD attr, LPCWSTR pszFormat, ...)
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

    SetColor (attr);
    m_strBuffer.append (s_szBuf);

Error:
    va_end (vaArgs);

    return SUCCEEDED (hr) ? cchFormatted : 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::WriteConsoleLine
//
//  Draws a line across the console at the current internal cursor position
//  and moves to the next line
// 
////////////////////////////////////////////////////////////////////////////////

void CConsole::WriteSeparatorLine (WORD attr)
{
    SetColor (attr);
    
    m_strBuffer.append (m_consoleScreenBufferInfoEx.dwSize.X, L'═');
    m_strBuffer.append (L"\n");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::Flush
//
// 
//  
////////////////////////////////////////////////////////////////////////////////

HRESULT CConsole::Flush (void)
{
    HRESULT  hr       = S_OK;
    BOOL     fSuccess = FALSE;
    DWORD    cch      = (DWORD) m_strBuffer.length();



    fSuccess = WriteConsole (m_hStdOut, m_strBuffer.c_str(), cch, &cch, nullptr);
    CWRA (fSuccess);

    m_strBuffer.clear();

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::SetColor
//
//
// 
////////////////////////////////////////////////////////////////////////////////

void CConsole::SetColor(WORD attr)
{
    // Mapping table for base colors (0-7): Black, Blue, Green, Cyan, Red, Magenta, Yellow, White
    static const int s_knAnsiColor[] = { 30, 34, 32, 36, 31, 35, 33, 37 };

    int foreground      = attr & 0x0F;
    int background      = (attr & 0xF0) >> 4;
    int idx             = 0;
    int ansiForeground  = 0;
    int ansiBackground  = 0;



    // Foreground
    idx = foreground & 0x07;
    ansiForeground = s_knAnsiColor[idx];
    if (foreground & 0x08)
    {
        ansiForeground += 60; // Bright
    }

    // Background
    idx = background & 0x07;
    ansiBackground = s_knAnsiColor[idx] + 10;
    if (background & 0x08)
    {
        ansiBackground += 60; // Bright
    }

    // Build ANSI sequence
    m_strBuffer.append (format (L"\x1b[{};{}m", ansiForeground, ansiBackground));
}





#ifdef _DEBUG
////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::Test
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CConsole::Test (void)
{
    //TestCanWriteMoreLinesThanWindowSize();
    //TestCanScrollWindow();

    //exit (0);
}
#endif _DEBUG
