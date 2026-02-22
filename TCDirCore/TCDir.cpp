// TCDir.cpp : Defines the entry point for the console application.
//

#include "pch.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "DirectoryLister.h"
#include "MaskGrouper.h"
#include "NerdFontDetector.h"
#include "PerfTimer.h"
#include "ResultsDisplayerBare.h"
#include "ResultsDisplayerNormal.h"
#include "ResultsDisplayerTree.h"
#include "ResultsDisplayerWide.h"
#include "Usage.h"





////////////////////////////////////////////////////////////////////////////////
//
//  ProcessCommandLine
//
//  Parse CLI arguments and handle informational modes (/help, /env, /config).
//  Returns S_FALSE when an informational mode was displayed (caller should
//  skip listing).  Returns S_OK when the caller should proceed with listing.
//
////////////////////////////////////////////////////////////////////////////////

static HRESULT ProcessCommandLine (
    int            argc,
    WCHAR        * argv[],
    CCommandLine & cmdline,
    CConsole     & console)
{
    HRESULT hr = S_OK;



    hr = cmdline.Parse (argc - 1, argv + 1);
    if (cmdline.m_fHelp || FAILED (hr))
    {
        CUsage::DisplayUsage (console, cmdline.GetSwitchPrefix(), cmdline.m_fIcons);

        if (!cmdline.m_strValidationError.empty())
        {
            console.Printf (CConfig::Error, L"\n  %s\n", cmdline.m_strValidationError.c_str());
        }

        BAIL_OUT_IF (cmdline.m_fHelp, S_FALSE);
        CHR (hr);
    }

    if (cmdline.m_fEnv)
    {
        CUsage::DisplayEnvVarHelp (console, cmdline.GetSwitchPrefix());
        BAIL_OUT_IF (cmdline.m_fEnv, S_FALSE);
    }

    if (cmdline.m_fConfig)
    {
        CUsage::DisplayCurrentConfiguration (console, cmdline.GetSwitchPrefix(), cmdline.m_fIcons);
        BAIL_OUT_IF (cmdline.m_fConfig, S_FALSE);
    }

    //
    // Use default mask if no mask is given
    //

    if (cmdline.m_listMask.empty())
    {
        cmdline.m_listMask.push_back (L"*");
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CreateDisplayer
//
//  Resolve icon activation state and create the appropriate displayer.
//  Priority: CLI flag > env var > auto-detection.
//
////////////////////////////////////////////////////////////////////////////////

static unique_ptr<IResultsDisplayer> CreateDisplayer (
    shared_ptr<CCommandLine>   cmdlinePtr,
    shared_ptr<CConsole>       consolePtr,
    shared_ptr<CConfig>        configPtr)
{
    bool fIconsActive = false;



    if (cmdlinePtr->m_fIcons.has_value())
    {
        // CLI flag always wins
        fIconsActive = cmdlinePtr->m_fIcons.value();
    }
    else if (configPtr->m_fIcons.has_value())
    {
        // TCDIR env var Icons/Icons- switch
        fIconsActive = configPtr->m_fIcons.value();
    }
    else
    {
        // Auto-detect: probe console font / enumerate system fonts
        CNerdFontDetector  detector;
        EDetectionResult   result = EDetectionResult::NotDetected;
        HANDLE             hOut   = GetStdHandle (STD_OUTPUT_HANDLE);

        if (SUCCEEDED (detector.Detect (hOut, *configPtr->m_pEnvironmentProvider, result)))
        {
            fIconsActive = (result == EDetectionResult::Detected);
        }
    }

    if (cmdlinePtr->m_fBareListing)
    {
        return make_unique<CResultsDisplayerBare> (cmdlinePtr, consolePtr, configPtr, fIconsActive);
    }
    else if (cmdlinePtr->m_fWideListing)
    {
        return make_unique<CResultsDisplayerWide> (cmdlinePtr, consolePtr, configPtr, fIconsActive);
    }
    else if (cmdlinePtr->m_fTree)
    {
        return make_unique<CResultsDisplayerTree> (cmdlinePtr, consolePtr, configPtr, fIconsActive);
    }
    else
    {
        return make_unique<CResultsDisplayerNormal> (cmdlinePtr, consolePtr, configPtr, fIconsActive);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  RunDirectoryListing
//
//  Create the displayer, group masks by directory, and run the listing.
//
////////////////////////////////////////////////////////////////////////////////

static void RunDirectoryListing (
    shared_ptr<CCommandLine>   cmdlinePtr,
    shared_ptr<CConsole>       consolePtr,
    shared_ptr<CConfig>        configPtr)
{
    unique_ptr<IResultsDisplayer> displayer = CreateDisplayer (cmdlinePtr, consolePtr, configPtr);
    CDirectoryLister              dirLister   (cmdlinePtr, consolePtr, configPtr, std::move (displayer));

    auto groups = CMaskGrouper::GroupMasksByDirectory (cmdlinePtr->m_listMask);



    for (const auto & group : groups)
    {
        dirLister.List (group);
    }
}





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
    // Process the commandline — returns S_FALSE for informational modes
    //

    hr = ProcessCommandLine (argc, argv, *cmdlinePtr, *consolePtr);
    CHR (hr);
    BAIL_OUT_IF (hr == S_FALSE, S_OK);

    //
    // Run the directory listing
    //

    if (cmdlinePtr->m_fPerfTimer)
    {
        perfTimerPtr = make_unique<PerfTimer> (L"TCDir time elapsed", PerfTimer::Automatic, PerfTimer::Msec, [] (const wchar_t * msg) { fputws (msg, stdout); });
    }

    RunDirectoryListing (cmdlinePtr, consolePtr, configPtr);

    //
    // Display any TCDIR environment variable issues at the end of the run
    //

    CUsage::DisplayEnvVarIssues (*consolePtr, cmdlinePtr->GetSwitchPrefix ());

    

Error:      

    return FAILED (hr) ? 1 : 0;
}
