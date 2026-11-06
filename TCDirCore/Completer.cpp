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
    WriteLines (candidates);

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

    if (!SplitPrefix (request.m_strWord, prefixes, strRest))
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
//  CCompleter::WriteLines
//
//  PowerShell decodes a native program's output with the console output
//  code page, so the lines are encoded with it.  Without a console the
//  lines are UTF-8.
//
////////////////////////////////////////////////////////////////////////////////

void CCompleter::WriteLines (const vector<SCompletionCandidate> & candidates)
{
    HANDLE    hOut      = GetStdHandle (STD_OUTPUT_HANDLE);
    UINT      codePage  = GetConsoleOutputCP();
    wstring   strText;
    string    strBytes;
    int       cb        = 0;
    DWORD     cbWritten = 0;



    for (const SCompletionCandidate & candidate : candidates)
    {
        strText += FormatLine (candidate) + L"\r\n";
    }

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





