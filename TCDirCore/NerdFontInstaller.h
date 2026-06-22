#pragma once

#include "NerdFontConstants.h"
#include "NerdFontSystemAccess.h"
#include "NerdFontTarget.h"

class CConsole;





class CNerdFontInstaller
{
public:
    static HRESULT Install                       (CConsole & console);
    static HRESULT Uninstall                     (CConsole & console);
    static HRESULT RunElevatedInstallIfRequested (int argc, WCHAR ** argv, CConsole & console, bool & fHandled);

    //
    // Console-profile configuration seams, exposed so unit tests can drive them
    // through a mock INerdFontSystemAccess.
    //
    static HRESULT ConfigureConsoleProfileKey    (LPCWSTR pszProfileKey, ENerdFontOperation op, INerdFontSystemAccess & sys);
    static bool    QueryConsoleProfileConfigured (LPCWSTR pszProfileKey, INerdFontSystemAccess & sys);

private:
    using FnApply = function<void(SNerdFontTargetState&)>;

    static HRESULT DetectTargetStates            (vector<SNerdFontTargetState> & rgTargets, CConsole & console);
    static void    DetectWtConfigured            (vector<SNerdFontTargetState> & rgTargets, bool fConfigured);
    static void    DetectConsoleHostsConfigured  (vector<SNerdFontTargetState> & rgTargets);
    static HRESULT PromptTargetSelection         (CConsole & console, ENerdFontOperation op, vector<SNerdFontTargetState> & rgTargets, FnApply fnApply = nullptr, int iStatusColumn = 0);
    static HRESULT PromptAndApplyTargets         (CConsole & console, ENerdFontOperation op, vector<SNerdFontTargetState> & rgTargets, int iStatusColumn);
    static int     ComputeInstallStatusColumn    (LPCWSTR pszReleaseTag, int cInstallSteps);
    static HRESULT ResolveLatestReleaseTag       (CConsole & console, wstring & strReleaseTag);
    static HRESULT DownloadAndInstallFonts       (CConsole & console, const wstring & strReleaseTag, int iStatusColumn, int cInstallSteps, EFontInstallOutcome & outcome);
    static HRESULT ApplyTargetConfig             (CConsole & console, const SNerdFontTargetState & target, ENerdFontOperation op);
    static HRESULT ConfigureConsoleHostSettings  (CConsole & console, const SNerdFontTargetState & target, ENerdFontOperation op);
    static bool    GetAlternateConsoleProfileKey (const SNerdFontTargetState & target, wstring & strAltProfileKey);
    static bool    IsConsoleHostConfigured       (const SNerdFontTargetState & target);
    static HRESULT ConfigureConsoleHostKeys      (const SNerdFontTargetState & target, ENerdFontOperation op);
    static bool    AnyTargetConfigured           (const vector<SNerdFontTargetState> & rgTargets);
    static bool    AnyTargetApplied              (const vector<SNerdFontTargetState> & rgTargets);
    static bool    AnyConfiguredTargetUnselected (const vector<SNerdFontTargetState> & rgTargets);
    static int     ComputeUninstallStatusColumn  (const vector<SNerdFontTargetState> & rgTargets);
    static HRESULT RemoveFontFilesIfRequested    (CConsole & console, const vector<SNerdFontTargetState> & rgTargets, bool fAnyConfigured, bool fAnyFontFilesInstalled);
    static void    PrintInstallStepPrefix        (CConsole & console, int iStep, int cSteps, LPCWSTR pszText);
    static void    PrintInstallStepStatus        (CConsole & console, int iStatusColumn, LPCWSTR pszStatus, bool fSuccess = true);
    static void    PrintAppliedTargetRow         (CConsole & console, const SNerdFontTargetState & target, bool fIsLocked, bool fIsSelected, int iStatusColumn);
    static HRESULT PromptReopenForTargets        (CConsole & console, const vector<SNerdFontTargetState> & rgTargets, LPCWSTR pszInstallCwd);
    static HRESULT RunFontInstallElevated        (CConsole & console, ENerdFontOperation op, LPCWSTR pszFontDir);
    static HRESULT RunFontInstallInProcess       (CConsole & console, ENerdFontOperation op, LPCWSTR pszFontDir);
    static bool    TryMatchModeSwitch            (LPCWSTR pszArg, ENerdFontOperation & op);
    static bool    TryParseElevatedInvocation    (int argc, WCHAR ** argv, ENerdFontOperation & op, wstring & strFontDir);

    static CNerdFontSystemAccessReal s_sysAccess;
};





