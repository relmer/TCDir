#include "pch.h"
#include "Usage.h"

#include "Color.h"
#include "Config.h"
#include "Console.h"
#include "Version.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::IsEnvVarSet
//
////////////////////////////////////////////////////////////////////////////////

bool CUsage::IsEnvVarSet (LPCWSTR kpszEnvVarName)
{
    DWORD cchBufNeeded = GetEnvironmentVariableW (kpszEnvVarName, nullptr, 0);



    // If cchBufNeeded > 0, the variable exists (even if its value is empty)
    return cchBufNeeded > 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::IsPowerShell
//
//  Detect if running in PowerShell by checking PSModulePath environment variable
//
////////////////////////////////////////////////////////////////////////////////

bool CUsage::IsPowerShell (void)
{
    return IsEnvVarSet (L"PSModulePath");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::IsTcdirEnvVarSet
//
////////////////////////////////////////////////////////////////////////////////

bool CUsage::IsTcdirEnvVarSet (void)
{
    return IsEnvVarSet (TCDIR_ENV_VAR_NAME);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayUsage
//
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayUsage (CConsole & console)
{
#define COPYRIGHT L"\x00A9"

    static constexpr LPCWSTR s_kpszArch = 
#if defined(_M_X64)
                                          L"x64";
#elif defined(_M_ARM64)
                                          L"ARM64";
#elif defined(_M_IX86)
                                          L"x86";
#else
                                          L"";
    #error "unknown architecture"
#endif

    static LPCWSTR s_usageLines[] =
    {
        L"Copyright " COPYRIGHT " 2004-" VERSION_YEAR_WSTRING  L" by Robert Elmer",
        L"",
        L"TCDIR [drive:][path][filename] [/A[[:]attributes]] [/O[[:]sortorder]] [/S] [/W] [/P] [/M] [/Env] [/Config]",
        L"",
        L"  [drive:][path][filename]",
        L"              Specifies drive, directory, and/or files to list.",
        L"",
        L"  /A          Displays files with specified attributes.",
        L"  attributes   D  Directories                R  Read-only files",
        L"               H  Hidden files               A  Files ready for archiving",
        L"               S  System files               T  Temporary files",
        L"               E  Encrypted files            C  Compressed files",
        L"               P  Reparse points             0  Sparse files",
        L"               -  Prefix meaning not",
        L"",
        L"  /O          List by files in sorted order.",
        L"  sortorder    N  By name (alphabetic)       S  By size (smallest first)",
        L"               E  By extension (alphabetic)  D  By date/time (oldest first)",
        L"               -  Prefix to reverse order",
        L"",
        L"  /S          Displays files in specified directory and all subdirectories.",
        L"  /W          Displays results in a wide listing format.",
        L"  /P          Displays performance timing information.",
        L"  /M          Enables multi-threaded enumeration (default). Use /M- to disable.",
        L"  /Env        Displays " TCDIR_ENV_VAR_NAME L" help, syntax, and current value.",
        L"  /Config     Displays current color configuration for all items and extensions.",
        L"",
        L"",
        L""
    };

    wstring buildTimestamp = VERSION_BUILD_TIMESTAMP;



    // Format build timestamp without seconds (drop last 3 chars ":SS" from __TIME__)
    buildTimestamp.resize (buildTimestamp.length () - 3);

    console.Puts (CConfig::EAttribute::Default, L"");
    console.PrintColorfulString (L"Technicolor");
    console.Printf (CConfig::EAttribute::Default, L" Directory version " VERSION_WSTRING L" %s (%s)\n", s_kpszArch, buildTimestamp.c_str ());

    for (LPCWSTR line : s_usageLines)
    {
        console.Puts (CConfig::EAttribute::Default, line);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayEnvVarIssues
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayEnvVarIssues (CConsole & console)
{
    CConfig::ValidationResult validationResult;



    validationResult = console.m_configPtr->ValidateEnvironmentVariable();

    if (!validationResult.hasIssues())
    {
        return;
    }

    console.Puts   (CConfig::EAttribute::Default, L"");
    console.Printf (CConfig::EAttribute::Error,   L"There are some problems with your %s environment variable (see /env for help):", 
                    TCDIR_ENV_VAR_NAME);
    console.Puts   (CConfig::EAttribute::Default, L"\n");

    DisplayEnvVarCurrentValue (console, TCDIR_ENV_VAR_NAME);

    for (const auto & error : validationResult.errors)
    {
        size_t  prefixLen = 2 + error.message.length() + 5 + error.invalidTextOffset;
        wstring underline   (error.invalidText.length(), L'\x203E');  // Unicode overline character
        


        console.Printf (CConfig::EAttribute::Error,   L"  %s in \"%s\"", error.message.c_str(), error.entry.c_str());
        console.Puts   (CConfig::EAttribute::Default, L"");
        console.Printf (CConfig::EAttribute::Default, L"%*s", static_cast<int>(prefixLen), L"");
        console.Puts   (CConfig::EAttribute::Error,   underline.c_str());
    }

    console.Puts (CConfig::EAttribute::Default, L"");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayConfigurationTable
//
//  Display all color configuration with sources (Default vs Environment)
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayConfigurationTable (CConsole & console)
{
    static constexpr WCHAR UNICODE_LINE_HORIZONTAL = L'\x2500';
    static constexpr int   COLUMN_WIDTH_ATTR       = 25;
    static constexpr int   COLUMN_WIDTH_SOURCE     = 15;

    wstring tableSeparator = L"  ";



    tableSeparator += wstring (COLUMN_WIDTH_ATTR + COLUMN_WIDTH_SOURCE + 2, UNICODE_LINE_HORIZONTAL);

    DisplayAttributeConfiguration     (console, COLUMN_WIDTH_ATTR, COLUMN_WIDTH_SOURCE);
    DisplayFileAttributeConfiguration (console, COLUMN_WIDTH_ATTR, COLUMN_WIDTH_SOURCE);
    DisplayExtensionConfiguration     (console, COLUMN_WIDTH_ATTR, COLUMN_WIDTH_SOURCE);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayAttributeConfiguration
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayAttributeConfiguration (CConsole & console, int columnWidthAttr, int columnWidthSource)
{
    struct AttrInfo
    {
        LPCWSTR             name;
        CConfig::EAttribute attr;
    };

    static constexpr AttrInfo s_attrInfos[] =
    {
        { L"Default",                CConfig::EAttribute::Default                 },
        { L"Date",                   CConfig::EAttribute::Date                    },
        { L"Time",                   CConfig::EAttribute::Time                    },
        { L"File attribute present", CConfig::EAttribute::FileAttributePresent    },
        { L"File attribute absent",  CConfig::EAttribute::FileAttributeNotPresent },
        { L"Size",                   CConfig::EAttribute::Size                    },
        { L"Directory",              CConfig::EAttribute::Directory               },
        { L"Information",            CConfig::EAttribute::Information             },
        { L"Info highlight",         CConfig::EAttribute::InformationHighlight    },
        { L"Separator line",         CConfig::EAttribute::SeparatorLine           },
        { L"Error",                  CConfig::EAttribute::Error                   },
    };



    console.Puts (CConfig::EAttribute::Information, L"");
    console.Puts (CConfig::EAttribute::Information, L"Current display item configuration:");
    console.Puts (CConfig::EAttribute::Information, L"");

    for (const auto & info : s_attrInfos)
    {
        WORD attr  = console.m_configPtr->m_rgAttributes[info.attr];
        bool isEnv = (console.m_configPtr->m_rgAttributeSources[info.attr] == CConfig::EAttributeSource::Environment);



        DisplayItemAndSource (console, info.name, attr, isEnv, columnWidthAttr, columnWidthSource, 0, EItemDisplayMode::SingleColumn);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayFileAttributeConfiguration
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayFileAttributeConfiguration (CConsole & console, int columnWidthAttr, int columnWidthSource)
{
    struct FileAttrInfo
    {
        LPCWSTR name;
        DWORD   dwAttribute;
        WCHAR   ch;
    };

    static constexpr FileAttrInfo s_attrInfos[] =
    {
        { L"Read-only",      FILE_ATTRIBUTE_READONLY,      L'R' },
        { L"Hidden",         FILE_ATTRIBUTE_HIDDEN,        L'H' },
        { L"System",         FILE_ATTRIBUTE_SYSTEM,        L'S' },
        { L"Archive",        FILE_ATTRIBUTE_ARCHIVE,       L'A' },
        { L"Temporary",      FILE_ATTRIBUTE_TEMPORARY,     L'T' },
        { L"Encrypted",      FILE_ATTRIBUTE_ENCRYPTED,     L'E' },
        { L"Compressed",     FILE_ATTRIBUTE_COMPRESSED,    L'C' },
        { L"Reparse point",  FILE_ATTRIBUTE_REPARSE_POINT, L'P' },
        { L"Sparse file",    FILE_ATTRIBUTE_SPARSE_FILE,   L'0' },
    };



    console.Puts (CConfig::EAttribute::Information, L"");
    console.Puts (CConfig::EAttribute::Information, L"File attribute color configuration:");
    console.Puts (CConfig::EAttribute::Information, L"");

    for (const auto & info : s_attrInfos)
    {
        auto iter = console.m_configPtr->m_mapFileAttributesTextAttr.find (info.dwAttribute);
        if (iter == console.m_configPtr->m_mapFileAttributesTextAttr.end())
        {
            continue;
        }

        WORD    attr  = iter->second.m_wAttr;
        bool    isEnv = (iter->second.m_source == CConfig::EAttributeSource::Environment);
        wstring label = format (L"{} {}", info.ch, info.name);
        
        DisplayItemAndSource (console, label, attr, isEnv, columnWidthAttr, columnWidthSource, 0, EItemDisplayMode::SingleColumn);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayExtensionConfigurationSingleColumn
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayExtensionConfigurationSingleColumn (CConsole & console, int columnWidthAttr, int columnWidthSource, const vector<pair<wstring, WORD>> & extensions)
{
    for (const auto & [ext, extAttr] : extensions)
    {
        auto    sourceIter = console.m_configPtr->m_mapExtensionSources.find (ext);
        bool    isEnv      = (sourceIter         != console.m_configPtr->m_mapExtensionSources.end () &&
                              sourceIter->second == CConfig::EAttributeSource::Environment);

        DisplayItemAndSource (console, ext, extAttr, isEnv, columnWidthAttr, columnWidthSource, 0, EItemDisplayMode::SingleColumn);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayItemAndSource
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayItemAndSource (CConsole & console, wstring_view item, WORD attr, bool isEnv, size_t columnWidthItem, size_t columnWidthSource, size_t cxColumnWidth, EItemDisplayMode mode)
{
    WORD    bgAttr     = console.m_configPtr->m_rgAttributes[CConfig::EAttribute::Default] & BC_Mask;
    WORD    sourceAttr = static_cast<WORD> (bgAttr | (isEnv ? FC_Cyan : FC_DarkGrey));
    LPCWSTR source     = isEnv ? L"Environment" : L"Default";
    int     pad        = max (0, static_cast<int> (columnWidthItem) - static_cast<int> (item.size ()));
    size_t  cxUsed     = columnWidthItem + 2 + columnWidthSource;



    if (mode == EItemDisplayMode::SingleColumn)
    {
        console.Printf (CConfig::EAttribute::Information, L"  ");
    }

    console.Printf (attr,                             L"%.*ls", static_cast<int> (item.size ()), item.data ());
    console.Printf (CConfig::EAttribute::Information, L"%*ls  ", pad, L"");
    console.Printf (sourceAttr,                       L"%-*ls", static_cast<int> (columnWidthSource), source);

    if (mode == EItemDisplayMode::MultiColumn)
    {
        if (cxColumnWidth > cxUsed)
        {
            console.Printf (CConfig::EAttribute::Information, L"%*ls", static_cast<int> (cxColumnWidth - cxUsed), L"");
        }
    }
    else
    {
        console.Printf (CConfig::EAttribute::Default, L"\n");
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayExtensionConfigurationMultiColumn
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayExtensionConfigurationMultiColumn (CConsole & console, const vector<pair<wstring, WORD>> & extensions, size_t maxExtLen, size_t cxSourceWidth, size_t cxAvailable, size_t cColumns)
{
    size_t cxColumnWidth   = max (static_cast<size_t> (1), cxAvailable / cColumns);
    size_t cRows           = (extensions.size () + cColumns - 1) / cColumns;
    size_t cItemsInLastRow = extensions.size () % cColumns;
    size_t fullRows        = cItemsInLastRow ? cRows - 1 : cRows;



    console.Puts (CConfig::EAttribute::Information, L"");

    for (size_t nRow = 0; nRow < cRows; ++nRow)
    {
        console.Printf (CConfig::EAttribute::Information, L"  ");

        for (size_t nCol = 0; nCol < cColumns; ++nCol)
        {
            if ((nRow * cColumns + nCol) >= extensions.size ())
            {
                break;
            }

            // Column-major ordering (see ResultsDisplayerWide).
            size_t idx = nRow + (nCol * fullRows);

            if (nCol < cItemsInLastRow)
            {
                idx += nCol;
            }
            else
            {
                idx += cItemsInLastRow;
            }

            const auto & ext     = extensions[idx].first;
            WORD         extAttr = extensions[idx].second;
            auto      sourceIter = console.m_configPtr->m_mapExtensionSources.find (ext);
            bool      isEnv      = (sourceIter         != console.m_configPtr->m_mapExtensionSources.end () &&
                                    sourceIter->second == CConfig::EAttributeSource::Environment);

            DisplayItemAndSource (console, ext, extAttr, isEnv, maxExtLen, cxSourceWidth, cxColumnWidth, EItemDisplayMode::MultiColumn);
        }

        console.Puts (CConfig::EAttribute::Default, L"");
    }

    console.Puts (CConfig::EAttribute::Default, L"");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayExtensionConfiguration
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayExtensionConfiguration (CConsole & console, int columnWidthAttr, int columnWidthSource)
{
    vector<pair<wstring, WORD>> extensions;
    UINT                        cxConsoleWidth  = console.GetWidth ();
    size_t                      maxExtLen       = 0;
    size_t                      cxIndent        = 2;
    size_t                      cxAvailable     = (cxConsoleWidth > cxIndent) ? (cxConsoleWidth - cxIndent) : cxConsoleWidth;
    size_t                      cxSourceWidth   = wcslen (L"Environment");
    size_t                      cColumns        = 1;



    console.Puts (CConfig::EAttribute::Information, L"");
    console.Puts (CConfig::EAttribute::Information, L"File extension color configuration:");

    extensions.reserve (console.m_configPtr->m_mapExtensionToTextAttr.size());

    for (const auto & [ext, extAttr] : console.m_configPtr->m_mapExtensionToTextAttr)
    {
        extensions.emplace_back (ext, extAttr);
    }

    std::ranges::sort (extensions, [] (const auto & a, const auto & b) { return a.first < b.first; });

    for (const auto & [ext, _] : extensions)
    {
        maxExtLen = max (maxExtLen, ext.size ());
    }

    // Minimum width per column: <extension> + "  " + <source> + " "
    // The extra space allows a gap between columns.
    {
        size_t cxMinColumnWidth = maxExtLen + 2 + cxSourceWidth + 1 + 2; // +3 for some padding

        if (cxMinColumnWidth > 0 && cxMinColumnWidth <= cxAvailable)
        {
            cColumns = max (static_cast<size_t> (1), cxAvailable / cxMinColumnWidth);
        }
    }

    if (cColumns == 1)
    {
        DisplayExtensionConfigurationSingleColumn (console, columnWidthAttr, columnWidthSource, extensions);
    }
    else
    {
        DisplayExtensionConfigurationMultiColumn (console, extensions, maxExtLen, cxSourceWidth, cxAvailable, cColumns);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::GetColorAttribute
//
////////////////////////////////////////////////////////////////////////////////

WORD CUsage::GetColorAttribute (CConsole & console, wstring_view colorName)
{
    WORD defaultAttr = console.m_configPtr->m_rgAttributes[CConfig::EAttribute::Default];
    WORD baseAttr    = defaultAttr & ~(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    WORD foreAttr    = console.m_configPtr->ParseColorName (colorName, false);



    if (foreAttr == 0)
    {
        foreAttr = FC_LightGrey;
    }

    return baseAttr | foreAttr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayColorConfiguration
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayColorConfiguration (CConsole & console)
{
    static constexpr int COLUMN_WIDTH_COLOR_LEFT = 18;

    struct ColorRow
    {
        LPCWSTR left;
        LPCWSTR right;
    };

    static constexpr ColorRow s_colorRows[] =
    {
        { L"Black",     L"DarkGrey"     },
        { L"Blue",      L"LightBlue"    },
        { L"Green",     L"LightGreen"   },
        { L"Cyan",      L"LightCyan"    },
        { L"Red",       L"LightRed"     },
        { L"Magenta",   L"LightMagenta" },
        { L"Brown",     L"Yellow"       },
        { L"LightGrey", L"White"        },
    };



    for (const auto & row : s_colorRows)
    {
        int pad = max (0, COLUMN_WIDTH_COLOR_LEFT - static_cast<int> (wcslen (row.left)));

        console.Printf (CConfig::EAttribute::Default,           L"                  ");
        console.Printf (GetColorAttribute (console, row.left),  L"%ls", row.left);
        console.Printf (CConfig::EAttribute::Default,           L"%*ls", pad, L"");
        console.Printf (GetColorAttribute (console, row.right), L"%ls", row.right);
        console.Printf (CConfig::EAttribute::Default,           L"\n");
    }

    console.Puts (CConfig::EAttribute::Default, L"");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayEnvVarSegment
//
//  Display a single TCDIR segment in its specified color.
//  Format: Key=ForeColor [on BackColor]
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUsage::DisplayEnvVarSegment (CConsole & console, wstring_view segment)
{
    HRESULT      hr           = S_OK;
    size_t       equalPos     = 0;
    WORD         colorAttr    = 0;
    wstring_view keyView;
    wstring_view valueView;
    wstring_view trimmedValue;



    equalPos = segment.find (L'=');

    if (equalPos == wstring_view::npos)
    {
        // No '=' sign - display entire segment in default color
        console.Printf (CConfig::EAttribute::Default, L"%.*s", 
                        static_cast<int>(segment.length()), segment.data());
        BAIL_OUT_IF (TRUE, S_OK);
    }

    keyView   = segment.substr (0, equalPos);
    valueView = segment.substr (equalPos + 1);

    // Trim leading/trailing whitespace from value for color parsing
    trimmedValue = valueView;
    while (!trimmedValue.empty() && iswspace (trimmedValue.front()))
    {
        trimmedValue.remove_prefix (1);
    }   

    while (!trimmedValue.empty() && iswspace (trimmedValue.back()))
    {
        trimmedValue.remove_suffix (1);
    }   

    // Parse the color spec to get the actual color
    colorAttr = console.m_configPtr->ParseColorSpec (trimmedValue);

    if (colorAttr == 0)
    {
        // Invalid color - display entire segment in default
        console.Printf (CConfig::EAttribute::Default, L"%.*s", 
                        static_cast<int>(segment.length()), segment.data());
    }
    else
    {
        // Print key in its color, = in default, value in its color
        console.Printf (colorAttr, L"%.*s", 
                        static_cast<int>(keyView.length()), keyView.data());
        console.Printf (CConfig::EAttribute::Default, L"=");
        console.Printf (colorAttr, L"%.*s", 
                        static_cast<int>(valueView.length()), valueView.data());
    }


    
Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayEnvVarCurrentValue
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayEnvVarCurrentValue (CConsole & console, LPCWSTR pszEnvVarName)
{
    HRESULT hr               = S_OK;
    DWORD   cchBufNeeded     = 0;
    DWORD   cchExcludingNull = 0;
    DWORD   cchCopied        = 0;
    wstring envValue;
    bool    firstSegment     = true;



    cchBufNeeded = GetEnvironmentVariableW (pszEnvVarName, nullptr, 0);
    BAIL_OUT_IF (cchBufNeeded == 0, S_OK);

    cchExcludingNull = cchBufNeeded - 1;
    envValue.resize (cchExcludingNull, L'\0');

    cchCopied = GetEnvironmentVariableW (pszEnvVarName, envValue.data(), cchBufNeeded);
    CWRA (cchCopied == cchExcludingNull);

    console.Printf (CConfig::EAttribute::Default,     L"  ");
    console.Printf (CConfig::EAttribute::Information, pszEnvVarName);
    console.Printf (CConfig::EAttribute::Default,     L" = ");

    if (envValue.empty())
    {
        console.Puts (CConfig::EAttribute::InformationHighlight, L"<empty>\n");
        BAIL_OUT_IF (true, S_OK);
    }

    console.Printf (CConfig::EAttribute::Default, L"\"");

    // Split by semicolons and display each segment in its specified color
    for (auto segment : std::views::split (wstring_view (envValue), L';'))
    {
        wstring_view segView (segment.begin(), segment.end());
        
        // Skip empty segments
        if (segView.empty())
        {
            continue;
        }

        if (!firstSegment)
        {
            console.Printf (CConfig::EAttribute::Default, L";");
        }

        firstSegment = false;

        DisplayEnvVarSegment (console, segView);
    }

    console.Puts (CConfig::EAttribute::Default, L"\"\n");

    

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayEnvVarHelp
//
//  Display help for the TCDIR environment variable syntax, available colors,
//  an example, and the current value with any issues.
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayEnvVarHelp (CConsole & console)
{
#define ENV_VAR_SYNTAX L"[ -<Switch> | /<Switch>] | [<Item> | Attr:<fileattr> | <.ext>] = <Fore> [on <Back>][;...]"

    static LPCWSTR s_setEnvVarCommand[] =
    {
        L"  set " TCDIR_ENV_VAR_NAME L"=" ENV_VAR_SYNTAX,               // CMD -- *no spaces around '='*
        L"  $env:" TCDIR_ENV_VAR_NAME L" = \"" ENV_VAR_SYNTAX L"\"",    // PowerShell -- quoted value
    };

    static LPCWSTR s_colorOverrideLines[] =
    {
        L"",
        L"  <Switch>    A command-line switch:",
        L"                  W  Wide listing format",
        L"                  P  Display performance timing information",
        L"                  S  Recurse into subdirectories",
        L"                  M  Enables multi-threaded enumeration (default); use /M- to disable",
        L"",
        L"  <Item>      A display item:",
        L"                  D  Date           T  Time",
        L"                  S  Size           R  Directory name",
        L"                  I  Information    H  Information highlight",
        L"                  E  Error          F  File (default)",
        L"",
        L"  <.ext>      A file extension, including the leading period.",
        L"",
        L"  <FileAttr>  A file attribute (see file attributes below)",
        L"                  R  Read-only      H  Hidden",
        L"                  S  System         A  Archive",
        L"                  T  Temporary      E  Encrypted",
        L"                  C  Compressed     P  Reparse point",
        L"                  0  Sparse file",
        L"",
        L"  <Fore>      Foreground color",
        L"  <Back>      Background color",
    };

    static LPCWSTR s_envVarExample[] =
    {
        L"  Example: set "  TCDIR_ENV_VAR_NAME L"=-W;D=LightGreen;S=Yellow;Attr:H=DarkGrey;.cpp=White on Blue",           // CMD
        L"  Example: $env:" TCDIR_ENV_VAR_NAME L" = \"-W;D=LightGreen;S=Yellow;Attr:H=DarkGrey;.cpp=White on Blue\"",    // PowerShell
    };

    bool  isPowerShell = IsPowerShell ();



    console.Puts   (CConfig::EAttribute::Default,     L"");
    console.Printf (CConfig::EAttribute::Default,     L"Set the ");
    console.Printf (CConfig::EAttribute::Information, TCDIR_ENV_VAR_NAME);
    console.Puts   (CConfig::EAttribute::Default,     L" environment variable to override default colors for display"
                                                      L" items, file attributes, or file extensions:\n");

    console.Puts (CConfig::EAttribute::Default, s_setEnvVarCommand[isPowerShell]);

    for (LPCWSTR line : s_colorOverrideLines)
    {
        console.Puts (CConfig::EAttribute::Default, line);
    }

    DisplayColorConfiguration (console);

    console.Puts (CConfig::EAttribute::Default, s_envVarExample[isPowerShell]);
    console.Puts (CConfig::EAttribute::Default, L"");

    if (IsTcdirEnvVarSet ())
    {
        DisplayEnvVarIssues (console);
    }
    else
    {
        console.Printf (CConfig::EAttribute::Default,     L"  ");
        console.Printf (CConfig::EAttribute::Information, TCDIR_ENV_VAR_NAME);
        console.Puts   (CConfig::EAttribute::Default,     L" environment variable is not set.");
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayCurrentConfiguration
//
//  Display the current color configuration tables for display items,
//  file attributes, and file extensions.
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayCurrentConfiguration (CConsole & console)
{
    console.Puts (CConfig::EAttribute::Default, L"");

    if (IsTcdirEnvVarSet ())
    {
        DisplayEnvVarIssues (console);
    }
    else
    {
        console.Printf (CConfig::EAttribute::Default,     L"  ");
        console.Printf (CConfig::EAttribute::Information, TCDIR_ENV_VAR_NAME);
        console.Puts   (CConfig::EAttribute::Default,     L" environment variable is not set; showing default configuration.");
    }

    DisplayConfigurationTable (console);
}
