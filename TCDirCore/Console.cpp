#include "pch.h"
#include "Color.h"
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
    static constexpr int s_cchBuf          = 9999;  // Max console buffer width
    static  WCHAR        s_szBuf[s_cchBuf] = { L'\0' };

    HRESULT   hr     = S_OK;
    va_list   vaArgs = 0;  
    LPWSTR    pszEnd = nullptr;



    va_start (vaArgs, pszFormat);
    
    hr = StringCchVPrintfEx (s_szBuf, s_cchBuf, &pszEnd, nullptr, 0, pszFormat, vaArgs);
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
    static constexpr int s_cchBuf          = 9999;  // Max console buffer width
    static  WCHAR        s_szBuf[s_cchBuf] = { L'\0' };

    HRESULT   hr       = S_OK;
    va_list   vaArgs   = 0;  
    LPWSTR    pszEnd   = nullptr;
    


    va_start (vaArgs, pszFormat);
    
    hr = StringCchVPrintfEx (s_szBuf, s_cchBuf, &pszEnd, nullptr, 0, pszFormat, vaArgs);
    CHRA (hr);

    ProcessMultiLineStringWithAttribute (s_szBuf, attr);

Error:
    va_end (vaArgs);

    return SUCCEEDED (hr) ? (int) (pszEnd - s_szBuf) : 0;
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
    static const WORD s_rgForegroundColors[] = 
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
    SetColor (attr);

    // Split on newlines and process each part
    auto lines = text | std::views::split (L'\n');
    bool firstLine = true;

    for (const auto & line : lines)
    {
        if (!firstLine)
        {
            // Reset to default color before the newline
            SetColor (m_configPtr->m_rgAttributes[CConfig::EAttribute::Default]);
            m_strBuffer.append (L"\n");

            // Restore the original color for the next line
            SetColor (attr);
        }

        // Append the line content
        wstring_view lineView (line.begin (), line.end ());
        m_strBuffer.append (lineView);

        firstLine = false;
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
    // Using format_to to avoid temporary string allocation
    //

    std::format_to (std::back_inserter (m_strBuffer), AnsiCodes::SGR_COLOR_FORMAT, nAnsiForeground, nAnsiBackground);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::DisplayConfigurationTable
//
//  Display all color configuration with sources (Default vs Environment)
//
////////////////////////////////////////////////////////////////////////////////  

void CConsole::DisplayConfigurationTable (void)
{
    static constexpr int COLUMN_WIDTH_ATTR = 25;
    static constexpr int COLUMN_WIDTH_VALUE = 10;
    static constexpr int COLUMN_WIDTH_SOURCE = 15;

    // Get attribute names
    struct AttrInfo
    {
        LPCWSTR name;
        CConfig::EAttribute attr;
    };

    static constexpr AttrInfo s_attrInfos[] =
    {
        { L"Default",                CConfig::EAttribute::Default                 },
        { L"Date",                   CConfig::EAttribute::Date                    },
        { L"Time",                   CConfig::EAttribute::Time                    },
        { L"File Attribute Present", CConfig::EAttribute::FileAttributePresent    },
        { L"File Attribute Absent",  CConfig::EAttribute::FileAttributeNotPresent },
        { L"Size",                   CConfig::EAttribute::Size                    },
        { L"Directory",              CConfig::EAttribute::Directory               },
        { L"Information",            CConfig::EAttribute::Information             },
        { L"Info Highlight",         CConfig::EAttribute::InformationHighlight    },
        { L"Separator Line",         CConfig::EAttribute::SeparatorLine           },
        { L"Error",                  CConfig::EAttribute::Error                   },
    };

    Puts (CConfig::EAttribute::Information, L"");
    Puts (CConfig::EAttribute::Information, L"Current Display Attribute Configuration:");

    // Header
    wstring header = format (L"  {:<{}}  {:<{}}  {:<{}}", 
        L"Attribute Type", COLUMN_WIDTH_ATTR,
        L"Value", COLUMN_WIDTH_VALUE,
        L"Source", COLUMN_WIDTH_SOURCE);
    Puts (CConfig::EAttribute::Information, header.c_str());

    // Separator line
    wstring separator = L"  ";
    separator += wstring(COLUMN_WIDTH_ATTR + COLUMN_WIDTH_VALUE + COLUMN_WIDTH_SOURCE + 4, L'─');
    Puts (CConfig::EAttribute::SeparatorLine, separator.c_str());

    // Display each attribute
    for (const auto& info : s_attrInfos)
    {
        WORD attr = m_configPtr->m_rgAttributes[info.attr];
        LPCWSTR source = (m_configPtr->m_rgAttributeSources[info.attr] == CConfig::EAttributeSource::Environment) 
            ? L"Environment" : L"Default";

        // Format: name, colored block, source
        wstring line = format(L"  {:<{}}  ", info.name, COLUMN_WIDTH_ATTR);
        m_strBuffer.append(line);
        
        // Add colored block
        SetColor(attr);
        m_strBuffer.append(L"███");  // Unicode block character
        SetColor(m_configPtr->m_rgAttributes[CConfig::EAttribute::Information]);
        
        line = format(L"       {:<{}}", source, COLUMN_WIDTH_SOURCE);
        m_strBuffer.append(line);
        m_strBuffer.append(L"\n");
    }

    // Display file extension colors (sample of configured extensions)
    Puts (CConfig::EAttribute::Information, L"");
    Puts (CConfig::EAttribute::Information, L"File Extension Color Configuration (sample):");

    // Header
    header = format (L"  {:<{}}  {:<{}}  {:<{}}", 
        L"Extension", COLUMN_WIDTH_ATTR,
        L"Value", COLUMN_WIDTH_VALUE,
        L"Source", COLUMN_WIDTH_SOURCE);
    Puts (CConfig::EAttribute::Information, header.c_str());

    // Separator line
    Puts (CConfig::EAttribute::SeparatorLine, separator.c_str());

    // Collect and sort extensions for display (show a sample)
    vector<pair<wstring, WORD>> extensions;
    for (const auto& [ext, attr] : m_configPtr->m_mapExtensionToTextAttr)
    {
        extensions.emplace_back(ext, attr);
    }
    
    std::ranges::sort(extensions, [](const auto& a, const auto& b) { return a.first < b.first; });

    // Display up to 20 extensions
    size_t count = 0;
    for (const auto& [ext, attr] : extensions)
    {
        if (count++ >= 20) 
        {
            Puts(CConfig::EAttribute::Information, L"  ... (and more)");
            break;
        }

        auto sourceIter = m_configPtr->m_mapExtensionSources.find(ext);
        LPCWSTR source = (sourceIter != m_configPtr->m_mapExtensionSources.end() && 
                         sourceIter->second == CConfig::EAttributeSource::Environment) 
            ? L"Environment" : L"Default";

        wstring line = format(L"  {:<{}}  ", ext.c_str(), COLUMN_WIDTH_ATTR);
        m_strBuffer.append(line);
        
        // Add colored block
        SetColor(attr);
        m_strBuffer.append(L"███");
        SetColor(m_configPtr->m_rgAttributes[CConfig::EAttribute::Information]);
        
        line = format(L"       {:<{}}", source, COLUMN_WIDTH_SOURCE);
        m_strBuffer.append(line);
        m_strBuffer.append(L"\n");
    }

    Puts (CConfig::EAttribute::Default, L"");
}

