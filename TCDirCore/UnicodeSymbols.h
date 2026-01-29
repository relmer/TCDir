#pragma once

////////////////////////////////////////////////////////////////////////////////
//
//  UnicodeSymbols.h
//
//  Named constants for Unicode characters used throughout the codebase.
//  Using named constants improves readability and maintainability.
//
////////////////////////////////////////////////////////////////////////////////

namespace UnicodeSymbols
{
    //
    // Cloud status indicators (geometric shapes)
    //

    static constexpr WCHAR CircleHollow     = L'\u25CB';  // ○ - Cloud-only (not locally available)
    static constexpr WCHAR CircleHalfFilled = L'\u25D0';  // ◐ - Locally available (can be freed)
    static constexpr WCHAR CircleFilled     = L'\u25CF';  // ● - Always locally available (pinned)

    //
    // Box drawing characters
    //

    static constexpr WCHAR LineHorizontal   = L'\u2500';  // ─ - Horizontal line for separators

    //
    // Typographic symbols
    //

    static constexpr WCHAR Copyright        = L'\u00A9';  // © - Copyright symbol
    static constexpr WCHAR Overline         = L'\u203E';  // ‾ - Overline for underlining text above

}   // namespace UnicodeSymbols
