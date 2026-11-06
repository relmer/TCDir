#pragma once

#include "SwitchCatalog.h"





////////////////////////////////////////////////////////////////////////////////
//
//  SCompletionRequest
//
//  The word under the cursor and the rest of the command line: an alias's
//  built-in switches first, then the other words the user typed.
//
////////////////////////////////////////////////////////////////////////////////

struct SCompletionRequest
{
    wstring           m_strWord;
    vector<wstring>   m_vOtherWords;
};





////////////////////////////////////////////////////////////////////////////////
//
//  SCompletionCandidate
//
////////////////////////////////////////////////////////////////////////////////

struct SCompletionCandidate
{
    wstring   m_strCompletion;     // Replaces the word
    wstring   m_strListText;       // Text of the Ctrl+Space list entry
    wstring   m_strTooltip;
};





////////////////////////////////////////////////////////////////////////////////
//
//  CCompleter
//
//  Produces PowerShell completion candidates for tcdir switches and switch
//  values.  Words that aren't switches produce no candidates, so PowerShell
//  completes paths.
//
////////////////////////////////////////////////////////////////////////////////

class CCompleter
{
public:
    static bool    IsCompleteRequest (int argc, WCHAR ** argv);
    static int     Run               (int argc, WCHAR ** argv);
    static void    Complete          (const SCompletionRequest & request, vector<SCompletionCandidate> & candidates);
    static wstring FormatLine        (const SCompletionCandidate & candidate);
    static void    WriteText         (const wstring & strText);

    static constexpr LPCWSTR s_kpszCompleteSwitch = L"--Complete";
    static constexpr LPCWSTR s_kpszWordPrefix     = L"w:";

private:
    struct SPrefixes
    {
        wstring   m_strTyped;          // The prefix of the word: "--", "-" or "/"
        wstring   m_strShort;          // Prefix for short switches in candidates and tooltips
        wstring   m_strLong;           // Prefix for long switches
    };

    static bool    SplitPrefix       (const wstring & strWord, SPrefixes & prefixes, wstring & strRest);
    static void    AddSwitches       (ESwitchForm eForm, const SPrefixes & prefixes, const wstring & strRest, vector<SCompletionCandidate> & candidates);
    static bool    StartsWithNoCase  (wstring_view text, wstring_view prefix);
    static wstring WithoutTabs       (wstring strText);
};
