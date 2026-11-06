#include "pch.h"

#include "Completer.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CCompleter::IsCompleteRequest
//
//  "--Complete" must be the first argument, so a command line can't turn it
//  on by accident after other switches.
//
////////////////////////////////////////////////////////////////////////////////

bool CCompleter::IsCompleteRequest (int argc, WCHAR ** argv)
{
    return argc > 1 && _wcsicmp (argv[1], s_kpszCompleteSwitch) == 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompleter::Run
//
//  tcdir --Complete w:<word> [<word> ...]
//
//  The "w:" prefix prevents Windows PowerShell from dropping an empty word:
//  it omits empty arguments to native programs.  Returns the
//  process exit code.
//
////////////////////////////////////////////////////////////////////////////////

int CCompleter::Run (int argc, WCHAR ** argv)
{
    SCompletionRequest             request;
    vector<SCompletionCandidate>   candidates;
    wstring_view                   wordArg    = (argc > 2) ? argv[2] : L"";
    wstring                        strText;



    if (StartsWithNoCase (wordArg, s_kpszWordPrefix))
    {
        wordArg.remove_prefix (wcslen (s_kpszWordPrefix));
    }

    request.m_strWord = wordArg;

    for (int iArg = 3; iArg < argc; ++iArg)
    {
        request.m_vOtherWords.push_back (argv[iArg]);
    }

    Complete (request, candidates);

    for (const SCompletionCandidate & candidate : candidates)
    {
        strText += FormatLine (candidate) + L"\r\n";
    }

    WriteText (strText);

    return 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompleter::Complete
//
//  A word that starts with "--" completes long switches.  A word that starts
//  with "-" completes short switches, then long switches written with "--".
//  A word that starts with "/" completes both with "/".
//
////////////////////////////////////////////////////////////////////////////////

void CCompleter::Complete (const SCompletionRequest & request, vector<SCompletionCandidate> & candidates)
{
    SPrefixes   prefixes;
    wstring     strRest;



    candidates.clear();

    if (!SplitPrefix (request.m_strWord, prefixes, strRest) || AddValues (prefixes, strRest, candidates))
    {
        return;
    }

    if (prefixes.m_strTyped != L"--")
    {
        AddSwitches (ESwitchForm::Short, prefixes, strRest, candidates);
    }

    AddSwitches (ESwitchForm::Long, prefixes, strRest, candidates);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompleter::SplitPrefix
//
//  Returns false for a word that isn't a switch.
//
////////////////////////////////////////////////////////////////////////////////

bool CCompleter::SplitPrefix (const wstring & strWord, SPrefixes & prefixes, wstring & strRest)
{
    bool fSwitch = true;



    if (strWord.starts_with (L"--"))
    {
        prefixes = { L"--", L"-", L"--" };
    }
    else if (strWord.starts_with (L"-"))
    {
        prefixes = { L"-", L"-", L"--" };
    }
    else if (strWord.starts_with (L"/"))
    {
        prefixes = { L"/", L"/", L"/" };
    }
    else
    {
        fSwitch = false;
    }

    if (fSwitch)
    {
        strRest = strWord.substr (prefixes.m_strTyped.size());
    }

    return fSwitch;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompleter::AddValues
//
//  Completes the value after a switch's ':' or '=' separator.  Returns
//  false when the word has no separator.  Attribute letters combine, so
//  each candidate extends the letters already typed; other values replace
//  them.
//
////////////////////////////////////////////////////////////////////////////////

bool CCompleter::AddValues (const SPrefixes & prefixes, const wstring & strRest, vector<SCompletionCandidate> & candidates)
{
    size_t                      iSeparator  = strRest.find_first_of (L":=");
    wstring                     strName;
    wstring                     strTyped;
    wchar_t                     chSeparator = 0;
    ESwitchForm                 eForm       = ESwitchForm::Long;
    const SSwitchDefinition   * pDefinition = nullptr;
    wstring                     strBase;



    if (iSeparator == wstring::npos)
    {
        return false;
    }

    strName     = strRest.substr (0, iSeparator);
    strTyped    = strRest.substr (iSeparator + 1);
    chSeparator = strRest[iSeparator];
    eForm       = (chSeparator == L':' && strName.size() == 1) ? ESwitchForm::Short : ESwitchForm::Long;
    pDefinition = CSwitchCatalog::Find (eForm, strName);

    if (pDefinition == nullptr || pDefinition->m_chValueSeparator != chSeparator)
    {
        return true;
    }

    strBase = CSwitchCatalog::PrefixFor (*pDefinition, prefixes.m_strShort, prefixes.m_strLong) + pDefinition->m_pszName + chSeparator;

    if (pDefinition->m_fCombinableValues)
    {
        AddCombinedValues (*pDefinition, strBase, strTyped, candidates);
    }
    else
    {
        AddSingleValues (*pDefinition, strBase, strTyped, candidates);
    }

    return true;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompleter::AddCombinedValues
//
//  Each letter not typed yet, appended to the typed letters.  The "-"
//  prefix is offered unless the typed letters already end with it.
//
////////////////////////////////////////////////////////////////////////////////

void CCompleter::AddCombinedValues (
    const SSwitchDefinition        & definition,
    const wstring                  & strBase,
    const wstring                  & strTyped,
    vector<SCompletionCandidate>   & candidates)
{
    bool fEndsWithPrefix = false;



    for (const SSwitchValue & value : definition.m_values)
    {
        if (value.m_fPrefix)
        {
            fEndsWithPrefix = !strTyped.empty() && StartsWithNoCase (strTyped.substr (strTyped.size() - 1), value.m_pszValue);

            if (!fEndsWithPrefix)
            {
                candidates.push_back ({ strBase + strTyped + value.m_pszValue, value.m_pszValue, value.m_pszDescription });
            }

            continue;
        }

        if (!ContainsNoCase (strTyped, value.m_pszValue))
        {
            candidates.push_back ({ strBase + strTyped + value.m_pszValue, value.m_pszValue, value.m_pszDescription });
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompleter::AddSingleValues
//
//  Values that start with the typed text.  A prefix value (the "-" that
//  reverses a sort) is offered in front of each other value.
//
////////////////////////////////////////////////////////////////////////////////

void CCompleter::AddSingleValues (
    const SSwitchDefinition        & definition,
    const wstring                  & strBase,
    const wstring                  & strTyped,
    vector<SCompletionCandidate>   & candidates)
{
    vector<pair<wstring, wstring>> choices;



    for (const SSwitchValue & value : definition.m_values)
    {
        if (!value.m_fPrefix)
        {
            choices.push_back ({ value.m_pszValue, value.m_pszDescription });
        }
    }

    for (const SSwitchValue & prefix : definition.m_values)
    {
        if (!prefix.m_fPrefix)
        {
            continue;
        }

        for (size_t i = 0, cChoices = choices.size(); i < cChoices; ++i)
        {
            choices.push_back ({ prefix.m_pszValue + choices[i].first, choices[i].second + L", " + prefix.m_pszDescription });
        }
    }

    for (const auto & [strValue, strDescription] : choices)
    {
        if (StartsWithNoCase (strValue, strTyped))
        {
            candidates.push_back ({ strBase + strValue, strValue, strDescription });
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompleter::ContainsNoCase
//
////////////////////////////////////////////////////////////////////////////////

bool CCompleter::ContainsNoCase (wstring_view text, wstring_view part)
{
    bool fFound = false;



    for (size_t ib = 0; !fFound && ib + part.size() <= text.size(); ++ib)
    {
        fFound = StartsWithNoCase (text.substr (ib), part);
    }

    return fFound;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompleter::AddSwitches
//
////////////////////////////////////////////////////////////////////////////////

void CCompleter::AddSwitches (
    ESwitchForm                      eForm,
    const SPrefixes                & prefixes,
    const wstring                  & strRest,
    vector<SCompletionCandidate>   & candidates)
{
    for (const SSwitchDefinition & definition : CSwitchCatalog::All())
    {
        if (definition.m_eForm != eForm || !CSwitchCatalog::IsAvailable (definition) || !StartsWithNoCase (definition.m_pszName, strRest))
        {
            continue;
        }

        SCompletionCandidate candidate;

        candidate.m_strCompletion = CSwitchCatalog::PrefixFor (definition, prefixes.m_strShort, prefixes.m_strLong) + definition.m_pszName;
        candidate.m_strListText   = CSwitchCatalog::PlainText (CSwitchCatalog::Label (definition, prefixes.m_strShort, prefixes.m_strLong));
        candidate.m_strTooltip    = CSwitchCatalog::PlainText (CSwitchCatalog::FormatText (definition.m_pszDescription, prefixes.m_strShort, prefixes.m_strLong));

        candidates.push_back (move (candidate));
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompleter::FormatLine
//
//  "<completion>\t<list text>\t<tooltip>".  A tab inside a field becomes a
//  space, so the fields can be split.
//
////////////////////////////////////////////////////////////////////////////////

wstring CCompleter::FormatLine (const SCompletionCandidate & candidate)
{
    return WithoutTabs (candidate.m_strCompletion) + L'\t' + WithoutTabs (candidate.m_strListText) + L'\t' + WithoutTabs (candidate.m_strTooltip);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompleter::WithoutTabs
//
////////////////////////////////////////////////////////////////////////////////

wstring CCompleter::WithoutTabs (wstring strText)
{
    replace (strText.begin(), strText.end(), L'\t', L' ');

    return strText;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompleter::StartsWithNoCase
//
////////////////////////////////////////////////////////////////////////////////

bool CCompleter::StartsWithNoCase (wstring_view text, wstring_view prefix)
{
    return text.size() >= prefix.size() &&
           CompareStringOrdinal (text.data(), static_cast<int> (prefix.size()), prefix.data(), static_cast<int> (prefix.size()), TRUE) == CSTR_EQUAL;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompleter::WriteText
//
//  Writes plain text, without color, to stdout.  PowerShell decodes a
//  native program's output with the console output code page, so the text
//  is encoded with it.  Without a console the text is UTF-8.
//
////////////////////////////////////////////////////////////////////////////////

void CCompleter::WriteText (const wstring & strText)
{
    HANDLE    hOut      = GetStdHandle (STD_OUTPUT_HANDLE);
    UINT      codePage  = GetConsoleOutputCP();
    string    strBytes;
    int       cb        = 0;
    DWORD     cbWritten = 0;



    if (strText.empty() || hOut == INVALID_HANDLE_VALUE || hOut == nullptr)
    {
        return;
    }

    codePage = (codePage != 0) ? codePage : CP_UTF8;
    cb       = WideCharToMultiByte (codePage, 0, strText.data(), static_cast<int> (strText.size()), nullptr, 0, nullptr, nullptr);

    strBytes.resize (static_cast<size_t> (max (cb, 0)));
    WideCharToMultiByte (codePage, 0, strText.data(), static_cast<int> (strText.size()), strBytes.data(), cb, nullptr, nullptr);

    WriteFile (hOut, strBytes.data(), static_cast<DWORD> (strBytes.size()), &cbWritten, nullptr);
}





