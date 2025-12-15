// TCDir.cpp : Defines the entry point for the console application.
//

#include "pch.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "DirectoryLister.h"
#include "PerfTimer.h"
#include "Usage.h"





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

    //
    // Process the commandline
    //

    hr = cmdlinePtr->Parse (argc - 1, argv + 1);
    if (cmdlinePtr->m_fHelp || FAILED (hr))
    {
        CUsage::DisplayUsage (*consolePtr);
        BAIL_OUT_IF (cmdlinePtr->m_fHelp, S_OK);
        CHR (hr);
    }

    if (cmdlinePtr->m_fEnv)
    {
        CUsage::DisplayEnvVarConfigurationReport (*consolePtr);
        BAIL_OUT_IF (cmdlinePtr->m_fEnv, S_OK);
    }

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
