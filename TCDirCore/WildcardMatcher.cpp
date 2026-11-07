#include "pch.h"

#include "WildcardMatcher.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CWildcardMatcher::Matches
//
//  Returns true when name matches spec.  "*.*" matches every name, including
//  names without a dot, and a trailing '.' in the spec matches the end of a
//  name that has no extension, as FindFirstFile does.  An empty spec matches
//  nothing.
//
////////////////////////////////////////////////////////////////////////////////

bool CWildcardMatcher::Matches (wstring_view name, wstring_view spec)
{
    return MatchesPrepared (name, PrepareSpec (spec));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CWildcardMatcher::PrepareSpec
//
//  Normalizes and uppercases spec once, so that MatchesPrepared can test many
//  entries against it.
//
////////////////////////////////////////////////////////////////////////////////

wstring CWildcardMatcher::PrepareSpec (wstring_view spec)
{
    return ToUpper (NormalizeSpec (spec));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CWildcardMatcher::MatchesPrepared
//
//  Returns true when name matches a spec from PrepareSpec.  An empty spec
//  matches nothing.
//
////////////////////////////////////////////////////////////////////////////////

bool CWildcardMatcher::MatchesPrepared (wstring_view name, const wstring & preparedSpec)
{
    return !preparedSpec.empty() && MatchUpper (ToUpper (name), preparedSpec);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CWildcardMatcher::NormalizeSpec
//
//  Rewrites "*.*" to "*" and drops a trailing '.' so that "readme." matches
//  "readme".
//
////////////////////////////////////////////////////////////////////////////////

wstring CWildcardMatcher::NormalizeSpec (wstring_view spec)
{
    wstring normalized (spec);



    if (normalized == L"*.*")
    {
        normalized = L"*";
    }
    else if (normalized.size() > 1 && normalized.back() == s_kchDot)
    {
        normalized.pop_back();
    }

    return normalized;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CWildcardMatcher::ToUpper
//
//  Uppercases text for case-insensitive comparison.
//
////////////////////////////////////////////////////////////////////////////////

wstring CWildcardMatcher::ToUpper (wstring_view text)
{
    wstring upper (text);



    if (!upper.empty())
    {
        CharUpperBuffW (upper.data(), static_cast<DWORD> (upper.size()));
    }

    return upper;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CWildcardMatcher::MatchUpper
//
//  Greedy glob match with single-star backtracking.  Both strings are
//  already uppercased.
//
////////////////////////////////////////////////////////////////////////////////

bool CWildcardMatcher::MatchUpper (const wstring & name, const wstring & spec)
{
    size_t iName     = 0;
    size_t iSpec     = 0;
    size_t iStar     = wstring::npos;
    size_t iRestart  = 0;



    while (iName < name.size())
    {
        if (iSpec < spec.size() && (spec[iSpec] == s_kchQuestion || spec[iSpec] == name[iName]))
        {
            ++iName;
            ++iSpec;
        }
        else if (iSpec < spec.size() && spec[iSpec] == s_kchStar)
        {
            iStar    = iSpec++;
            iRestart = iName;
        }
        else if (iStar != wstring::npos)
        {
            iSpec = iStar + 1;
            iName = ++iRestart;
        }
        else
        {
            break;
        }
    }

    while (iSpec < spec.size() && spec[iSpec] == s_kchStar)
    {
        ++iSpec;
    }

    return iName == name.size() && iSpec == spec.size();
}





