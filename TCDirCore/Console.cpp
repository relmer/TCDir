#include "pch.h"
#include "Console.h"
#include "AnsiCodes.h"





////////////////////////////////////////////////////////////////////////////////  
//  
//  CConsole::CConsole  
//  
//  
////////////////////////////////////////////////////////////////////////////////  

CConsole::CConsole(void)
{  
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
    // Use ANSI reset sequence to restore terminal to default colors
    // This only affects future output, not already-rendered text
    m_strBuffer.append (AnsiCodes::RESET_ALL);
    Flush();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::Initialize
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

HRESULT CConsole::Initialize (shared_ptr<CConfig> configPtr)
{
    HRESULT hr = S_OK;



    CBRAEx (configPtr != nullptr, E_INVALIDARG);



    m_strBuffer.reserve (s_kcchInitialBufferSize);



    m_hStdOut = GetStdHandle (STD_OUTPUT_HANDLE);
    CWRA (m_hStdOut != INVALID_HANDLE_VALUE);
    
    hr = InitializeConsoleMode();
    CHR (hr);

    hr = InitializeConsoleWidth();
    CHR (hr);



    m_configPtr = configPtr;
    m_configPtr->Initialize (m_attrDefault);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::InitializeConsoleMode
//
////////////////////////////////////////////////////////////////////////////////  

HRESULT CConsole::InitializeConsoleMode ()
{
    HRESULT hr       = S_OK;
    BOOL    fSuccess = FALSE;
    DWORD   mode     = 0;



    // Try to enable virtual terminal processing for ANSI escape sequences
    fSuccess = GetConsoleMode (m_hStdOut, &mode);
    CWREx (fSuccess, S_OK);
    
    // If GetConsoleMode succeeded, the console is not redirected
    m_fIsRedirected = false;

    mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    fSuccess = SetConsoleMode (m_hStdOut, mode);
    CWRA (fSuccess);


Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::InitializeConsoleWidth
//
////////////////////////////////////////////////////////////////////////////////  

HRESULT CConsole::InitializeConsoleWidth (void)
{
    HRESULT                    hr       = S_OK;
    BOOL                       fSuccess = FALSE;
    CONSOLE_SCREEN_BUFFER_INFO csbi     = { 0 };



    // Can't query the width if the console is redirected.
    BAIL_OUT_IF (m_fIsRedirected, S_OK);

    fSuccess = GetConsoleScreenBufferInfo (m_hStdOut, &csbi);
    CWRA (fSuccess);

    m_cxConsoleWidth = csbi.srWindow.Right - csbi.srWindow.Left + 1;

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

void CConsole::Puts (int attributeIndex, LPCWSTR psz)
{
    SetColor (m_configPtr->m_rgAttributes[attributeIndex]);

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

int CConsole::Printf (CConfig::EAttribute attributeIndex, LPCWSTR pszFormat, ...)
{
    static constexpr int s_cchBuf          = 9999;  // Max console buffer width
    static  WCHAR        s_szBuf[s_cchBuf] = { L'\0' };

    HRESULT   hr     = S_OK;
    va_list   vaArgs = 0;  
    LPWSTR    pszEnd = nullptr;



    va_start (vaArgs, pszFormat);
    
    hr = StringCchVPrintfEx (s_szBuf, s_cchBuf, &pszEnd, nullptr, 0, pszFormat, vaArgs);
    CHRA (hr);

    SetColor (m_configPtr->m_rgAttributes[attributeIndex]);
    m_strBuffer.append (s_szBuf);

Error:
    va_end (vaArgs);

    return SUCCEEDED (hr) ? (int) (pszEnd - s_szBuf) : 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::Printf
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

int CConsole::Printf (const WIN32_FIND_DATA & wfd, LPCWSTR pszFormat, ...)
{
    static constexpr int s_cchBuf          = 9999;  // Max console buffer width
    static  WCHAR        s_szBuf[s_cchBuf] = { L'\0' };

    HRESULT   hr       = S_OK;
    va_list   vaArgs   = 0;  
    LPWSTR    pszEnd   = nullptr;
    WORD      textAttr = m_configPtr->GetTextAttrForFile (wfd);
    


    va_start (vaArgs, pszFormat);
    
    hr = StringCchVPrintfEx (s_szBuf, s_cchBuf, &pszEnd, nullptr, 0, pszFormat, vaArgs);
    CHRA (hr);

    SetColor (textAttr);
    m_strBuffer.append (s_szBuf);

Error:
    va_end (vaArgs);

    return SUCCEEDED (hr) ? (int) (pszEnd - s_szBuf) : 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::WriteSeparatorLine
//
//  Draws a line across the console at the current internal cursor position
//  and moves to the next line
// 
////////////////////////////////////////////////////////////////////////////////  

void CConsole::WriteSeparatorLine (WORD /* attr */)
{
    //SetColor (attr);
    //
    //m_strBuffer.append (m_consoleScreenBufferInfoEx.dwSize.X, L'═');
    //m_strBuffer.append (L"\n");
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



    if (!m_fIsRedirected)
    {
        fSuccess = WriteConsole (m_hStdOut, m_strBuffer.c_str (), cch, &cch, nullptr);
        CWRA (fSuccess);
    }
    else
    {
        // Need to use WriteFile since WriteConsole is not valid for redirected output (e.g., in unit tests).
        //
        // Convert wide string to UTF-8 for proper output
        // ANSI sequences will be preserved in the UTF-8 output (they're ASCII-compatible)
        int         cb           = 0;
        DWORD       bytesWritten = 0;
        std::string utf8Buffer;
            


        cb = WideCharToMultiByte (CP_UTF8, 
                                  0, 
                                  m_strBuffer.c_str(), 
                                  (int) cch, 
                                  nullptr, 
                                  0, 
                                  nullptr, 
                                  nullptr);
        CWRA (cb > 0);

        utf8Buffer.resize (cb);
        cb = WideCharToMultiByte (CP_UTF8, 
                                    0, 
                                    m_strBuffer.c_str(), 
                                    (int) cch, 
                                    utf8Buffer.data(), 
                                    cb, 
                                    nullptr, 
                                    nullptr);

        fSuccess = WriteFile (m_hStdOut, utf8Buffer.data (), cb, &bytesWritten, nullptr);
        CWRA (fSuccess);
    }

    m_strBuffer.clear();

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::SetColor
//
//  Converts Windows console attributes to ANSI SGR (Select Graphic Rendition) codes
//
////////////////////////////////////////////////////////////////////////////////  

void CConsole::SetColor (WORD attr)
{
    static WORD s_wPrevAttr = (WORD) -1;

    int nForegroundColor = 0;
    int nBackgroundColor = 0;
    int nBaseColorIndex  = 0;
    int nAnsiForeground  = 0;
    int nAnsiBackground  = 0;



    // Nothing to do if the color is unchanged
    if (attr == s_wPrevAttr)
    {
        return; 
    }

    s_wPrevAttr = attr;

    // Extract foreground and background color components from Windows console attribute
    nForegroundColor = attr & 0x0F;         // Lower 4 bits (0-15)
    nBackgroundColor = (attr & 0xF0) >> 4;  // Upper 4 bits (0-15)

    //
    // Map Windows foreground color to ANSI foreground code
    //

    nBaseColorIndex = nForegroundColor & 0x07;  // Get base color index (0-7)
    nAnsiForeground = AnsiCodes::CONSOLE_COLOR_TO_ANSI_COLOR[nBaseColorIndex];
    
    if (nForegroundColor & 0x08)  // Check intensity/bright bit
    {
        nAnsiForeground += AnsiCodes::BRIGHT_OFFSET;  // Convert to bright (90-97)
    }

    //
    // Map Windows background color to ANSI background code
    //

    nBaseColorIndex = nBackgroundColor & 0x07;  // Get base color index (0-7)
    nAnsiBackground = AnsiCodes::CONSOLE_COLOR_TO_ANSI_COLOR[nBaseColorIndex] + AnsiCodes::BG_OFFSET;
    
    if (nBackgroundColor & 0x08)  // Check intensity/bright bit
    {
        nAnsiBackground += AnsiCodes::BRIGHT_OFFSET;  // Convert to bright (100-107)
    }

    //
    // Build and append ANSI SGR color sequence
    // Format: ESC [ <foreground> ; <background> m
    // Example: "\x1b[91;40m" = bright red text on black background
    //

    m_strBuffer.append (format (AnsiCodes::SGR_COLOR_FORMAT, nAnsiForeground, nAnsiBackground));
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
