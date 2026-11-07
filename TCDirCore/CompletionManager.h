#pragma once

#include "ProfilePathResolver.h"





class CCommandLine;
class CConsole;
class CProfileFileManager;





////////////////////////////////////////////////////////////////////////////////
//
//  ECompletionUpdate
//
//  How UpdateLines changes the completion block in a profile.
//
////////////////////////////////////////////////////////////////////////////////

enum class ECompletionUpdate
{
    Refresh,        // Regenerate the block only when the profile has one
    Add,            // Add the block, or regenerate it
    Remove          // Remove the block
};





////////////////////////////////////////////////////////////////////////////////
//
//  CCompletionManager
//
//  Runs --Enable-Completion and --Disable-Completion, and regenerates the
//  completion block in a profile when its alias block changes.
//
////////////////////////////////////////////////////////////////////////////////

class CCompletionManager
{
public:
    static HRESULT Run                (const CCommandLine & cmdline, CConsole & console);
    static HRESULT Enable             (CConsole & console, bool fWhatIf);
    static HRESULT Disable            (CConsole & console, bool fWhatIf);
    static void    UpdateLines        (vector<wstring> & rgLines, ECompletionUpdate eUpdate, const wstring & strInvocation, const wstring & strVersion);
    static bool    HasCompletionBlock (const vector<wstring> & rgLines);

private:
    static HRESULT ResolveLocations   (CConsole & console, vector<SProfileLocation> & rgLocations);
    static size_t  ChooseTarget       (CProfileFileManager & fileMgr, const vector<SProfileLocation> & rgLocations);
    static HRESULT ReadLines          (CProfileFileManager & fileMgr, const wstring & strPath, vector<wstring> & rgLines, bool & fHasBom);
    static HRESULT WriteLines         (CProfileFileManager & fileMgr, const wstring & strPath, const vector<wstring> & rgLines, bool fHasBom);
    static void    PrintBlock         (CConsole & console, const vector<wstring> & rgLines);
};
