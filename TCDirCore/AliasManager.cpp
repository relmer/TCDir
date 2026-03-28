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
        console.Printf (CConfig::Information, L"\n  Aliases written to: %s\n\n", strTargetPath.c_str());
        console.Puts   (CConfig::Information, L"  To activate, open a new PowerShell window or paste this command:");
        console.Printf (CConfig::Default,     L"    . \"%s\"\n", strTargetPath.c_str());
        console.Puts   (CConfig::Information, L"    ^--- the dot is required; paste the entire line exactly as shown");
    }
    else
    {
        console.Printf (CConfig::Error, L"\n  Error: Could not write to %s\n", strTargetPath.c_str());
        console.Puts   (CConfig::Error, L"  Check that the file is not locked and you have write permission.");
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
//  CAliasManager::PrintIntroduction
//
//  Displays the wizard header and description text.
//
////////////////////////////////////////////////////////////////////////////////

void CAliasManager::PrintIntroduction (CConsole & console, bool fWhatIf)
{
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
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::BuildConfigFromWizard
//
//  Populates an SAliasConfig from the wizard's collected results.
//
////////////////////////////////////////////////////////////////////////////////

void CAliasManager::BuildConfigFromWizard (
    SAliasConfig                     & config,
    const wstring                    & strRootAlias,
    const vector<SAliasDefinition>   & rgSubAliases,
    const vector<SProfileLocation>   & rgLocations,
    int                                iSelected,
    bool                               fWhatIf)
{
    config.strRootAlias = strRootAlias;
    config.rgSubAliases = rgSubAliases;
    config.fWhatIf      = fWhatIf;

    if (iSelected < static_cast<int>(rgLocations.size()))
    {
        config.eTargetScope  = rgLocations[iSelected].eScope;
        config.strTargetPath = rgLocations[iSelected].strResolvedPath;
        config.fSessionOnly  = false;
    }
    else
    {
        config.eTargetScope  = EProfileScope::SessionOnly;
        config.fSessionOnly  = true;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::ConfirmAndApply
//
//  Shows the preview, asks for confirmation (unless Whatif), and applies.
//  In Whatif mode, prints the preview and exits without confirmation.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CAliasManager::ConfirmAndApply (
    CConsole              & console,
    CTuiWidgets           & tui,
    SAliasConfig          & config,
    const vector<wstring> & rgBlockLines)
{
    HRESULT         hr        = S_OK;
    vector<wstring> rgPreview;
    ETuiResult      tuiResult = ETuiResult::Cancelled;



    //
    // In Whatif mode, skip confirmation — print preview and exit
    //

    if (config.fWhatIf)
    {
        tui.Cleanup();
        PrintWhatIfPreview (console, config, rgBlockLines);
        BAIL_OUT_IF (true, S_OK);
    }

    //
    // Session-only — skip confirmation, just output the function definitions
    //

    if (config.fSessionOnly)
    {
        tui.Cleanup();

        console.Printf (CConfig::Information, L"\n  Paste the following into your PowerShell session:\n\n");

        for (const auto & line : rgBlockLines)
        {
            if (!line.empty() && line[0] != L'#')
            {
                console.Printf (CConfig::Default, L"  %s\n", line.c_str());
            }
        }

        console.Flush();
        BAIL_OUT_IF (true, S_OK);
    }

    //
    // Build preview and confirm
    //

    for (const auto & line : rgBlockLines)
    {
        rgPreview.push_back (line);
    }

    tuiResult = tui.Confirmation (L"Apply these changes?", rgPreview);

    if (tuiResult == ETuiResult::Cancelled)
    {
        console.Puts (CConfig::Information, L"\n  Cancelled.\n");
        console.Flush();
        BAIL_OUT_IF (true, S_OK);
    }

    tui.Cleanup();

    hr = ApplyAliasBlock (console, config, rgBlockLines);
    CHR (hr);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::PromptRootAlias
//
//  Asks the user for the root alias name (1-4 alphanumeric chars).
//
////////////////////////////////////////////////////////////////////////////////

ETuiResult CAliasManager::PromptRootAlias (
    CConsole              & console,
    CTuiWidgets           & tui,
    const SAliasBlock     & existingBlock,
    wstring               & strRootAlias)
{
    wstring strDefault = L"d";

    if (existingBlock.fFound && !existingBlock.strRootAlias.empty())
    {
        strDefault = existingBlock.strRootAlias;
    }

    ETuiResult result = tui.TextInput (L"Root alias name (1-4 chars)", strDefault, strRootAlias, 4);

    if (result == ETuiResult::Confirmed)
    {
        console.Printf (CConfig::Information, L"\n");
        console.Flush();
    }

    return result;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::PromptSubAliases
//
//  Presents the sub-alias checkbox list and applies selections.
//
////////////////////////////////////////////////////////////////////////////////

ETuiResult CAliasManager::PromptSubAliases (
    CConsole                 & console,
    CTuiWidgets              & tui,
    const wstring            & strRootAlias,
    vector<SAliasDefinition> & rgSubAliases)
{
    vector<pair<wstring, bool>> rgCheckItems;
    bool                        fProceed     = true;



    BuildDefaultSubAliases (strRootAlias, rgSubAliases);
    BuildSubAliasLabels    (strRootAlias, rgSubAliases, rgCheckItems);

    ETuiResult result = tui.CheckboxList (L"Select sub-aliases:", rgCheckItems);

    if (result == ETuiResult::Confirmed)
    {
        for (size_t i = 0; i < rgSubAliases.size(); ++i)
        {
            rgSubAliases[i].fEnabled = rgCheckItems[i].second;
        }

        CheckConflicts (console, strRootAlias, rgSubAliases, fProceed);
    }

    return result;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::PromptProfileLocation
//
//  Presents the profile location radio list and returns the selection index.
//
////////////////////////////////////////////////////////////////////////////////

ETuiResult CAliasManager::PromptProfileLocation (
    CConsole                       & console,
    CTuiWidgets                    & tui,
    const vector<SProfileLocation> & rgLocations,
    bool                             fWhatIf,
    int                            & iSelected)
{
    vector<wstring>      rgRadioItems;
    bool                 fIsAdmin     = false;
    CProfilePathResolver resolver;



    resolver.IsRunningAsAdmin (fIsAdmin);

    BuildProfileLabels (rgLocations, fIsAdmin, static_cast<int> (console.GetWidth()), rgRadioItems, iSelected);

    LPCWSTR pszPrompt = fWhatIf
                      ? L"Save aliases to: {Error}(Whatif: no changes will be written)"
                      : L"Save aliases to:";

    return tui.RadioButtonList (pszPrompt, rgRadioItems, iSelected);
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
    HRESULT                  hr            = S_OK;
    CTuiWidgets              tui             (console);
    vector<SProfileLocation> rgLocations;
    SAliasBlock              existingBlock;
    wstring                  strRootAlias;
    vector<SAliasDefinition> rgSubAliases;
    int                      iSelected     = 0;
    SAliasConfig             config;
    vector<wstring>          rgBlockLines;
    ETuiResult               tuiResult     = ETuiResult::Cancelled;



    hr = ScanProfiles (console, tui, rgLocations, existingBlock);
    CHR (hr);

    hr = tui.Init();
    CHR (hr);

    PrintIntroduction (console, fWhatIf);

    tuiResult = PromptRootAlias (console, tui, existingBlock, strRootAlias);
    BAIL_OUT_IF (tuiResult == ETuiResult::Cancelled, S_OK);

    tuiResult = PromptSubAliases (console, tui, strRootAlias, rgSubAliases);
    BAIL_OUT_IF (tuiResult == ETuiResult::Cancelled, S_OK);

    tuiResult = PromptProfileLocation (console, tui, rgLocations, fWhatIf, iSelected);
    BAIL_OUT_IF (tuiResult == ETuiResult::Cancelled, S_OK);

    BuildConfigFromWizard (config, strRootAlias, rgSubAliases, rgLocations, iSelected, fWhatIf);

    hr = ResolveTcDirInvocation (config.strTcDirInvocation);
    CHR (hr);

    CAliasBlockGenerator::Generate (config, wstring (VERSION_WSTRING), rgBlockLines);

    hr = ConfirmAndApply (console, tui, config, rgBlockLines);
    CHR (hr);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::DisplayProfileAliases
//
//  Reads a single profile file, finds an alias block, and displays it.
//  Returns true if aliases were found and displayed, false otherwise.
//
////////////////////////////////////////////////////////////////////////////////

bool CAliasManager::DisplayProfileAliases (
    CConsole               & console,
    CProfileFileManager    & fileMgr,
    const SProfileLocation & loc)
{
    vector<wstring> rgLines;
    bool            fHasBom = false;
    SAliasBlock     block;



    if (!loc.fExists)
    {
        return false;
    }

    if (FAILED (fileMgr.ReadProfileFile (loc.strResolvedPath, rgLines, fHasBom)))
    {
        return false;
    }

    fileMgr.FindAliasBlock (rgLines, block);

    if (!block.fFound)
    {
        return false;
    }

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

    return true;
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
    HRESULT                  hr          = S_OK;
    CProfilePathResolver     resolver;
    CProfileFileManager      fileMgr;
    EPowerShellVersion       eVersion    = EPowerShellVersion::Unknown;
    bool                     fFoundAny   = false;
    vector<SProfileLocation> rgLocations;



    hr = resolver.DetectPowerShellVersion (eVersion);
    CHR (hr);

    if (eVersion == EPowerShellVersion::Unknown)
    {
        console.Puts (CConfig::Error, L"\n  This command must be run from PowerShell (pwsh.exe or powershell.exe).\n");
        CHR (E_FAIL);
    }

    hr = resolver.ResolveProfilePaths (eVersion, rgLocations);
    CHR (hr);

    console.Puts (CConfig::Information, L"\n");

    for (const auto & loc : rgLocations)
    {
        if (DisplayProfileAliases (console, fileMgr, loc))
        {
            fFoundAny = true;
        }
    }

    if (!fFoundAny)
    {
        console.Puts (CConfig::Information, L"  No tcdir aliases found.\n");
        console.Puts (CConfig::Information, L"  Run 'tcdir --set-aliases' to configure aliases.\n");
    }

    
Error:
    console.Flush();

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::RemoveAliasBlockFromFile
//
//  Reads the profile, creates a backup, removes the alias block, and writes
//  the file back.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CAliasManager::RemoveAliasBlockFromFile (
    CConsole      & console,
    const wstring & strPath)
{
    HRESULT             hr          = S_OK;
    CProfileFileManager fileMgr;
    vector<wstring>     rgFileLines;
    bool                fHasBom     = false;
    SAliasBlock         block;



    hr = fileMgr.ReadProfileFile (strPath, rgFileLines, fHasBom);
    CHR (hr);

    hr = fileMgr.CreateBackup (strPath);
    CHR (hr);

    fileMgr.FindAliasBlock (rgFileLines, block);

    if (block.fFound)
    {
        fileMgr.RemoveAliasBlock (rgFileLines, block);

        hr = fileMgr.WriteProfileFile (strPath, rgFileLines, fHasBom);
        CHR (hr);
    }

Error:
    if (SUCCEEDED (hr))
    {
        console.Printf (CConfig::Information, L"\n  Aliases removed from: %s\n", strPath.c_str());
    }
    else
    {
        console.Printf (CConfig::Error, L"\n  Error: Could not write to %s\n", strPath.c_str());
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasManager::FindProfilesWithAliases
//
//  Scans all profile locations for alias blocks.  For each profile that has
//  a block, builds a radio label and records the resolved path and alias names.
//  Returns true if at least one profile has aliases.
//
////////////////////////////////////////////////////////////////////////////////

bool CAliasManager::FindProfilesWithAliases (
    vector<SProfileLocation> & rgLocations,
    vector<wstring>          & rgRadioLabels,
    vector<wstring>          & rgResolvedPaths,
    vector<vector<wstring>>  & rgAliasNames)
{
    CProfileFileManager fileMgr;



    for (size_t i = 0; i < rgLocations.size(); ++i)
    {
        if (!rgLocations[i].fExists) continue;

        vector<wstring> rgLines;
        bool            fBom = false;
        SAliasBlock     block;

        if (FAILED (fileMgr.ReadProfileFile (rgLocations[i].strResolvedPath, rgLines, fBom))) continue;

        fileMgr.FindAliasBlock (rgLines, block);

        if (!block.fFound) continue;

        rgLocations[i].fHasAliasBlock = true;

        //
        // Build the radio label: "$PROFILE.xxx (path) - aliases: a, b, c"
        //

        wstring label = format (L"{} ({}) - aliases: ", rgLocations[i].strVariableName, rgLocations[i].strResolvedPath);

        for (size_t j = 0; j < block.rgAliasNames.size(); ++j)
        {
            if (j > 0) label += L", ";
            label += block.rgAliasNames[j];
        }

        rgRadioLabels.push_back (label);
        rgResolvedPaths.push_back (rgLocations[i].strResolvedPath);
        rgAliasNames.push_back (block.rgAliasNames);
    }

    return !rgRadioLabels.empty();
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
    HRESULT                  hr              = S_OK;
    CProfilePathResolver     resolver;
    EPowerShellVersion       eVersion        = EPowerShellVersion::Unknown;
    CTuiWidgets              tui               (console);
    vector<SProfileLocation> rgLocations;
    vector<wstring>          rgRadioLabels;
    vector<wstring>          rgResolvedPaths;
    vector<vector<wstring>>  rgAliasNames;
    int                      iSelected       = 0;
    ETuiResult               tuiResult       = ETuiResult::Cancelled;



    hr = resolver.DetectPowerShellVersion (eVersion);
    CHR (hr);

    if (eVersion == EPowerShellVersion::Unknown)
    {
        console.Puts (CConfig::Error, L"\n  This command must be run from PowerShell (pwsh.exe or powershell.exe).\n");
        CHR (E_FAIL);
    }

    hr = resolver.ResolveProfilePaths (eVersion, rgLocations);
    CHR (hr);

    if (!FindProfilesWithAliases (rgLocations, rgRadioLabels, rgResolvedPaths, rgAliasNames))
    {
        console.Puts (CConfig::Information, L"\n  No tcdir aliases found in any profile.\n");
        BAIL_OUT_IF (true, S_OK);
    }

    hr = tui.Init();
    CHR (hr);

    console.Puts (CConfig::Information, L"\n");

    tuiResult = tui.RadioButtonList (L"Remove aliases from:", rgRadioLabels, iSelected);
    BAIL_OUT_IF (tuiResult == ETuiResult::Cancelled, S_OK);

    tui.Cleanup();

    if (fWhatIf)
    {
        console.Printf (CConfig::Information, L"\n  Whatif: The following aliases would be removed from:\n  %s\n\n",
                        rgResolvedPaths[iSelected].c_str());

        for (const auto & name : rgAliasNames[iSelected])
        {
            console.Printf (CConfig::Information, L"    %s\n", name.c_str());
        }

        console.Printf (CConfig::Error, L"\n  Whatif: No changes were made.\n");
        BAIL_OUT_IF (true, S_OK);
    }

    hr = RemoveAliasBlockFromFile (console, rgResolvedPaths[iSelected]);
    CHR (hr);

Error:
    console.Flush();
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
