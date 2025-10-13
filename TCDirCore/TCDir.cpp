// TCDir.cpp : Defines the entry point for the console application.
//

#include "pch.h"
#include "TCDir.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "DirectoryLister.h"
#include "PerfTimer.h"





/*
void TestColors ()
{
    static LPCWSTR s_kpszColors[] =
    {
        L"Black",
        L"Blue",
        L"Green",
        L"Cyan",
        L"Red",
        L"Magenta",
        L"Brown",
        L"Light grey",
        L"Dark grey",
        L"Light blue",
        L"Light green",
        L"Light cyan",
        L"Light red",
        L"Light magenta",
        L"Yellow",
        L"White",
    };

    for (WORD back = 0; back < 16; ++back)
    {
        for (WORD fore = 0; fore < 16; ++fore)
        {
            g_util.SetTextAttr ((back << 4) | fore);
            g_util.ConsolePrintf (s_kpszColors[fore]);
            g_util.ConsolePrintf (L"    ");
        }

        g_util.ConsolePrintf (L"\n");
    }
}
*/




////////////////////////////////////////////////////////////////////////////////
//
//  wmain
//
//  
//
////////////////////////////////////////////////////////////////////////////////

int wmain (int argc, WCHAR * argv[])
{
    HRESULT                  hr           = S_OK;      
    shared_ptr<CCommandLine> cmdlinePtr   = make_shared<CCommandLine>();
    unique_ptr<PerfTimer>    perfTimerPtr;
    shared_ptr<CConfig>      configPtr    = make_shared<CConfig>();
    shared_ptr<CConsole>     consolePtr   = make_shared<CConsole>();

 

    //
    // Initialize the debug heap
    //

#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
#endif



    hr = consolePtr->Initialize (configPtr);
    CHR (hr);



#ifdef _DEBUG
    //consolePtr.Test();
#endif



    //
    // Process the commandline
    //

    hr = cmdlinePtr->Parse (argc - 1, argv + 1);
    if (FAILED (hr))
    {
        DisplayUsage (consolePtr.get());
    }
    CHR (hr);



    //
    // Use default mask if no mask is given
    //

    if (cmdlinePtr->m_listMask.empty())
    {
        cmdlinePtr->m_listMask.push_back (L"*");
    }


    
    //
    // For each mask, show a directory listing
    //

    {
        if (cmdlinePtr->m_fPerfTimer)
        {
            perfTimerPtr = make_unique<PerfTimer> (L"TCDir time elapsed", PerfTimer::Automatic, PerfTimer::Msec, [] (const wchar_t * msg) { fputws (msg, stdout); });
        }

        CDirectoryLister dirLister (cmdlinePtr, consolePtr, configPtr);

        for (const wstring & mask : cmdlinePtr->m_listMask)
        {
            dirLister.List (mask);
        }
    }


Error:      

    return FAILED (hr) ? 1 : 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  IsPowerShell
//
//  Detect if running in PowerShell by checking PSModulePath environment variable
//
////////////////////////////////////////////////////////////////////////////////

bool IsPowerShell (void)
{
    WCHAR buffer[1];
    DWORD result = GetEnvironmentVariableW (L"PSModulePath", buffer, ARRAYSIZE(buffer));
    
    // If result > 0, the variable exists (even if buffer is too small)
    return result > 0;
}




////////////////////////////////////////////////////////////////////////////////
//
//  DisplayUsage
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void DisplayUsage (__in CConsole * pConsole)
{
    static LPCWSTR s_usageLines[] =
    {
        L"",
        L"Relmerator's Technicolor Directory (C)2004-2025 by Robert Elmer",
        L"",
        L"TCDIR [drive:][path][filename] [/A[[:]attributes]] [/O[[:]sortorder]] [/S]",
        L"",
        L"  [drive:][path][filename]",
        L"              Specifies drive, directory, and/or files to list.",
        L"",
        L"  /A          Displays files with specified attributes.",
        L"  attributes   D  Directories                R  Read-only files",
        L"               H  Hidden files               A  Files ready for archiving",
        L"               S  System files               T  Temporary files",
        L"               E  Encrypted files            C  Compressed files",
        L"               P  Reparse points             0  Sparse files",
        L"               -  Prefix meaning not",
        L"",
        L"  /O          List by files in sorted order.",
        L"  sortorder    N  By name (alphabetic)       S  By size (smallest first)",
        L"               E  By extension (alphabetic)  D  By date/time (oldest first)",
        L"               -  Prefix to reverse order",
        L"",
        L"  /S          Displays files in specified directory and all subdirectories.",
        L"",
        L"",
        L"",
        L"  Set the " TCDIR_ENV_VAR_NAME L" environment variable to override default colors for file ",
        L"  extensions or attributes:",
        L""
    };
    
    static LPCWSTR s_envVarSyntax[] =
    {
        L"      set " TCDIR_ENV_VAR_NAME L"=<attr|.ext>=<fore>[on <back>][;...]",           // CMD
        L"      $env:" TCDIR_ENV_VAR_NAME L" = \"<attr|.ext>=<fore>[on <back>][;...]\"",    // PowerShell
    };
    
    static LPCWSTR s_colorOverrideLines[] =
    {
        L"",
        L"      <attr>  An attribute (single character, see 'attributes' above)",
        L"      <.ext>  A file extension, including the leading period.",
        L"      <fore>  Foreground color",
        L"      <back>  Background color",
        L"",
        L"      Colors:",
        L"          Black        DarkGrey",
        L"          Blue         LightBlue",
        L"          Green        LightGreen",
        L"          Cyan         LightCyan",
        L"          Red          LightRed",
        L"          Magenta      LightMagenta",
        L"          Brown        Yellow",
        L"          LightGrey    White",
        L"",
    };
    
    static LPCWSTR s_envVarExample[] =
    {
        L"      Example: set " TCDIR_ENV_VAR_NAME L"=.cpp=Yellow;.h=LightCyan on Blue",           // CMD
        L"      Example: $env:" TCDIR_ENV_VAR_NAME L" = \".cpp=Yellow;.h=LightCyan on Blue\"",    // PowerShell
    };
    
    bool isPowerShell = IsPowerShell ();



    //
    // Display usage
    //
    
    for (LPCWSTR line : s_usageLines)
    {
        pConsole->Puts (CConfig::EAttribute::Default, line);
    }

    pConsole->Puts (CConfig::EAttribute::Default, s_envVarSyntax[isPowerShell]);
    
    for (LPCWSTR line : s_colorOverrideLines)
    {
        pConsole->Puts (CConfig::EAttribute::Default, line);
    }
    
    pConsole->Puts (CConfig::EAttribute::Default, s_envVarExample[isPowerShell]);
    pConsole->Puts (CConfig::EAttribute::Default, L"");
}
