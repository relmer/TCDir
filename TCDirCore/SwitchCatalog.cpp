#include "pch.h"

#include "SwitchCatalog.h"

#include "Config.h"
#include "NerdFontConstants.h"





//
// Values are in help order: the help grid lays them out in pairs, left column
// first.
//

const SSwitchValue CSwitchCatalog::s_krgAttributeValues[] =
{
    { L"D", L"Directories"              },
    { L"R", L"Read-only files"          },
    { L"H", L"Hidden files"             },
    { L"A", L"Files ready for archiving" },
    { L"S", L"System files"             },
    { L"T", L"Temporary files"          },
    { L"E", L"Encrypted files"          },
    { L"C", L"Compressed files"         },
    { L"P", L"Reparse points"           },
    { L"0", L"Sparse files"             },
    { L"X", L"Not content indexed"      },
    { L"I", L"Integrity stream (ReFS)"  },
    { L"B", L"No scrub data (ReFS)"     },
    { L"O", L"Cloud-only (not local)"   },
    { L"L", L"Locally available"        },
    { L"V", L"Always locally available" },
    { L"-", L"Prefix meaning not",      true },
};

const SSwitchValue CSwitchCatalog::s_krgSortValues[] =
{
    { L"N", L"By name (alphabetic)"       },
    { L"S", L"By size (smallest first)"   },
    { L"E", L"By extension (alphabetic)"  },
    { L"D", L"By date/time (oldest first)" },
    { L"-", L"Prefix to reverse order",   true },
};

const SSwitchValue CSwitchCatalog::s_krgTimeValues[] =
{
    { L"C", L"Creation time"             },
    { L"A", L"Last access time"          },
    { L"W", L"Last write time (default)" },
};

const SSwitchValue CSwitchCatalog::s_krgSizeValues[] =
{
    { L"Auto",  L"Abbreviated (KB/MB/GB)" },
    { L"Bytes", L"Exact with commas"      },
};

const SSwitchValue CSwitchCatalog::s_krgShellValues[] =
{
    { L"pwsh", L"PowerShell 7 and Windows PowerShell 5.1" },
};

const SSwitchValue CSwitchCatalog::s_krgTreeIndentValues[] =
{
    { L"1", L"Indent width" }, { L"2", L"Indent width" }, { L"3", L"Indent width" }, { L"4", L"Indent width (default)" },
    { L"5", L"Indent width" }, { L"6", L"Indent width" }, { L"7", L"Indent width" }, { L"8", L"Indent width" },
};





//
// The first three rows (-A, -O, -T) are followed by value grids in the help,
// so they stay first.
//

const SSwitchDefinition CSwitchCatalog::s_krgSwitches[] =
{
    { ESwitchForm::Short, L"A", L"", L':', s_krgAttributeValues, false, true,  ESwitchSection::Primary,
      L"Displays files with specified attributes.", L"", false },
    { ESwitchForm::Short, L"O", L"", L':', s_krgSortValues,      false, false, ESwitchSection::Primary,
      L"List by files in sorted order.", L"", false },
    { ESwitchForm::Short, L"T", L"", L':', s_krgTimeValues,      false, false, ESwitchSection::Primary,
      L"Selects the time field for display and sorting.", L"", false },
    { ESwitchForm::Short, L"S", L"", 0, {}, false, false, ESwitchSection::Primary,
      L"Displays files in specified directory and all subdirectories.", L"", false },
    { ESwitchForm::Short, L"W", L"", 0, {}, false, false, ESwitchSection::Primary,
      L"Displays results in a wide listing format.", L"", false },
    { ESwitchForm::Short, L"B", L"", 0, {}, false, false, ESwitchSection::Primary,
      L"Displays bare file names only (no headers, footers, or details).", L"", false },
    { ESwitchForm::Short, L"P", L"", 0, {}, false, false, ESwitchSection::Primary,
      L"Displays performance timing information.", L"", false },
    { ESwitchForm::Short, L"M", L"", 0, {}, true,  false, ESwitchSection::Primary,
      L"Enables multi-threaded enumeration (default). Use{{InformationHighlight}} {0}M-{{Information}} to disable.", L"", false },
    { ESwitchForm::Long,  L"Env", L"", 0, {}, false, false, ESwitchSection::Primary,
      L"Displays " TCDIR_ENV_VAR_NAME L" help, syntax, and current value.", L"", false },
    { ESwitchForm::Long,  L"Config", L"", 0, {}, false, false, ESwitchSection::Primary,
      L"Displays config file diagnostics, syntax reference, and parse errors.", L"", false },
    { ESwitchForm::Long,  L"Settings", L"", 0, {}, false, false, ESwitchSection::Primary,
      L"Displays current merged configuration for all items and extensions.", L"", false },
    { ESwitchForm::Long,  L"Owner", L"", 0, {}, false, false, ESwitchSection::Primary,
      L"Displays the owner of each file and directory. Not allowed with {{InformationHighlight}}{1}Tree{{Information}}.", L"", false },
    { ESwitchForm::Long,  L"Streams", L"", 0, {}, false, false, ESwitchSection::Primary,
      L"Displays alternate data streams (NTFS only).", L"", false },
    { ESwitchForm::Long,  L"Icons", L"", 0, {}, true,  false, ESwitchSection::Primary,
      L"Enables file-type icons (Nerd Font required). Use {{InformationHighlight}}{1}Icons-{{Information}} to disable.", L"", false },
    { ESwitchForm::Long,  L"Tree", L"", 0, {}, true,  false, ESwitchSection::Primary,
      L"Displays a hierarchical directory tree view. Use {{InformationHighlight}}{1}Tree-{{Information}} to disable.", L"", false },
    { ESwitchForm::Long,  L"Ellipsize", L"", 0, {}, true,  false, ESwitchSection::Primary,
      L"Truncates long link target paths with \u2026 to prevent line wrapping. Default: on. Use {{InformationHighlight}}{1}Ellipsize-{{Information}} to disable.", L"", false },
    { ESwitchForm::Long,  L"Depth", L"={InformationHighlight}N{Information}", L'=', {}, false, false, ESwitchSection::Primary,
      L"Limits tree depth to N levels (requires {{InformationHighlight}}{1}Tree{{Information}}).", L"", false },
    { ESwitchForm::Long,  L"TreeIndent", L"={InformationHighlight}N{Information}", L'=', s_krgTreeIndentValues, false, false, ESwitchSection::Primary,
      L"Sets tree indent width (1-8, default 4; requires {{InformationHighlight}}{1}Tree{{Information}}).", L"", false },
    { ESwitchForm::Long,  L"Size", L"={InformationHighlight}Auto{Information}|{InformationHighlight}Bytes{Information}", L'=', s_krgSizeValues, false, false, ESwitchSection::Primary,
      L"File size format: {{InformationHighlight}}Auto{{Information}} = abbreviated (KB/MB/GB), {{InformationHighlight}}Bytes{{Information}} = exact with commas.",
      L"Default: {{InformationHighlight}}Auto{{Information}} in tree mode, {{InformationHighlight}}Bytes{{Information}} otherwise.", false },

    { ESwitchForm::Long,  L"Set-Aliases", L"", 0, {}, false, false, ESwitchSection::Alias,
      L"Interactive wizard to configure PowerShell aliases for tcdir.", L"", false },
    { ESwitchForm::Long,  L"Get-Aliases", L"", 0, {}, false, false, ESwitchSection::Alias,
      L"Display currently configured tcdir aliases and their source locations.", L"", false },
    { ESwitchForm::Long,  L"Remove-Aliases", L"", 0, {}, false, false, ESwitchSection::Alias,
      L"Interactive wizard to remove tcdir aliases from a profile.", L"", false },
    { ESwitchForm::Long,  L"WhatIf", L"", 0, {}, false, false, ESwitchSection::Alias,
      L"Preview changes without modifying files (use with {{InformationHighlight}}{1}Set-Aliases{{Information}} or {{InformationHighlight}}{1}Remove-Aliases{{Information}}).", L"", false },
    { ESwitchForm::Long,  L"Completion", L"={InformationHighlight}pwsh{Information}", L'=', s_krgShellValues, false, false, ESwitchSection::Alias,
      L"Prints the PowerShell tab completion script. To enable it, add {{InformationHighlight}}tcdir {1}Completion=pwsh | Out-String | Invoke-Expression{{Information}} to your profile.",
      L"The script calls {{InformationHighlight}}tcdir --Complete{{Information}} for the candidates. {{InformationHighlight}}{1}Set-Aliases{{Information}} adds it to the alias block.", false },
    { ESwitchForm::Long,  L"Install-NerdFonts", L"", 0, {}, false, false, ESwitchSection::Alias,
      L"Download and install {2} system-wide, then optionally configure terminal profiles.", L"", false },
    { ESwitchForm::Long,  L"Uninstall-NerdFonts", L"", 0, {}, false, false, ESwitchSection::Alias,
      L"Remove Nerd Font terminal configuration and optionally remove installed font files.", L"", false },
    { ESwitchForm::Long,  L"Debug", L"", 0, {}, false, false, ESwitchSection::Alias,
      L"Displays raw file attributes in hex for diagnosing edge cases.", L"", true },
};





////////////////////////////////////////////////////////////////////////////////
//
//  CSwitchCatalog::All
//
////////////////////////////////////////////////////////////////////////////////

span<const SSwitchDefinition> CSwitchCatalog::All()
{
    return s_krgSwitches;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CSwitchCatalog::IsAvailable
//
//  Debug-only switches exist only in Debug builds.
//
////////////////////////////////////////////////////////////////////////////////

bool CSwitchCatalog::IsAvailable (const SSwitchDefinition & definition)
{
#ifdef _DEBUG
    UNREFERENCED_PARAMETER (definition);
    return true;
#else
    return !definition.m_fDebugOnly;
#endif
}





////////////////////////////////////////////////////////////////////////////////
//
//  CSwitchCatalog::Find
//
//  Case-insensitive lookup of an available switch.
//
////////////////////////////////////////////////////////////////////////////////

const SSwitchDefinition * CSwitchCatalog::Find (ESwitchForm eForm, wstring_view name)
{
    const SSwitchDefinition * pFound = nullptr;



    for (const SSwitchDefinition & definition : s_krgSwitches)
    {
        if (definition.m_eForm == eForm && IsAvailable (definition) &&
            CompareStringOrdinal (definition.m_pszName, -1, name.data(), static_cast<int> (name.size()), TRUE) == CSTR_EQUAL)
        {
            pFound = &definition;
            break;
        }
    }

    return pFound;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CSwitchCatalog::Label
//
//  The help label: the prefixed switch in the highlight color, then the
//  value syntax.
//
////////////////////////////////////////////////////////////////////////////////

wstring CSwitchCatalog::Label (const SSwitchDefinition & definition, wstring_view shortPrefix, wstring_view longPrefix)
{
    return L"{InformationHighlight}" + PrefixFor (definition, shortPrefix, longPrefix) + definition.m_pszName +
           L"{Information}" + definition.m_pszValueSyntax;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CSwitchCatalog::FormatText
//
//  Fills a description template.  The result has color markers in single
//  braces.
//
////////////////////////////////////////////////////////////////////////////////

wstring CSwitchCatalog::FormatText (LPCWSTR pszTemplate, wstring_view shortPrefix, wstring_view longPrefix)
{
    wstring_view fontFace = NerdFontConst::kpszWtFontFace;



    return vformat (pszTemplate, make_wformat_args (shortPrefix, longPrefix, fontFace));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CSwitchCatalog::PlainText
//
//  Removes {Name} color markers.
//
////////////////////////////////////////////////////////////////////////////////

wstring CSwitchCatalog::PlainText (wstring_view markup)
{
    wstring   strText;
    size_t    ib      = 0;
    size_t    ibClose = 0;



    while (ib < markup.size())
    {
        ibClose = (markup[ib] == L'{') ? markup.find (L'}', ib) : wstring_view::npos;

        if (ibClose != wstring_view::npos &&
            all_of (markup.begin() + ib + 1, markup.begin() + ibClose, [] (wchar_t ch) { return iswalpha (ch) != 0; }))
        {
            ib = ibClose + 1;
            continue;
        }

        strText += markup[ib++];
    }

    return strText;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CSwitchCatalog::PrefixFor
//
////////////////////////////////////////////////////////////////////////////////

wstring CSwitchCatalog::PrefixFor (const SSwitchDefinition & definition, wstring_view shortPrefix, wstring_view longPrefix)
{
    return wstring (definition.m_eForm == ESwitchForm::Short ? shortPrefix : longPrefix);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CSwitchCatalog::ValuesOf
//
//  The values of a short switch, such as the attribute letters of -A.
//
////////////////////////////////////////////////////////////////////////////////

span<const SSwitchValue> CSwitchCatalog::ValuesOf (wstring_view shortName)
{
    const SSwitchDefinition * pDefinition = Find (ESwitchForm::Short, shortName);



    return (pDefinition != nullptr) ? pDefinition->m_values : span<const SSwitchValue>();
}





