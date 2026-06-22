#include "pch.h"

#include "NerdFontTarget.h"




//
// Console host profile keys under HKCU\Console.  The profile name is the
// executable's full path with backslashes replaced by underscores.
//

static constexpr LPCWSTR s_kpszPwshProfileKey  = L"Console\\C:_Program Files_PowerShell_7_pwsh.exe";
static constexpr LPCWSTR s_kpszWinPsProfileKey = L"Console\\%SystemRoot%_System32_WindowsPowerShell_v1.0_powershell.exe";
static constexpr LPCWSTR s_kpszCmdProfileKey   = L"Console\\%SystemRoot%_System32_cmd.exe";





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontTargetCatalog::Build
//
//  Populates the catalog of terminal targets.  Every target starts selected;
//  detection later marks which are already configured.
//
////////////////////////////////////////////////////////////////////////////////

void CNerdFontTargetCatalog::Build (vector<SNerdFontTargetState> & rgTargets)
{
    rgTargets.clear();

    rgTargets.push_back ({ .kind = ENerdFontTarget::WindowsTerminal,
                           .pszDisplayName = L"Windows Terminal",
                           .fSelected = true });

    rgTargets.push_back ({ .kind = ENerdFontTarget::Pwsh,
                           .pszDisplayName = L"PowerShell (pwsh.exe)",
                           .pszConsoleProfileKey = s_kpszPwshProfileKey,
                           .fSelected = true });

    rgTargets.push_back ({ .kind = ENerdFontTarget::WindowsPowerShell,
                           .pszDisplayName = L"Windows PowerShell (powershell.exe)",
                           .pszConsoleProfileKey = s_kpszWinPsProfileKey,
                           .fSelected = true });

    rgTargets.push_back ({ .kind = ENerdFontTarget::Cmd,
                           .pszDisplayName = L"Command Prompt (cmd.exe)",
                           .pszConsoleProfileKey = s_kpszCmdProfileKey,
                           .fSelected = true });
}





