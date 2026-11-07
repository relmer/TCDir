#include "pch.h"

#include "CompletionManager.h"

#include "AliasManager.h"
#include "CommandLine.h"
#include "CompletionScript.h"
#include "Console.h"
#include "ProfileFileManager.h"
#include "Version.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CCompletionManager::Run
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CCompletionManager::Run (const CCommandLine & cmdline, CConsole & console)
{
    HRESULT hr = S_OK;



    if (cmdline.m_fEnableCompletion)
    {
        hr = Enable (console, cmdline.m_fWhatIf);
        CHR (hr);
    }
    else if (cmdline.m_fDisableCompletion)
    {
        hr = Disable (console, cmdline.m_fWhatIf);
        CHR (hr);
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompletionManager::Enable
//
//  Writes the completion block to the profile that contains the alias
//  block, or to the current user's all-hosts profile when no profile has
//  one.  The block registers tcdir, tcdir.exe and every alias in the
//  profile.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CCompletionManager::Enable (CConsole & console, bool fWhatIf)
{
    HRESULT                    hr            = S_OK;
    CProfileFileManager        fileMgr;
    vector<SProfileLocation>   rgLocations;
    vector<wstring>            rgLines;
    bool                       fHasBom       = false;
    wstring                    strInvocation;
    wstring                    strPath;
    SAliasBlock                completion;



    hr = ResolveLocations (console, rgLocations);
    CHR (hr);

    hr = CAliasManager::ResolveTcDirInvocation (strInvocation);
    CHR (hr);

    strPath = rgLocations[ChooseTarget (fileMgr, rgLocations)].strResolvedPath;

    hr = ReadLines (fileMgr, strPath, rgLines, fHasBom);
    CHR (hr);

    UpdateLines (rgLines, ECompletionUpdate::Add, strInvocation, VERSION_WSTRING);

    if (fWhatIf)
    {
        fileMgr.FindCompletionBlock (rgLines, completion);

        console.Printf (CConfig::Information, L"\n  Whatif: The following completion block would be written to:\n  %s\n\n", strPath.c_str());
        PrintBlock (console, vector<wstring> (rgLines.begin() + completion.iStartLine, rgLines.begin() + completion.iEndLine + 1));
        console.Printf (CConfig::Error, L"\n  Whatif: No changes were made.\n");
        BAIL_OUT_IF (true, S_OK);
    }

    hr = WriteLines (fileMgr, strPath, rgLines, fHasBom);
    CHR (hr);

    console.Printf (CConfig::Information, L"\n  Tab completion written to: %s\n\n", strPath.c_str());
    console.Puts   (CConfig::Information, L"  To activate, open a new PowerShell window or paste this command:");
    console.Printf (CConfig::InformationHighlight, L"    . \"%s\"\n", strPath.c_str());



Error:
    console.Flush();
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompletionManager::Disable
//
//  Removes the completion block from every profile that has one.  Alias
//  blocks are left as they are.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CCompletionManager::Disable (CConsole & console, bool fWhatIf)
{
    HRESULT                    hr       = S_OK;
    HRESULT                    hrRead   = S_OK;
    CProfileFileManager        fileMgr;
    vector<SProfileLocation>   rgLocations;
    vector<wstring>            rgLines;
    bool                       fHasBom  = false;
    bool                       fRemoved = false;



    hr = ResolveLocations (console, rgLocations);
    CHR (hr);

    for (const SProfileLocation & loc : rgLocations)
    {
        if (!loc.fExists)
        {
            continue;
        }

        hrRead = ReadLines (fileMgr, loc.strResolvedPath, rgLines, fHasBom);

        if (FAILED (hrRead) || !HasCompletionBlock (rgLines))
        {
            continue;
        }

        fRemoved = true;

        if (fWhatIf)
        {
            console.Printf (CConfig::Information, L"\n  Whatif: Tab completion would be removed from:\n  %s\n", loc.strResolvedPath.c_str());
            continue;
        }

        UpdateLines (rgLines, ECompletionUpdate::Remove, L"", L"");

        hr = WriteLines (fileMgr, loc.strResolvedPath, rgLines, fHasBom);
        CHR (hr);

        console.Printf (CConfig::Information, L"\n  Tab completion removed from: %s\n", loc.strResolvedPath.c_str());
    }

    if (!fRemoved)
    {
        console.Printf (CConfig::Information, L"\n  Tab completion isn't enabled in any profile.\n");
    }
    else if (fWhatIf)
    {
        console.Printf (CConfig::Error, L"\n  Whatif: No changes were made.\n");
    }



Error:
    console.Flush();
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompletionManager::UpdateLines
//
//  Changes the completion block in a profile's lines.  The registrations
//  come from the alias block in the same lines, so they match the aliases
//  the profile defines.
//
////////////////////////////////////////////////////////////////////////////////

void CCompletionManager::UpdateLines (
    vector<wstring>     & rgLines,
    ECompletionUpdate     eUpdate,
    const wstring       & strInvocation,
    const wstring       & strVersion)
{
    CProfileFileManager          fileMgr;
    SAliasBlock                  aliases;
    SAliasBlock                  completion;
    vector<SCompletionCommand>   commands;
    vector<wstring>              rgBlock;



    fileMgr.FindCompletionBlock (rgLines, completion);

    if (eUpdate == ECompletionUpdate::Remove)
    {
        if (completion.fFound)
        {
            fileMgr.RemoveAliasBlock (rgLines, completion);
        }

        return;
    }

    if (eUpdate == ECompletionUpdate::Refresh && !completion.fFound)
    {
        return;
    }

    fileMgr.FindAliasBlock (rgLines, aliases);
    CCompletionScript::CommandsFromAliasBlock (aliases, commands);
    CCompletionScript::GenerateBlock (strVersion, strInvocation, commands, rgBlock);

    if (completion.fFound)
    {
        fileMgr.ReplaceAliasBlock (rgLines, completion, rgBlock);
    }
    else
    {
        fileMgr.AppendAliasBlock (rgLines, rgBlock);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompletionManager::HasCompletionBlock
//
////////////////////////////////////////////////////////////////////////////////

bool CCompletionManager::HasCompletionBlock (const vector<wstring> & rgLines)
{
    CProfileFileManager   fileMgr;
    SAliasBlock           completion;



    fileMgr.FindCompletionBlock (rgLines, completion);

    return completion.fFound;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompletionManager::ResolveLocations
//
//  The profile paths of the PowerShell edition that started tcdir.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CCompletionManager::ResolveLocations (CConsole & console, vector<SProfileLocation> & rgLocations)
{
    HRESULT                hr       = S_OK;
    CProfilePathResolver   resolver;
    EPowerShellVersion     eVersion = EPowerShellVersion::Unknown;



    hr = resolver.DetectPowerShellVersion (eVersion);
    CHR (hr);

    CBRFEx (eVersion != EPowerShellVersion::Unknown, E_FAIL,
            console.Printf (CConfig::Error, L"  This command must be run from PowerShell (pwsh.exe or powershell.exe).\n"));

    hr = resolver.ResolveProfilePaths (eVersion, rgLocations);
    CHR (hr);

    CBRA (!rgLocations.empty());



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompletionManager::ChooseTarget
//
//  The first existing profile with an alias block, or the current user's
//  all-hosts profile.
//
////////////////////////////////////////////////////////////////////////////////

size_t CCompletionManager::ChooseTarget (CProfileFileManager & fileMgr, const vector<SProfileLocation> & rgLocations)
{
    size_t            iTarget  = 0;
    vector<wstring>   rgLines;
    bool              fHasBom  = false;
    SAliasBlock       aliases;
    HRESULT           hr       = S_OK;



    for (size_t i = 0; i < rgLocations.size(); ++i)
    {
        if (rgLocations[i].eScope == EProfileScope::CurrentUserAllHosts)
        {
            iTarget = i;
        }
    }

    for (size_t i = 0; i < rgLocations.size(); ++i)
    {
        if (!rgLocations[i].fExists)
        {
            continue;
        }

        hr = fileMgr.ReadProfileFile (rgLocations[i].strResolvedPath, rgLines, fHasBom);

        if (SUCCEEDED (hr))
        {
            fileMgr.FindAliasBlock (rgLines, aliases);

            if (aliases.fFound)
            {
                iTarget = i;
                break;
            }
        }
    }

    return iTarget;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompletionManager::ReadLines
//
//  A missing profile reads as empty.  A non-empty profile that reads as
//  empty is an error, because writing it back would destroy the file.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CCompletionManager::ReadLines (CProfileFileManager & fileMgr, const wstring & strPath, vector<wstring> & rgLines, bool & fHasBom)
{
    HRESULT      hr       = S_OK;
    uintmax_t    cbOnDisk = 0;
    error_code   ec;



    rgLines.clear();
    fHasBom = false;

    BAIL_OUT_IF (!filesystem::exists (strPath, ec), S_OK);

    hr = fileMgr.ReadProfileFile (strPath, rgLines, fHasBom);
    CHR (hr);

    cbOnDisk = filesystem::file_size (strPath, ec);
    CBREx (ec || !rgLines.empty() || cbOnDisk == 0, HRESULT_FROM_WIN32 (ERROR_READ_FAULT));



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompletionManager::WriteLines
//
//  Backs up an existing profile, then writes it.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CCompletionManager::WriteLines (CProfileFileManager & fileMgr, const wstring & strPath, const vector<wstring> & rgLines, bool fHasBom)
{
    HRESULT      hr = S_OK;
    error_code   ec;



    if (filesystem::exists (strPath, ec))
    {
        hr = fileMgr.CreateBackup (strPath);
        CHR (hr);
    }

    hr = fileMgr.WriteProfileFile (strPath, rgLines, fHasBom);
    CHR (hr);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCompletionManager::PrintBlock
//
////////////////////////////////////////////////////////////////////////////////

void CCompletionManager::PrintBlock (CConsole & console, const vector<wstring> & rgLines)
{
    for (const wstring & strLine : rgLines)
    {
        console.Printf (CConfig::Default, L"  %s\n", strLine.c_str());
    }
}





