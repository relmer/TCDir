#pragma once

#include "CommandLine.h"
#include "Console.h"
#include "ProfilePathResolver.h"
#include "ProfileFileManager.h"
#include "AliasBlockGenerator.h"





////////////////////////////////////////////////////////////////////////////////
//
//  SAliasDefinition
//
//  A single alias (root or sub) to be generated.
//
////////////////////////////////////////////////////////////////////////////////

struct SAliasDefinition
{
    wstring strName;         // Alias function name (e.g., "d", "dd", "ds")
    wstring strFlags;        // tcdir flags to prepend (empty for root; e.g., "/a:d")
    wstring strDescription;  // Human-readable description (e.g., "directories only")
    bool    fEnabled = true; // Whether selected by user in the checkbox step
};





////////////////////////////////////////////////////////////////////////////////
//
//  SAliasConfig
//
//  The complete user configuration from the TUI wizard.
//
////////////////////////////////////////////////////////////////////////////////

struct SAliasConfig
{
    wstring                  strRootAlias;        // Root alias name chosen by user (default: "d")
    wstring                  strTcDirInvocation;  // How to invoke tcdir ("tcdir" or full path)
    vector<SAliasDefinition> rgSubAliases;        // Sub-aliases with enabled/disabled state
    EProfileScope            eTargetScope  = EProfileScope::CurrentUserAllHosts;
    wstring                  strTargetPath;       // Resolved path for the chosen profile
    bool                     fSessionOnly  = false;
    bool                     fWhatIf       = false;
};





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager
//
//  Orchestrates the three alias flows: set, get, remove.
//
////////////////////////////////////////////////////////////////////////////////

class CAliasManager
{
public:
    static HRESULT Run            (CCommandLine & cmdline, CConsole & console);

    static HRESULT SetAliases     (CConsole & console, bool fWhatIf);
    static HRESULT GetAliases     (CConsole & console);
    static HRESULT RemoveAliases  (CConsole & console, bool fWhatIf);

private:
    static void    BuildDefaultSubAliases  (const wstring & strRoot, vector<SAliasDefinition> & rgOut);
    static HRESULT ResolveTcDirInvocation  (wstring & strInvocation);
    static HRESULT CheckConflicts          (CConsole & console, const wstring & strRoot,
                                            const vector<SAliasDefinition> & rgSubs,
                                            bool & fProceed);
};
