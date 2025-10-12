#pragma once

// ANSI escape sequence constants for terminal control
// Reference: https://en.wikipedia.org/wiki/ANSI_escape_code
//
// Anatomy of an ANSI SGR (Select Graphic Rendition) sequence:
//   ESC [ <params> m
//   Example: "\x1b[91;40m" = bright red text on black background

namespace AnsiCodes
{
    //
    // Base components - building blocks for ANSI sequences
    // Note: These are constexpr rather than #define to avoid macro expansion issues
    //

    #define ESC     L"\x1b"                                     // Escape character (0x1B)
    #define CSI ESC L"["                                        // Control Sequence Introducer (composed: ESC + "[")
    #define SGR_END L"m"                                        // SGR (Select Graphic Rendition) command terminator
    
    //
    // Complete SGR sequences built from components
    //
    
    constexpr const wchar_t * RESET_ALL = CSI L"0" SGR_END;     // Reset all attributes (composed: CSI + "0" + "m")
    
    //
    // SGR format string for SetColor (use with std::format)
    // Usage: format(SGR_COLOR_FORMAT, foregroundCode, backgroundCode)
    // Example: format(SGR_COLOR_FORMAT, 91, 40) -> "\x1b[91;40m"
    // Composed: CSI + "{};{}" + "m"
    //
    
    constexpr const wchar_t * SGR_COLOR_FORMAT = CSI L"{};{}" SGR_END;
    
    //
    // Foreground ANSI color codes (30-37 = normal, 90-97 = bright)
    //

    enum Color
    {
        BG_OFFSET       = 10,         // Background = Foreground + 10

        FG_BLACK        = 30,
        FG_RED          = 31,
        FG_GREEN        = 32,
        FG_YELLOW       = 33,
        FG_BLUE         = 34,
        FG_MAGENTA      = 35,
        FG_CYAN         = 36,
        FG_WHITE        = 37,        
        
        BG_BLACK        = BG_OFFSET + FG_BLACK,
        BG_RED          = BG_OFFSET + FG_RED,
        BG_GREEN        = BG_OFFSET + FG_GREEN,
        BG_YELLOW       = BG_OFFSET + FG_YELLOW,
        BG_BLUE         = BG_OFFSET + FG_BLUE,
        BG_MAGENTA      = BG_OFFSET + FG_MAGENTA,
        BG_CYAN         = BG_OFFSET + FG_CYAN,
        BG_WHITE        = BG_OFFSET + FG_WHITE,
        
        BRIGHT_OFFSET   = 60      // Add to base color for bright variant (100-107)
    };
    
    //
    // Color mapping table
    // Maps Windows console color index (0-7) to ANSI foreground color code
    // Index: 0=Black, 1=Blue, 2=Green, 3=Cyan, 4=Red, 5=Magenta, 6=Yellow, 7=White
    //

    constexpr int CONSOLE_COLOR_TO_ANSI_COLOR[] = { 
        FG_BLACK,     // 0: Black   -> 30
        FG_BLUE,      // 1: Blue    -> 34
        FG_GREEN,     // 2: Green   -> 32
        FG_CYAN,      // 3: Cyan    -> 36
        FG_RED,       // 4: Red     -> 31
        FG_MAGENTA,   // 5: Magenta -> 35
        FG_YELLOW,    // 6: Yellow  -> 33
        FG_WHITE      // 7: White   -> 37
    };
}
