#include "pch.h"
#include "NerdFontInstaller.h"

#include "AutoHandle.h"
#include "Console.h"
#include "NerdFontSystemAccess.h"
#include "NerdFontConstants.h"
#include "NerdFontPackage.h"
#include "NerdFontRegistrar.h"
#include "NerdFontTarget.h"
#include "WindowsTerminalSettings.h"
#include "TuiWidgets.h"
#include "UnicodeSymbols.h"





////////////////////////////////////////////////////////////////////////////////
//
//  TryMatchModeSwitch
//
//  Matches one argument against the elevated install/remove mode switches,
//  yielding the corresponding operation.  Factored out so the parse loop below
//  stays flat instead of nesting a second loop.
//
////////////////////////////////////////////////////////////////////////////////

bool CNerdFontInstaller::TryMatchModeSwitch (LPCWSTR pszArg, ENerdFontOperation & op)
{
    struct SModeSwitch
    {
        LPCWSTR            pszSwitch;
        ENerdFontOperation op;
    };

    static constexpr SModeSwitch s_rgModeSwitches[] =
    {
        { NerdFontConst::kpszElevatedInstallSwitch, ENerdFontOperation::Install   },
        { NerdFontConst::kpszElevatedRemoveSwitch,  ENerdFontOperation::Uninstall },
    };



    for (const SModeSwitch & sw : s_rgModeSwitches)
    {
        if (_wcsicmp (pszArg, sw.pszSwitch) == 0)
        {
            op = sw.op;
            return true;
        }
    }

    return false;
}





////////////////////////////////////////////////////////////////////////////////
//
//  TryParseElevatedInvocation
//
//  Parse the command line written for the elevated child by RunFontInstallElevated.
//  Succeeds only for a well-formed invocation carrying exactly one mode switch
//  (install or remove) and a font directory, returning the operation and path.
//
////////////////////////////////////////////////////////////////////////////////

bool CNerdFontInstaller::TryParseElevatedInvocation (int argc, WCHAR ** argv, ENerdFontOperation & op, wstring & strFontDir)
{
    HRESULT hr              = S_OK;
    wstring strDirPrefix    = wstring (NerdFontConst::kpszElevatedDirSwitch) + L"=";
    bool    fSawElevatedArg = false;
    bool    fSawMode        = false;
    bool    fModeConflict   = false;
    bool    fResult         = false;



    op = ENerdFontOperation::Uninstall;
    strFontDir.clear();

    BAIL_OUT_IF (argc <= 1 || argv == nullptr, S_OK);

    for (int i = 1; i < argc; ++i)
    {
        LPCWSTR            pszArg = argv[i];
        ENerdFontOperation swOp   = ENerdFontOperation::Uninstall;

        if (TryMatchModeSwitch (pszArg, swOp))
        {
            fModeConflict   = fModeConflict || (fSawMode && op != swOp);
            fSawElevatedArg = true;
            fSawMode        = true;
            op              = swOp;
            continue;
        }

        if (_wcsicmp (pszArg, NerdFontConst::kpszElevatedDirSwitch) == 0)
        {
            fSawElevatedArg = true;

            if (i + 1 < argc)
            {
                strFontDir = argv[++i];
            }

            continue;
        }

        if (_wcsnicmp (pszArg, strDirPrefix.c_str(), strDirPrefix.length()) == 0)
        {
            fSawElevatedArg = true;
            strFontDir      = pszArg + strDirPrefix.length();
        }
    }

    fResult = fSawElevatedArg && !fModeConflict && fSawMode && !strFontDir.empty();

Error:
    return fResult;
}





CNerdFontSystemAccessReal CNerdFontInstaller::s_sysAccess;





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::Install
//
//  Main entry point for installing Nerd Fonts.
//  Coordinates download, extraction, and system-wide font installation.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontInstaller::Install (CConsole & console)
{
    HRESULT                       hr                     = S_OK;
    wchar_t                       szInstallCwd[MAX_PATH] = { };
    DWORD                         dwRet                  = 0;
    vector<SNerdFontTargetState>  rgTargets;
    wstring                       strReleaseTag;
    int                           iStatusColumn          = 0;
    EFontInstallOutcome           fontOutcome            = EFontInstallOutcome::AlreadyPresent;
    static constexpr int          kcInstallSteps         = 5;



    console.SetAutoFlush (true);

    dwRet = GetCurrentDirectoryW (ARRAYSIZE (szInstallCwd), szInstallCwd);
    CBRAEx (dwRet > 0 && dwRet < ARRAYSIZE (szInstallCwd), E_UNEXPECTED);

    console.Printf (CConfig::Information, L"\nInstalling Nerd Fonts\n");

    //
    // Step 1: find the latest release.  The step label is printed BEFORE the
    // network call so the user always sees progress even if the request stalls.
    //

    PrintInstallStepPrefix (console, 1, kcInstallSteps, L"Finding latest Nerd Font release...");

    hr = ResolveLatestReleaseTag (console, strReleaseTag);
    CHR (hr);

    iStatusColumn = ComputeInstallStatusColumn (strReleaseTag.c_str(), kcInstallSteps);
    PrintInstallStepStatus (console, iStatusColumn, L"done.");

    //
    // Steps 2-5: download the package and install the font files system-wide.
    //

    hr = DownloadAndInstallFonts (console, strReleaseTag, iStatusColumn, kcInstallSteps, fontOutcome);
    CHR (hr);

    //
    // Configure terminal profiles to use the Nerd Font.
    //

    hr = DetectTargetStates (rgTargets, console);
    CHR (hr);

    console.Printf (CConfig::Information, L"\n");
    hr = PromptAndApplyTargets (console, ENerdFontOperation::Install, rgTargets, iStatusColumn);
    CHR (hr);

    //
    // If nothing was actually installed or configured, say so plainly rather
    // than implying work was done.
    //

    if (fontOutcome == EFontInstallOutcome::AlreadyPresent && !AnyTargetApplied (rgTargets))
    {
        console.Printf (CConfig::Information, L"\nNo changes were made. Nerd Fonts were already installed and no profile configuration changed.\n");
        BAIL_OUT_IF (true, S_OK);
    }

    hr = PromptReopenForTargets (console, rgTargets, szInstallCwd);
    CHR (hr);

    console.Printf (CConfig::Information, L"\nInstallation complete.\n");

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::ResolveLatestReleaseTag
//
//  Resolves the latest Nerd Font release tag, printing a user-facing message on
//  failure (distinguishing a GitHub rate limit from a general network error).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontInstaller::ResolveLatestReleaseTag (CConsole & console, wstring & strReleaseTag)
{
    HRESULT hr           = S_OK;
    bool    fRateLimited = false;



    hr = CNerdFontPackage::ResolveLatestTag (strReleaseTag, fRateLimited);
    CHRF (hr, console.Printf (CConfig::Error, fRateLimited
                              ? L"\n  GitHub API rate limit reached. Please wait 60 seconds and try again.\n"
                              : L"\n  Could not determine the latest Nerd Font release. Check your network connection.\n"));

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::DownloadAndInstallFonts
//
//  Steps 2-5 of the install flow: prepare a working directory, download the font
//  package, extract it, and install the font files system-wide (elevating only
//  when the files are not already present).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontInstaller::DownloadAndInstallFonts (CConsole & console, const wstring & strReleaseTag, int iStatusColumn, int cInstallSteps, EFontInstallOutcome & outcome)
{
    HRESULT         hr                     = S_OK;
    wchar_t         szTempDir[MAX_PATH]    = { };
    wchar_t         szExtractDir[MAX_PATH] = { };
    DWORD           dwRet                  = 0;
    std::error_code ec;
    wstring         strZipPath;



    outcome = EFontInstallOutcome::AlreadyPresent;

    //
    // Step 2: prepare the working directory.
    //

    PrintInstallStepPrefix (console, 2, cInstallSteps, L"Preparing installer...");

    dwRet = GetTempPathW (ARRAYSIZE(szTempDir), szTempDir);
    CBRAEx (dwRet > 0 && dwRet <= ARRAYSIZE(szTempDir), E_UNEXPECTED);

    hr = PathCchCombine (szExtractDir, ARRAYSIZE (szExtractDir), szTempDir, L"TCDir-NerdFonts");
    CHRA (hr);

    std::filesystem::remove_all (szExtractDir, ec);
    CBREx (CreateDirectoryW (szExtractDir, NULL) || GetLastError() == ERROR_ALREADY_EXISTS, E_FAIL);

    PrintInstallStepStatus (console, iStatusColumn, L"done.");

    //
    // Step 3: download the font package.
    //

    PrintInstallStepPrefix (console, 3, cInstallSteps, format (L"Downloading Cascadia Cove Nerd Font {}...", strReleaseTag).c_str());
    hr = CNerdFontPackage::Download (szTempDir, strReleaseTag.c_str(), strZipPath);
    CHR (hr);

    PrintInstallStepStatus (console, iStatusColumn, L"done.");

    //
    // Step 4: extract the font files.
    //

    PrintInstallStepPrefix (console, 4, cInstallSteps, L"Extracting font files...");
    hr = CNerdFontPackage::Extract (strZipPath.c_str(), szExtractDir);
    CHR (hr);

    PrintInstallStepStatus (console, iStatusColumn, L"done.");

    //
    // Step 5: install the font files system-wide, elevating if necessary.
    //

    PrintInstallStepPrefix (console, 5, cInstallSteps, L"Installing fonts to system...");
    if (!CNerdFontRegistrar::AreFontFilesPresent (szExtractDir))
    {
        hr = RunFontInstallElevated (console, ENerdFontOperation::Install, szExtractDir);
        CHR (hr);

        outcome = EFontInstallOutcome::Installed;
        PrintInstallStepStatus (console, iStatusColumn, L"done.");
    }
    else
    {
        PrintInstallStepStatus (console, iStatusColumn, L"already installed.");
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::Uninstall
//
//  Removes Nerd Font terminal configuration, and optionally removes the font
//  files themselves.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontInstaller::Uninstall (CConsole & console)
{
    HRESULT                      hr                     = S_OK;
    wchar_t                      szInstallCwd[MAX_PATH] = { };
    DWORD                        dwRet                  = 0;
    vector<SNerdFontTargetState> rgTargets;
    bool                         fAnyConfigured         = false;
    bool                         fAnyFontFilesInstalled = false;



    console.SetAutoFlush (true);

    dwRet = GetCurrentDirectoryW (ARRAYSIZE (szInstallCwd), szInstallCwd);
    CBRAEx (dwRet > 0 && dwRet < ARRAYSIZE (szInstallCwd), E_UNEXPECTED);

    console.Printf (CConfig::Information, L"\nUninstalling Nerd Fonts\n");

    hr = DetectTargetStates (rgTargets, console);
    CHR (hr);

    fAnyConfigured         = AnyTargetConfigured (rgTargets);
    fAnyFontFilesInstalled = CNerdFontRegistrar::HasSystemNerdFontFilesInstalled();

    if (!fAnyConfigured && !fAnyFontFilesInstalled)
    {
        console.Printf (CConfig::Information, L"  No Nerd Font target profiles are currently configured and no installed Nerd Font files were found.\n");
        BAIL_OUT_IF (true, S_OK);
    }

    if (fAnyConfigured)
    {
        int iStatusColumn = ComputeUninstallStatusColumn (rgTargets);

        hr = PromptAndApplyTargets (console, ENerdFontOperation::Uninstall, rgTargets, iStatusColumn);
        CHR (hr);
    }
    else
    {
        console.Printf (CConfig::Information, L"  No Nerd Font target profiles are currently configured.\n");
    }

    hr = RemoveFontFilesIfRequested (console, rgTargets, fAnyConfigured, fAnyFontFilesInstalled);
    CHR (hr);

    hr = PromptReopenForTargets (console, rgTargets, szInstallCwd);
    CHR (hr);

    console.Printf (CConfig::Information, L"\nUninstallation complete.\n");

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::AnyTargetConfigured
//
//  True if any target currently has the Nerd Font configured.
//
////////////////////////////////////////////////////////////////////////////////

bool CNerdFontInstaller::AnyTargetConfigured (const vector<SNerdFontTargetState> & rgTargets)
{
    for (const SNerdFontTargetState & target : rgTargets)
    {
        if (target.fConfigured)
        {
            return true;
        }
    }

    return false;
}




////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::AnyTargetApplied
//
//  True if any target's profile was actually written this run (configured or
//  removed), as opposed to being left untouched because it was already in the
//  desired state.
//
////////////////////////////////////////////////////////////////////////////////

bool CNerdFontInstaller::AnyTargetApplied (const vector<SNerdFontTargetState> & rgTargets)
{
    for (const SNerdFontTargetState & target : rgTargets)
    {
        if (target.fApplyAttempted && target.fApplySucceeded)
        {
            return true;
        }
    }

    return false;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::AnyConfiguredTargetUnselected
//
//  True if any configured target was left unselected, meaning the Nerd Font is
//  still in use and the system font files must not be removed.
//
////////////////////////////////////////////////////////////////////////////////

bool CNerdFontInstaller::AnyConfiguredTargetUnselected (const vector<SNerdFontTargetState> & rgTargets)
{
    for (const SNerdFontTargetState & target : rgTargets)
    {
        if (target.fConfigured && !target.fSelected)
        {
            return true;
        }
    }

    return false;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::ComputeUninstallStatusColumn
//
//  Column at which to print per-target status, sized to the widest configured
//  target's "[x] <name>" row.
//
////////////////////////////////////////////////////////////////////////////////

int CNerdFontInstaller::ComputeUninstallStatusColumn (const vector<SNerdFontTargetState> & rgTargets)
{
    int iStatusColumn = 0;

    for (const SNerdFontTargetState & target : rgTargets)
    {
        if (target.fConfigured)
        {
            iStatusColumn = max (iStatusColumn, static_cast<int> (8 + wcslen (target.pszDisplayName) + 2));
        }
    }

    return iStatusColumn;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::RemoveFontFilesIfRequested
//
//  Removes the installed system font files, unless the Nerd Font is still in use
//  by an unselected profile or no font files are present.  Prompts for, and then
//  performs, the elevated removal when appropriate.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontInstaller::RemoveFontFilesIfRequested (CConsole & console, const vector<SNerdFontTargetState> & rgTargets, bool fAnyConfigured, bool fAnyFontFilesInstalled)
{
    HRESULT         hr                      = S_OK;
    CTuiWidgets     tui                     (console);
    ETuiResult      tuiResult               = ETuiResult::Cancelled;
    wchar_t         szWinFontsDir[MAX_PATH] = { };
    DWORD           dwFontsLen              = 0;
    vector<wstring> rgPreview;
    wstring         strPrompt;



    if (fAnyConfigured && AnyConfiguredTargetUnselected (rgTargets))
    {
        console.Printf (CConfig::Information, L"  Nerd Fonts are still configured for one or more profiles; font files will not be removed.\n");
        BAIL_OUT_IF (true, S_OK);
    }

    if (!fAnyFontFilesInstalled)
    {
        console.Printf (CConfig::Information, L"  No installed Nerd Font files were found in the system Fonts folder.\n");
        BAIL_OUT_IF (true, S_OK);
    }

    dwFontsLen = GetWindowsDirectoryW (szWinFontsDir, ARRAYSIZE (szWinFontsDir));
    CBRAEx (dwFontsLen > 0 && dwFontsLen < ARRAYSIZE (szWinFontsDir), E_UNEXPECTED);

    hr = PathCchAppend (szWinFontsDir, ARRAYSIZE (szWinFontsDir), L"Fonts");
    CHRA (hr);

    hr = tui.Init();
    CHR (hr);

    strPrompt = format (L"Remove Nerd Fonts from {}?", szWinFontsDir);
    tuiResult = tui.PromptConfirmation (strPrompt.c_str(), rgPreview);

    if (tuiResult == ETuiResult::Interrupted)
    {
        hr = HRESULT_FROM_WIN32 (ERROR_CANCELLED);
        CHR (hr);
    }

    if (tuiResult == ETuiResult::Confirmed)
    {
        hr = RunFontInstallElevated (console, ENerdFontOperation::Uninstall, NerdFontConst::kpszManifestSentinel);
        CHR (hr);
    }

Error:
    tui.Cleanup();
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::PromptAndApplyTargets
//
//  Prompt the user to select targets, apply the install/remove to each selected
//  target, and record per-target status.  Returns ERROR_CANCELLED (after printing
//  an aborted message) when the user cancels, or E_FAIL if any target failed.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontInstaller::PromptAndApplyTargets (CConsole & console, ENerdFontOperation op, vector<SNerdFontTargetState> & rgTargets, int iStatusColumn)
{
    HRESULT hr              = S_OK;
    bool    fAnyApplyFailed = false;
    FnApply fnApply         = [&] (SNerdFontTargetState & target)
    {
        HRESULT hrTarget = ApplyTargetConfig (console, target, op);



        target.fApplyAttempted = true;
        target.fApplySucceeded = SUCCEEDED (hrTarget);
        target.strApplyStatus  = target.fApplySucceeded
            ? (op == ENerdFontOperation::Install ? L"configured." : L"removed.")
            : (op == ENerdFontOperation::Install ? L"error: failed to configure profile." : L"error: failed to remove profile.");
        if (FAILED (hrTarget))
        {
            fAnyApplyFailed = true;
        }
    };



    hr = PromptTargetSelection (console, op, rgTargets, fnApply, iStatusColumn);
    if (hr == HRESULT_FROM_WIN32 (ERROR_CANCELLED))
    {
        console.Printf (CConfig::Information, op == ENerdFontOperation::Install
            ? L"  Nerd Font installation aborted.\n"
            : L"  Nerd Font uninstallation aborted.\n");
        BAIL_OUT_IF (true, hr);
    }
    CHR (hr);

    if (fAnyApplyFailed)
    {
        hr = E_FAIL;
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::ComputeInstallStatusColumn
//
//  Compute the column at which right-aligned step/profile status text is printed,
//  from the widest install-step line and the widest target profile name.
//
////////////////////////////////////////////////////////////////////////////////

int CNerdFontInstaller::ComputeInstallStatusColumn (LPCWSTR pszReleaseTag, int cInstallSteps)
{
    vector<SNerdFontTargetState> rgStatusTargets;
    size_t                       cchLongestProfile = 0;
    size_t                       cchLongestStep    = 0;



    CNerdFontTargetCatalog::Build (rgStatusTargets);

    for (const SNerdFontTargetState & target : rgStatusTargets)
    {
        cchLongestProfile = max (cchLongestProfile, wcslen (target.pszDisplayName));
    }

    cchLongestStep = max (cchLongestStep, format (L"  [1/{}] {}", cInstallSteps, L"Finding latest Nerd Font release...").size());
    cchLongestStep = max (cchLongestStep, format (L"  [2/{}] {}", cInstallSteps, L"Preparing installer...").size());
    cchLongestStep = max (cchLongestStep, format (L"  [3/{}] {}", cInstallSteps, format (L"Downloading Cascadia Cove Nerd Font {}...", pszReleaseTag)).size());
    cchLongestStep = max (cchLongestStep, format (L"  [4/{}] {}", cInstallSteps, L"Extracting font files...").size());
    cchLongestStep = max (cchLongestStep, format (L"  [5/{}] {}", cInstallSteps, L"Installing fonts to system...").size());

    return static_cast<int> (max (cchLongestStep, 8 + cchLongestProfile) + 2);
}





////////////////////////////////////////////////////////////////////////////////
//
//  DetectWtConfigured
//
//  Marks the Windows Terminal target as configured (or not) and selected.
//
////////////////////////////////////////////////////////////////////////////////

void CNerdFontInstaller::DetectWtConfigured (vector<SNerdFontTargetState> & rgTargets, bool fConfigured)
{
    for (auto & target : rgTargets)
    {
        if (target.kind != ENerdFontTarget::WindowsTerminal)
        {
            continue;
        }

        target.fConfigured = fConfigured;
        target.fSelected   = true;
        break;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  DetectConsoleHostsConfigured
//
//  Marks each console-host target configured if its HKCU profile (primary or
//  alternate key) already has the Nerd Font face name.
//
////////////////////////////////////////////////////////////////////////////////

void CNerdFontInstaller::DetectConsoleHostsConfigured (vector<SNerdFontTargetState> & rgTargets)
{
    for (auto & target : rgTargets)
    {
        if (target.kind == ENerdFontTarget::WindowsTerminal)
        {
            continue;
        }

        target.fConfigured = IsConsoleHostConfigured (target);
        target.fSelected   = true;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  DetectTargetStates
//
//  Builds the target catalog, then marks which targets are already configured
//  (Windows Terminal via its settings.json, console hosts via the registry).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontInstaller::DetectTargetStates (vector<SNerdFontTargetState> & rgTargets, CConsole & console)
{
    HRESULT hr      = S_OK;
    string  strJson;
    bool    fExists = false;



    UNREFERENCED_PARAMETER (console);



    CNerdFontTargetCatalog::Build (rgTargets);

    hr = CWindowsTerminalSettings::ReadSettings (strJson, fExists);
    CHR (hr);

    DetectWtConfigured (rgTargets, fExists ? CWindowsTerminalSettings::IsConfigured (strJson) : false);
    DetectConsoleHostsConfigured (rgTargets);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::GetAlternateConsoleProfileKey
//
//  Builds the alternate WindowsApps pwsh console profile key.
//
////////////////////////////////////////////////////////////////////////////////

bool CNerdFontInstaller::GetAlternateConsoleProfileKey (const SNerdFontTargetState & target, wstring & strAltProfileKey)
{
    HRESULT hr                       = S_OK;
    wchar_t szLocalAppData[MAX_PATH] = { };
    DWORD   dwLen                    = 0;
    wstring strPath;
    bool    fResult                  = false;



    strAltProfileKey.clear();

    CBR (target.kind == ENerdFontTarget::Pwsh);

    dwLen = GetEnvironmentVariableW (L"LOCALAPPDATA", szLocalAppData, ARRAYSIZE (szLocalAppData));
    CBR (dwLen != 0 && dwLen < ARRAYSIZE (szLocalAppData));

    strPath = szLocalAppData;
    if (!strPath.empty() && strPath.back() != L'\\')
    {
        strPath += L'\\';
    }
    strPath += L"Microsoft\\WindowsApps\\pwsh.exe";

    replace (strPath.begin(), strPath.end(), L'\\', L'_');
    strAltProfileKey  = L"Console\\";
    strAltProfileKey += strPath;
    fResult           = true;

Error:
    return fResult;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::IsConsoleHostConfigured
//
//  True if the console host's NF font face is set on its primary profile key or,
//  for pwsh, its alternate (WindowsApps) profile key.
//
////////////////////////////////////////////////////////////////////////////////

bool CNerdFontInstaller::IsConsoleHostConfigured (const SNerdFontTargetState & target)
{
    wstring strAltProfileKey;
    bool    fConfigured = QueryConsoleProfileConfigured (target.pszConsoleProfileKey, s_sysAccess);



    if (!fConfigured && GetAlternateConsoleProfileKey (target, strAltProfileKey))
    {
        fConfigured = QueryConsoleProfileConfigured (strAltProfileKey.c_str(), s_sysAccess);
    }

    return fConfigured;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::ConfigureConsoleHostKeys
//
//  Install/uninstall the NF font face on the console host's primary profile key
//  and, for pwsh, its alternate (WindowsApps) profile key.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontInstaller::ConfigureConsoleHostKeys (const SNerdFontTargetState & target, ENerdFontOperation op)
{
    HRESULT hr         = S_OK;
    wstring strAltProfileKey;



    hr = ConfigureConsoleProfileKey (target.pszConsoleProfileKey, op, s_sysAccess);
    CHR (hr);

    if (GetAlternateConsoleProfileKey (target, strAltProfileKey))
    {
        (void) ConfigureConsoleProfileKey (strAltProfileKey.c_str(), op, s_sysAccess);
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::ApplyTargetConfig
//
//  Applies the requested font configuration to one target.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontInstaller::ApplyTargetConfig (CConsole & console, const SNerdFontTargetState & target, ENerdFontOperation op)
{
    HRESULT hr       = S_OK;
    bool    fSkipped = false;



    if (target.kind != ENerdFontTarget::WindowsTerminal)
    {
        hr = ConfigureConsoleHostSettings (console, target, op);
        CHR (hr);
    }
    else
    {
        hr = CWindowsTerminalSettings::Apply (op, fSkipped);
        CHR (hr);

        if (fSkipped)
        {
            console.Printf (CConfig::Information, L"        Windows Terminal settings not found. Skipped.\n");
        }
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::RunFontInstallElevated
//
//  Runs system font installation or removal through elevation.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontInstaller::RunFontInstallElevated (CConsole & console, ENerdFontOperation op, LPCWSTR pszFontDir)
{
    HRESULT           hr                  = S_OK;
    SHELLEXECUTEINFOW shExInfo            = { };
    WCHAR             szExePath[MAX_PATH] = { };
    wstring           strParams;
    DWORD             cchExePath          = 0;
    BOOL              fLaunched           = FALSE;



    //
    // Already elevated: perform the install/remove directly in this process.
    //

    if (IsUserAnAdmin())
    {
        hr = RunFontInstallInProcess (console, op, pszFontDir);
        BAIL_OUT_IF (true, hr);
    }

    console.Printf (CConfig::Information, op == ENerdFontOperation::Install
        ? L"\nRequesting elevation to install fonts system-wide...\n"
        : L"\nRequesting elevation to remove fonts system-wide...\n");

    cchExePath = GetModuleFileNameW (NULL, szExePath, ARRAYSIZE (szExePath));
    CBREx (cchExePath != 0, E_UNEXPECTED);

    strParams = format (L"{} {} \"{}\"",
                        op == ENerdFontOperation::Install ? NerdFontConst::kpszElevatedInstallSwitch : NerdFontConst::kpszElevatedRemoveSwitch,
                        NerdFontConst::kpszElevatedDirSwitch,
                        pszFontDir);

    shExInfo.cbSize       = sizeof (shExInfo);
    shExInfo.fMask        = SEE_MASK_NOCLOSEPROCESS;
    shExInfo.hwnd         = NULL;
    shExInfo.lpVerb       = L"runas";
    shExInfo.lpFile       = szExePath;
    shExInfo.lpParameters = strParams.c_str();
    shExInfo.lpDirectory  = NULL;
    shExInfo.nShow        = SW_NORMAL;

    fLaunched = ShellExecuteExW (&shExInfo);
    CBR (fLaunched);

    console.Printf (CConfig::Information, L"Waiting for the elevated installer to finish...\n");

    if (shExInfo.hProcess != NULL)
    {
        AutoHandle hProcess     = shExInfo.hProcess;
        DWORD      dwWaitResult = WaitForSingleObject (hProcess, INFINITE);
        DWORD      dwExitCode   = 0;
        BOOL       fGotExitCode = GetExitCodeProcess (hProcess, &dwExitCode);

        CBR (dwWaitResult == WAIT_OBJECT_0 && fGotExitCode);

        if (dwExitCode != 0)
        {
            console.Printf (CConfig::Error, L"\nThe elevated installer failed (exit code %lu).\n", dwExitCode);
            CBREx (false, E_FAIL);
        }
    }

    console.Printf (CConfig::Information, L"\nThe elevated font install completed successfully.\n");

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::RunElevatedInstallIfRequested
//
//  Handles the private elevated install/remove invocation.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontInstaller::RunElevatedInstallIfRequested (int argc, WCHAR ** argv, CConsole & console, bool & fHandled)
{
    HRESULT            hr         = S_OK;
    ENerdFontOperation op         = ENerdFontOperation::Uninstall;
    bool               fParsed    = false;
    wstring            strFontDir;



    fHandled = false;

    fParsed = TryParseElevatedInvocation (argc, argv, op, strFontDir);
    BAIL_OUT_IF (!fParsed, S_OK);

    console.SetAutoFlush (true);

    fHandled = true;
    hr = RunFontInstallInProcess (console, op, strFontDir.c_str());

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::RunFontInstallInProcess
//
//  Installs or removes system font files in this process.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontInstaller::RunFontInstallInProcess (CConsole & console, ENerdFontOperation op, LPCWSTR pszFontDir)
{
    HRESULT hr         = S_OK;



    if (op == ENerdFontOperation::Install)
    {
        hr = CNerdFontRegistrar::Install (pszFontDir, console);
    }
    else
    {
        hr = CNerdFontRegistrar::Remove (pszFontDir, console);
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  PrintInstallStepPrefix
//
//  Prints a colored numbered install step prefix.
//
////////////////////////////////////////////////////////////////////////////////

void CNerdFontInstaller::PrintInstallStepPrefix (CConsole & console, int iStep, int cSteps, LPCWSTR pszText)
{
    console.ColorPrintf (L"  {Information}[{InformationHighlight}%d{Information}/{InformationHighlight}%d{Information}] %s",
                         iStep,
                         cSteps,
                         pszText);
}





////////////////////////////////////////////////////////////////////////////////
//
//  PrintInstallStepStatus
//
//  Prints right-column step status (for example "done.").
//
////////////////////////////////////////////////////////////////////////////////

void CNerdFontInstaller::PrintInstallStepStatus (CConsole & console, int iStatusColumn, LPCWSTR pszStatus, bool fSuccess)
{
    WORD attr = fSuccess ? (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
                         : (FOREGROUND_RED   | FOREGROUND_INTENSITY);

    console.Printf (CConfig::Information, L"\x1b[%dG", iStatusColumn);
    console.Printf (attr, L"%s\n", pszStatus);
}





////////////////////////////////////////////////////////////////////////////////
//
//  ConfigureConsoleProfileKey
//
//  Install:   writes FaceName/FontFamily/FontWeight to HKCU\<pszProfileKey>.
//  Uninstall: deletes those three values.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontInstaller::ConfigureConsoleProfileKey (LPCWSTR pszProfileKey, ENerdFontOperation op, INerdFontSystemAccess & sys)
{
    HRESULT          hr              = S_OK;
    HKEY             hkey            = NULL;
    LONG             lResult         = ERROR_SUCCESS;
    static constexpr DWORD kdwFontFamily = 54;
    static constexpr DWORD kdwFontWeight = 400;



    lResult = sys.CreateOrOpenRegKey (HKEY_CURRENT_USER, pszProfileKey, KEY_SET_VALUE, hkey);
    CBRAEx (lResult == ERROR_SUCCESS, HRESULT_FROM_WIN32 (lResult));

    if (op == ENerdFontOperation::Install)
    {
        lResult = sys.SetRegString (hkey, L"FaceName", NerdFontConst::kpszConsoleFontFace);
        CBRAEx (lResult == ERROR_SUCCESS, HRESULT_FROM_WIN32 (lResult));

        lResult = sys.SetRegDword (hkey, L"FontFamily", kdwFontFamily);
        CBRAEx (lResult == ERROR_SUCCESS, HRESULT_FROM_WIN32 (lResult));

        lResult = sys.SetRegDword (hkey, L"FontWeight", kdwFontWeight);
        CBRAEx (lResult == ERROR_SUCCESS, HRESULT_FROM_WIN32 (lResult));
    }
    else
    {
        sys.DeleteRegValue (hkey, L"FaceName");
        sys.DeleteRegValue (hkey, L"FontFamily");
        sys.DeleteRegValue (hkey, L"FontWeight");
    }

Error:
    if (hkey != NULL)
    {
        sys.CloseRegKey (hkey);
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  QueryConsoleProfileConfigured
//
//  Returns true when HKCU\<pszProfileKey>\FaceName equals the NF face name.
//
////////////////////////////////////////////////////////////////////////////////

bool CNerdFontInstaller::QueryConsoleProfileConfigured (LPCWSTR pszProfileKey, INerdFontSystemAccess & sys)
{
    HKEY    hkey    = NULL;
    wstring strFaceName;
    bool    fFound  = false;



    if (sys.OpenRegKey (HKEY_CURRENT_USER, pszProfileKey, KEY_QUERY_VALUE, hkey) == ERROR_SUCCESS)
    {
        if (sys.QueryRegString (hkey, L"FaceName", strFaceName) == ERROR_SUCCESS
            && _wcsicmp (strFaceName.c_str(), NerdFontConst::kpszConsoleFontFace) == 0)
        {
            fFound = true;
        }

        sys.CloseRegKey (hkey);
    }

    return fFound;
}





////////////////////////////////////////////////////////////////////////////////
//
//  PromptTargetSelection
//
//  Interactive wizard for selecting terminal profiles to configure/remove.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontInstaller::PromptTargetSelection (CConsole & console, ENerdFontOperation op, vector<SNerdFontTargetState> & rgTargets, FnApply fnApply, int iStatusColumn)
{
    HRESULT                     hr             = S_OK;
    CTuiWidgets                 tui            (console);
    ETuiResult                  tuiResult      = ETuiResult::Cancelled;
    int                         cRenderedLines = 0;
    vector<pair<wstring, bool>> rgItems;
    vector<bool>                rgLocked;
    vector<size_t>              rgIndexMap;



    for (size_t i = 0; i < rgTargets.size(); ++i)
    {
        const SNerdFontTargetState & target = rgTargets[i];

        if (op == ENerdFontOperation::Uninstall && !target.fConfigured)
        {
            continue;
        }

        rgItems.push_back ({ target.pszDisplayName, true });
        rgLocked.push_back (op == ENerdFontOperation::Install && target.fConfigured);
        rgIndexMap.push_back (i);
    }

    if (rgItems.empty())
    {
        console.Printf (CConfig::Information, L"    No terminal profiles are currently configured with Nerd Fonts.\n");
        BAIL_OUT_IF (true, S_OK);
    }

    hr = tui.Init();
    CHR (hr);

    tuiResult = tui.PromptCheckboxList (op == ENerdFontOperation::Install
                                        ? L"Select terminal profiles to configure:"
                                        : L"Select profiles to remove Nerd Fonts from:",
                                        rgItems,
                                        rgLocked,
                                        L"",
                                        &cRenderedLines,
                                        iStatusColumn,
                                        L"already configured.");

    if (tuiResult != ETuiResult::Confirmed)
    {
        hr = HRESULT_FROM_WIN32 (ERROR_CANCELLED);
        CHR (hr);
    }

    //
    // Propagate selection back to rgTargets.
    //

    for (auto & target : rgTargets)
    {
        target.fSelected = false;
    }

    for (size_t i = 0; i < rgItems.size(); ++i)
    {
        rgTargets[rgIndexMap[i]].fSelected = rgItems[i].second;
    }

    //
    // If an apply callback is provided, apply each selected non-locked target
    // and redraw the list in place with per-item success/failure annotations.
    // Cursor is currently cRenderedLines below the first item row.
    //

    if (fnApply != nullptr)
    {
        tui.MoveCursorUp (cRenderedLines + 1);

        for (size_t i = 0; i < rgItems.size(); ++i)
        {
            bool                    fIsLocked   = (i < rgLocked.size() && rgLocked[i]);
            bool                    fIsSelected = rgItems[i].second;
            SNerdFontTargetState & target      = rgTargets[rgIndexMap[i]];

            if (fIsSelected && !fIsLocked)
            {
                fnApply (target);
            }

            tui.ClearCurrentLine();
            PrintAppliedTargetRow (console, target, fIsLocked, fIsSelected, iStatusColumn);
        }

        console.Printf (CConfig::Information, L"\n");
    }

Error:
    tui.Cleanup();
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontInstaller::PrintAppliedTargetRow
//
//  Redraws one profile row after applying: its checkbox, name, and status text at
//  iStatusColumn (green for success/already-configured, red for failure; an
//  unchecked, status-free row when the profile was not selected).
//
////////////////////////////////////////////////////////////////////////////////

void CNerdFontInstaller::PrintAppliedTargetRow (CConsole & console, const SNerdFontTargetState & target, bool fIsLocked, bool fIsSelected, int iStatusColumn)
{
    bool    fOk = false;
    wstring strStatus;



    if (!fIsSelected)
    {
        console.ColorPrintf (L"    {Information}[ ] {Information}%s\n", target.pszDisplayName);
        return;
    }

    fOk       = fIsLocked || target.fApplySucceeded;
    strStatus = fIsLocked ? L"already configured." : target.strApplyStatus;

    if (fOk)
    {
        console.Printf (CConfig::Information, L"    [");
        console.Printf (FOREGROUND_GREEN | FOREGROUND_INTENSITY, L"%c", UnicodeSymbols::CheckMark);
        console.Printf (CConfig::Information, L"] %s", target.pszDisplayName);
    }
    else
    {
        console.ColorPrintf (L"    {Information}[{Error}x{Information}] {Information}%s", target.pszDisplayName);
    }

    if (iStatusColumn > 0)
    {
        console.Printf (CConfig::Information, L"\x1b[%dG", iStatusColumn);
    }
    else
    {
        console.Printf (CConfig::Information, L" ");
    }

    if (fOk)
    {
        console.Printf (FOREGROUND_GREEN | FOREGROUND_INTENSITY, L"%s\n", strStatus.c_str());
    }
    else
    {
        console.ColorPrintf (L"{Error}%s\n", strStatus.c_str());
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  ConfigureConsoleHostSettings
//
//  Configure or unconfigure font face for classic console hosts (cmd/powershell/pwsh).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontInstaller::ConfigureConsoleHostSettings (CConsole & console, const SNerdFontTargetState & target, ENerdFontOperation op)
{
    HRESULT hr = S_OK;



    UNREFERENCED_PARAMETER (console);

    BAIL_OUT_IF (target.kind == ENerdFontTarget::WindowsTerminal, S_OK);

    hr = ConfigureConsoleHostKeys (target, op);
    CHR (hr);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  PromptReopenForTargets
//
//  Displays a restart reminder for targets that were changed.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontInstaller::PromptReopenForTargets (CConsole & console, const vector<SNerdFontTargetState> & rgTargets, LPCWSTR pszInstallCwd)
{
    UNREFERENCED_PARAMETER (pszInstallCwd);

    if (AnyTargetApplied (rgTargets))
    {
        console.Printf (CConfig::Information, L"\n  Restart any affected terminal windows/tabs for font changes to take effect.\n");
    }

    return S_OK;
}





