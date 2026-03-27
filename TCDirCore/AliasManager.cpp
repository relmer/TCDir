#include "pch.h"
#include "AliasManager.h"

#include "TuiWidgets.h"
#include "Version.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::Run
//
//  Entry point: dispatches to SetAliases, GetAliases, or RemoveAliases.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CAliasManager::Run (CCommandLine & cmdline, CConsole & console)
{
    HRESULT hr = S_OK;



    if (cmdline.m_fSetAliases)
    {
        hr = SetAliases (console, cmdline.m_fWhatIf);
        CHR (hr);
    }
    else if (cmdline.m_fGetAliases)
    {
        hr = GetAliases (console);
        CHR (hr);
    }
    else if (cmdline.m_fRemoveAliases)
    {
        hr = RemoveAliases (console, cmdline.m_fWhatIf);
        CHR (hr);
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::ResolveTcDirInvocation
//
//  Determines how the generated functions should invoke tcdir:
//  by short name if on PATH, or by full path otherwise.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CAliasManager::ResolveTcDirInvocation (wstring & strInvocation)
{
    HRESULT hr                    = S_OK;
    WCHAR   szExePath[MAX_PATH]   = {};
    WCHAR   szFoundPath[MAX_PATH] = {};
    DWORD   cch                   = 0;
    DWORD   cchFound              = 0;



    //
    // Get the running exe's full path
    //

    cch = GetModuleFileNameW (nullptr, szExePath, MAX_PATH);
    CBRAEx (cch > 0 && cch < MAX_PATH, HRESULT_FROM_WIN32 (GetLastError()));

    //
    // Check if "tcdir.exe" is findable via PATH
    //

    cchFound = SearchPathW (nullptr, L"tcdir.exe", nullptr, MAX_PATH, szFoundPath, nullptr);

    if (cchFound > 0 && _wcsicmp (szExePath, szFoundPath) == 0)
    {
        strInvocation = L"tcdir";
    }
    else
    {
        strInvocation = szExePath;
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::BuildDefaultSubAliases
//
//  Creates the predefined sub-alias list derived from the root alias.
//
////////////////////////////////////////////////////////////////////////////////

void CAliasManager::BuildDefaultSubAliases (const wstring & strRoot, vector<SAliasDefinition> & rgOut)
{
    rgOut.clear();

    rgOut.push_back ({ strRoot + L"t",  L"--tree",  L"Tree view",        true });
    rgOut.push_back ({ strRoot + L"w",  L"-w",      L"Wide format",      true });
    rgOut.push_back ({ strRoot + L"d",  L"-a:d",    L"Directories only", true });
    rgOut.push_back ({ strRoot + L"s",  L"-s",      L"Recursive",        true });
    rgOut.push_back ({ strRoot + L"sb", L"-s -b",   L"Recursive bare",   true });
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::ScanProfiles
//
//  Shows a spinner while detecting the PS version, resolving profile paths,
//  and scanning all profiles for existing alias blocks.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CAliasManager::ScanProfiles (
    CConsole                 & console, 
    CTuiWidgets              & tui,
    vector<SProfileLocation> & rgLocations,
    SAliasBlock              & existingBlock)
{
    HRESULT              hr       = S_OK;
    CProfilePathResolver resolver;
    CProfileFileManager  fileMgr;
    EPowerShellVersion   eVersion = EPowerShellVersion::Unknown;



    console.Printf (CConfig::Information, L"\n");
    console.Flush();
    tui.ShowSpinner (L"Scanning PowerShell profiles...");

    hr = resolver.DetectPowerShellVersion (eVersion);
    CHR (hr);
    CBR (eVersion != EPowerShellVersion::Unknown);

    hr = resolver.ResolveProfilePaths (eVersion, rgLocations);
    CHR (hr);

    for (auto & loc : rgLocations)
    {
        vector<wstring> rgLines;
        bool            fBom    = false;
        SAliasBlock     block;



        if (!loc.fExists) continue;

        hr = fileMgr.ReadProfileFile (loc.strResolvedPath, rgLines, fBom);
        if (FAILED (hr)) continue;
        
        fileMgr.FindAliasBlock (rgLines, block);
        if (block.fFound)
        {
            loc.fHasAliasBlock = true;

            if (!existingBlock.fFound)
            {
                existingBlock = block;
            }
        }
    }

    
Error:
    tui.HideSpinner();

    if (eVersion == EPowerShellVersion::Unknown)
    {
        console.Printf (CConfig::Error, L"  This command must be run from PowerShell (pwsh.exe or powershell.exe).\n");
        console.Flush();
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::BuildSubAliasLabels
//
//  Creates column-aligned checkbox labels from the sub-alias definitions.
//
////////////////////////////////////////////////////////////////////////////////

void CAliasManager::BuildSubAliasLabels (
    const wstring                  & strRoot,
    const vector<SAliasDefinition> & rgSubs,
    vector<pair<wstring, bool>>    & rgCheckItems)
{
    size_t cchMaxName  = 0;
    size_t cchMaxFlags = 0;



    for (const auto & sub : rgSubs)
    {
        cchMaxName  = max (cchMaxName,  sub.strName.size());
        cchMaxFlags = max (cchMaxFlags, sub.strFlags.size());
    }

    for (const auto & sub : rgSubs)
    {
        wstring label = format (L"{:<{}} = {} {:<{}}  ({})",
                                sub.strName,  cchMaxName,
                                strRoot,
                                sub.strFlags, cchMaxFlags,
                                sub.strDescription);

        rgCheckItems.push_back ({ label, sub.fEnabled });
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::BuildProfileLabels
//
//  Creates radio button labels for profile locations with aligned columns
//  and path wrapping at backslash boundaries when the line exceeds console width.
//
////////////////////////////////////////////////////////////////////////////////

void CAliasManager::BuildProfileLabels (
    const vector<SProfileLocation> & rgLocations,
    bool                             fIsAdmin,
    int                              cxConsole,
    vector<wstring>                & rgRadioItems,
    int                            & iDefault)
{
    //
    // Compute column width for variable names
    //

    size_t cchMaxVarName = 0;



    for (const auto & loc : rgLocations)
    {
        cchMaxVarName = max (cchMaxVarName, loc.strVariableName.size());
    }

    //
    // The radio button prefix "  ❯ (●) " is 8 visible columns.
    // The path starts after: prefix + padded varname + "  ("
    //

    size_t cchPrefix    = 8;
    size_t cchPathStart = cchPrefix + cchMaxVarName + 3;  // prefix + varname + "  ("

    for (size_t i = 0; i < rgLocations.size(); ++i)
    {
        wstring strVarPadded = format (L"{:<{}}", rgLocations[i].strVariableName, cchMaxVarName);
        wstring strSuffix;

        if (rgLocations[i].fRequiresAdmin && !fIsAdmin)
        {
            strSuffix = L" (requires admin)";
        }

        if (rgLocations[i].fHasAliasBlock)
        {
            strSuffix = L" [has aliases]";
        }

        //
        // Build the path portion with manual wrapping.
        // If the full line fits, emit it as-is.
        // Otherwise, break the path at a '\' boundary and indent the
        // continuation line to align under the start of the path text.
        //

        wstring strPath    = rgLocations[i].strResolvedPath;
        wstring strOneLine = format (L"{}  ({}){}", strVarPadded, strPath, strSuffix);
        int     cchFull    = static_cast<int>(cchPrefix + strOneLine.size());

        if (cchFull <= cxConsole)
        {
            rgRadioItems.push_back (strOneLine);
        }
        else
        {
            int     cchAvail = cxConsole - static_cast<int>(cchPathStart);
            wstring strIndent (cchPathStart, L' ');
            wstring label = format (L"{}  (", strVarPadded);

            if (cchAvail > 0)
            {
                size_t cchFirstLine = min (static_cast<size_t>(cchAvail), strPath.size());
                size_t posBreak     = strPath.rfind (L'\\', cchFirstLine - 1);

                if (posBreak != wstring::npos && posBreak > 0)
                {
                    label += strPath.substr (0, posBreak + 1);
                    label += L"\n";
                    label += strIndent;
                    label += strPath.substr (posBreak + 1);
                }
                else
                {
                    label += strPath;
                }
            }
            else
            {
                label += strPath;
            }

            label += L")";
            label += strSuffix;

            rgRadioItems.push_back (label);
        }

        if (rgLocations[i].eScope == EProfileScope::CurrentUserAllHosts)
        {
            iDefault = static_cast<int>(i);
        }
    }

    // Add "Session only" option, aligned with the profile variable names
    rgRadioItems.push_back (format (L"{:<{}}  (not persisted)", L"Current session only", cchMaxVarName));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::WriteAliasBlockToFile
//
//  Reads the target profile, creates a backup, inserts or replaces the alias
//  block, and writes the file back.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CAliasManager::WriteAliasBlockToFile (
    CConsole              & console,
    const wstring         & strTargetPath,
    const vector<wstring> & rgBlockLines)
{
    HRESULT             hr          = S_OK;
    CProfileFileManager fileMgr;
    vector<wstring>     rgFileLines;
    bool                fHasBom     = false;
    SAliasBlock         targetBlock;



    if (filesystem::exists (strTargetPath))
    {
        hr = fileMgr.ReadProfileFile (strTargetPath, rgFileLines, fHasBom);
        CHR (hr);

        hr = fileMgr.CreateBackup (strTargetPath);
        CHR (hr);
    }

    fileMgr.FindAliasBlock (rgFileLines, targetBlock);

    if (targetBlock.fFound)
    {
        fileMgr.ReplaceAliasBlock (rgFileLines, targetBlock, rgBlockLines);
    }
    else
    {
        fileMgr.AppendAliasBlock (rgFileLines, rgBlockLines);
    }

    hr = fileMgr.WriteProfileFile (strTargetPath, rgFileLines, fHasBom);
    CHR (hr);

Error:
    if (SUCCEEDED (hr))
    {
        console.Printf (CConfig::Information, L"\n  Aliases written to: %s\n", strTargetPath.c_str());
        console.Puts (CConfig::Information, L"  Reload your profile to activate: . $PROFILE\n");
    }
    else
    {
        console.Printf (CConfig::Error, L"\n  Error: Could not write to %s\n", strTargetPath.c_str());
        console.Puts (CConfig::Error, L"  Check that the file is not locked and you have write permission.\n");
    }

    console.Flush();
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::PrintWhatIfPreview
//
//  Displays a preview of what would be written without modifying any files.
//
////////////////////////////////////////////////////////////////////////////////

void CAliasManager::PrintWhatIfPreview (
    CConsole              & console,
    const SAliasConfig    & config,
    const vector<wstring> & rgBlockLines)
{
    console.Printf (CConfig::Information, L"\n  Whatif: The following alias block would be written");

    if (!config.fSessionOnly)
    {
        console.Printf (CConfig::Information, L" to:\n  %s\n", config.strTargetPath.c_str());
    }
    else
    {
        console.Printf (CConfig::Information, L" to console.\n");
    }

    console.Printf (CConfig::Information, L"\n");

    for (const auto & line : rgBlockLines)
    {
        console.Printf (CConfig::Default, L"  %s\n", line.c_str());
    }

    console.Printf (CConfig::Error, L"\n  Whatif: No changes were made.\n");
    console.Flush();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::PrintSessionOnlyBlock
//
//  Outputs the alias block to the console for the user to paste manually.
//
////////////////////////////////////////////////////////////////////////////////

void CAliasManager::PrintSessionOnlyBlock (
    CConsole              & console,
    const vector<wstring> & rgBlockLines)
{
    console.Printf (CConfig::Information, L"\n  Paste the following into your PowerShell session:\n\n");

    for (const auto & line : rgBlockLines)
    {
        console.Printf (CConfig::Information, L"%s\n", line.c_str());
    }

    console.Flush();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::ApplyAliasBlock
//
//  Routes to WhatIf preview, session-only console output, or file write.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CAliasManager::ApplyAliasBlock (
    CConsole              & console,
    const SAliasConfig    & config,
    const vector<wstring> & rgBlockLines)
{
    HRESULT hr = S_OK;



    if (config.fWhatIf)
    {
        PrintWhatIfPreview (console, config, rgBlockLines);
    }
    else if (config.fSessionOnly)
    {
        PrintSessionOnlyBlock (console, rgBlockLines);
    }
    else
    {
        hr = WriteAliasBlockToFile (console, config.strTargetPath, rgBlockLines);
        CHR (hr);
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::SetAliases
//
//  Interactive wizard for setting up aliases.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CAliasManager::SetAliases (CConsole & console, bool fWhatIf)
{
    HRESULT                         hr = S_OK;
    CTuiWidgets                     tui (console);
    vector<SProfileLocation>        rgLocations;
    SAliasBlock                     existingBlock;
    wstring                         strRootAlias = L"d";
    wstring                         strResult;
    vector<SAliasDefinition>        rgSubAliases;
    vector<pair<wstring, bool>>     rgCheckItems;
    vector<wstring>                 rgRadioItems;
    int                             iDefault = 0;
    SAliasConfig                    config;
    vector<wstring>                 rgBlockLines;
    vector<wstring>                 rgPreview;
    ETuiResult                      tuiResult = ETuiResult::Cancelled;
    bool                            fProceedAfterConflicts = true;
    bool                            fIsAdmin = false;



    //
    // Scan profiles (with spinner)
    //

    hr = ScanProfiles (console, tui, rgLocations, existingBlock);
    CHR (hr);

    //
    // Initialize TUI
    //

    hr = tui.Init();
    CHR (hr);

    if (existingBlock.fFound && !existingBlock.strRootAlias.empty())
    {
        strRootAlias = existingBlock.strRootAlias;
    }

    //
    // Introduction text
    //

    console.Printf (CConfig::InformationHighlight, L"  TCDir Alias Setup");

    if (fWhatIf)
    {
        console.Printf (CConfig::Error, L"  (Whatif: preview only, no changes will be made)");
    }

    console.Printf (CConfig::Information, L"\n\n");
    console.Printf (CConfig::Information, L"  This wizard configures PowerShell aliases so you can invoke tcdir\n");
    console.Printf (CConfig::Information, L"  with short commands (e.g., 'd' instead of 'tcdir'). Aliases will be\n");
    console.Printf (CConfig::Information, L"  saved to your PowerShell profile and loaded automatically.\n");
    console.Printf (CConfig::Information, L"\n");
    console.Flush();

    //
    // Step 1: Root alias
    //

    tuiResult = tui.TextInput (L"Root alias name (1-4 chars)", strRootAlias, strResult, 4);

    if (tuiResult == ETuiResult::Cancelled)
    {
        console.Puts (CConfig::Information, L"\n  Cancelled.\n");
        console.Flush();
        BAIL_OUT_IF (true, S_OK);
    }

    strRootAlias = strResult;

    console.Printf (CConfig::Information, L"\n");
    console.Flush();

    //
    // Step 2: Sub-aliases
    //

    BuildDefaultSubAliases (strRootAlias, rgSubAliases);
    BuildSubAliasLabels (strRootAlias, rgSubAliases, rgCheckItems);

    tuiResult = tui.CheckboxList (L"Select sub-aliases:", rgCheckItems);

    if (tuiResult == ETuiResult::Cancelled)
    {
        console.Puts (CConfig::Information, L"\n  Cancelled.\n");
        console.Flush();
        BAIL_OUT_IF (true, S_OK);
    }

    for (size_t i = 0; i < rgSubAliases.size(); ++i)
    {
        rgSubAliases[i].fEnabled = rgCheckItems[i].second;
    }

    CheckConflicts (console, strRootAlias, rgSubAliases, fProceedAfterConflicts);

    //
    // Step 3: Profile location
    //

    {
        CProfilePathResolver resolver;

        resolver.IsRunningAsAdmin (fIsAdmin);
    }

    BuildProfileLabels (rgLocations, fIsAdmin, static_cast<int>(console.GetWidth()), rgRadioItems, iDefault);

    {
        LPCWSTR pszPrompt = fWhatIf
                          ? L"Save aliases to: {Error}(Whatif: no changes will be written)"
                          : L"Save aliases to:";

        tuiResult = tui.RadioButtonList (pszPrompt, rgRadioItems, iDefault);
    }

    if (tuiResult == ETuiResult::Cancelled)
    {
        console.Puts (CConfig::Information, L"\n  Cancelled.\n");
        console.Flush();
        BAIL_OUT_IF (true, S_OK);
    }

    //
    // Build config from wizard results
    //

    config.strRootAlias = strRootAlias;
    config.rgSubAliases = rgSubAliases;
    config.fWhatIf      = fWhatIf;

    hr = ResolveTcDirInvocation (config.strTcDirInvocation);
    CHR (hr);

    if (iDefault < static_cast<int>(rgLocations.size()))
    {
        config.eTargetScope  = rgLocations[iDefault].eScope;
        config.strTargetPath = rgLocations[iDefault].strResolvedPath;
        config.fSessionOnly  = false;
    }
    else
    {
        config.eTargetScope  = EProfileScope::SessionOnly;
        config.fSessionOnly  = true;
    }

    //
    // Generate the alias block
    //

    CAliasBlockGenerator::Generate (config, wstring (VERSION_WSTRING), rgBlockLines);

    //
    // Step 4: Preview / confirmation
    //
    // In WhatIf mode, skip confirmation — just show the preview and the
    // "no changes" message.
    //

    if (config.fSessionOnly)
    {
        rgPreview.push_back (L"Output: Console only (paste into your session)");
        rgPreview.push_back (L"");
    }

    for (const auto & line : rgBlockLines)
    {
        rgPreview.push_back (line);
    }

    if (config.fWhatIf)
    {
        tui.Cleanup();
        PrintWhatIfPreview (console, config, rgBlockLines);
        BAIL_OUT_IF (true, S_OK);
    }

    tuiResult = tui.Confirmation (L"Apply these changes?", rgPreview);

    if (tuiResult == ETuiResult::Cancelled)
    {
        console.Puts (CConfig::Information, L"\n  Cancelled.\n");
        console.Flush();
        BAIL_OUT_IF (true, S_OK);
    }

    //
    // Apply
    //

    tui.Cleanup();

    hr = ApplyAliasBlock (console, config, rgBlockLines);
    CHR (hr);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::GetAliases
//
//  Scans profile files and displays found alias blocks.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CAliasManager::GetAliases (CConsole & console)
{
    HRESULT                  hr         = S_OK;
    CProfilePathResolver     resolver;
    CProfileFileManager      fileMgr;
    EPowerShellVersion       eVersion   = EPowerShellVersion::Unknown;
    bool                     fFoundAny  = false;
    vector<SProfileLocation> rgLocations;



    //
    // Detect PowerShell version
    //

    hr = resolver.DetectPowerShellVersion (eVersion);
    CHR (hr);

    if (eVersion == EPowerShellVersion::Unknown)
    {
        console.Puts (CConfig::Error, L"\n  This command must be run from PowerShell (pwsh.exe or powershell.exe).\n");
        console.Flush();
        CHR (E_FAIL);
    }

    //
    // Resolve and scan all profile paths
    //

    hr = resolver.ResolveProfilePaths (eVersion, rgLocations);
    CHR (hr);

    console.Puts (CConfig::Information, L"\n");

    for (auto & loc : rgLocations)
    {
        if (!loc.fExists)
        {
            continue;
        }

        vector<wstring> rgLines;
        bool            fHasBom = false;

        if (FAILED (fileMgr.ReadProfileFile (loc.strResolvedPath, rgLines, fHasBom)))
        {
            continue;
        }

        SAliasBlock block;

        fileMgr.FindAliasBlock (rgLines, block);

        if (!block.fFound)
        {
            continue;
        }

        fFoundAny = true;

        console.Printf (CConfig::InformationHighlight, L"  %s\n", loc.strVariableName.c_str());
        console.Printf (CConfig::Information, L"  %s\n", loc.strResolvedPath.c_str());

        if (!block.strVersion.empty())
        {
            console.Printf (CConfig::Information, L"  Generated by tcdir v%s\n", block.strVersion.c_str());
        }

        console.Puts (CConfig::Information, L"\n");

        for (const auto & name : block.rgAliasNames)
        {
            console.Printf (CConfig::Information, L"    %s\n", name.c_str());
        }

        console.Puts (CConfig::Information, L"\n");
    }

    if (!fFoundAny)
    {
        console.Puts (CConfig::Information, L"  No tcdir aliases found.\n");
        console.Puts (CConfig::Information, L"  Run 'tcdir --set-aliases' to configure aliases.\n");
    }

    console.Flush();

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::RemoveAliases
//
//  Interactive wizard for removing aliases from a profile file.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CAliasManager::RemoveAliases (CConsole & console, bool fWhatIf)
{
    HRESULT                  hr = S_OK;
    CProfilePathResolver     resolver;
    CProfileFileManager      fileMgr;
    EPowerShellVersion       eVersion = EPowerShellVersion::Unknown;
    CTuiWidgets              tui (console);
    vector<SProfileLocation> rgLocations;
    vector<wstring>          rgRadioItems;
    int                      iSelected = 0;
    ETuiResult               tuiResult = ETuiResult::Cancelled;
    vector<wstring>          rgFileLines;
    bool                     fHasBom = false;
    SAliasBlock              removalBlock;



    //
    // Detect PowerShell version
    //

    hr = resolver.DetectPowerShellVersion (eVersion);
    CHR (hr);

    if (eVersion == EPowerShellVersion::Unknown)
    {
        console.Puts (CConfig::Error, L"\n  This command must be run from PowerShell (pwsh.exe or powershell.exe).\n");
        console.Flush();
        CHR (E_FAIL);
    }

    //
    // Resolve and scan all profile paths
    //

    hr = resolver.ResolveProfilePaths (eVersion, rgLocations);
    CHR (hr);

    //
    // Find profiles that have alias blocks
    //

    {
        struct SFoundBlock
        {
            size_t      iLocation;
            SAliasBlock block;
            bool        fHasBom;
        };

        vector<SFoundBlock> rgFound;

        for (size_t i = 0; i < rgLocations.size(); ++i)
        {
            if (!rgLocations[i].fExists)
            {
                continue;
            }

            vector<wstring> rgLines;
            bool            fBom = false;

            if (FAILED (fileMgr.ReadProfileFile (rgLocations[i].strResolvedPath, rgLines, fBom)))
            {
                continue;
            }

            SAliasBlock block;

            fileMgr.FindAliasBlock (rgLines, block);

            if (block.fFound)
            {
                rgLocations[i].fHasAliasBlock = true;
                rgFound.push_back ({ i, block, fBom });
            }
        }

        if (rgFound.empty())
        {
            console.Puts (CConfig::Information, L"\n  No tcdir aliases found in any profile.\n");
            console.Flush();
            BAIL_OUT_IF (true, S_OK);
        }

        //
        // Initialize TUI and present options
        //

        hr = tui.Init();
        CHR (hr);

        console.Puts (CConfig::Information, L"\n");

        for (const auto & found : rgFound)
        {
            const auto & loc   = rgLocations[found.iLocation];
            wstring      label = format (L"{} ({}) - aliases: ", loc.strVariableName, loc.strResolvedPath);

            for (size_t j = 0; j < found.block.rgAliasNames.size(); ++j)
            {
                if (j > 0) label += L", ";
                label += found.block.rgAliasNames[j];
            }

            rgRadioItems.push_back (label);
        }

        tuiResult = tui.RadioButtonList (L"Remove aliases from:", rgRadioItems, iSelected);

        if (tuiResult == ETuiResult::Cancelled)
        {
            console.Puts (CConfig::Information, L"\n  Cancelled.\n");
            console.Flush();
            BAIL_OUT_IF (true, S_OK);
        }

        //
        // Cleanup TUI before file operations
        //

        tui.Cleanup();

        //
        // WhatIf mode
        //

        if (fWhatIf)
        {
            console.Printf (CConfig::Information, L"\n  Whatif: The following aliases would be removed from:\n  %s\n\n",
                            rgLocations[rgFound[iSelected].iLocation].strResolvedPath.c_str());

            for (const auto & name : rgFound[iSelected].block.rgAliasNames)
            {
                console.Printf (CConfig::Information, L"    %s\n", name.c_str());
            }

            console.Puts (CConfig::Information, L"\n  Whatif: No changes were made.\n");
            console.Flush();
            BAIL_OUT_IF (true, S_OK);
        }

        //
        // Perform removal
        //

        hr = fileMgr.ReadProfileFile (rgLocations[rgFound[iSelected].iLocation].strResolvedPath, rgFileLines, fHasBom);
        CHR (hr);

        hr = fileMgr.CreateBackup (rgLocations[rgFound[iSelected].iLocation].strResolvedPath);
        CHR (hr);

        fileMgr.FindAliasBlock (rgFileLines, removalBlock);

        if (removalBlock.fFound)
        {
            fileMgr.RemoveAliasBlock (rgFileLines, removalBlock);

            hr = fileMgr.WriteProfileFile (rgLocations[rgFound[iSelected].iLocation].strResolvedPath, rgFileLines, fHasBom);

            if (FAILED (hr))
            {
                console.Printf (CConfig::Error, L"\n  Error: Could not write to %s\n",
                                rgLocations[rgFound[iSelected].iLocation].strResolvedPath.c_str());
                console.Flush();
                CHR (hr);
            }

            console.Printf (CConfig::Information, L"\n  Aliases removed from: %s\n",
                            rgLocations[rgFound[iSelected].iLocation].strResolvedPath.c_str());
            console.Flush();
        }
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::CheckConflicts
//
//  Checks if alias names conflict with existing commands/aliases.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CAliasManager::CheckConflicts (
    CConsole                         & console,
    const wstring                    & strRoot,
    const vector<SAliasDefinition>   & rgSubs,
    bool                             & fProceed)
{
    HRESULT hr = S_OK;



    fProceed = true;

    //
    // Known PowerShell built-in aliases that commonly conflict
    //

    static constexpr struct { LPCWSTR alias; LPCWSTR command; } s_krgBuiltins[] =
    {
        { L"ac",  L"Add-Content"           },
        { L"cat", L"Get-Content"           },
        { L"cd",  L"Set-Location"          },
        { L"cls", L"Clear-Host"            },
        { L"cp",  L"Copy-Item"             },
        { L"diff",L"Compare-Object"        },
        { L"dir", L"Get-ChildItem"         },
        { L"echo",L"Write-Output"          },
        { L"fc",  L"Format-Custom"         },
        { L"fl",  L"Format-List"           },
        { L"ft",  L"Format-Table"          },
        { L"fw",  L"Format-Wide"           },
        { L"gc",  L"Get-Content"           },
        { L"gm",  L"Get-Member"            },
        { L"gp",  L"Get-ItemProperty"      },
        { L"h",   L"Get-History"           },
        { L"ls",  L"Get-ChildItem"         },
        { L"man", L"help"                  },
        { L"md",  L"mkdir"                 },
        { L"mi",  L"Move-Item"             },
        { L"mp",  L"Move-ItemProperty"     },
        { L"mv",  L"Move-Item"             },
        { L"ni",  L"New-Item"              },
        { L"ps",  L"Get-Process"           },
        { L"r",   L"Invoke-History"        },
        { L"rd",  L"Remove-Item"           },
        { L"ri",  L"Remove-Item"           },
        { L"rm",  L"Remove-Item"           },
        { L"sc",  L"Set-Content"           },
        { L"si",  L"Set-Item"              },
        { L"sl",  L"Set-Location"          },
        { L"sp",  L"Set-ItemProperty"      },
        { L"sv",  L"Set-Variable"          },
        { L"type",L"Get-Content"           },
    };

    //
    // Collect all alias names to check
    //

    vector<wstring> rgNames = { strRoot };

    for (const auto & sub : rgSubs)
    {
        if (sub.fEnabled)
        {
            rgNames.push_back (sub.strName);
        }
    }

    //
    // Check each name against built-in list and SearchPath
    //

    vector<wstring> rgConflicts;

    for (const auto & name : rgNames)
    {
        for (const auto & builtin : s_krgBuiltins)
        {
            if (_wcsicmp (name.c_str(), builtin.alias) == 0)
            {
                rgConflicts.push_back (format (L"'{}' conflicts with PowerShell alias for {}", name, builtin.command));
                break;
            }
        }

        //
        // Check if an executable with this name exists on PATH
        //

        WCHAR szFound[MAX_PATH] = {};
        wstring strExeName = name + L".exe";

        DWORD cchFound = SearchPathW (nullptr, strExeName.c_str(), nullptr, MAX_PATH, szFound, nullptr);

        if (cchFound > 0)
        {
            rgConflicts.push_back (format (L"'{}' conflicts with executable: {}", name, szFound));
        }
    }

    if (!rgConflicts.empty())
    {
        console.Puts (CConfig::Information, L"\n");
        console.Printf (CConfig::Error, L"  Warning: %zu conflict(s) detected:\n\n", rgConflicts.size());

        for (const auto & conflict : rgConflicts)
        {
            console.Printf (CConfig::Information, L"    %s\n", conflict.c_str());
        }

        console.Puts (CConfig::Information, L"\n");
    }

    return hr;
}
