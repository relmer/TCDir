#include "pch.h"
#include "Usage.h"

#include "Color.h"
#include "Config.h"
#include "Console.h"
#include "UnicodeSymbols.h"
#include "Version.h"





////////////////////////////////////////////////////////////////////////////////
//
//  GetArchitecture
//
//  Returns the architecture string at compile time.
//
////////////////////////////////////////////////////////////////////////////////

consteval wstring_view GetArchitecture (void)
{
#if defined(_M_X64)
    return L"x64";
#elif defined(_M_ARM64)
    return L"ARM64";
#elif defined(_M_IX86)
    return L"x86";
#else
    #error "unknown architecture"
#endif
}





////////////////////////////////////////////////////////////////////////////////
//
//  Shared data structures for display items, file attributes, and switches
//
////////////////////////////////////////////////////////////////////////////////

struct SDisplayItemInfo
{
    LPCWSTR             name;
    CConfig::EAttribute attr;
};

static constexpr SDisplayItemInfo s_kDisplayItemInfos[] =
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
    { L"Owner",                  CConfig::EAttribute::Owner                   },
    { L"Stream",                 CConfig::EAttribute::Stream                  },
};

struct SCloudStatusInfo
{
    CConfig::EAttribute attr;
    LPCWSTR             baseName;
    wchar_t             symbol;
};

static constexpr SCloudStatusInfo s_kCloudStatusInfos[] =
{
    { CConfig::EAttribute::CloudStatusCloudOnly,              L"CloudOnly",              UnicodeSymbols::CircleHollow     },
    { CConfig::EAttribute::CloudStatusLocallyAvailable,       L"LocallyAvailable",       UnicodeSymbols::CircleHalfFilled },
    { CConfig::EAttribute::CloudStatusAlwaysLocallyAvailable, L"AlwaysLocallyAvailable", UnicodeSymbols::CircleFilled     },
};

struct SFileAttrInfo
{
    LPCWSTR name;
    wchar_t letter;
    DWORD   dwAttribute;
};

static constexpr SFileAttrInfo s_kFileAttrInfos[] =
{
    { L"Read-only",      L'R', FILE_ATTRIBUTE_READONLY      },
    { L"Hidden",         L'H', FILE_ATTRIBUTE_HIDDEN        },
    { L"System",         L'S', FILE_ATTRIBUTE_SYSTEM        },
    { L"Archive",        L'A', FILE_ATTRIBUTE_ARCHIVE       },
    { L"Temporary",      L'T', FILE_ATTRIBUTE_TEMPORARY     },
    { L"Encrypted",      L'E', FILE_ATTRIBUTE_ENCRYPTED     },
    { L"Compressed",     L'C', FILE_ATTRIBUTE_COMPRESSED    },
    { L"Reparse point",  L'P', FILE_ATTRIBUTE_REPARSE_POINT },
    { L"Sparse file",    L'0', FILE_ATTRIBUTE_SPARSE_FILE   },
};

struct SSwitchInfo
{
    LPCWSTR name;
    LPCWSTR description;
};

static constexpr SSwitchInfo s_kSwitchInfos[] =
{
    { L"W",       L"Wide listing format"            },
    { L"S",       L"Recurse into subdirectories"    },
    { L"P",       L"Display performance timing"     },
    { L"M",       L"Multi-threaded enumeration"     },
    { L"B",       L"Bare listing format"            },
    { L"Owner",   L"Display file ownership"         },
    { L"Streams", L"Display alternate data streams" },
};





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
    // Format strings with indexed placeholders:
    // {0} = szShort (single-char switch prefix: "-" or "/")
    // {1} = pszLong (long switch prefix: "--" or "/")
    // {2} = pszMDisable (" -M-" or " /M-")
    // {3} = CircleHollow (with color marker)
    // {4} = CircleHalfFilled (with color marker)
    // {5} = CircleFilled (with color marker)
    // {6} = pszLongPad (extra padding for "/" mode to align descriptions)
    // {7} = GetArchitecture()
    // {8} = buildTimestamp
    // {9} = Copyright symbol
    //
    // Color markers use {{EAttributeName}} which format() escapes to {EAttributeName}
    // for ColorPuts to parse.

    // Header continuation after "Technicolor" (printed with PrintColorfulString)
    static constexpr wchar_t k_wszUsageHeader[] =
        L"{{Information}} Directory version " VERSION_WSTRING L" {7} ({8})\n"
        L"Copyright {9} 2004-" VERSION_YEAR_WSTRING L" by Robert Elmer\n"
        L"\n"
        L"{{InformationHighlight}}TCDIR{{Information}} "
        L"[{{InformationHighlight}}drive:{{Information}}]"
        L"[{{InformationHighlight}}path{{Information}}]"
        L"[{{InformationHighlight}}filename{{Information}}] "
        L"[{{InformationHighlight}}{0}A{{Information}}[[:]{{InformationHighlight}}attributes{{Information}}]] "
        L"[{{InformationHighlight}}{0}O{{Information}}[[:]{{InformationHighlight}}sortorder{{Information}}]] "
        L"[{{InformationHighlight}}{0}T{{Information}}[[:]{{InformationHighlight}}timefield{{Information}}]] "
        L"[{{InformationHighlight}}{0}S{{Information}}] "
        L"[{{InformationHighlight}}{0}W{{Information}}] "
        L"[{{InformationHighlight}}{0}B{{Information}}] "
        L"[{{InformationHighlight}}{0}P{{Information}}] "
        L"[{{InformationHighlight}}{0}M{{Information}}] "
        L"[{{InformationHighlight}}{1}Env{{Information}}] "
        L"[{{InformationHighlight}}{1}Config{{Information}}] "
        L"[{{InformationHighlight}}{1}Owner{{Information}}] "
        L"[{{InformationHighlight}}{1}Streams{{Information}}]"
#ifdef _DEBUG
        L" [{{InformationHighlight}}{1}Debug{{Information}}]"
#endif
        L"\n";

    static constexpr wchar_t k_wszUsageBody[] =
        L"{{Information}}\n"
        L"  [drive:][path][filename]\n"
        L"              Specifies drive, directory, and/or files to list.\n"
        L"\n"
        L"  {{InformationHighlight}}{0}A{{Information}}          Displays files with specified attributes.\n"
        L"  attributes   {{InformationHighlight}}D{{Information}}  Directories                {{InformationHighlight}}R{{Information}}  Read-only files\n"
        L"               {{InformationHighlight}}H{{Information}}  Hidden files               {{InformationHighlight}}A{{Information}}  Files ready for archiving\n"
        L"               {{InformationHighlight}}S{{Information}}  System files               {{InformationHighlight}}T{{Information}}  Temporary files\n"
        L"               {{InformationHighlight}}E{{Information}}  Encrypted files            {{InformationHighlight}}C{{Information}}  Compressed files\n"
        L"               {{InformationHighlight}}P{{Information}}  Reparse points             {{InformationHighlight}}0{{Information}}  Sparse files\n"
        L"               {{InformationHighlight}}X{{Information}}  Not content indexed        {{InformationHighlight}}I{{Information}}  Integrity stream (ReFS)\n"
        L"               {{InformationHighlight}}B{{Information}}  No scrub data (ReFS)       {{InformationHighlight}}O{{Information}}  Cloud-only (not local)\n"
        L"               {{InformationHighlight}}L{{Information}}  Locally available          {{InformationHighlight}}V{{Information}}  Always locally available\n"
        L"               {{InformationHighlight}}-{{Information}}  Prefix meaning not\n"
        L"\n"
        L"  Cloud status symbols shown between file size and name:\n"
        L"               {{CloudStatusCloudOnly}}{3}{{Information}}  Cloud-only (not locally available)\n"
        L"               {{CloudStatusLocallyAvailable}}{4}{{Information}}  Locally available (can be freed)\n"
        L"               {{CloudStatusAlwaysLocallyAvailable}}{5}{{Information}}  Always locally available (pinned)\n"
        L"\n"
        L"  {{InformationHighlight}}{0}O{{Information}}          List by files in sorted order.\n"
        L"  sortorder    {{InformationHighlight}}N{{Information}}  By name (alphabetic)       {{InformationHighlight}}S{{Information}}  By size (smallest first)\n"
        L"               {{InformationHighlight}}E{{Information}}  By extension (alphabetic)  {{InformationHighlight}}D{{Information}}  By date/time (oldest first)\n"
        L"               {{InformationHighlight}}-{{Information}}  Prefix to reverse order\n"
        L"\n"
        L"  {{InformationHighlight}}{0}T{{Information}}          Selects the time field for display and sorting.\n"
        L"  timefield    {{InformationHighlight}}C{{Information}}  Creation time              {{InformationHighlight}}A{{Information}}  Last access time\n"
        L"               {{InformationHighlight}}W{{Information}}  Last write time (default)\n"
        L"\n"
        L"  {{InformationHighlight}}{0}S{{Information}}          Displays files in specified directory and all subdirectories.\n"
        L"  {{InformationHighlight}}{0}W{{Information}}          Displays results in a wide listing format.\n"
        L"  {{InformationHighlight}}{0}B{{Information}}          Displays bare file names only (no headers, footers, or details).\n"
        L"  {{InformationHighlight}}{0}P{{Information}}          Displays performance timing information.\n"
        L"  {{InformationHighlight}}{0}M{{Information}}          Enables multi-threaded enumeration (default). Use{{InformationHighlight}}{2}{{Information}} to disable.\n"
        L"  {{InformationHighlight}}{1}Env{{Information}}       {6}Displays " TCDIR_ENV_VAR_NAME L" help, syntax, and current value.\n"
        L"  {{InformationHighlight}}{1}Config{{Information}}    {6}Displays current color configuration for all items and extensions.\n"
        L"  {{InformationHighlight}}{1}Owner{{Information}}     {6}Displays file owner (DOMAIN\\User) for each file.\n"
        L"  {{InformationHighlight}}{1}Streams{{Information}}   {6}Displays alternate data streams (NTFS only)."
#ifdef _DEBUG
        L"\n  {{InformationHighlight}}{1}Debug{{Information}}     {6}Displays raw file attributes in hex for diagnosing edge cases."
#endif
    ;

    // Determine prefix strings for single-char and multi-char switches
    wstring_view szShort        = (chPrefix == L'-') ? L"-"    : L"/";
    wstring_view pszLong        = (chPrefix == L'-') ? L"--"   : L"/";
    wstring_view pszMDisable    = (chPrefix == L'-') ? L" -M-" : L" /M-";
    wstring_view pszLongPad     = (chPrefix == L'-') ? L""     : L" ";
    wstring      buildTimestamp = VERSION_BUILD_TIMESTAMP;

    // Cloud status symbols (color markers are in the format string itself)
    wchar_t szCloudOnly    = UnicodeSymbols::CircleHollow;
    wchar_t szLocallyAvail = UnicodeSymbols::CircleHalfFilled;
    wchar_t szAlwaysLocal  = UnicodeSymbols::CircleFilled;



    // Format build timestamp without seconds (drop last 3 chars ":SS" from __TIME__)
    buildTimestamp.resize (buildTimestamp.length () - 3);

    // "Technicolor" is printed with special per-character color cycling
    console.Puts (CConfig::EAttribute::Default, L"");
    console.PrintColorfulString (L"Technicolor");

    // Print header and body using format() with indexed placeholders, then ColorPuts to parse color markers
    console.ColorPuts (format (k_wszUsageHeader,
                               szShort, 
                               pszLong, 
                               pszMDisable,
                               szCloudOnly, 
                               szLocallyAvail, 
                               szAlwaysLocal,
                               pszLongPad, 
                               GetArchitecture(), 
                               buildTimestamp, 
                               UnicodeSymbols::Copyright
                       ).c_str ());

    console.ColorPuts (format (k_wszUsageBody,
                               szShort, 
                               pszLong, 
                               pszMDisable,
                               szCloudOnly, 
                               szLocallyAvail, 
                               szAlwaysLocal,
                               pszLongPad, 
                               GetArchitecture(), 
                               buildTimestamp, 
                               UnicodeSymbols::Copyright
                       ).c_str ());
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

    console.ColorPrintf (L"{Default}\n{Error}There are some problems with your %s environment variable (see %senv for help):\n", 
                         TCDIR_ENV_VAR_NAME, pszLong);

    for (const auto & error : validationResult.errors)
    {
        size_t  prefixLen = 2 + error.message.length() + 5 + error.invalidTextOffset;
        wstring underline   (error.invalidText.length(), UnicodeSymbols::Overline);
        


        console.ColorPrintf (L"{Error}  %s in \"%s\"\n", error.message.c_str(), error.entry.c_str());
        console.ColorPrintf (L"{Default}%*s{Error}%s\n\n", static_cast<int>(prefixLen), L"", underline.c_str());
    }
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
    static constexpr int   COLUMN_WIDTH_ATTR       = 27;  // Sized for "AlwaysLocallyAvailable (●)"
    static constexpr int   COLUMN_WIDTH_SOURCE     = 15;

    wstring tableSeparator = L"  ";



    tableSeparator += wstring (COLUMN_WIDTH_ATTR + COLUMN_WIDTH_SOURCE + 2, UnicodeSymbols::LineHorizontal);

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
    console.Puts (CConfig::EAttribute::Information, L"\nCurrent display item configuration:\n");

    for (const auto & info : s_kDisplayItemInfos)
    {
        WORD attr  = console.m_configPtr->m_rgAttributes[info.attr];
        bool isEnv = (console.m_configPtr->m_rgAttributeSources[info.attr] == CConfig::EAttributeSource::Environment);

        DisplayItemAndSource (console, info.name, attr, isEnv, columnWidthAttr, columnWidthSource, 0, EItemDisplayMode::SingleColumn);
    }

    for (const auto & info : s_kCloudStatusInfos)
    {
        WORD    attr    = console.m_configPtr->m_rgAttributes[info.attr];
        bool    isEnv   = (console.m_configPtr->m_rgAttributeSources[info.attr] == CConfig::EAttributeSource::Environment);
        wstring display = format (L"{} ({})", info.baseName, info.symbol);

        DisplayItemAndSource (console, display, attr, isEnv, columnWidthAttr, columnWidthSource, 0, EItemDisplayMode::SingleColumn);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayFileAttributeConfiguration
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayFileAttributeConfiguration (CConsole & console, int columnWidthAttr, int columnWidthSource)
{
    console.Puts (CConfig::EAttribute::Information, L"\nFile attribute color configuration:\n");

    for (const auto & info : s_kFileAttrInfos)
    {
        auto iter = console.m_configPtr->m_mapFileAttributesTextAttr.find (info.dwAttribute);
        if (iter == console.m_configPtr->m_mapFileAttributesTextAttr.end())
        {
            continue;
        }

        WORD    attr  = iter->second.m_wAttr;
        bool    isEnv = (iter->second.m_source == CConfig::EAttributeSource::Environment);
        wstring label = format (L"{} {}", info.letter, info.name);
        
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

            const auto & ext        = extensions[idx].first;
            WORD         extAttr    = extensions[idx].second;
            auto         sourceIter = console.m_configPtr->m_mapExtensionSources.find (ext);
            bool         isEnv      = (sourceIter         != console.m_configPtr->m_mapExtensionSources.end () &&
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



    console.Puts (CConfig::EAttribute::Information, L"\nFile extension color configuration:");

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

    console.ColorPrintf (L"{Information}Your settings:{Default}\n\n  {Information}%s{Default} = ", 
                         pszEnvVarName);

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
//  CUsage::HasEnvVarSwitches
//
////////////////////////////////////////////////////////////////////////////////

static bool HasEnvVarSwitches (const CConfig & config)
{
    return config.m_fWideListing.has_value()   ||
           config.m_fRecurse.has_value()       ||
           config.m_fPerfTimer.has_value()     ||
           config.m_fMultiThreaded.has_value() ||
           config.m_fBareListing.has_value()   ||
           config.m_fShowOwner.has_value()     ||
           config.m_fShowStreams.has_value();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::HasEnvVarDisplayItems
//
////////////////////////////////////////////////////////////////////////////////

static bool HasEnvVarDisplayItems (const CConfig & config)
{
    auto isFromEnv = [&config](const auto & info)
    {
        return config.m_rgAttributeSources[info.attr] == CConfig::EAttributeSource::Environment;
    };

    return ranges::any_of (s_kDisplayItemInfos, isFromEnv) ||
           ranges::any_of (s_kCloudStatusInfos, isFromEnv);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::HasEnvVarFileAttrs
//
////////////////////////////////////////////////////////////////////////////////

static bool HasEnvVarFileAttrs (const CConfig & config)
{
    return ranges::any_of (config.m_mapFileAttributesTextAttr, [](const auto & pair)
    {
        return pair.second.m_source == CConfig::EAttributeSource::Environment;
    });
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::HasEnvVarExtensions
//
////////////////////////////////////////////////////////////////////////////////

static bool HasEnvVarExtensions (const CConfig & config)
{
    return ranges::any_of (config.m_mapExtensionSources, [](const auto & pair)
    {
        return pair.second == CConfig::EAttributeSource::Environment;
    });
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayEnvVarSwitchesSection
//
////////////////////////////////////////////////////////////////////////////////

static void DisplayEnvVarSwitchesSection (CConsole & console, const CConfig & config)
{
    const optional<bool> * switchValues[] =
    {
        &config.m_fWideListing,
        &config.m_fRecurse,
        &config.m_fPerfTimer,
        &config.m_fMultiThreaded,
        &config.m_fBareListing,
        &config.m_fShowOwner,
        &config.m_fShowStreams,
    };

    static_assert (_countof (switchValues) == _countof (s_kSwitchInfos), "Switch arrays must match");



    console.Puts (CConfig::EAttribute::Information, L"    Switches:");

    for (size_t i = 0; i < _countof (s_kSwitchInfos); ++i)
    {
        if (switchValues[i]->has_value())
        {
            console.Printf (CConfig::EAttribute::Default, L"      %-8ls %ls\n", 
                            s_kSwitchInfos[i].name, s_kSwitchInfos[i].description);
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayEnvVarDisplayItemsSection
//
////////////////////////////////////////////////////////////////////////////////

static void DisplayEnvVarDisplayItemsSection (CConsole & console, const CConfig & config)
{
    console.ColorPuts (L"{Default}\n    {Information}Display item colors:");

    for (const auto & info : s_kDisplayItemInfos)
    {
        if (config.m_rgAttributeSources[info.attr] == CConfig::EAttributeSource::Environment)
        {
            WORD attr = config.m_rgAttributes[info.attr];

            console.Printf (CConfig::EAttribute::Default, L"      ");
            console.Printf (attr, L"%ls", info.name);
            console.Puts   (CConfig::EAttribute::Default, L"");
        }
    }

    for (const auto & info : s_kCloudStatusInfos)
    {
        if (config.m_rgAttributeSources[info.attr] == CConfig::EAttributeSource::Environment)
        {
            WORD    attr    = config.m_rgAttributes[info.attr];
            wstring display = format (L"{} ({})", info.baseName, info.symbol);

            console.Printf (CConfig::EAttribute::Default, L"      ");
            console.Printf (attr, L"%ls", display.c_str());
            console.Puts   (CConfig::EAttribute::Default, L"");
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayEnvVarFileAttrsSection
//
////////////////////////////////////////////////////////////////////////////////

static void DisplayEnvVarFileAttrsSection (CConsole & console, const CConfig & config)
{
    console.ColorPuts (L"{Default}\n    {Information}File attribute colors:");

    for (const auto & info : s_kFileAttrInfos)
    {
        auto iter = config.m_mapFileAttributesTextAttr.find (info.dwAttribute);
        if (iter != config.m_mapFileAttributesTextAttr.end() &&
            iter->second.m_source == CConfig::EAttributeSource::Environment)
        {
            WORD attr = iter->second.m_wAttr;

            console.Printf (CConfig::EAttribute::Default, L"      ");
            console.Printf (attr, L"%lc %ls", info.letter, info.name);
            console.Puts   (CConfig::EAttribute::Default, L"");
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayEnvVarExtensionsSection
//
////////////////////////////////////////////////////////////////////////////////

static void DisplayEnvVarExtensionsSection (CConsole & console, const CConfig & config)
{
    console.ColorPuts (L"{Default}\n    {Information}File extension colors:");

    // Collect extensions from env var, sorted
    vector<pair<wstring, WORD>> envExtensions;
    for (const auto & [ext, source] : config.m_mapExtensionSources)
    {
        if (source == CConfig::EAttributeSource::Environment)
        {
            auto iter = config.m_mapExtensionToTextAttr.find (ext);
            if (iter != config.m_mapExtensionToTextAttr.end())
            {
                envExtensions.emplace_back (ext, iter->second);
            }
        }
    }

    ranges::sort (envExtensions, {}, &pair<wstring, WORD>::first);

    for (const auto & [ext, attr] : envExtensions)
    {
        console.Printf (CConfig::EAttribute::Default, L"      ");
        console.Printf (attr, L"%ls", ext.c_str());
        console.Puts   (CConfig::EAttribute::Default, L"");
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayEnvVarDecodedSettings
//
//  Display decoded settings from the TCDIR environment variable.
//  Shows switches (with descriptions), display items, file attributes, 
//  and file extensions - only those set via the environment variable.
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayEnvVarDecodedSettings (CConsole & console)
{
    CConfig & config      = *console.m_configPtr;
    bool hasSwitches      = HasEnvVarSwitches     (config);
    bool hasDisplayItems  = HasEnvVarDisplayItems (config);
    bool hasFileAttrs     = HasEnvVarFileAttrs    (config);
    bool hasExtensions    = HasEnvVarExtensions   (config);



    if (!hasSwitches && !hasDisplayItems && !hasFileAttrs && !hasExtensions)
    {
        return;
    }

    if (hasSwitches)
    {
        DisplayEnvVarSwitchesSection (console, config);
    }

    if (hasDisplayItems)
    {
        DisplayEnvVarDisplayItemsSection (console, config);
    }

    if (hasFileAttrs)
    {
        DisplayEnvVarFileAttrsSection (console, config);
    }

    if (hasExtensions)
    {
        DisplayEnvVarExtensionsSection (console, config);
    }
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
    // Format strings with indexed placeholders:
    // {0} = syntax command (set TCDIR= or $env:TCDIR = ")
    // {1} = syntax suffix (nothing or closing quote)
    // {2} = example command
    //
    // Color markers use {{EAttributeName}} which format() escapes to {EAttributeName}
    // for ColorPuts to parse.

    static constexpr wchar_t k_wszEnvVarHelpBody[] =
        L"\n"
        L"{{Information}}Set the {{InformationHighlight}}" TCDIR_ENV_VAR_NAME L"{{Information}} environment variable to override default colors for display"
        L" items, file attributes, or file extensions:\n"
        L"{0}[{{InformationHighlight}}<Switch>{{Information}}] | "
        L"[{{InformationHighlight}}<Item>{{Information}} | "
        L"{{InformationHighlight}}Attr:<fileattr>{{Information}} | "
        L"{{InformationHighlight}}<.ext>{{Information}}] = "
        L"{{InformationHighlight}}<Fore>{{Information}} [on {{InformationHighlight}}<Back>{{Information}}][;...]"
        L"{1}\n"
        L"\n"
        L"  {{InformationHighlight}}<Switch>{{Information}}    A command-line switch:\n"
        L"                  {{InformationHighlight}}W{{Information}}        Wide listing format\n"
        L"                  {{InformationHighlight}}P{{Information}}        Display performance timing information\n"
        L"                  {{InformationHighlight}}S{{Information}}        Recurse into subdirectories\n"
        L"                  {{InformationHighlight}}M{{Information}}        Enables multi-threaded enumeration (default); use {{InformationHighlight}}M-{{Information}} to disable\n"
        L"                  {{InformationHighlight}}Owner{{Information}}    Display file ownership\n"
        L"                  {{InformationHighlight}}Streams{{Information}}  Display alternate data streams (NTFS)\n"
        L"\n"
        L"  {{InformationHighlight}}<Item>{{Information}}      A display item:\n"
        L"                  {{InformationHighlight}}D{{Information}}  Date                     {{InformationHighlight}}T{{Information}}  Time\n"
        L"                  {{InformationHighlight}}S{{Information}}  Size                     {{InformationHighlight}}R{{Information}}  Directory name\n"
        L"                  {{InformationHighlight}}I{{Information}}  Information              {{InformationHighlight}}H{{Information}}  Information highlight\n"
        L"                  {{InformationHighlight}}E{{Information}}  Error                    {{InformationHighlight}}F{{Information}}  File (default)\n"
        L"                  {{InformationHighlight}}O{{Information}}  Owner                    {{InformationHighlight}}M{{Information}}  Stream\n"
        L"\n"
        L"              Cloud status (use full name, e.g., {{InformationHighlight}}CloudOnly=Blue{{Information}}):\n"
        L"                  {{InformationHighlight}}CloudOnly{{Information}}                   {{InformationHighlight}}LocallyAvailable{{Information}}\n"
        L"                  {{InformationHighlight}}AlwaysLocallyAvailable{{Information}}\n"
        L"\n"
        L"  {{InformationHighlight}}<.ext>{{Information}}      A file extension, including the leading period.\n"
        L"\n"
        L"  {{InformationHighlight}}<FileAttr>{{Information}}  A file attribute (see file attributes below)\n"
        L"                  {{InformationHighlight}}R{{Information}}  Read-only                {{InformationHighlight}}H{{Information}}  Hidden\n"
        L"                  {{InformationHighlight}}S{{Information}}  System                   {{InformationHighlight}}A{{Information}}  Archive\n"
        L"                  {{InformationHighlight}}T{{Information}}  Temporary                {{InformationHighlight}}E{{Information}}  Encrypted\n"
        L"                  {{InformationHighlight}}C{{Information}}  Compressed               {{InformationHighlight}}P{{Information}}  Reparse point\n"
        L"                  {{InformationHighlight}}0{{Information}}  Sparse file\n"
        L"\n"
        L"  {{InformationHighlight}}<Fore>{{Information}}      Foreground color\n"
        L"  {{InformationHighlight}}<Back>{{Information}}      Background color";

    wstring_view syntaxCommand;
    wstring_view syntaxSuffix;
    wstring_view exampleCmd; 
    
    if (IsPowerShell())
    {
        syntaxCommand = L"  {InformationHighlight}$env:" TCDIR_ENV_VAR_NAME L"{Information} = \"";
        syntaxSuffix  = L"\"";
        exampleCmd    = L"{Information}  Example: {InformationHighlight}$env:" TCDIR_ENV_VAR_NAME L"{Information} = \"W;D=LightGreen;S=Yellow;Attr:H=DarkGrey;.cpp=White on Blue\"";
    }
    else
    {
        syntaxCommand = L"  set {InformationHighlight}" TCDIR_ENV_VAR_NAME L"{Information} =";
        syntaxSuffix  = L"";
        exampleCmd    = L"{Information}  Example: {InformationHighlight}set "  TCDIR_ENV_VAR_NAME L"{Information} = W;D=LightGreen;S=Yellow;Attr:H=DarkGrey;.cpp=White on Blue";
    }

    console.ColorPuts (format (k_wszEnvVarHelpBody, syntaxCommand, syntaxSuffix, exampleCmd).c_str());

    DisplayColorConfiguration (console);

    console.ColorPrintf (L"{Default}%s\n\n", exampleCmd.data ());

    if (IsTcdirEnvVarSet ())
    {
        DisplayEnvVarCurrentValue    (console, TCDIR_ENV_VAR_NAME);
        DisplayEnvVarDecodedSettings (console);
        DisplayEnvVarIssues          (console);
    }
    else
    {
        console.ColorPuts (L"  {InformationHighlight}" TCDIR_ENV_VAR_NAME L"{Information} environment variable is not set.");
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
    if (IsTcdirEnvVarSet ())
    {
        DisplayEnvVarIssues (console, chPrefix);
    }
    else
    {
        console.ColorPuts (L"\n  {Information}" TCDIR_ENV_VAR_NAME L"{Default} environment variable is not set; showing default configuration.");
    }

    DisplayConfigurationTable (console);
}
