#pragma once




//
// Transparent hash for wstring-keyed unordered containers, enabling
// heterogeneous lookup with a wstring_view (or const wchar_t *) without
// constructing a wstring key.  All overloads route through
// std::hash<wstring_view> so a stored wstring key and a wstring_view probe of
// equal content hash identically.  Pair with std::equal_to<> for a fully
// transparent container.
//

struct STransparentWStringHash
{
    using is_transparent = void;

    size_t operator() (wstring_view sv)      const noexcept { return std::hash<wstring_view>{} (sv); }
    size_t operator() (const wstring & s)    const noexcept { return std::hash<wstring_view>{} (s); }
    size_t operator() (const wchar_t * psz)  const noexcept { return std::hash<wstring_view>{} (psz); }
};
