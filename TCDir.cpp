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
        Usage (consolePtr.get());
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

	return 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Usage
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void Usage (__in CConsole * pConsole)
{
    pConsole->Puts (CConfig::EAttribute::Default, L"");
    pConsole->Puts (CConfig::EAttribute::Default, L"Relmerator's Technicolor Directory (C)2004-2025 by Robert Elmer");
    pConsole->Puts (CConfig::EAttribute::Default, L"");
    pConsole->Puts (CConfig::EAttribute::Default, L"TCDIR [drive:][path][filename] [/A[[:]attributes]] [/O[[:]sortorder]] [/S]");
    pConsole->Puts (CConfig::EAttribute::Default, L"");
    pConsole->Puts (CConfig::EAttribute::Default, L"  [drive:][path][filename]");
    pConsole->Puts (CConfig::EAttribute::Default, L"              Specifies drive, directory, and/or files to list.");
    pConsole->Puts (CConfig::EAttribute::Default, L"");
    pConsole->Puts (CConfig::EAttribute::Default, L"  /A          Displays files with specified attributes.");
    pConsole->Puts (CConfig::EAttribute::Default, L"  attributes   D  Directories                R  Read-only files");
    pConsole->Puts (CConfig::EAttribute::Default, L"               H  Hidden files               A  Files ready for archiving");
    pConsole->Puts (CConfig::EAttribute::Default, L"               S  System files               T  Temporary files");
    pConsole->Puts (CConfig::EAttribute::Default, L"               E  Encrypted files            C  Compressed files");
    pConsole->Puts (CConfig::EAttribute::Default, L"               P  Reparse points             0  Sparse files");
    pConsole->Puts (CConfig::EAttribute::Default, L"               -  Prefix meaning not");
    pConsole->Puts (CConfig::EAttribute::Default, L"");
    pConsole->Puts (CConfig::EAttribute::Default, L"  /O          List by files in sorted order.");
    pConsole->Puts (CConfig::EAttribute::Default, L"  sortorder    N  By name (alphabetic)       S  By size (smallest first)");
    pConsole->Puts (CConfig::EAttribute::Default, L"               E  By extension (alphabetic)  D  By date/time (oldest first)");
    pConsole->Puts (CConfig::EAttribute::Default, L"               -  Prefix to reverse order");
    pConsole->Puts (CConfig::EAttribute::Default, L"");
    pConsole->Puts (CConfig::EAttribute::Default, L"  /S          Displays files in specified directory and all subdirectories.");
    pConsole->Puts (CConfig::EAttribute::Default, L"");
    pConsole->Puts (CConfig::EAttribute::Default, L"");
    pConsole->Puts (CConfig::EAttribute::Default, L"");
}






