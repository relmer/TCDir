#pragma once





////////////////////////////////////////////////////////////////////////////////
//
//  ESizeFormat
//
//  Controls how file sizes are displayed.  Used by CCommandLine (CLI switch)
//  and CConfig (environment variable override).
//
////////////////////////////////////////////////////////////////////////////////

enum class ESizeFormat
{
    Default,        // Not explicitly set; tree mode uses Auto, non-tree uses Bytes
    Auto,           // Explorer-style abbreviated (1024-based, 3 significant digits, 7-char)
    Bytes           // Exact byte count with comma separators (existing behavior)
};
