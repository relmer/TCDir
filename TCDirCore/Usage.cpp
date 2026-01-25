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

void CUsage::DisplayUsage (CConsole & console, wchar_t chPrefix)
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

    // Determine prefix strings for single-char and multi-char switches
    wchar_t szShort[2]  = { chPrefix, L'\0' };                      // "-" or "/"
    LPCWSTR pszLong     = (chPrefix == L'-') ? L"--" : L"/";        // "--" or "/"
    LPCWSTR pszMDisable = (chPrefix == L'-') ? L" -M-" : L" /M-";   // " -M-" or " /M-"

    wstring buildTimestamp = VERSION_BUILD_TIMESTAMP;



    // Format build timestamp without seconds (drop last 3 chars ":SS" from __TIME__)
    buildTimestamp.resize (buildTimestamp.length () - 3);

    console.Puts (CConfig::EAttribute::Default, L"");
    console.PrintColorfulString (L"Technicolor");
    console.Printf (CConfig::EAttribute::Default, L" Directory version " VERSION_WSTRING L" %s (%s)\n", s_kpszArch, buildTimestamp.c_str ());

    console.Printf (CConfig::EAttribute::Default, L"Copyright " COPYRIGHT " 2004-" VERSION_YEAR_WSTRING  L" by Robert Elmer\n");
    console.Printf (CConfig::EAttribute::Default, L"\n");
#ifdef _DEBUG
    console.Printf (CConfig::EAttribute::Default, L"TCDIR [drive:][path][filename] [%sA[[:]attributes]] [%sO[[:]sortorder]] [%sS] [%sW] [%sB] [%sP] [%sM] [%sEnv] [%sConfig] [%sDebug]\n",
                    szShort, szShort, szShort, szShort, szShort, szShort, szShort, pszLong, pszLong, pszLong);
#else
    console.Printf (CConfig::EAttribute::Default, L"TCDIR [drive:][path][filename] [%sA[[:]attributes]] [%sO[[:]sortorder]] [%sS] [%sW] [%sB] [%sP] [%sM] [%sEnv] [%sConfig]\n",
                    szShort, szShort, szShort, szShort, szShort, szShort, szShort, pszLong, pszLong);
#endif
    console.Printf (CConfig::EAttribute::Default, L"\n");
    console.Printf (CConfig::EAttribute::Default, L"  [drive:][path][filename]\n");
    console.Printf (CConfig::EAttribute::Default, L"              Specifies drive, directory, and/or files to list.\n");
    console.Printf (CConfig::EAttribute::Default, L"\n");
    console.Printf (CConfig::EAttribute::Default, L"  %sA          Displays files with specified attributes.\n", szShort);
    console.Printf (CConfig::EAttribute::Default, L"  attributes   D  Directories                R  Read-only files\n");
    console.Printf (CConfig::EAttribute::Default, L"               H  Hidden files               A  Files ready for archiving\n");
    console.Printf (CConfig::EAttribute::Default, L"               S  System files               T  Temporary files\n");
    console.Printf (CConfig::EAttribute::Default, L"               E  Encrypted files            C  Compressed files\n");
    console.Printf (CConfig::EAttribute::Default, L"               P  Reparse points             0  Sparse files\n");
    console.Printf (CConfig::EAttribute::Default, L"               X  Not content indexed        I  Integrity stream (ReFS)\n");
    console.Printf (CConfig::EAttribute::Default, L"               B  No scrub data (ReFS)       O  Cloud-only (placeholder)\n");
    console.Printf (CConfig::EAttribute::Default, L"               F  Pinned (always local)      U  Unpinned (can dehydrate)\n");
    console.Printf (CConfig::EAttribute::Default, L"               -  Prefix meaning not\n");
    console.Printf (CConfig::EAttribute::Default, L"\n");
    console.Printf (CConfig::EAttribute::Default, L"  Cloud status symbols shown between file size and name:\n");
    console.Printf (CConfig::EAttribute::Default, L"               \x25CB  Cloud-only (not locally available)\n");
    console.Printf (CConfig::EAttribute::Default, L"               \x25D0  Local (synced, can be dehydrated)\n");
    console.Printf (CConfig::EAttribute::Default, L"               \x25CF  Pinned (always available locally)\n");
    console.Printf (CConfig::EAttribute::Default, L"\n");
    console.Printf (CConfig::EAttribute::Default, L"  %sO          List by files in sorted order.\n", szShort);
    console.Printf (CConfig::EAttribute::Default, L"  sortorder    N  By name (alphabetic)       S  By size (smallest first)\n");
    console.Printf (CConfig::EAttribute::Default, L"               E  By extension (alphabetic)  D  By date/time (oldest first)\n");
    console.Printf (CConfig::EAttribute::Default, L"               -  Prefix to reverse order\n");
    console.Printf (CConfig::EAttribute::Default, L"\n");
    console.Printf (CConfig::EAttribute::Default, L"  %sS          Displays files in specified directory and all subdirectories.\n", szShort);
    console.Printf (CConfig::EAttribute::Default, L"  %sW          Displays results in a wide listing format.\n", szShort);
    console.Printf (CConfig::EAttribute::Default, L"  %sB          Displays bare file names only (no headers, footers, or details).\n", szShort);
    console.Printf (CConfig::EAttribute::Default, L"  %sP          Displays performance timing information.\n", szShort);
    console.Printf (CConfig::EAttribute::Default, L"  %sM          Enables multi-threaded enumeration (default). Use%s to disable.\n", szShort, pszMDisable);
    console.Printf (CConfig::EAttribute::Default, L"  %sEnv        Displays " TCDIR_ENV_VAR_NAME L" help, syntax, and current value.\n", pszLong);
    console.Printf (CConfig::EAttribute::Default, L"  %sConfig     Displays current color configuration for all items and extensions.\n", pszLong);
#ifdef _DEBUG
    console.Printf (CConfig::EAttribute::Default, L"  %sDebug      Displays raw file attributes in hex for diagnosing edge cases.\n", pszLong);
#endif
    console.Printf (CConfig::EAttribute::Default, L"\n");
    console.Printf (CConfig::EAttribute::Default, L"\n");
    console.Printf (CConfig::EAttribute::Default, L"\n");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayEnvVarIssues
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayEnvVarIssues (CConsole & console, wchar_t chPrefix)
{
    CConfig::ValidationResult validationResult;

    // Determine prefix strings for multi-char switches
    LPCWSTR pszLong = (chPrefix == L'-') ? L"--" : L"/";



    validationResult = console.m_configPtr->ValidateEnvironmentVariable();

    if (!validationResult.hasIssues())
    {
        return;
    }

    console.Puts   (CConfig::EAttribute::Default, L"");
    console.Printf (CConfig::EAttribute::Error,   L"There are some problems with your %s environment variable (see %senv for help):", 
                    TCDIR_ENV_VAR_NAME, pszLong);
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
        { L"CloudOnly (\x25CB)",       CConfig::EAttribute::CloudStatusCloudOnly    },
        { L"Local (\x25D0)",           CConfig::EAttribute::CloudStatusLocal        },
        { L"Pinned (\x25CF)",        CConfig::EAttribute::CloudStatusPinned       },
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
//  CUsage::EnsureVisibleColorAttr
//
//  Given a color attribute and a default attribute, returns a modified color
//  attribute that ensures visibility. If the foreground matches the default
//  background, a contrasting background is applied.
//
////////////////////////////////////////////////////////////////////////////////

WORD CUsage::EnsureVisibleColorAttr (WORD colorAttr, WORD defaultAttr)
{
    WORD foreAttr        = colorAttr   & (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    WORD backAttr        = colorAttr   & (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY);
    WORD defaultBackAttr = defaultAttr & (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY);



    //
    // If no explicit background in colorAttr, use the default background
    //

    if (backAttr == 0)
    {
        backAttr = defaultBackAttr;
    }

    //
    // If foreground matches background, use a contrasting background so text is visible.
    // Compare by shifting foreground bits to background position.
    //

    if ((foreAttr << 4) == backAttr)
    {
        // Use opposite brightness: if dark background, use light; if light, use dark
        WORD contrastBack = (backAttr & BACKGROUND_INTENSITY) ? static_cast<WORD>(BC_Black) : static_cast<WORD>(BC_LightGrey);
        return foreAttr | contrastBack;
    }

    return foreAttr | backAttr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::GetColorAttribute
//
////////////////////////////////////////////////////////////////////////////////

WORD CUsage::GetColorAttribute (CConsole & console, wstring_view colorName)
{
    WORD    defaultAttr = console.m_configPtr->m_rgAttributes[CConfig::EAttribute::Default];
    WORD    foreAttr    = 0;
    HRESULT hr          = console.m_configPtr->ParseColorName (colorName, false, foreAttr);



    if (FAILED (hr))
    {
        foreAttr = FC_LightGrey;
    }

    return EnsureVisibleColorAttr (foreAttr, defaultAttr);
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
    WORD         visibleAttr  = 0;
    WORD         defaultAttr  = console.m_configPtr->m_rgAttributes[CConfig::EAttribute::Default];
    HRESULT      hrColor      = S_OK;
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
    hrColor = console.m_configPtr->ParseColorSpec (trimmedValue, colorAttr);

    if (FAILED (hrColor))
    {
        // Invalid color - display entire segment in default
        console.Printf (CConfig::EAttribute::Default, L"%.*s", 
                        static_cast<int>(segment.length()), segment.data());
    }
    else
    {
        // Ensure the color is visible against the default background
        visibleAttr = EnsureVisibleColorAttr (colorAttr, defaultAttr);

        // Print key in its color, = in default, value in its color
        console.Printf (visibleAttr, L"%.*s", 
                        static_cast<int>(keyView.length()), keyView.data());
        console.Printf (CConfig::EAttribute::Default, L"=");
        console.Printf (visibleAttr, L"%.*s", 
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

void CUsage::DisplayEnvVarHelp (CConsole & console, wchar_t chPrefix)
{
    // Determine prefix strings for single-char switches
    wchar_t szShort[2]  = { chPrefix, L'\0' };                      // "-" or "/"
    LPCWSTR pszMDisable = (chPrefix == L'-') ? L"-M-" : L"/M-";     // "-M-" or "/M-"

    static LPCWSTR s_setEnvVarCommandCmd        = L"  set " TCDIR_ENV_VAR_NAME L"=";
    static LPCWSTR s_setEnvVarCommandPowerShell = L"  $env:" TCDIR_ENV_VAR_NAME L" = \"";

    bool  isPowerShell = IsPowerShell ();



    console.Puts   (CConfig::EAttribute::Default,     L"");
    console.Printf (CConfig::EAttribute::Default,     L"Set the ");
    console.Printf (CConfig::EAttribute::Information, TCDIR_ENV_VAR_NAME);
    console.Puts   (CConfig::EAttribute::Default,     L" environment variable to override default colors for display"
                                                      L" items, file attributes, or file extensions:\n");

    // Display the syntax line with dynamic switch prefix
    if (isPowerShell)
    {
        console.Printf (CConfig::EAttribute::Default, L"%s[ %s<Switch>] | [<Item> | Attr:<fileattr> | <.ext>] = <Fore> [on <Back>][;...]\"\n",
                        s_setEnvVarCommandPowerShell, szShort);
    }
    else
    {
        console.Printf (CConfig::EAttribute::Default, L"%s[ %s<Switch>] | [<Item> | Attr:<fileattr> | <.ext>] = <Fore> [on <Back>][;...]\n",
                        s_setEnvVarCommandCmd, szShort);
    }

    console.Printf (CConfig::EAttribute::Default, L"\n");
    console.Printf (CConfig::EAttribute::Default, L"  <Switch>    A command-line switch:\n");
    console.Printf (CConfig::EAttribute::Default, L"                  W  Wide listing format\n");
    console.Printf (CConfig::EAttribute::Default, L"                  P  Display performance timing information\n");
    console.Printf (CConfig::EAttribute::Default, L"                  S  Recurse into subdirectories\n");
    console.Printf (CConfig::EAttribute::Default, L"                  M  Enables multi-threaded enumeration (default); use %s to disable\n", pszMDisable);
    console.Printf (CConfig::EAttribute::Default, L"\n");
    console.Printf (CConfig::EAttribute::Default, L"  <Item>      A display item:\n");
    console.Printf (CConfig::EAttribute::Default, L"                  D  Date           T  Time\n");
    console.Printf (CConfig::EAttribute::Default, L"                  S  Size           R  Directory name\n");
    console.Printf (CConfig::EAttribute::Default, L"                  I  Information    H  Information highlight\n");
    console.Printf (CConfig::EAttribute::Default, L"                  E  Error          F  File (default)\n");
    console.Printf (CConfig::EAttribute::Default, L"                  CloudOnly        Local          Pinned\n");
    console.Printf (CConfig::EAttribute::Default, L"\n");
    console.Printf (CConfig::EAttribute::Default, L"  <.ext>      A file extension, including the leading period.\n");
    console.Printf (CConfig::EAttribute::Default, L"\n");
    console.Printf (CConfig::EAttribute::Default, L"  <FileAttr>  A file attribute (see file attributes below)\n");
    console.Printf (CConfig::EAttribute::Default, L"                  R  Read-only      H  Hidden\n");
    console.Printf (CConfig::EAttribute::Default, L"                  S  System         A  Archive\n");
    console.Printf (CConfig::EAttribute::Default, L"                  T  Temporary      E  Encrypted\n");
    console.Printf (CConfig::EAttribute::Default, L"                  C  Compressed     P  Reparse point\n");
    console.Printf (CConfig::EAttribute::Default, L"                  0  Sparse file\n");
    console.Printf (CConfig::EAttribute::Default, L"\n");
    console.Printf (CConfig::EAttribute::Default, L"  <Fore>      Foreground color\n");
    console.Printf (CConfig::EAttribute::Default, L"  <Back>      Background color\n");

    DisplayColorConfiguration (console);

    // Display the example line with dynamic switch prefix
    if (isPowerShell)
    {
        console.Printf (CConfig::EAttribute::Default, L"  Example: $env:" TCDIR_ENV_VAR_NAME L" = \"%sW;D=LightGreen;S=Yellow;Attr:H=DarkGrey;.cpp=White on Blue\"\n", szShort);
    }
    else
    {
        console.Printf (CConfig::EAttribute::Default, L"  Example: set "  TCDIR_ENV_VAR_NAME L"=%sW;D=LightGreen;S=Yellow;Attr:H=DarkGrey;.cpp=White on Blue\n", szShort);
    }

    console.Puts (CConfig::EAttribute::Default, L"");

    if (IsTcdirEnvVarSet ())
    {
        DisplayEnvVarIssues (console, chPrefix);
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

void CUsage::DisplayCurrentConfiguration (CConsole & console, wchar_t chPrefix)
{
    console.Puts (CConfig::EAttribute::Default, L"");

    if (IsTcdirEnvVarSet ())
    {
        DisplayEnvVarIssues (console, chPrefix);
    }
    else
    {
        console.Printf (CConfig::EAttribute::Default,     L"  ");
        console.Printf (CConfig::EAttribute::Information, TCDIR_ENV_VAR_NAME);
        console.Puts   (CConfig::EAttribute::Default,     L" environment variable is not set; showing default configuration.");
    }

    DisplayConfigurationTable (console);
}
