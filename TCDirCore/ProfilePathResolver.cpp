#include "pch.h"
#include "ProfilePathResolver.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CProfilePathResolver::FindParentPid
//
//  Uses a ToolHelp snapshot to find the parent PID of the current process.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CProfilePathResolver::FindParentPid (DWORD & dwParentPid)
{
    HRESULT         hr           = S_OK;
    HANDLE          hSnapshot    = INVALID_HANDLE_VALUE;
    DWORD           dwCurrentPid = GetCurrentProcessId();
    BOOL            fSuccess     = FALSE;
    PROCESSENTRY32W pe           = { .dwSize = sizeof (pe) };
    bool            fFound       = false;



    hSnapshot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
    CWRA (hSnapshot != INVALID_HANDLE_VALUE);

    for (fSuccess = Process32FirstW (hSnapshot, &pe); 
         fSuccess; 
         fSuccess = Process32NextW (hSnapshot, &pe))
    {
        if (pe.th32ProcessID == dwCurrentPid)
        {
            dwParentPid = pe.th32ParentProcessID;
            fFound      = true;
            break;
        }
    }

    CBRAEx (fFound, E_FAIL);


    
Error:
    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        CloseHandle (hSnapshot);
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CProfilePathResolver::QueryProcessImageName
//
//  Opens a process by PID and queries its full image path, returning just
//  the filename (e.g., "pwsh.exe").
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CProfilePathResolver::QueryProcessImageName (DWORD dwPid, wstring & strImageName)
{
    HRESULT hr               = S_OK;
    HANDLE  hProc            = nullptr;
    WCHAR   szPath[MAX_PATH] = {};
    BOOL    fSuccess         = FALSE;
    DWORD   cchPath          = MAX_PATH;



    hProc = OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwPid);
    CWRA (hProc != nullptr);

    fSuccess = QueryFullProcessImageNameW (hProc, 0, szPath, &cchPath);
    CWRA (fSuccess);

    strImageName = filesystem::path (szPath).filename().wstring();

Error:
    if (hProc != nullptr)
    {
        CloseHandle (hProc);
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CProfilePathResolver::GetParentProcessImageName
//
//  Finds the parent process and returns its image filename.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CProfilePathResolver::GetParentProcessImageName (wstring & strImageName)
{
    HRESULT hr          = S_OK;
    DWORD   dwParentPid = 0;



    hr = FindParentPid (dwParentPid);
    CHR (hr);

    hr = QueryProcessImageName (dwParentPid, strImageName);
    CHR (hr);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CProfilePathResolver::MapImageNameToVersion
//
//  Pure-logic mapping from a process image filename to PS version enum.
//  No system calls — directly unit-testable.
//
////////////////////////////////////////////////////////////////////////////////

EPowerShellVersion CProfilePathResolver::MapImageNameToVersion (const wstring & strImageName)
{
    if (_wcsicmp (strImageName.c_str(), L"pwsh.exe") == 0)
    {
        return EPowerShellVersion::PowerShell;
    }

    if (_wcsicmp (strImageName.c_str(), L"powershell.exe") == 0)
    {
        return EPowerShellVersion::WindowsPowerShell;
    }

    return EPowerShellVersion::Unknown;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CProfilePathResolver::DetectPowerShellVersion
//
//  Inspects the parent process image name to determine PS version.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CProfilePathResolver::DetectPowerShellVersion (EPowerShellVersion & eVersion)
{
    HRESULT hr           = S_OK;
    wstring strImageName;



    eVersion = EPowerShellVersion::Unknown;

    hr = GetParentProcessImageName (strImageName);
    CHR (hr);

    eVersion = MapImageNameToVersion (strImageName);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CProfilePathResolver::BuildProfileLocations
//
//  Pure-logic path construction — no system calls, directly unit-testable.
//  Builds the four profile SProfileLocation structs from base directory paths.
//  Does NOT check filesystem::exists (caller does that).
//
////////////////////////////////////////////////////////////////////////////////

void CProfilePathResolver::BuildProfileLocations (
    const wstring            & strDocuments, 
    const wstring            & strProgramData,
    EPowerShellVersion         eVersion, 
    vector<SProfileLocation> & rgLocations)
{
    struct SProfileDef
    {
        EProfileScope eScope;
        LPCWSTR       pszVarName;
        const wstring & strBaseDir;
        LPCWSTR       pszFilename;
        bool          fRequiresAdmin;
    };

    SProfileDef defs[] =
    {
        { EProfileScope::CurrentUserCurrentHost, L"$PROFILE.CurrentUserCurrentHost", strDocuments,   L"Microsoft.PowerShell_profile.ps1", false },
        { EProfileScope::CurrentUserAllHosts,    L"$PROFILE.CurrentUserAllHosts",    strDocuments,   L"profile.ps1",                      false },
        { EProfileScope::AllUsersCurrentHost,    L"$PROFILE.AllUsersCurrentHost",    strProgramData, L"Microsoft.PowerShell_profile.ps1", true  },
        { EProfileScope::AllUsersAllHosts,       L"$PROFILE.AllUsersAllHosts",       strProgramData, L"profile.ps1",                      true  },
    };

    LPCWSTR pszPsDir = (eVersion == EPowerShellVersion::PowerShell)
                      ? L"PowerShell"
                      : L"WindowsPowerShell";
                  


    rgLocations.clear();

    for (const auto & def : defs)
    {
        filesystem::path profilePath = filesystem::path (def.strBaseDir) / pszPsDir / def.pszFilename;
        SProfileLocation loc;

        loc.eScope          = def.eScope;
        loc.strVariableName = def.pszVarName;
        loc.strResolvedPath = profilePath.wstring();
        loc.fExists         = false;
        loc.fRequiresAdmin  = def.fRequiresAdmin;
        loc.fHasAliasBlock  = false;

        rgLocations.push_back (std::move (loc));
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CProfilePathResolver::ResolveProfilePaths
//
//  Resolves the four profile paths for the given PS version using shell APIs.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CProfilePathResolver::ResolveProfilePaths (EPowerShellVersion eVersion, vector<SProfileLocation> & rgLocations)
{
    HRESULT hr             = S_OK;
    PWSTR   pszDocuments   = nullptr;
    PWSTR   pszProgramData = nullptr;



    //
    // Resolve Documents folder (handles OneDrive KFM redirection)
    //

    hr = SHGetKnownFolderPath (FOLDERID_Documents, 0, nullptr, &pszDocuments);
    CHR (hr);

    //
    // Resolve ProgramData folder
    //

    hr = SHGetKnownFolderPath (FOLDERID_ProgramData, 0, nullptr, &pszProgramData);
    CHR (hr);

    //
    // Build profile locations using the pure-logic helper
    //

    BuildProfileLocations (pszDocuments, pszProgramData, eVersion, rgLocations);

    //
    // Check which files actually exist on disk
    //

    for (auto & loc : rgLocations)
    {
        loc.fExists = filesystem::exists (loc.strResolvedPath);
    }

Error:
    if (pszDocuments != nullptr)
    {
        CoTaskMemFree (pszDocuments);
    }

    if (pszProgramData != nullptr)
    {
        CoTaskMemFree (pszProgramData);
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CProfilePathResolver::IsRunningAsAdmin
//
//  Checks if the current process is running with elevated privileges.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CProfilePathResolver::IsRunningAsAdmin (bool & fIsAdmin)
{
    HRESULT         hr        = S_OK;
    BOOL            fSuccess  = FALSE;
    HANDLE          hToken    = nullptr;
    TOKEN_ELEVATION elevation = {};
    DWORD           cbSize    = sizeof (elevation);



    fIsAdmin = false;
    
    fSuccess = OpenProcessToken (GetCurrentProcess(), TOKEN_QUERY, &hToken);
    CWRA (fSuccess);

    fSuccess = GetTokenInformation (hToken, TokenElevation, &elevation, sizeof (elevation), &cbSize);
    CWRA (fSuccess);

    fIsAdmin = (elevation.TokenIsElevated != 0);

Error:
    if (hToken != nullptr)
    {
        CloseHandle (hToken);
    }

    return hr;
}
