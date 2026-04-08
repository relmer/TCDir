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
    // Tree drawing characters
    //

    static constexpr WCHAR TreeVertical     = L'\u2502';  // │ - Vertical line for tree continuation
    static constexpr WCHAR TreeTee          = L'\u251C';  // ├ - Vertical-and-right (middle entry)
    static constexpr WCHAR TreeCorner       = L'\u2514';  // └ - Up-and-right (last entry)
    static constexpr WCHAR TreeHorizontal   = L'\u2500';  // ─ - Horizontal line for tree connectors

    //
    // Typographic symbols
    //

    static constexpr WCHAR Copyright        = L'\u00A9';  // © - Copyright symbol
    static constexpr WCHAR Overline         = L'\u203E';  // ‾ - Overline for underlining text above
    static constexpr WCHAR EmDash           = L'\u2014';  // — - Em dash

    //
    // Interactive menu symbols
    //

    static constexpr WCHAR FocusIndicator   = L'\u276F';  // ❯ - Focus/cursor indicator
    static constexpr WCHAR RadioSelected    = L'\u25CF';  // ● - Selected radio button
    static constexpr WCHAR CheckMark        = L'\u2713';  // ✓ - Checkbox checked

    //
    // Braille spinner animation frames
    //

    static constexpr LPCWSTR SpinnerFrames[] =
    {
        L"\u2808",  // ⠈
        L"\u2818",  // ⠘
        L"\u2830",  // ⠰
        L"\u2860",  // ⡠
        L"\u28C0",  // ⣀
        L"\u2840",  // ⡀
        L"\u2820",  // ⠠
        L"\u2810",  // ⠐
    };

    static constexpr int SpinnerFrameCount = _countof (SpinnerFrames);

}   // namespace UnicodeSymbols
