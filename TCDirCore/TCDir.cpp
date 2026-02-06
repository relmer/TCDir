// TCDir.cpp : Defines the entry point for the console application.
//

#include "pch.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "DirectoryLister.h"
#include "MaskGrouper.h"
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
    // Apply switch defaults from TCDIR environment variable
    //

    cmdlinePtr->ApplyConfigDefaults (*configPtr);

    //
    // Process the commandline
    //

    hr = cmdlinePtr->Parse (argc - 1, argv + 1);
    if (cmdlinePtr->m_fHelp || FAILED (hr))
    {
        CUsage::DisplayUsage (*consolePtr, cmdlinePtr->GetSwitchPrefix ());
        BAIL_OUT_IF (cmdlinePtr->m_fHelp, S_OK);
        CHR (hr);
    }

    if (cmdlinePtr->m_fEnv)
    {
        CUsage::DisplayEnvVarHelp (*consolePtr, cmdlinePtr->GetSwitchPrefix ());
        BAIL_OUT_IF (cmdlinePtr->m_fEnv, S_OK);
    }

    if (cmdlinePtr->m_fConfig)
    {
        CUsage::DisplayCurrentConfiguration (*consolePtr, cmdlinePtr->GetSwitchPrefix ());
        BAIL_OUT_IF (cmdlinePtr->m_fConfig, S_OK);
    }

    //
    // Use default mask if no mask is given
    //

    if (cmdlinePtr->m_listMask.empty())
    {
        cmdlinePtr->m_listMask.push_back (L"*");
    }
    
    //
    // Group masks by their target directories, then process each group.
    // Pure masks (like *.cpp, *.h) are combined for the current directory.
    // Directory-qualified masks (like src\*.cpp) are grouped by their directory.
    // This allows "tcdir -s *.cpp *.h" to show combined results in each directory.
    //

    {
        if (cmdlinePtr->m_fPerfTimer)
        {
            perfTimerPtr = make_unique<PerfTimer> (L"TCDir time elapsed", PerfTimer::Automatic, PerfTimer::Msec, [] (const wchar_t * msg) { fputws (msg, stdout); });
        }

        CDirectoryLister dirLister (cmdlinePtr, consolePtr, configPtr);

        auto groups = CMaskGrouper::GroupMasksByDirectory (cmdlinePtr->m_listMask);

        for (const auto & group : groups)
        {
            dirLister.List (group);
        }
    }

    //
    // Display any TCDIR environment variable issues at the end of the run
    //

    CUsage::DisplayEnvVarIssues (*consolePtr, cmdlinePtr->GetSwitchPrefix ());

    

Error:      

    return FAILED (hr) ? 1 : 0;
}
