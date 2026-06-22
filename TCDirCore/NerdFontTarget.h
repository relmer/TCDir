#pragma once

//
// The set of terminal targets the Nerd Font installer can configure, and the
// per-target state tracked while detecting, selecting, and applying.
//





////////////////////////////////////////////////////////////////////////////////
//
//  ENerdFontTarget
//
////////////////////////////////////////////////////////////////////////////////

enum class ENerdFontTarget
{
    WindowsTerminal,
    Pwsh,
    WindowsPowerShell,
    Cmd
};





////////////////////////////////////////////////////////////////////////////////
//
//  SNerdFontTargetState
//
//  One terminal target plus the state gathered as the installer detects whether
//  it is already configured, the user selects it, and the change is applied.
//
////////////////////////////////////////////////////////////////////////////////

struct SNerdFontTargetState
{
    ENerdFontTarget kind                 = ENerdFontTarget::WindowsTerminal;
    LPCWSTR         pszDisplayName       = nullptr;
    LPCWSTR         pszConsoleProfileKey = nullptr;   // nullptr for Windows Terminal
    bool            fConfigured          = false;
    bool            fSelected            = false;
    bool            fLocked              = false;
    bool            fApplyAttempted      = false;
    bool            fApplySucceeded      = false;
    wstring         strApplyStatus;
};





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontTargetCatalog
//
////////////////////////////////////////////////////////////////////////////////

class CNerdFontTargetCatalog
{
public:
    static void Build (vector<SNerdFontTargetState> & rgTargets);
};





