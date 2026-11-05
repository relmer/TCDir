#pragma once





////////////////////////////////////////////////////////////////////////////////
//
//  CWildcardMatcher
//
//  Case-insensitive '*' and '?' matching with FindFirstFile conventions, for
//  entries that do not come from the file system.
//
////////////////////////////////////////////////////////////////////////////////

class CWildcardMatcher
{
public:
    static bool    Matches       (wstring_view name, wstring_view spec);
    static wstring ToUpper       (wstring_view text);

private:
    static wstring NormalizeSpec (wstring_view spec);
    static bool    MatchUpper    (const wstring & name, const wstring & spec);

    static constexpr WCHAR s_kchStar     = L'*';
    static constexpr WCHAR s_kchQuestion = L'?';
    static constexpr WCHAR s_kchDot      = L'.';
};
