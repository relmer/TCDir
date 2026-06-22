#include "pch.h"
#include "Usage.h"

#include "Color.h"
#include "Config.h"
#include "Console.h"
#include "IconMapping.h"
#include "NerdFontConstants.h"
#include "NerdFontDetector.h"
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
    { L"W",          L"Wide listing format"             },
    { L"S",          L"Recurse into subdirectories"     },
    { L"P",          L"Display performance timing"      },
    { L"M",          L"Multi-threaded enumeration"      },
    { L"B",          L"Bare listing format"             },
    { L"Owner",      L"Display file ownership"          },
    { L"Streams",    L"Display alternate data streams"  },
    { L"Icons",      L"Enable file-type icons"          },
    { L"Tree",       L"Display directory tree view"     },
    { L"Ellipsize",  L"Truncate long link target paths" },
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
//  MeasureVisibleWidth
//
//  Returns the visible character count of a string that contains color
//  markers in the form {MarkerName}.  Characters inside braces are skipped.
//
////////////////////////////////////////////////////////////////////////////////

static int MeasureVisibleWidth (wstring_view text)
{
    int    cch    = 0;
    bool   fInTag = false;



    for (wchar_t ch : text)
    {
        if (ch == L'{')
        {
            fInTag = true;
            continue;
        }

        if (ch == L'}')
        {
            fInTag = false;
            continue;
        }

        if (!fInTag)
        {
            ++cch;
        }
    }

    return cch;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplaySynopsis
//
//  Prints the TCDIR synopsis line with dynamic word-wrapping.
//  Options that would exceed the console width wrap to a new line,
//  indented to align under [drive:] from the first line.
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplaySynopsis (CConsole & console, const vector<wstring> & tokens)
{
    constexpr int INDENT = 6;

    int     cxConsole = static_cast<int>(console.GetWidth());
    int     col       = INDENT;
    wstring line;

    wstring indentStr (INDENT, L' ');



    // Start with "TCDIR "
    line.append (L"{InformationHighlight}TCDIR{Information} ");

    // Append each token, inserting line breaks when necessary
    for (const wstring & token : tokens)
    {
        int cchVisible = MeasureVisibleWidth (token);

        if (col + cchVisible > cxConsole && col > INDENT)
        {
            // Wrap to next line, indented under [drive:]
            line.append (L"\n");
            line.append (indentStr);
            col = INDENT;
        }

        line.append (token);
        col += cchVisible;
    }

    console.ColorPuts (line.c_str());
}





////////////////////////////////////////////////////////////////////////////////
//
//  Usage option row data
//
////////////////////////////////////////////////////////////////////////////////

struct SUsageOptionRow
{
    wstring label;
    wstring description;
    wstring continuation;
};

struct SThreeColumnRow
{
    wstring col1;
    wstring col2;
    wstring col3;
};




////////////////////////////////////////////////////////////////////////////////
//
//  CodePointToWString
//
////////////////////////////////////////////////////////////////////////////////

static wstring CodePointToWString (char32_t cp)
{
    WideCharPair pair = CodePointToWideChars (cp);
    return wstring (pair.chars, pair.count);
}




////////////////////////////////////////////////////////////////////////////////
//
//  GetMaxVisibleWidth
//
////////////////////////////////////////////////////////////////////////////////

static int GetMaxVisibleWidth (const vector<wstring> & values)
{
    int cchMax = 0;

    for (const wstring & value : values)
    {
        cchMax = (std::max) (cchMax, MeasureVisibleWidth (value));
    }

    return cchMax;
}




////////////////////////////////////////////////////////////////////////////////
//
//  AppendAlignedRow
//
////////////////////////////////////////////////////////////////////////////////

static void AppendAlignedRow (wstring    & text,
                              int          indent,
                              int          descCol,
                              wstring_view label,
                              wstring_view description)
{
    int labelEnd = indent + MeasureVisibleWidth (label);
    int pad      = (std::max) (1, descCol - labelEnd);

    text.append (indent, L' ');
    text.append (label);
    text.append (pad, L' ');
    text.append (description);
    text.push_back (L'\n');
}




////////////////////////////////////////////////////////////////////////////////
//
//  AppendThreeColumnRows
//
////////////////////////////////////////////////////////////////////////////////

static void AppendThreeColumnRows (wstring                       & text,
                                   int                             indent,
                                   int                             col2,
                                   int                             gap,
                                   const vector<SThreeColumnRow> & rows,
                                   optional<int>                   col3OffsetFromCol2 = nullopt)
{
    vector<wstring> col2Values;
    col2Values.reserve (rows.size ());

    for (const SThreeColumnRow & row : rows)
    {
        col2Values.push_back (row.col2);
    }

    int maxCol2Width = GetMaxVisibleWidth (col2Values);
    int col3         = col3OffsetFromCol2.has_value() ? col2 + col3OffsetFromCol2.value()
                                                      : col2 + maxCol2Width + gap;

    for (const SThreeColumnRow & row : rows)
    {
        text.append (indent, L' ');
        text.append (row.col1);

        int col1End = indent + MeasureVisibleWidth (row.col1);
        int pad1    = (std::max) (1, col2 - col1End);

        text.append (pad1, L' ');
        text.append (row.col2);

        if (!row.col3.empty ())
        {
            int col2End = col2 + MeasureVisibleWidth (row.col2);
            int pad2    = (std::max) (1, col3 - col2End);

            text.append (pad2, L' ');
            text.append (row.col3);
        }

        text.push_back (L'\n');
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  AppendDetailGrid
//
//  Renders a sub-level three-column detail grid (e.g. attribute/sort/time
//  legends), auto-sizing the second column.  Returns the computed col2 so a
//  following grid can be aligned to it.
//
////////////////////////////////////////////////////////////////////////////////

static int AppendDetailGrid (wstring                       & usageBody,
                             int                             indent,
                             int                             subLevelDetailCol,
                             int                             columnGap,
                             int                             secondColOffset,
                             const vector<SThreeColumnRow> & rows)
{
    vector<wstring> labels;
    labels.reserve (rows.size ());

    for (const SThreeColumnRow & row : rows)
    {
        labels.push_back (row.col1);
    }

    int col2 = (std::max) (subLevelDetailCol, indent + GetMaxVisibleWidth (labels) + columnGap);

    AppendThreeColumnRows (usageBody, indent, col2, columnGap, rows, secondColOffset);
    return col2;
}




////////////////////////////////////////////////////////////////////////////////
//
//  ResolveUsageIconState
//
//  Resolves whether file-type icons are active: CLI flag > config/env var >
//  Nerd Font auto-detection.
//
////////////////////////////////////////////////////////////////////////////////

static bool ResolveUsageIconState (CConsole & console, optional<bool> fIconsCli)
{
    const CConfig & config = *console.m_configPtr;

    if (fIconsCli.has_value ())
    {
        return fIconsCli.value ();
    }

    if (config.m_fIcons.has_value ())
    {
        return config.m_fIcons.value ();
    }

    CNerdFontDetector detector;
    EDetectionResult  result = EDetectionResult::NotDetected;
    HANDLE            hOut   = GetStdHandle (STD_OUTPUT_HANDLE);

    if (SUCCEEDED (detector.Detect (hOut, *config.m_pEnvironmentProvider, result)))
    {
        return (result == EDetectionResult::Detected);
    }

    return false;
}




////////////////////////////////////////////////////////////////////////////////
//
//  PrintUsageHeader
//
//  Prints the "Technicolor" banner and the version/copyright header line.
//
////////////////////////////////////////////////////////////////////////////////

static void PrintUsageHeader (CConsole & console)
{
    // Header continuation after "Technicolor" (printed with PrintColorfulString)
    static constexpr wchar_t k_wszUsageHeader[] =
        L"{{Information}} Directory version " VERSION_WSTRING L" {0} ({1})\n"
        L"Copyright {2} 2004-" VERSION_YEAR_WSTRING L" by Robert Elmer\n";

    wstring buildTimestamp = VERSION_BUILD_TIMESTAMP;

    // Format build timestamp without seconds (drop last 3 chars ":SS" from __TIME__)
    buildTimestamp.resize (buildTimestamp.length () - 3);

    // "Technicolor" is printed with special per-character color cycling
    console.Puts (CConfig::EAttribute::Default, L"");
    console.PrintColorfulString (L"Technicolor");

    // Print header using format() with indexed placeholders, then ColorPuts to parse color markers
    console.ColorPuts (format (k_wszUsageHeader,
                              GetArchitecture(),
                              buildTimestamp,
                              UnicodeSymbols::Copyright
                       ).c_str ());
}




////////////////////////////////////////////////////////////////////////////////
//
//  BuildPrimaryOptionRows
//
////////////////////////////////////////////////////////////////////////////////

static vector<SUsageOptionRow> BuildPrimaryOptionRows (wstring_view szShort, wstring_view pszLong, wstring_view pszMDisable)
{
    return
    {
        { format (L"{{InformationHighlight}}{0}A{{Information}}", szShort), 
          L"Displays files with specified attributes.",
          L"" },
        { format (L"{{InformationHighlight}}{0}O{{Information}}", szShort),
          L"List by files in sorted order.",
          L"" },
        { format (L"{{InformationHighlight}}{0}T{{Information}}", szShort),
          L"Selects the time field for display and sorting.",
          L"" },
        { format (L"{{InformationHighlight}}{0}S{{Information}}", szShort),
          L"Displays files in specified directory and all subdirectories.",
          L"" },
        { format (L"{{InformationHighlight}}{0}W{{Information}}", szShort),
          L"Displays results in a wide listing format.",
          L"" },
        { format (L"{{InformationHighlight}}{0}B{{Information}}", szShort),
          L"Displays bare file names only (no headers, footers, or details).",
          L"" },
        { format (L"{{InformationHighlight}}{0}P{{Information}}", szShort),
          L"Displays performance timing information.",
          L"" },
        { format (L"{{InformationHighlight}}{0}M{{Information}}", szShort),
          format (L"Enables multi-threaded enumeration (default). Use{{InformationHighlight}}{0}{{Information}} to disable.", pszMDisable),
          L"" },
        { format (L"{{InformationHighlight}}{0}Env{{Information}}", pszLong),
          L"Displays " TCDIR_ENV_VAR_NAME L" help, syntax, and current value.",
          L"" },
        { format (L"{{InformationHighlight}}{0}Config{{Information}}", pszLong),
          L"Displays config file diagnostics, syntax reference, and parse errors.",
          L"" },
        { format (L"{{InformationHighlight}}{0}Settings{{Information}}", pszLong),
          L"Displays current merged configuration for all items and extensions.",
          L"" },
        { format (L"{{InformationHighlight}}{0}Owner{{Information}}", pszLong),
          format (L"Displays the owner of each file and directory. Not allowed with {{InformationHighlight}}{0}Tree{{Information}}.", pszLong),
          L"" },
        { format (L"{{InformationHighlight}}{0}Streams{{Information}}", pszLong),
          L"Displays alternate data streams (NTFS only).",
          L"" },
        { format (L"{{InformationHighlight}}{0}Icons{{Information}}", pszLong),
          format (L"Enables file-type icons (Nerd Font required). Use {{InformationHighlight}}{0}Icons-{{Information}} to disable.", pszLong),
          L"" },
        { format (L"{{InformationHighlight}}{0}Tree{{Information}}", pszLong),
          format (L"Displays a hierarchical directory tree view. Use {{InformationHighlight}}{0}Tree-{{Information}} to disable.", pszLong),
          L"" },
        { format (L"{{InformationHighlight}}{0}Ellipsize{{Information}}", pszLong),
          format (L"Truncates long link target paths with \u2026 to prevent line wrapping. Default: on. Use {{InformationHighlight}}{0}Ellipsize-{{Information}} to disable.", pszLong),
          L"" },
        { format (L"{{InformationHighlight}}{0}Depth{{Information}}={{InformationHighlight}}N{{Information}}", pszLong),
          format (L"Limits tree depth to N levels (requires {{InformationHighlight}}{0}Tree{{Information}}).", pszLong),
          L"" },
        { format (L"{{InformationHighlight}}{0}TreeIndent{{Information}}={{InformationHighlight}}N{{Information}}", pszLong),
          format (L"Sets tree indent width (1-8, default 4; requires {{InformationHighlight}}{0}Tree{{Information}}).", pszLong),
          L"" },
        { format (L"{{InformationHighlight}}{0}Size{{Information}}={{InformationHighlight}}Auto{{Information}}|{{InformationHighlight}}Bytes{{Information}}", pszLong),
          L"File size format: {InformationHighlight}Auto{Information} = abbreviated (KB/MB/GB), {InformationHighlight}Bytes{Information} = exact with commas.",
          L"Default: {InformationHighlight}Auto{Information} in tree mode, {InformationHighlight}Bytes{Information} otherwise." },
    };
}




////////////////////////////////////////////////////////////////////////////////
//
//  BuildAliasRows
//
////////////////////////////////////////////////////////////////////////////////

static vector<SUsageOptionRow> BuildAliasRows (wstring_view pszLong)
{
    vector<SUsageOptionRow> aliasRows =
    {
        { format (L"{{InformationHighlight}}{0}Set-Aliases{{Information}}", pszLong),
          L"Interactive wizard to configure PowerShell aliases for tcdir.",
          L"" },
        { format (L"{{InformationHighlight}}{0}Get-Aliases{{Information}}", pszLong),
          L"Display currently configured tcdir aliases and their source locations.",
          L"" },
        { format (L"{{InformationHighlight}}{0}Remove-Aliases{{Information}}", pszLong),
          L"Interactive wizard to remove tcdir aliases from a profile.",
          L"" },
        { format (L"{{InformationHighlight}}{0}WhatIf{{Information}}", pszLong),
          format (L"Preview changes without modifying files (use with {{InformationHighlight}}{0}Set-Aliases{{Information}} or {{InformationHighlight}}{0}Remove-Aliases{{Information}}).", pszLong),
          L"" },
        { format (L"{{InformationHighlight}}{0}Install-NerdFonts{{Information}}", pszLong),
          format (L"Download and install {} system-wide, then optionally configure terminal profiles.", NerdFontConst::kpszWtFontFace),
          L"" },
        { format (L"{{InformationHighlight}}{0}Uninstall-NerdFonts{{Information}}", pszLong),
          L"Remove Nerd Font terminal configuration and optionally remove installed font files.",
          L"" },
    };

#ifdef _DEBUG
    aliasRows.push_back ({
        format (L"{{InformationHighlight}}{0}Debug{{Information}}", pszLong),
        L"Displays raw file attributes in hex for diagnosing edge cases.",
        L""
    });
#endif

    return aliasRows;
}




////////////////////////////////////////////////////////////////////////////////
//
//  BuildSynopsisTokens
//
////////////////////////////////////////////////////////////////////////////////

static vector<wstring> BuildSynopsisTokens (const vector<SUsageOptionRow> & primaryRows, const vector<SUsageOptionRow> & aliasRows, wstring_view szShort)
{
    vector<wstring> synopsisTokens;
    synopsisTokens.reserve (primaryRows.size () + aliasRows.size () + 4);

    synopsisTokens.push_back (
        L"[{InformationHighlight}drive:{Information}]"
        L"[{InformationHighlight}path{Information}]"
        L"[{InformationHighlight}filename{Information}] ");
    synopsisTokens.push_back (format (L"[{{InformationHighlight}}{0}A{{Information}}[[:]{{InformationHighlight}}attributes{{Information}}]] ", szShort));
    synopsisTokens.push_back (format (L"[{{InformationHighlight}}{0}O{{Information}}[[:]{{InformationHighlight}}sortorder{{Information}}]] ", szShort));
    synopsisTokens.push_back (format (L"[{{InformationHighlight}}{0}T{{Information}}[[:]{{InformationHighlight}}timefield{{Information}}]] ", szShort));

    for (size_t i = 3; i < primaryRows.size (); ++i)
    {
        synopsisTokens.push_back (L"[" + primaryRows[i].label + L"] ");
    }

    for (const SUsageOptionRow & row : aliasRows)
    {
        synopsisTokens.push_back (L"[" + row.label + L"] ");
    }

    return synopsisTokens;
}




////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayUsage
//
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayUsage (CConsole & console, wchar_t chPrefix, optional<bool> fIconsCli)
{
    // Determine prefix strings for single-char and multi-char switches
    wstring_view szShort     = (chPrefix == L'-') ? L"-"    : L"/";
    wstring_view pszLong     = (chPrefix == L'-') ? L"--"   : L"/";
    wstring_view pszMDisable = (chPrefix == L'-') ? L" -M-" : L" /M-";

    // Resolve icon state, then pick cloud status symbols (Nerd Font icons or Unicode circles)
    const CConfig & config = *console.m_configPtr;
    bool            fIcons = ResolveUsageIconState (console, fIconsCli);

    wstring szCloudOnly    = fIcons ? CodePointToWString (config.m_iconCloudOnly)        : wstring (1, UnicodeSymbols::CircleHollow);
    wstring szLocallyAvail = fIcons ? CodePointToWString (config.m_iconLocallyAvailable) : wstring (1, UnicodeSymbols::CircleHalfFilled);
    wstring szAlwaysLocal  = fIcons ? CodePointToWString (config.m_iconAlwaysLocal)      : wstring (1, UnicodeSymbols::CircleFilled);

    PrintUsageHeader (console);

    constexpr int  INDENT                     = 2;
    constexpr int  COLUMN_GAP                 = 1;
    constexpr int  SUBLEVEL_DETAIL_OFFSET     = 2;
    constexpr int  SUBLEVEL_SECOND_COL_OFFSET = 30;
    constexpr int  SIZE_CONTINUATION_OFFSET   = 2;
    constexpr int  CLOUD_ICON_DESC_GAP        = 2;

    vector<SUsageOptionRow> primaryRows = BuildPrimaryOptionRows (szShort, pszLong, pszMDisable);
    vector<SUsageOptionRow> aliasRows   = BuildAliasRows (pszLong);

    vector<wstring> synopsisTokens = BuildSynopsisTokens (primaryRows, aliasRows, szShort);

    // Print the synopsis line with dynamic word-wrapping
    DisplaySynopsis (console, synopsisTokens);

    vector<wstring> optionLabels;
    optionLabels.reserve (primaryRows.size () + aliasRows.size ());

    for (const SUsageOptionRow & row : primaryRows)
    {
        optionLabels.push_back (row.label);
    }

    for (const SUsageOptionRow & row : aliasRows)
    {
        optionLabels.push_back (row.label);
    }

    int optionDescCol      = INDENT + GetMaxVisibleWidth (optionLabels) + COLUMN_GAP;
    int sectionDescCol     = optionDescCol - 1;
    int subLevelDetailCol  = sectionDescCol + SUBLEVEL_DETAIL_OFFSET;
    int continuationCol    = optionDescCol + SIZE_CONTINUATION_OFFSET;

    wstring usageBody (L"{Information}\n");

    AppendAlignedRow (usageBody, INDENT, optionDescCol,
                      L"[drive:][path][filename]",
                      L"Specifies drive, directory, and/or files to list.");

    usageBody.push_back (L'\n');

    AppendAlignedRow (usageBody, INDENT, sectionDescCol,
                      primaryRows[0].label,
                      primaryRows[0].description);

    vector<SThreeColumnRow> attributeRows =
    {
        { L"attributes", L"{InformationHighlight}D{Information}  Directories",             L"{InformationHighlight}R{Information}  Read-only files" },
        { L"",           L"{InformationHighlight}H{Information}  Hidden files",            L"{InformationHighlight}A{Information}  Files ready for archiving" },
        { L"",           L"{InformationHighlight}S{Information}  System files",            L"{InformationHighlight}T{Information}  Temporary files" },
        { L"",           L"{InformationHighlight}E{Information}  Encrypted files",         L"{InformationHighlight}C{Information}  Compressed files" },
        { L"",           L"{InformationHighlight}P{Information}  Reparse points",          L"{InformationHighlight}0{Information}  Sparse files" },
        { L"",           L"{InformationHighlight}X{Information}  Not content indexed",     L"{InformationHighlight}I{Information}  Integrity stream (ReFS)" },
        { L"",           L"{InformationHighlight}B{Information}  No scrub data (ReFS)",    L"{InformationHighlight}O{Information}  Cloud-only (not local)" },
        { L"",           L"{InformationHighlight}L{Information}  Locally available",       L"{InformationHighlight}V{Information}  Always locally available" },
        { L"",           L"{InformationHighlight}-{Information}  Prefix meaning not",      L"" },
    };

    int attributeCol2 = AppendDetailGrid (usageBody, INDENT, subLevelDetailCol, COLUMN_GAP, SUBLEVEL_SECOND_COL_OFFSET, attributeRows);

    usageBody.push_back (L'\n');
    usageBody.append (L"  Cloud status symbols shown between file size and name:\n");

    vector<SThreeColumnRow> cloudRows =
    {
        { L"", format (L"{{CloudStatusCloudOnly}}{0}{{Information}}", szCloudOnly),                L"Cloud-only (not locally available)" },
        { L"", format (L"{{CloudStatusLocallyAvailable}}{0}{{Information}}", szLocallyAvail),      L"Locally available (can be freed)" },
        { L"", format (L"{{CloudStatusAlwaysLocallyAvailable}}{0}{{Information}}", szAlwaysLocal), L"Always locally available (pinned)" },
    };

    AppendThreeColumnRows (usageBody, INDENT, attributeCol2, CLOUD_ICON_DESC_GAP, cloudRows);

    usageBody.push_back (L'\n');

    AppendAlignedRow (usageBody, INDENT, sectionDescCol,
                      primaryRows[1].label,
                      primaryRows[1].description);

    vector<SThreeColumnRow> sortRows =
    {
        { L"sortorder", L"{InformationHighlight}N{Information}  By name (alphabetic)",      L"{InformationHighlight}S{Information}  By size (smallest first)" },
        { L"",          L"{InformationHighlight}E{Information}  By extension (alphabetic)", L"{InformationHighlight}D{Information}  By date/time (oldest first)" },
        { L"",          L"{InformationHighlight}-{Information}  Prefix to reverse order",   L"" },
    };

    AppendDetailGrid (usageBody, INDENT, subLevelDetailCol, COLUMN_GAP, SUBLEVEL_SECOND_COL_OFFSET, sortRows);

    usageBody.push_back (L'\n');

    AppendAlignedRow (usageBody, INDENT, sectionDescCol,
                      primaryRows[2].label,
                      primaryRows[2].description);

    vector<SThreeColumnRow> timeRows =
    {
        { L"timefield", L"{InformationHighlight}C{Information}  Creation time",             L"{InformationHighlight}A{Information}  Last access time" },
        { L"",          L"{InformationHighlight}W{Information}  Last write time (default)", L"" },
    };

    AppendDetailGrid (usageBody, INDENT, subLevelDetailCol, COLUMN_GAP, SUBLEVEL_SECOND_COL_OFFSET, timeRows);

    usageBody.push_back (L'\n');

    for (size_t i = 3; i < primaryRows.size (); ++i)
    {
        const SUsageOptionRow & row = primaryRows[i];
        AppendAlignedRow (usageBody, INDENT, optionDescCol, row.label, row.description);

        if (!row.continuation.empty ())
        {
            usageBody.append (continuationCol, L' ');
            usageBody.append (row.continuation);
            usageBody.push_back (L'\n');
        }
    }

    usageBody.push_back (L'\n');

    for (const SUsageOptionRow & row : aliasRows)
    {
        AppendAlignedRow (usageBody, INDENT, optionDescCol, row.label, row.description);
    }

    console.ColorPuts (usageBody.c_str ());
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayConfigFileIssues
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayConfigFileIssues (CConsole & console, wchar_t chPrefix, bool fShowHint)
{
    CConfig::ValidationResult validationResult;

    // Determine prefix strings for multi-char switches
    LPCWSTR pszLong = (chPrefix == L'-') ? L"--" : L"/";



    validationResult = console.m_configPtr->ValidateConfigFile();

    if (!validationResult.hasIssues())
    {
        return;
    }

    console.ColorPrintf (L"{Default}\n{Error}There are some problems with your config file%s:\n",
                         fShowHint ? format (L" (see {}config for help)", pszLong).c_str() : L"");

    for (const auto & error : validationResult.errors)
    {
        //
        // Format with line number if available
        //

        if (error.lineNumber > 0)
        {
            console.ColorPrintf (L"{Error}  Line %zu: %s in \"%s\"\n",
                                 error.lineNumber, error.message.c_str(), error.entry.c_str());

            size_t  prefixLen = 2 + 5 + std::to_wstring (error.lineNumber).length() + 2 +
                                error.message.length() + 5 + error.invalidTextOffset;
            wstring underline (error.invalidText.length(), UnicodeSymbols::Overline);

            console.ColorPrintf (L"{Default}%*s{Error}%s\n\n",
                                 static_cast<int>(prefixLen), L"", underline.c_str());
        }
        else
        {
            // File-level error (no line number)
            console.ColorPrintf (L"{Error}  %s: %s\n\n",
                                 error.entry.c_str(), error.message.c_str());
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayEnvVarIssues
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayEnvVarIssues (CConsole & console, wchar_t chPrefix, bool fShowHint)
{
    CConfig::ValidationResult validationResult;

    // Determine prefix strings for multi-char switches
    LPCWSTR pszLong = (chPrefix == L'-') ? L"--" : L"/";



    validationResult = console.m_configPtr->ValidateEnvironmentVariable();

    if (!validationResult.hasIssues())
    {
        return;
    }

    console.ColorPrintf (L"{Default}\n{Error}There are some problems with your %s environment variable%s:\n", 
                         TCDIR_ENV_VAR_NAME,
                         fShowHint ? format (L" (see {}env for help)", pszLong).c_str() : L"");

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

void CUsage::DisplayConfigurationTable (CConsole & console, bool fShowIcons)
{
    static constexpr int   COLUMN_WIDTH_ATTR       = 27;  // Sized for "AlwaysLocallyAvailable (●)"
    static constexpr int   COLUMN_WIDTH_SOURCE     = 15;

    wstring tableSeparator = L"  ";



    tableSeparator += wstring (COLUMN_WIDTH_ATTR + COLUMN_WIDTH_SOURCE + 2, UnicodeSymbols::LineHorizontal);

    DisplaySwitchConfiguration        (console, COLUMN_WIDTH_ATTR, COLUMN_WIDTH_SOURCE);
    DisplayAttributeConfiguration     (console, COLUMN_WIDTH_ATTR, COLUMN_WIDTH_SOURCE, fShowIcons);
    DisplayFileAttributeConfiguration (console, COLUMN_WIDTH_ATTR, COLUMN_WIDTH_SOURCE);
    DisplayExtensionConfiguration     (console, COLUMN_WIDTH_ATTR, COLUMN_WIDTH_SOURCE, fShowIcons);
    DisplayWellKnownDirConfiguration  (console, fShowIcons);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplaySwitchConfiguration
//
//  Display switches and parameters set via config file or environment variable,
//  with source tracking.
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplaySwitchConfiguration (CConsole & console, int columnWidthAttr, int columnWidthSource)
{
    const CConfig & config = *console.m_configPtr;
    WORD            bgAttr = config.m_rgAttributes[CConfig::EAttribute::Default] & BC_Mask;

    const optional<bool> * switchValues[] =
    {
        &config.m_fWideListing,
        &config.m_fRecurse,
        &config.m_fPerfTimer,
        &config.m_fMultiThreaded,
        &config.m_fBareListing,
        &config.m_fShowOwner,
        &config.m_fShowStreams,
        &config.m_fIcons,
        &config.m_fTree,
        &config.m_fEllipsize,
    };

    static_assert (_countof (switchValues) == _countof (s_kSwitchInfos), "Switch arrays must match");

    // Check if any switches or parameters are set
    bool fHasSwitches = false;

    for (size_t i = 0; i < _countof (switchValues); ++i)
    {
        if (switchValues[i]->has_value())
        {
            fHasSwitches = true;
            break;
        }
    }

    bool fHasParams = config.m_cMaxDepth.has_value() ||
                      config.m_cTreeIndent.has_value() ||
                      config.m_eSizeFormat.has_value();

    if (!fHasSwitches && !fHasParams)
    {
        return;
    }

    console.Puts (CConfig::EAttribute::Information, L"\nSwitch and parameter overrides:\n");

    WORD sourceAttr = static_cast<WORD> (bgAttr | FC_Cyan);

    for (size_t i = 0; i < _countof (switchValues); ++i)
    {
        if (!switchValues[i]->has_value())
        {
            continue;
        }

        bool fValue = switchValues[i]->value();
        CConfig::EAttributeSource source = config.m_rgSwitchSources[i];

        LPCWSTR pszSource = L"Default";
        if (source == CConfig::EAttributeSource::ConfigFile)
            pszSource = L"Config file";
        else if (source == CConfig::EAttributeSource::Environment)
            pszSource = L"Environment";

        wstring display = format (L"{:<8s}  {}", s_kSwitchInfos[i].name, fValue ? L"ON" : L"OFF");

        int pad = max (0, columnWidthAttr - static_cast<int> (display.size()));

        console.Printf (CConfig::EAttribute::Information, L"  ");
        console.Printf (CConfig::EAttribute::Default,     L"%ls%*ls  ", display.c_str(), pad, L"");
        console.Printf (sourceAttr,                        L"%-*ls", columnWidthSource, pszSource);
        console.Puts   (CConfig::EAttribute::Default,     L"");
    }

    // Parameters
    if (config.m_cMaxDepth.has_value())
    {
        LPCWSTR pszSource = L"Default";
        if (config.m_eMaxDepthSource == CConfig::EAttributeSource::ConfigFile)
            pszSource = L"Config file";
        else if (config.m_eMaxDepthSource == CConfig::EAttributeSource::Environment)
            pszSource = L"Environment";

        wstring display = format (L"Depth     {}", config.m_cMaxDepth.value());
        int pad = max (0, columnWidthAttr - static_cast<int> (display.size()));

        console.Printf (CConfig::EAttribute::Information, L"  ");
        console.Printf (CConfig::EAttribute::Default,     L"%ls%*ls  ", display.c_str(), pad, L"");
        console.Printf (sourceAttr,                        L"%-*ls", columnWidthSource, pszSource);
        console.Puts   (CConfig::EAttribute::Default,     L"");
    }

    if (config.m_cTreeIndent.has_value())
    {
        LPCWSTR pszSource = L"Default";
        if (config.m_eTreeIndentSource == CConfig::EAttributeSource::ConfigFile)
            pszSource = L"Config file";
        else if (config.m_eTreeIndentSource == CConfig::EAttributeSource::Environment)
            pszSource = L"Environment";

        wstring display = format (L"TreeIndent  {}", config.m_cTreeIndent.value());
        int pad = max (0, columnWidthAttr - static_cast<int> (display.size()));

        console.Printf (CConfig::EAttribute::Information, L"  ");
        console.Printf (CConfig::EAttribute::Default,     L"%ls%*ls  ", display.c_str(), pad, L"");
        console.Printf (sourceAttr,                        L"%-*ls", columnWidthSource, pszSource);
        console.Puts   (CConfig::EAttribute::Default,     L"");
    }

    if (config.m_eSizeFormat.has_value())
    {
        LPCWSTR pszSource = L"Default";
        if (config.m_eSizeFormatSource == CConfig::EAttributeSource::ConfigFile)
            pszSource = L"Config file";
        else if (config.m_eSizeFormatSource == CConfig::EAttributeSource::Environment)
            pszSource = L"Environment";

        LPCWSTR pszValue = (config.m_eSizeFormat.value() == ESizeFormat::Auto) ? L"Auto" : L"Bytes";
        wstring display  = format (L"Size      {}", pszValue);
        int pad = max (0, columnWidthAttr - static_cast<int> (display.size()));

        console.Printf (CConfig::EAttribute::Information, L"  ");
        console.Printf (CConfig::EAttribute::Default,     L"%ls%*ls  ", display.c_str(), pad, L"");
        console.Printf (sourceAttr,                        L"%-*ls", columnWidthSource, pszSource);
        console.Puts   (CConfig::EAttribute::Default,     L"");
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayAttributeConfiguration
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayAttributeConfiguration (CConsole & console, int columnWidthAttr, int columnWidthSource, bool fShowIcons)
{
    console.Puts (CConfig::EAttribute::Information, L"\nCurrent display item configuration:\n");

    for (const auto & info : s_kDisplayItemInfos)
    {
        WORD attr  = console.m_configPtr->m_rgAttributes[info.attr];
        CConfig::EAttributeSource source = console.m_configPtr->m_rgAttributeSources[info.attr];

        DisplayItemAndSource (console, info.name, attr, source, columnWidthAttr, columnWidthSource, 0, EItemDisplayMode::SingleColumn);
    }

    for (const auto & info : s_kCloudStatusInfos)
    {
        const CConfig & config = *console.m_configPtr;
        WORD            attr   = config.m_rgAttributes[info.attr];
        CConfig::EAttributeSource source = config.m_rgAttributeSources[info.attr];

        wstring symbol;

        if (fShowIcons)
        {
            // Use the Nerd Font glyph from config instead of the Unicode geometric shape
            char32_t cp = 0;

            switch (info.attr)
            {
                case CConfig::EAttribute::CloudStatusCloudOnly:              cp = config.m_iconCloudOnly;        break;
                case CConfig::EAttribute::CloudStatusLocallyAvailable:       cp = config.m_iconLocallyAvailable; break;
                case CConfig::EAttribute::CloudStatusAlwaysLocallyAvailable: cp = config.m_iconAlwaysLocal;      break;
                default:                                                                                         break;
            }

            if (cp != 0)
            {
                WideCharPair wcp = CodePointToWideChars (cp);
                symbol += wcp.chars[0];
                if (wcp.chars[1] != L'\0')
                    symbol += wcp.chars[1];
            }
        }

        if (symbol.empty ())
        {
            symbol = info.symbol;
        }

        wstring display = format (L"{} ({})", info.baseName, symbol);

        // Surrogate pairs are 2 wchar_t but 1 display cell; widen the column
        // for this item so the source label still aligns with non-surrogate rows.
        int cxSurrogateAdjust = max (0, static_cast<int> (symbol.size ()) - 1);

        DisplayItemAndSource (console, display, attr, source, columnWidthAttr + cxSurrogateAdjust, columnWidthSource, 0, EItemDisplayMode::SingleColumn);
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
        CConfig::EAttributeSource source = iter->second.m_source;
        wstring label = format (L"{} {}", info.letter, info.name);
        
        DisplayItemAndSource (console, label, attr, source, columnWidthAttr, columnWidthSource, 0, EItemDisplayMode::SingleColumn);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayExtensionConfigurationSingleColumn
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayExtensionConfigurationSingleColumn (CConsole & console, int columnWidthAttr, int columnWidthSource, const vector<pair<wstring, WORD>> & extensions, bool fShowIcons)
{
    const CConfig & config = *console.m_configPtr;

    for (const auto & [ext, extAttr] : extensions)
    {
        auto    sourceIter = config.m_mapExtensionSources.find (ext);
        CConfig::EAttributeSource source = (sourceIter != config.m_mapExtensionSources.end ())
                                          ? sourceIter->second
                                          : CConfig::EAttributeSource::Default;

        wstring iconPrefix;
        if (fShowIcons)
        {
            auto iconIter = config.m_mapExtensionToIcon.find (ext);
            if (iconIter != config.m_mapExtensionToIcon.end ())
            {
                WideCharPair wcp = CodePointToWideChars (iconIter->second);
                iconPrefix += wcp.chars[0];
                if (wcp.chars[1] != L'\0')
                    iconPrefix += wcp.chars[1];
            }
        }

        DisplayItemAndSource (console, ext, extAttr, source, columnWidthAttr, columnWidthSource, 0, EItemDisplayMode::SingleColumn, iconPrefix, fShowIcons);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayItemAndSource
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayItemAndSource (CConsole & console, wstring_view item, WORD attr, CConfig::EAttributeSource source, size_t columnWidthItem, size_t columnWidthSource, size_t cxColumnWidth, EItemDisplayMode mode, wstring_view iconPrefix, bool fShowIcons)
{
    static constexpr size_t CX_ICON_COLUMN = 3;  // 2 display cells (glyph) + 1 space

    WORD    bgAttr       = console.m_configPtr->m_rgAttributes[CConfig::EAttribute::Default] & BC_Mask;
    WORD    sourceAttr   = static_cast<WORD> (bgAttr | FC_DarkGrey);
    LPCWSTR sourceName   = L"Default";

    switch (source)
    {
        case CConfig::EAttributeSource::ConfigFile:
            sourceAttr = static_cast<WORD> (bgAttr | FC_Cyan);
            sourceName = L"Config file";
            break;

        case CConfig::EAttributeSource::Environment:
            sourceAttr = static_cast<WORD> (bgAttr | FC_Cyan);
            sourceName = L"Environment";
            break;

        default:
            break;
    }

    int     pad          = max (0, static_cast<int> (columnWidthItem) - static_cast<int> (item.size ()));
    size_t  cxIconWidth  = fShowIcons ? CX_ICON_COLUMN : 0;
    size_t  cxUsed       = cxIconWidth + columnWidthItem + 2 + columnWidthSource;
    WORD    defaultAttr  = console.m_configPtr->m_rgAttributes[CConfig::EAttribute::Default];
    WORD    visibleAttr  = CConfig::EnsureVisibleColorAttr (attr, defaultAttr);



    if (mode == EItemDisplayMode::SingleColumn)
    {
        console.Printf (CConfig::EAttribute::Information, L"  ");
    }

    if (!iconPrefix.empty ())
    {
        console.Printf (visibleAttr, L"%.*ls ", static_cast<int> (iconPrefix.size ()), iconPrefix.data ());
    }
    else if (fShowIcons)
    {
        console.Printf (CConfig::EAttribute::Information, L"   ");
    }

    console.Printf (visibleAttr,                      L"%.*ls", static_cast<int> (item.size ()), item.data ());
    console.Printf (CConfig::EAttribute::Information, L"%*ls  ", pad, L"");
    console.Printf (sourceAttr,                       L"%-*ls", static_cast<int> (columnWidthSource), sourceName);

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

void CUsage::DisplayExtensionConfigurationMultiColumn (CConsole & console, const vector<pair<wstring, WORD>> & extensions, size_t maxExtLen, size_t cxSourceWidth, size_t cxAvailable, size_t cColumns, bool fShowIcons)
{
    size_t          cxColumnWidth   = max (static_cast<size_t> (1), cxAvailable / cColumns);
    size_t          cRows           = (extensions.size () + cColumns - 1) / cColumns;
    size_t          cItemsInLastRow = extensions.size () % cColumns;
    size_t          fullRows        = cItemsInLastRow ? cRows - 1 : cRows;
    const CConfig & config          = *console.m_configPtr;



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
            auto         sourceIter = config.m_mapExtensionSources.find (ext);
            CConfig::EAttributeSource source = (sourceIter != config.m_mapExtensionSources.end ())
                                              ? sourceIter->second
                                              : CConfig::EAttributeSource::Default;

            wstring iconPrefix;
            if (fShowIcons)
            {
                auto iconIter = config.m_mapExtensionToIcon.find (ext);
                if (iconIter != config.m_mapExtensionToIcon.end ())
                {
                    WideCharPair wcp = CodePointToWideChars (iconIter->second);
                    iconPrefix += wcp.chars[0];
                    if (wcp.chars[1] != L'\0')
                        iconPrefix += wcp.chars[1];
                }
            }

            DisplayItemAndSource (console, ext, extAttr, source, maxExtLen, cxSourceWidth, cxColumnWidth, EItemDisplayMode::MultiColumn, iconPrefix, fShowIcons);
        }

        console.Puts (CConfig::EAttribute::Default, L"");
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayExtensionConfiguration
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayExtensionConfiguration (CConsole & console, int columnWidthAttr, int columnWidthSource, bool fShowIcons)
{
    vector<pair<wstring, WORD>> extensions;
    UINT                        cxConsoleWidth  = console.GetWidth ();
    size_t                      maxExtLen       = 0;
    size_t                      cxIndent        = 2;
    size_t                      cxAvailable     = (cxConsoleWidth > cxIndent) ? (cxConsoleWidth - cxIndent) : cxConsoleWidth;
    size_t                      cxSourceWidth   = wcslen (L"Environment");
    size_t                      cColumns        = 1;
    const CConfig &             config          = *console.m_configPtr;



    console.Puts (CConfig::EAttribute::Information, fShowIcons ? L"\nFile extension color and icon configuration:" : L"\nFile extension color configuration:");

    extensions.reserve (config.m_mapExtensionToTextAttr.size());

    for (const auto & [ext, extAttr] : config.m_mapExtensionToTextAttr)
    {
        extensions.emplace_back (ext, extAttr);
    }

    std::ranges::sort (extensions, [] (const auto & a, const auto & b) { return a.first < b.first; });

    for (const auto & [ext, _] : extensions)
    {
        maxExtLen = max (maxExtLen, ext.size ());
    }

    // Minimum width per column: [icon(2) + " "] + <extension> + "  " + <source> + padding
    {
        size_t cxIconWidth      = fShowIcons ? 3 : 0;  // 2 display cells (glyph) + 1 space
        size_t cxMinColumnWidth = cxIconWidth + maxExtLen + 2 + cxSourceWidth + 1 + 2;

        if (cxMinColumnWidth > 0 && cxMinColumnWidth <= cxAvailable)
        {
            cColumns = max (static_cast<size_t> (1), cxAvailable / cxMinColumnWidth);
        }
    }

    if (cColumns == 1)
    {
        DisplayExtensionConfigurationSingleColumn (console, columnWidthAttr, columnWidthSource, extensions, fShowIcons);
    }
    else
    {
        DisplayExtensionConfigurationMultiColumn (console, extensions, maxExtLen, cxSourceWidth, cxAvailable, cColumns, fShowIcons);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayWellKnownDirConfiguration
//
//  Display well-known directory icon mappings in columnar layout using
//  the Directory display color.  Only shown when icons are active.
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayWellKnownDirConfiguration (CConsole & console, bool fShowIcons)
{
    if (!fShowIcons)
    {
        return;
    }

    const CConfig & config          = *console.m_configPtr;
    WORD            dirAttr         = config.m_rgAttributes[CConfig::EAttribute::Directory];
    UINT            cxConsoleWidth  = console.GetWidth ();
    size_t          cxIndent        = 2;
    size_t          cxAvailable     = (cxConsoleWidth > cxIndent) ? (cxConsoleWidth - cxIndent) : cxConsoleWidth;
    size_t          cxSourceWidth   = wcslen (L"Environment");
    size_t          maxNameLen      = 0;
    size_t          cColumns        = 1;



    vector<pair<wstring, char32_t>> dirs (config.m_mapWellKnownDirToIcon.begin (), config.m_mapWellKnownDirToIcon.end ());
    ranges::sort (dirs, {}, &pair<wstring, char32_t>::first);

    if (dirs.empty ())
    {
        return;
    }

    console.Puts (CConfig::EAttribute::Information, L"\nWell-known directory icon configuration:");

    for (const auto & [name, _] : dirs)
    {
        maxNameLen = max (maxNameLen, name.size ());
    }

    // Minimum width per column: icon(2) + " " + <name> + "  " + <source> + padding
    {
        size_t cxIconWidth      = 3;  // 2 display cells (glyph) + 1 space
        size_t cxMinColumnWidth = cxIconWidth + maxNameLen + 2 + cxSourceWidth + 1 + 2;

        if (cxMinColumnWidth > 0 && cxMinColumnWidth <= cxAvailable)
        {
            cColumns = max (static_cast<size_t> (1), cxAvailable / cxMinColumnWidth);
        }
    }

    if (cColumns == 1)
    {
        for (const auto & [name, cp] : dirs)
        {
            auto    sourceIter = config.m_mapWellKnownDirIconSources.find (name);
            CConfig::EAttributeSource source = (sourceIter != config.m_mapWellKnownDirIconSources.end ())
                                              ? sourceIter->second
                                              : CConfig::EAttributeSource::Default;

            WideCharPair wcp = CodePointToWideChars (cp);
            wstring      iconPrefix;
            iconPrefix += wcp.chars[0];
            if (wcp.chars[1] != L'\0')
                iconPrefix += wcp.chars[1];

            DisplayItemAndSource (console, name, dirAttr, source, maxNameLen, cxSourceWidth, 0, EItemDisplayMode::SingleColumn, iconPrefix, true);
        }
    }
    else
    {
        size_t cxColumnWidth   = max (static_cast<size_t> (1), cxAvailable / cColumns);
        size_t cRows           = (dirs.size () + cColumns - 1) / cColumns;
        size_t cItemsInLastRow = dirs.size () % cColumns;
        size_t fullRows        = cItemsInLastRow ? cRows - 1 : cRows;



        console.Puts (CConfig::EAttribute::Information, L"");

        for (size_t nRow = 0; nRow < cRows; ++nRow)
        {
            console.Printf (CConfig::EAttribute::Information, L"  ");

            for (size_t nCol = 0; nCol < cColumns; ++nCol)
            {
                if ((nRow * cColumns + nCol) >= dirs.size ())
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

                const auto & name       = dirs[idx].first;
                char32_t     cp         = dirs[idx].second;
                auto         sourceIter = config.m_mapWellKnownDirIconSources.find (name);
                CConfig::EAttributeSource source = (sourceIter != config.m_mapWellKnownDirIconSources.end ())
                                                  ? sourceIter->second
                                                  : CConfig::EAttributeSource::Default;

                WideCharPair wcp = CodePointToWideChars (cp);
                wstring      iconPrefix;
                iconPrefix += wcp.chars[0];
                if (wcp.chars[1] != L'\0')
                    iconPrefix += wcp.chars[1];

                DisplayItemAndSource (console, name, dirAttr, source, maxNameLen, cxSourceWidth, cxColumnWidth, EItemDisplayMode::MultiColumn, iconPrefix, true);
            }

            console.Puts (CConfig::EAttribute::Default, L"");
        }
    }
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

    return CConfig::EnsureVisibleColorAttr (foreAttr, defaultAttr);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayColorConfiguration
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayColorConfiguration (CConsole & console)
{
    static constexpr int COLUMN_WIDTH_COLOR_LEFT = 28;

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
        visibleAttr = CConfig::EnsureVisibleColorAttr (colorAttr, defaultAttr);

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
    for (auto && segment : std::views::split (wstring_view (envValue), L';'))
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
           config.m_fShowStreams.has_value()    ||
           config.m_fIcons.has_value();
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
//  HasEnvVarIconOverrides
//
////////////////////////////////////////////////////////////////////////////////

static bool HasEnvVarIconOverrides (const CConfig & config)
{
    auto isFromEnv = [](const auto & pair) { return pair.second == CConfig::EAttributeSource::Environment; };

    return ranges::any_of (config.m_mapExtensionIconSources,     isFromEnv) ||
           ranges::any_of (config.m_mapWellKnownDirIconSources,  isFromEnv);
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
        &config.m_fIcons,
        &config.m_fTree,
        &config.m_fEllipsize,
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
//  DisplayEnvVarIconOverridesSection
//
////////////////////////////////////////////////////////////////////////////////

static void DisplayEnvVarIconOverridesSection (CConsole & console, const CConfig & config)
{
    console.ColorPuts (L"{Default}\n    {Information}Icon overrides:");

    // Extension icon overrides
    for (const auto & [ext, source] : config.m_mapExtensionIconSources)
    {
        if (source != CConfig::EAttributeSource::Environment)
            continue;

        auto iter = config.m_mapExtensionToIcon.find (ext);
        if (iter == config.m_mapExtensionToIcon.end())
        {
            console.Printf (CConfig::EAttribute::Default, L"      %ls (suppressed)\n", ext.c_str());
        }
        else
        {
            WideCharPair wcp = CodePointToWideChars (iter->second);
            wchar_t      buf[3] = { wcp.chars[0], wcp.chars[1], L'\0' };

            console.Printf (CConfig::EAttribute::Default, L"      %ls %ls\n", ext.c_str(), buf);
        }
    }

    // Well-known directory icon overrides
    for (const auto & [dirName, source] : config.m_mapWellKnownDirIconSources)
    {
        if (source != CConfig::EAttributeSource::Environment)
            continue;

        auto iter = config.m_mapWellKnownDirToIcon.find (dirName);
        if (iter == config.m_mapWellKnownDirToIcon.end())
        {
            console.Printf (CConfig::EAttribute::Default, L"      dir:%ls (suppressed)\n", dirName.c_str());
        }
        else
        {
            WideCharPair wcp = CodePointToWideChars (iter->second);
            wchar_t      buf[3] = { wcp.chars[0], wcp.chars[1], L'\0' };

            console.Printf (CConfig::EAttribute::Default, L"      dir:%ls %ls\n", dirName.c_str(), buf);
        }
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
    CConfig & config       = *console.m_configPtr;
    bool hasSwitches       = HasEnvVarSwitches      (config);
    bool hasDisplayItems   = HasEnvVarDisplayItems  (config);
    bool hasFileAttrs      = HasEnvVarFileAttrs     (config);
    bool hasExtensions     = HasEnvVarExtensions    (config);
    bool hasIconOverrides  = HasEnvVarIconOverrides (config);



    if (!hasSwitches && !hasDisplayItems && !hasFileAttrs && !hasExtensions && !hasIconOverrides)
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

    if (hasIconOverrides)
    {
        DisplayEnvVarIconOverridesSection (console, config);
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

void CUsage::DisplayEnvVarHelp (CConsole & console, wchar_t chPrefix)
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
        L"{{InformationHighlight}}Attr:<FileAttr>{{Information}} | "
        L"{{InformationHighlight}}<.ext>{{Information}} | "
        L"{{InformationHighlight}}dir:<name>{{Information}}] = "
        L"[{{InformationHighlight}}<Fore>{{Information}} [on {{InformationHighlight}}<Back>{{Information}}]]"
        L"[{{InformationHighlight}},<Icon>{{Information}}][;...]"
        L"{1}\n"
        L"\n"
        L"  {{InformationHighlight}}<Switch>{{Information}}    A command-line switch:\n"
        L"                  {{InformationHighlight}}W{{Information}}        Wide listing format\n"
        L"                  {{InformationHighlight}}P{{Information}}        Display performance timing information\n"
        L"                  {{InformationHighlight}}S{{Information}}        Recurse into subdirectories\n"
        L"                  {{InformationHighlight}}M{{Information}}        Enables multi-threaded enumeration (default); use {{InformationHighlight}}M-{{Information}} to disable\n"
        L"                  {{InformationHighlight}}Owner{{Information}}    Display file ownership\n"
        L"                  {{InformationHighlight}}Streams{{Information}}  Display alternate data streams (NTFS)\n"
        L"                  {{InformationHighlight}}Icons{{Information}}    Enable file-type icons; use {{InformationHighlight}}Icons-{{Information}} to disable\n"
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
        L"  {{InformationHighlight}}<FileAttr>{{Information}}  A file attribute (see file attributes below)\n"
        L"                  {{InformationHighlight}}R{{Information}}  Read-only                {{InformationHighlight}}H{{Information}}  Hidden\n"
        L"                  {{InformationHighlight}}S{{Information}}  System                   {{InformationHighlight}}A{{Information}}  Archive\n"
        L"                  {{InformationHighlight}}T{{Information}}  Temporary                {{InformationHighlight}}E{{Information}}  Encrypted\n"
        L"                  {{InformationHighlight}}C{{Information}}  Compressed               {{InformationHighlight}}P{{Information}}  Reparse point\n"
        L"                  {{InformationHighlight}}0{{Information}}  Sparse file\n"
        L"\n"
        L"  {{InformationHighlight}}<.ext>{{Information}}      A file extension, including the leading period.\n"
        L"\n"
        L"  {{InformationHighlight}}<name>    {{Information}}  A well-known directory name (case-insensitive, e.g., {{InformationHighlight}}dir:.git{{Information}}).\n"
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
        exampleCmd    = L"{Information}  Example: {InformationHighlight}$env:" TCDIR_ENV_VAR_NAME L"{Information} = \"W;D=LightGreen;S=Yellow;Attr:H=DarkGrey;.cpp=White on Blue,U+E61D\"";
    }
    else
    {
        syntaxCommand = L"  set {InformationHighlight}" TCDIR_ENV_VAR_NAME L"{Information} =";
        syntaxSuffix  = L"";
        exampleCmd    = L"{Information}  Example: {InformationHighlight}set "  TCDIR_ENV_VAR_NAME L"{Information} = W;D=LightGreen;S=Yellow;Attr:H=DarkGrey;.cpp=White on Blue,U+E61D";
    }

    console.ColorPuts (format (k_wszEnvVarHelpBody, syntaxCommand, syntaxSuffix, exampleCmd).c_str());

    DisplayColorConfiguration (console);

    console.ColorPuts (
        L"  {InformationHighlight}<Icon>{Information}      An icon code point (requires Nerd Font):\n"
        L"                  {InformationHighlight}U+XXXX{Information}   Hex code point (e.g., {InformationHighlight}U+E61D{Information})\n"
        L"                  {InformationHighlight}<glyph>{Information}  A literal Nerd Font glyph character\n"
        L"                  (empty)  Suppresses the icon for that entry\n");

    console.ColorPrintf (L"{Default}%s\n", exampleCmd.data ());

    if (IsPowerShell ())
    {
        console.ColorPuts (
            L"  {Information}Persist: {InformationHighlight}[Environment]::SetEnvironmentVariable(\"" TCDIR_ENV_VAR_NAME L"\", $env:" TCDIR_ENV_VAR_NAME L", \"User\"){Information}");
    }

    console.Puts (CConfig::EAttribute::Default, L"");

    if (IsTcdirEnvVarSet ())
    {
        DisplayEnvVarCurrentValue    (console, TCDIR_ENV_VAR_NAME);
        DisplayEnvVarDecodedSettings (console);
        DisplayEnvVarIssues          (console, chPrefix, false /* fShowHint */);
    }
    else
    {
        console.ColorPuts (L"  {InformationHighlight}" TCDIR_ENV_VAR_NAME L"{Information} environment variable is not set.");
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  DisplayIconStatus
//
//  Run Nerd Font detection and display icon activation status.
//  Shows: resolved state (ON/OFF) and reason (CLI, env var, auto-detection).
//
////////////////////////////////////////////////////////////////////////////////

static bool DisplayIconStatus (CConsole & console, optional<bool> fIconsCli = nullopt)
{
    const CConfig & config = *console.m_configPtr;

    // Nerd Font detection (always shown)
    CNerdFontDetector  detector;
    EDetectionResult   detResult = EDetectionResult::NotDetected;
    HANDLE             hOut      = GetStdHandle (STD_OUTPUT_HANDLE);

    if (SUCCEEDED (detector.Detect (hOut, *config.m_pEnvironmentProvider, detResult)))
    {
        // detection succeeded
    }

    bool fNerdFont = (detResult == EDetectionResult::Detected);
    WORD bgAttr    = config.m_rgAttributes[CConfig::EAttribute::Default] & BC_Mask;
    WORD fontAttr  = static_cast<WORD> (bgAttr | (fNerdFont ? FC_Green : FC_DarkGrey));

    console.Puts (CConfig::EAttribute::Information, L"\nNerd Font:");
    console.Printf (CConfig::EAttribute::Default, L"  ");
    console.Printf (fontAttr, L"%ls", fNerdFont ? L"Detected" : L"Not detected");
    console.Puts   (CConfig::EAttribute::Default, L"");

    // Icon status (from CLI, env var, config file, or auto-detection)
    LPCWSTR  pszSource = L"";
    bool     fActive   = false;

    if (fIconsCli.has_value())
    {
        fActive  = fIconsCli.value();
        pszSource = fActive ? L"/Icons" : L"/Icons-";
    }
    else if (config.m_fIcons.has_value())
    {
        fActive = config.m_fIcons.value();

        // Find the source of the Icons switch
        CConfig::EAttributeSource iconSource = CConfig::EAttributeSource::Default;
        for (size_t i = 0; i < _countof (CConfig::s_switchMemberOrder); ++i)
        {
            if (CConfig::s_switchMemberOrder[i] == &CConfig::m_fIcons)
            {
                iconSource = config.m_rgSwitchSources[i];
                break;
            }
        }

        if (iconSource == CConfig::EAttributeSource::ConfigFile)
            pszSource = fActive ? L"Config file" : L"Config file (Icons-)";
        else
            pszSource = fActive ? L"TCDIR=Icons" : L"TCDIR=Icons-";
    }
    else
    {
        fActive   = fNerdFont;
        pszSource = L"Auto-detection";
    }

    WORD stateAttr = static_cast<WORD> (bgAttr | (fActive ? FC_Green : FC_DarkGrey));

    console.Puts (CConfig::EAttribute::Information, L"\nIcon status:");
    console.Printf (CConfig::EAttribute::Default, L"  ");
    console.Printf (stateAttr, L"%ls", fActive ? L"ON" : L"OFF");
    console.Printf (CConfig::EAttribute::Default, L"  %ls\n", pszSource);

    return fActive;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplayConfigFileHelp
//
//  Display config file syntax reference, file path, load status,
//  and config file parse errors.
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplayConfigFileHelp (CConsole & console, wchar_t chPrefix)
{
    // Same body as k_wszEnvVarHelpBody but phrased for config file.
    // {0} = config file path
    // Color markers use {{EAttributeName}} which format() escapes to {EAttributeName}
    // for ColorPuts to parse.

    static constexpr wchar_t k_wszConfigFileHelpBody[] =
        L"\n"
        L"{{Information}}Create a file at {{InformationHighlight}}{0}{{Information}} to set default colors"
        L" for display items, file attributes, or file extensions:\n"
        L"  One setting per line.  Lines beginning with {{InformationHighlight}}#{{Information}} are comments.\n"
        L"\n"
        L"  [{{InformationHighlight}}<Switch>{{Information}}] | "
        L"[{{InformationHighlight}}<Item>{{Information}} | "
        L"{{InformationHighlight}}Attr:<FileAttr>{{Information}} | "
        L"{{InformationHighlight}}<.ext>{{Information}} | "
        L"{{InformationHighlight}}dir:<name>{{Information}}] = "
        L"[{{InformationHighlight}}<Fore>{{Information}} [on {{InformationHighlight}}<Back>{{Information}}]]"
        L"[{{InformationHighlight}},<Icon>{{Information}}]\n"
        L"\n"
        L"  {{InformationHighlight}}<Switch>{{Information}}    A command-line switch:\n"
        L"                  {{InformationHighlight}}W{{Information}}        Wide listing format\n"
        L"                  {{InformationHighlight}}P{{Information}}        Display performance timing information\n"
        L"                  {{InformationHighlight}}S{{Information}}        Recurse into subdirectories\n"
        L"                  {{InformationHighlight}}M{{Information}}        Enables multi-threaded enumeration (default); use {{InformationHighlight}}M-{{Information}} to disable\n"
        L"                  {{InformationHighlight}}Owner{{Information}}    Display file ownership\n"
        L"                  {{InformationHighlight}}Streams{{Information}}  Display alternate data streams (NTFS)\n"
        L"                  {{InformationHighlight}}Icons{{Information}}    Enable file-type icons; use {{InformationHighlight}}Icons-{{Information}} to disable\n"
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
        L"  {{InformationHighlight}}<FileAttr>{{Information}}  A file attribute (see file attributes below)\n"
        L"                  {{InformationHighlight}}R{{Information}}  Read-only                {{InformationHighlight}}H{{Information}}  Hidden\n"
        L"                  {{InformationHighlight}}S{{Information}}  System                   {{InformationHighlight}}A{{Information}}  Archive\n"
        L"                  {{InformationHighlight}}T{{Information}}  Temporary                {{InformationHighlight}}E{{Information}}  Encrypted\n"
        L"                  {{InformationHighlight}}C{{Information}}  Compressed               {{InformationHighlight}}P{{Information}}  Reparse point\n"
        L"                  {{InformationHighlight}}0{{Information}}  Sparse file\n"
        L"\n"
        L"  {{InformationHighlight}}<.ext>{{Information}}      A file extension, including the leading period.\n"
        L"\n"
        L"  {{InformationHighlight}}<name>    {{Information}}  A well-known directory name (case-insensitive, e.g., {{InformationHighlight}}dir:.git{{Information}}).\n"
        L"\n"
        L"  {{InformationHighlight}}<Fore>{{Information}}      Foreground color\n"
        L"  {{InformationHighlight}}<Back>{{Information}}      Background color";

    wstring filePath    = console.m_configPtr->GetConfigFilePath();
    wstring displayPath = filePath.empty() ? L"~\\.tcdirconfig" : filePath;



    console.ColorPuts (format (k_wszConfigFileHelpBody, displayPath).c_str());

    DisplayColorConfiguration (console);

    console.ColorPuts (
        L"  {InformationHighlight}<Icon>{Information}      An icon code point (requires Nerd Font):\n"
        L"                  {InformationHighlight}U+XXXX{Information}   Hex code point (e.g., {InformationHighlight}U+E61D{Information})\n"
        L"                  {InformationHighlight}<glyph>{Information}  A literal Nerd Font glyph character\n"
        L"                  (empty)  Suppresses the icon for that entry\n");

    console.ColorPuts (
        L"  {Information}Example {Default}.tcdirconfig{Information} file:\n"
        L"\n"
        L"  {Default}  # Enable tree view with icons\n"
        L"    Tree\n"
        L"    Icons\n"
        L"\n"
        L"    # Set colors\n"
        L"    D = LightGreen\n"
        L"    .cpp = White on Blue,U+E61D\n"
        L"    Attr:H = DarkGrey\n");

    console.ColorPuts (L"  {Information}Environment variable settings override config file settings.");

    console.Puts (CConfig::EAttribute::Default, L"");

    //
    // Show file status and issues
    //

    if (filePath.empty())
    {
        console.ColorPrintf (L"  {Information}Config file: {Default}(not resolved %lc USERPROFILE not set)\n", UnicodeSymbols::EmDash);
    }
    else if (console.m_configPtr->IsConfigFileLoaded())
    {
        console.ColorPrintf (L"  {Information}Config file: {InformationHighlight}%s {Information}found\n", filePath.c_str());
    }
    else
    {
        console.ColorPrintf (L"  {Information}Config file: {Default}%s {Information}not found\n", filePath.c_str());
    }

    DisplayConfigFileIssues (console, chPrefix, false);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUsage::DisplaySettings
//
//  Display the current color configuration tables for display items,
//  file attributes, and file extensions, with three-source column.
//
////////////////////////////////////////////////////////////////////////////////

void CUsage::DisplaySettings (CConsole & console, wchar_t chPrefix, optional<bool> fIconsCli)
{
    bool fHasConfig = console.m_configPtr->IsConfigFileLoaded();
    bool fHasEnv    = IsTcdirEnvVarSet();

    if (!fHasConfig && !fHasEnv)
    {
        console.ColorPuts (L"\n  {Information}No config file or " TCDIR_ENV_VAR_NAME L" environment variable set; showing defaults.");
    }

    bool fActive = DisplayIconStatus (console, fIconsCli);

    DisplayConfigurationTable (console, fActive);

    DisplayConfigFileIssues (console, chPrefix, false);
    DisplayEnvVarIssues (console, chPrefix, false);
}
