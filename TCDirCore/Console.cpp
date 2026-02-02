#include "pch.h"
#include "Console.h"

#include "Color.h"
#include "AnsiCodes.h"





////////////////////////////////////////////////////////////////////////////////  
//  
//  CConsole::CConsole  
//  
//  
////////////////////////////////////////////////////////////////////////////////  

CConsole::CConsole (void)
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

HRESULT CConsole::InitializeConsoleMode()
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
//  CConsole::Putchar
//
//  Write a single character to the console 
//
////////////////////////////////////////////////////////////////////////////////  

void CConsole::Putchar (WORD attr, WCHAR ch)
{
    SetColor (attr);
    m_strBuffer.push_back (ch);
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
    ProcessMultiLineStringWithAttribute (psz, m_configPtr->m_rgAttributes[attributeIndex]);
    
    // Reset to default color before final newline to prevent color bleeding
    SetColor (m_configPtr->m_rgAttributes[CConfig::EAttribute::Default]);
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
    constexpr int      k_cchBuf          = 9999;  // Max console buffer width
    thread_local WCHAR s_szBuf[k_cchBuf] = { L'\0' };

    HRESULT   hr     = S_OK;
    va_list   vaArgs = 0;  
    LPWSTR    pszEnd = nullptr;



    va_start (vaArgs, pszFormat);
    
    hr = StringCchVPrintfEx (s_szBuf, k_cchBuf, &pszEnd, nullptr, 0, pszFormat, vaArgs);
    CHRA (hr);

    ProcessMultiLineStringWithAttribute (s_szBuf, m_configPtr->m_rgAttributes[attributeIndex]);

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

int CConsole::Printf (WORD attr, LPCWSTR pszFormat, ...)
{
    constexpr int      k_cchBuf          = 9999;  // Max console buffer width
    thread_local WCHAR s_szBuf[k_cchBuf] = { L'\0' };

    HRESULT   hr       = S_OK;
    va_list   vaArgs   = 0;  
    LPWSTR    pszEnd   = nullptr;
    


    va_start (vaArgs, pszFormat);
    
    hr = StringCchVPrintfEx (s_szBuf, k_cchBuf, &pszEnd, nullptr, 0, pszFormat, vaArgs);
    CHRA (hr);

    ProcessMultiLineStringWithAttribute (s_szBuf, attr);



Error:
    va_end (vaArgs);

    return SUCCEEDED (hr) ? (int) (pszEnd - s_szBuf) : 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::ParseColorMarker
//
//  Parses a color marker at the given position in the text.
//  Format: {EAttributeName}
//  Returns true if a valid marker was found, false otherwise.
//
////////////////////////////////////////////////////////////////////////////////

bool CConsole::ParseColorMarker (wstring_view text, size_t pos, CConfig::EAttribute & outAttr, size_t & outMarkerLen)
{
    // Generate the lookup table from the X-Macro list in Config.h
    static const struct { LPCWSTR name; CConfig::EAttribute attr; } s_markers[] =
    {
        #define MARKER_ENTRY(name) { L## #name, CConfig::EAttribute::name },
        EATTRIBUTE_LIST(MARKER_ENTRY)
        #undef MARKER_ENTRY
    };

    static_assert (ARRAYSIZE (s_markers) == CConfig::EAttribute::__count,
                   "s_markers and EAttribute are out of sync");

    HRESULT      hr         = S_OK;
    bool         fFound     = false;
    size_t       endPos     = wstring_view::npos;
    wstring_view markerName;



    // Must start with '{'
    CBRAEx (pos < text.size () && text[pos] == L'{', E_UNEXPECTED);

    // Find the closing '}'
    endPos = text.find (L'}', pos + 1);
    CBRAEx (endPos != wstring_view::npos, E_UNEXPECTED);  // Unclosed color marker brace

    // Extract the marker name (between { and })
    markerName = text.substr (pos + 1, endPos - pos - 1);

    // Look up the marker name
    for (const auto & marker : s_markers)
    {
        if (markerName == marker.name)
        {
            outAttr      = marker.attr;
            outMarkerLen = endPos - pos + 1;  // Include both { and }

            fFound = true;
            BAIL_OUT_IF (true, S_OK);
        }
    }

    // Has closing brace but unknown marker name - developer error
    CBRAEx (FALSE, E_UNEXPECTED);  // Unknown color marker name


Error:
    return fFound;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::ColorPrint
//
//  Write a string with embedded color markers (no trailing newline).
//  Format: {EAttributeName} switches to that color.
//  Colors are "sticky" - they remain until the next marker.
//  Text before the first marker is emitted without changing color.
//
////////////////////////////////////////////////////////////////////////////////

void CConsole::ColorPrint (LPCWSTR psz)
{
    wstring_view text        = psz;
    size_t       chunkStart  = 0;
    WORD         currentAttr = m_configPtr->m_rgAttributes[CConfig::EAttribute::Default];



    while (chunkStart < text.size())
    {
        // Find the next potential marker
        size_t              chunkEnd  = text.find (L'{', chunkStart);
        size_t              chunkLen  = wstring_view::npos;
        CConfig::EAttribute newAttr   = CConfig::EAttribute::Default;
        size_t              markerLen = 0;



        if (chunkEnd != wstring_view::npos)
        {
            chunkLen = chunkEnd - chunkStart;
        }

        // Emit text before the marker
        ProcessMultiLineStringWithAttribute (text.substr (chunkStart, chunkLen), currentAttr);

        if (chunkEnd == wstring_view::npos)
        {
            // No more markers
            break;
        }

        if (ParseColorMarker (text, chunkEnd, newAttr, markerLen))
        {
            // Valid marker - switch colors
            currentAttr = m_configPtr->m_rgAttributes[newAttr];
            chunkStart  = chunkEnd + markerLen;
        }
        else
        {
            // Not a valid marker - emit the '{' as literal text
            // (ParseColorMarker already ASSERTs if there's an unknown marker name)
            ProcessMultiLineStringWithAttribute (text.substr (chunkEnd, 1), currentAttr);
            chunkStart = chunkEnd + 1;
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::ColorPuts
//
//  Write a string with embedded color markers, followed by a newline.
//
////////////////////////////////////////////////////////////////////////////////

void CConsole::ColorPuts (LPCWSTR psz)
{
    ColorPrint (psz);

    // Reset to default color before newline to prevent color bleeding
    SetColor (m_configPtr->m_rgAttributes[CConfig::EAttribute::Default]);
    m_strBuffer.append (L"\n");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::ColorPrintf
//
//  Format a string and then process it with ColorPrint (no trailing newline).
//
////////////////////////////////////////////////////////////////////////////////

void CConsole::ColorPrintf (LPCWSTR pszFormat, ...)
{
    constexpr int      k_cchBuf          = 9999;
    thread_local WCHAR s_szBuf[k_cchBuf] = { L'\0' };

    HRESULT hr     = S_OK;
    va_list vaArgs = 0;



    va_start (vaArgs, pszFormat);

    hr = StringCchVPrintfEx (s_szBuf, k_cchBuf, nullptr, nullptr, 0, pszFormat, vaArgs);

    va_end (vaArgs);

    if (SUCCEEDED (hr))
    {
        ColorPrint (s_szBuf);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::PrintColorfulString
//
//  Print a string, cycling through colors for each character  
//
////////////////////////////////////////////////////////////////////////////////  

void CConsole::PrintColorfulString (LPCWSTR psz)
{
    static constexpr WORD s_rgForegroundColors[] = 
    {
        FC_Black,
        FC_Blue,
        FC_Green,
        FC_Cyan,
        FC_Red,
        FC_Magenta,
        FC_Brown,
        FC_LightGrey,
        FC_DarkGrey,
        FC_LightBlue,
        FC_LightGreen,
        FC_LightCyan,
        FC_LightRed,
        FC_LightMagenta,
        FC_Yellow,
        FC_White,
    };

    WORD   defaultAttr     = m_configPtr->m_rgAttributes[CConfig::EAttribute::Default];
    WORD   backgroundColor = defaultAttr >> 4;
    size_t idxColor        = 0;
    


    for (LPCWCHAR pch = psz; *pch != L'\0'; ++pch)
    {
        WORD color = s_rgForegroundColors[idxColor];
        
        idxColor = (idxColor + 1) % ARRAYSIZE (s_rgForegroundColors);
        
        // Skip if it matches the background
        if (color == backgroundColor)
        {
            color = s_rgForegroundColors[idxColor];
            idxColor = (idxColor + 1) % ARRAYSIZE (s_rgForegroundColors);
        }
        
        WORD colorAttr = color | backgroundColor;
        Putchar (colorAttr, *pch);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::ProcessMultiLineStringWithAttribute
//
//  Helper function to process text with proper color handling for embedded newlines.
//  Resets to default color before each newline, then restores the desired color.
//
////////////////////////////////////////////////////////////////////////////////

void CConsole::ProcessMultiLineStringWithAttribute (wstring_view text, WORD attr)
{
    size_t pos   = 0;
    size_t start = 0;



    SetColor (attr);

    while ((pos = text.find (L'\n', start)) != wstring_view::npos)
    {
        // Append text before newline
        m_strBuffer.append (text.substr (start, pos - start));

        // Reset to default color before newline
        SetColor (m_configPtr->m_rgAttributes[CConfig::EAttribute::Default]);
        m_strBuffer.append (L"\n");

        // Restore color for next line
        SetColor (attr);

        start = pos + 1;
    }

    // Append remaining text after last newline (or all text if no newlines)
    if (start < text.size())
    {
        m_strBuffer.append (text.substr (start));
    }
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
        fSuccess = WriteConsole (m_hStdOut, m_strBuffer.c_str(), cch, &cch, nullptr);
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

        fSuccess = WriteFile (m_hStdOut, utf8Buffer.data(), cb, &bytesWritten, nullptr);
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
    // Using format_to to avoid temporary string allocation
    //

    std::format_to (std::back_inserter (m_strBuffer), AnsiCodes::SGR_COLOR_FORMAT, nAnsiForeground, nAnsiBackground);
}
