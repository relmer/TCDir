#pragma once





enum EForeColor
{
    FC_Black        = 0,
    FC_Blue         = FOREGROUND_BLUE,
    FC_Green        = FOREGROUND_GREEN,
    FC_Cyan         = FOREGROUND_BLUE | FOREGROUND_GREEN,
    FC_Red          = FOREGROUND_RED,
    FC_Magenta      = FOREGROUND_RED | FOREGROUND_BLUE,
    FC_Brown        = FOREGROUND_RED | FOREGROUND_GREEN,
    FC_LightGrey    = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
    FC_DarkGrey     = FOREGROUND_INTENSITY | FC_Black,
    FC_LightBlue    = FOREGROUND_INTENSITY | FC_Blue,
    FC_LightGreen   = FOREGROUND_INTENSITY | FC_Green,
    FC_LightCyan    = FOREGROUND_INTENSITY | FC_Cyan,
    FC_LightRed     = FOREGROUND_INTENSITY | FC_Red,
    FC_LightMagenta = FOREGROUND_INTENSITY | FC_Magenta,
    FC_Yellow       = FOREGROUND_INTENSITY | FC_Brown,
    FC_White        = FOREGROUND_INTENSITY | FC_LightGrey,
    FC_Mask         = FC_White,
};                





enum EBackColor
{
    BC_Black        = 0,
    BC_Blue         = BACKGROUND_BLUE,
    BC_Green        = BACKGROUND_GREEN,
    BC_Cyan         = BACKGROUND_BLUE | BACKGROUND_GREEN,
    BC_Red          = BACKGROUND_RED,
    BC_Magenta      = BACKGROUND_RED | BACKGROUND_BLUE,
    BC_Brown        = BACKGROUND_RED | BACKGROUND_GREEN,
    BC_LightGrey    = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
    BC_DarkGrey     = BACKGROUND_INTENSITY | BC_Black,
    BC_LightBlue    = BACKGROUND_INTENSITY | BC_Blue,
    BC_LightGreen   = BACKGROUND_INTENSITY | BC_Green,
    BC_LightCyan    = BACKGROUND_INTENSITY | BC_Cyan,
    BC_LightRed     = BACKGROUND_INTENSITY | BC_Red,
    BC_LightMagenta = BACKGROUND_INTENSITY | BC_Magenta,
    BC_Yellow       = BACKGROUND_INTENSITY | BC_Brown,
    BC_White        = BACKGROUND_INTENSITY | BC_LightGrey,
    BC_Mask         = BC_White,
};                
