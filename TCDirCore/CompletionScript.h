#pragma once





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
    static bool    IsRegistration   (const wstring & strLine);
    static wstring QuoteLiteral     (const wstring & strText);
    static wstring FlagArray        (const wstring & strFlags);

    static constexpr LPCWSTR s_kpszHelperName = L"Invoke-TCDirCompletion";

private:
    static void    AddHelper        (const wstring & strInvocation, vector<wstring> & lines);
};
