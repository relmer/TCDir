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
    WCHAR buffer[1];
    DWORD result = GetEnvironmentVariableW (kpszEnvVarName, buffer, ARRAYSIZE(buffer));

    // If result > 0, the variable exists (even if buffer is too small)
    return result > 0;
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

    static LPCWSTR s_usageLines[] =
    {
        L"Copyright " COPYRIGHT " 2004-" VERSION_YEAR_WSTRING  L" by Robert Elmer",
        L"",
        L"TCDIR [drive:][path][filename] [/A[[:]attributes]] [/O[[:]sortorder]] [/S] [/W] [/P] [/M] [/Env]",
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
        L"  /Env        Displays " TCDIR_ENV_VAR_NAME L" help, configuration, and current colors.",
        L"",
        L"",
        L""
    };



    //
    // Display usage
    //

    console.Puts (CConfig::EAttribute::Default, L"");
    console.PrintColorfulString (L"Technicolor");
    console.Puts (CConfig::EAttribute::Default, L" Directory version " VERSION_WSTRING);

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

HRESULT CUsage::DisplayEnvVarIssues (CConsole & console)
{
    HRESULT                   hr               = S_OK;
    CConfig::ValidationResult validationResult;



    validationResult = console.m_configPtr->ValidateEnvironmentVariable();
    BAIL_OUT_IF (!validationResult.hasIssues(), S_OK);

    console.Puts (CConfig::EAttribute::Error, L"");
    console.Puts (CConfig::EAttribute::Error, L"TCDIR Configuration Issues Detected:");
    console.Puts (CConfig::EAttribute::Error, L"");

    for (const auto & error : validationResult.errors)
    {
        console.Printf (CConfig::EAttribute::Error, L"  ? Error:  %s", error.c_str());
        console.Puts   (CConfig::EAttribute::Default, L"");
    }

    for (const auto & warning : validationResult.warnings)
    {
        console.Printf (CConfig::EAttribute::Information, L"  ? Warning:  %s", warning.c_str());
        console.Puts   (CConfig::EAttribute::Default, L"");
    }

    console.Puts (CConfig::EAttribute::Default, L"");

Error:
    return hr;
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

    DisplayAttributeConfiguration (console, tableSeparator, COLUMN_WIDTH_ATTR, COLUMN_WIDTH_SOURCE);
    DisplayExtensionConfiguration (console, tableSeparator, COLUMN_WIDTH_ATTR, COLUMN_WIDTH_SOURCE);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayAttributeConfiguration
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayAttributeConfiguration (CConsole & console, const wstring & tableSeparator, int columnWidthAttr, int columnWidthSource)
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

    wstring tableHeader;



    tableHeader = format (L"  {:<{}}  {:<{}}",
                          L"Attribute type", columnWidthAttr,
                          L"Source",         columnWidthSource);

    console.Puts (CConfig::EAttribute::Information, L"");
    console.Puts (CConfig::EAttribute::Information, L"Current display attribute configuration:");

    console.Puts (CConfig::EAttribute::Information,   tableHeader.c_str());
    console.Puts (CConfig::EAttribute::SeparatorLine, tableSeparator.c_str());

    for (const auto & info : s_attrInfos)
    {
        WORD    attr   = console.m_configPtr->m_rgAttributes[info.attr];
        bool    isEnv  = (console.m_configPtr->m_rgAttributeSources[info.attr] == CConfig::EAttributeSource::Environment);
        LPCWSTR source = isEnv ? L"Environment" : L"Default";
        WORD    bgAttr = console.m_configPtr->m_rgAttributes[CConfig::EAttribute::Default] & BC_Mask;
        WORD    sourceAttr = static_cast<WORD> (bgAttr | (isEnv ? FC_Cyan : FC_DarkGrey));
        int     pad    = max (0, columnWidthAttr - static_cast<int> (wcslen (info.name)));

        console.Printf (CConfig::EAttribute::Information, L"  ");
        console.Printf (attr,                             L"%ls", info.name);
        console.Printf (CConfig::EAttribute::Information, L"%*ls  ", pad, L"");
        console.Printf (sourceAttr,                       L"%-*ls", columnWidthSource, source);
        console.Printf (CConfig::EAttribute::Default,     L"\n");
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayExtensionConfigurationSingleColumn
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayExtensionConfigurationSingleColumn (CConsole & console, const wstring & tableSeparator, int columnWidthAttr, int columnWidthSource, const vector<pair<wstring, WORD>> & extensions)
{
    wstring tableHeader;
    WORD    bgAttr = console.m_configPtr->m_rgAttributes[CConfig::EAttribute::Default] & BC_Mask;



    tableHeader = format (L"  {:<{}}  {:<{}}",
                          L"Extension", columnWidthAttr,
                          L"Source",    columnWidthSource);

    console.Puts (CConfig::EAttribute::Information,   tableHeader.c_str());
    console.Puts (CConfig::EAttribute::SeparatorLine, tableSeparator.c_str());

    for (const auto & [ext, extAttr] : extensions)
    {
        auto    sourceIter = console.m_configPtr->m_mapExtensionSources.find (ext);
        bool    isEnv      = (sourceIter         != console.m_configPtr->m_mapExtensionSources.end () &&
                              sourceIter->second == CConfig::EAttributeSource::Environment);
        LPCWSTR source     = isEnv ? L"Environment" : L"Default";
        WORD    sourceAttr = static_cast<WORD> (bgAttr | (isEnv ? FC_Cyan : FC_DarkGrey));
        int pad = max (0, columnWidthAttr - static_cast<int> (ext.size ()));



        console.Printf (CConfig::EAttribute::Information, L"  ");
        console.Printf (extAttr,                          L"%ls", ext.c_str ());
        console.Printf (CConfig::EAttribute::Information, L"%*ls  ", pad, L"");
        console.Printf (sourceAttr,                       L"%-*ls", columnWidthSource, source);
        console.Printf (CConfig::EAttribute::Default,     L"\n");
    }

    console.Puts (CConfig::EAttribute::Default, L"");
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
    WORD   bgAttr          = console.m_configPtr->m_rgAttributes[CConfig::EAttribute::Default] & BC_Mask;



    console.Puts (CConfig::EAttribute::Information, L"");

    for (size_t nRow = 0; nRow < cRows; ++nRow)
    {
        console.Printf (CConfig::EAttribute::Information, L"  ");

        for (size_t nCol = 0; nCol < cColumns; ++nCol)
        {
            size_t  idx      = 0;
            size_t  fullRows = cItemsInLastRow ? cRows - 1 : cRows;
            LPCWSTR source   = nullptr;
            size_t  cxUsed   = 0;



            if ((nRow * cColumns + nCol) >= extensions.size ())
            {
                break;
            }

            // Column-major ordering (see ResultsDisplayerWide).
            idx = nRow + (nCol * fullRows);

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

            auto sourceIter = console.m_configPtr->m_mapExtensionSources.find (ext);
            bool isEnv = (sourceIter         != console.m_configPtr->m_mapExtensionSources.end () &&
                          sourceIter->second == CConfig::EAttributeSource::Environment);
            WORD sourceAttr = static_cast<WORD> (bgAttr | (isEnv ? FC_Cyan : FC_DarkGrey));
            source = isEnv ? L"Environment" : L"Default";

            console.Printf (extAttr,                          L"%ls", ext.c_str ());
            console.Printf (CConfig::EAttribute::Information, L"%*ls", static_cast<int> (maxExtLen - ext.size ()), L"");
            console.Printf (CConfig::EAttribute::Information, L"  ");
            console.Printf (sourceAttr,                       L"%-*ls", static_cast<int> (cxSourceWidth), source);

            cxUsed = maxExtLen + 2 + cxSourceWidth;
            if (cxColumnWidth > cxUsed)
            {
                console.Printf (CConfig::EAttribute::Information, L"%*ls", static_cast<int> (cxColumnWidth - cxUsed), L"");
            }
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

void CUsage::DisplayExtensionConfiguration (CConsole & console, const wstring & tableSeparator, int columnWidthAttr, int columnWidthSource)
{
    vector<pair<wstring, WORD>> extensions;

    UINT   cxConsoleWidth  = console.GetWidth ();
    size_t maxExtLen       = 0;
    size_t cxIndent        = 2;
    size_t cxAvailable     = (cxConsoleWidth > cxIndent) ? (cxConsoleWidth - cxIndent) : cxConsoleWidth;
    size_t cxSourceWidth   = wcslen (L"Environment");
    size_t cColumns        = 1;



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
        size_t cxMinColumnWidth = maxExtLen + 2 + cxSourceWidth + 1;

        if (cxMinColumnWidth > 0 && cxMinColumnWidth <= cxAvailable)
        {
            cColumns = max (static_cast<size_t> (1), cxAvailable / cxMinColumnWidth);
        }
    }

    if (cColumns == 1)
    {
        DisplayExtensionConfigurationSingleColumn (console, tableSeparator, columnWidthAttr, columnWidthSource, extensions);
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
    static constexpr int COLUMN_WIDTH_COLOR_LEFT = 13;

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



    console.Puts (CConfig::EAttribute::Default, L"  Colors:");

    for (const auto & row : s_colorRows)
    {
        int pad = max (0, COLUMN_WIDTH_COLOR_LEFT - static_cast<int> (wcslen (row.left)));

        console.Printf (CConfig::EAttribute::Default, L"      ");
        console.Printf (GetColorAttribute (console, row.left),  L"%ls", row.left);
        console.Printf (CConfig::EAttribute::Default, L"%*ls", pad, L"");
        console.Printf (GetColorAttribute (console, row.right), L"%ls", row.right);
        console.Printf (CConfig::EAttribute::Default, L"\n");
    }

    console.Puts (CConfig::EAttribute::Default, L"");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayEnvVarConfigurationReport
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayEnvVarConfigurationReport (CConsole & console)
{
    static LPCWSTR s_envVarSyntax[] =
    {
        L"  set " TCDIR_ENV_VAR_NAME L"=<attr|.ext>=<fore>[on <back>][;...]",           // CMD
        L"  $env:" TCDIR_ENV_VAR_NAME L" = \"<attr|.ext>=<fore>[on <back>][;...]\"",    // PowerShell
    };

    static LPCWSTR s_colorOverrideLines[] =
    {
        L"",
        L"  <attr>  A display attribute (single character)",
        L"  <.ext>  A file extension, including the leading period.",
        L"  <fore>  Foreground color",
        L"  <back>  Background color",
        L"",
        L"  Display attributes:",
        L"      D  Date                   T  Time",
        L"      A  File attribute present -  File attribute not present",
        L"      S  Size                   R  Directory name",
        L"      I  Information            H  Information highlight",
        L"      E  Error                  F  File (default)",
        L"",
    };

    static LPCWSTR s_envVarExample[] =
    {
        L"  Example: set "  TCDIR_ENV_VAR_NAME L"=D=LightGreen;S=Yellow;.cpp=White on Blue",           // CMD
        L"  Example: $env:" TCDIR_ENV_VAR_NAME L" = \"D=LightGreen;S=Yellow;.cpp=White on Blue\"",    // PowerShell
    };

    bool  isPowerShell = IsPowerShell ();



    console.Puts   (CConfig::EAttribute::Default,     L"");
    console.Printf (CConfig::EAttribute::Default,     L"Set the ");
    console.Printf (CConfig::EAttribute::Information, TCDIR_ENV_VAR_NAME);
    console.Printf (CConfig::EAttribute::Default,     L" environment variable to override default colors for file ");
    console.Puts   (CConfig::EAttribute::Default,     L"extensions or display attributes:");
    console.Puts   (CConfig::EAttribute::Default,     L"");

    console.Puts (CConfig::EAttribute::Default, s_envVarSyntax[isPowerShell]);

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
        console.Puts (CConfig::EAttribute::Default, L"");
        console.Puts (CConfig::EAttribute::Default, L"TCDIR environment variable is not set; showing default configuration:");
        console.Puts (CConfig::EAttribute::Default, L"");
    }

    DisplayConfigurationTable (console);
}
