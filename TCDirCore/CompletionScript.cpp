#include "pch.h"

#include "CompletionScript.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CCompletionScript::Generate
//
//  Registering a completer for a command name replaces any earlier one, so
//  running the script twice doesn't duplicate candidates.
//
////////////////////////////////////////////////////////////////////////////////

void CCompletionScript::Generate (const wstring & strInvocation, const vector<SCompletionCommand> & commands, vector<wstring> & lines)
{
    AddHelper (strInvocation, lines);

    lines.push_back (L"");

    for (const SCompletionCommand & command : commands)
    {
        lines.push_back (format (L"Register-ArgumentCompleter -Native -CommandName {} -ScriptBlock {{ param ($w, $a, $p) {} {} $w $a $p }}",
                                 QuoteLiteral (command.m_strName),
                                 s_kpszHelperName,
                                 FlagArray (command.m_strFlags)));
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompletionScript::AddHelper
//
//  PowerShell splits "-A:H" into a parameter and an argument and passes
//  only "H" as the word to complete.  The helper sends tcdir the whole word
//  up to the cursor, then trims each candidate back to the part PowerShell
//  replaces.
//
////////////////////////////////////////////////////////////////////////////////

void CCompletionScript::AddHelper (const wstring & strInvocation, vector<wstring> & lines)
{
    static constexpr LPCWSTR s_krgBody[] =
    {
        L"{",
        L"    $word   = $WordToComplete",
        L"    $others = @($Flags)",
        L"",
        L"    foreach ($element in @($CommandAst.CommandElements | Select-Object -Skip 1))",
        L"    {",
        L"        if ($element.Extent.StartOffset -lt $CursorPosition -and $element.Extent.EndOffset -ge $CursorPosition)",
        L"        {",
        L"            $word = $element.Extent.Text.Substring(0, $CursorPosition - $element.Extent.StartOffset)",
        L"        }",
        L"        else",
        L"        {",
        L"            $others += $element.Extent.Text",
        L"        }",
        L"    }",
        L"",
        L"    $cchTrim = [Math]::Max(0, $word.Length - $WordToComplete.Length)",
        L"",
    };



    lines.push_back (format (L"function {} ([string[]] $Flags, $WordToComplete, $CommandAst, $CursorPosition)", s_kpszHelperName));
    lines.insert (lines.end(), begin (s_krgBody), end (s_krgBody));
    lines.push_back (format (L"    & {} --Complete \"w:$word\" @others 2>$null | ForEach-Object {{", QuoteLiteral (strInvocation)));
    lines.push_back (L"        $fields = $_ -split \"`t\"");
    lines.push_back (L"        [System.Management.Automation.CompletionResult]::new($fields[0].Substring($cchTrim), $fields[1], 'ParameterValue', $fields[2])");
    lines.push_back (L"    }");
    lines.push_back (L"}");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompletionScript::IsRegistration
//
////////////////////////////////////////////////////////////////////////////////

bool CCompletionScript::IsRegistration (const wstring & strLine)
{
    return strLine.find (L"Register-ArgumentCompleter") != wstring::npos && strLine.find (s_kpszHelperName) != wstring::npos;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompletionScript::QuoteLiteral
//
//  A PowerShell single-quoted string, where a quote is doubled.
//
////////////////////////////////////////////////////////////////////////////////

wstring CCompletionScript::QuoteLiteral (const wstring & strText)
{
    wstring strQuoted = L"'";



    for (wchar_t ch : strText)
    {
        strQuoted += ch;

        if (ch == L'\'')
        {
            strQuoted += L'\'';
        }
    }

    return strQuoted + L"'";
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompletionScript::FlagArray
//
//  "-w /a:d" becomes "@('-w', '/a:d')"; no flags become "@()".
//
////////////////////////////////////////////////////////////////////////////////

wstring CCompletionScript::FlagArray (const wstring & strFlags)
{
    wistringstream   stream    (strFlags);
    wstring          strToken;
    wstring          strArray  = L"@(";



    while (stream >> strToken)
    {
        strArray += (strArray.size() > 2 ? L", " : L"") + QuoteLiteral (strToken);
    }

    return strArray + L")";
}





