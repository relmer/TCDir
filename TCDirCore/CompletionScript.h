#pragma once

#include "ProfileFileManager.h"





////////////////////////////////////////////////////////////////////////////////
//
//  SCompletionCommand
//
//  A command name that gets PowerShell completion, and the tcdir switches
//  the command passes before the user's arguments (an alias's flags).
//
////////////////////////////////////////////////////////////////////////////////

struct SCompletionCommand
{
    wstring   m_strName;
    wstring   m_strFlags;
};





////////////////////////////////////////////////////////////////////////////////
//
//  CCompletionScript
//
//  Generates the PowerShell lines that register tcdir completion: a helper
//  function that calls tcdir --Complete, and one native completer
//  registration per command.
//
////////////////////////////////////////////////////////////////////////////////

class CCompletionScript
{
public:
    static void    Generate         (const wstring & strInvocation, const vector<SCompletionCommand> & commands, vector<wstring> & lines);
    static void    GenerateBlock    (const wstring & strVersion, const wstring & strInvocation, const vector<SCompletionCommand> & commands, vector<wstring> & lines);
    static void    CommandsFromAliasBlock (const SAliasBlock & block, vector<SCompletionCommand> & commands);
    static wstring FlagsOfFunction  (const wstring & strFunctionLine, const wstring & strRootAlias);
    static bool    IsRegistration   (const wstring & strLine);
    static wstring QuoteLiteral     (const wstring & strText);
    static wstring FlagArray        (const wstring & strFlags);

    static constexpr LPCWSTR s_kpszHelperName   = L"Invoke-TCDirCompletion";
    static constexpr LPCWSTR s_kpszHeaderMarker = L"#  TCDir Completion";
    static constexpr LPCWSTR s_kpszFooterMarker = L"#  End TCDir Completion";
    static constexpr LPCWSTR s_kpszBannerLine   = L"################################################################################";

private:
    static void    AddHelper        (const wstring & strInvocation, vector<wstring> & lines);
};
