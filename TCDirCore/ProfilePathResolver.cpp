#include "pch.h"
#include "ProfilePathResolver.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CProfilePathResolver::GetParentProcessImageName
//
//  Uses ToolHelp snapshot to find parent PID, then queries its image name.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CProfilePathResolver::GetParentProcessImageName (wstring & strImageName)
{
    HRESULT hr           = S_OK;
    HANDLE  hSnapshot    = INVALID_HANDLE_VALUE;
    HANDLE  hParent      = nullptr;
    DWORD   dwCurrentPid = GetCurrentProcessId();
    DWORD   dwParentPid  = 0;
    bool    fFoundParent = false;



    //
    // Take a snapshot of all processes and find our parent PID
    //

    hSnapshot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);
    CBRAEx (hSnapshot != INVALID_HANDLE_VALUE, HRESULT_FROM_WIN32 (GetLastError()));

    {
        PROCESSENTRY32W pe = {};

        pe.dwSize = sizeof (pe);

        BOOL fOk = Process32FirstW (hSnapshot, &pe);

        while (fOk)
        {
            if (pe.th32ProcessID == dwCurrentPid)
            {
                dwParentPid  = pe.th32ParentProcessID;
                fFoundParent = true;
                break;
            }

            pe.dwSize = sizeof (pe);
            fOk       = Process32NextW (hSnapshot, &pe);
        }
    }

    CBRAEx (fFoundParent, E_FAIL);

    //
    // Open the parent process and query its image name
    //

    hParent = OpenProcess (PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwParentPid);
    CBRAEx (hParent != nullptr, HRESULT_FROM_WIN32 (GetLastError()));

    {
        WCHAR szPath[MAX_PATH] = {};
        DWORD cchPath          = MAX_PATH;

        BOOL fOk = QueryFullProcessImageNameW (hParent, 0, szPath, &cchPath);
        CBRAEx (fOk, HRESULT_FROM_WIN32 (GetLastError()));

        //
        // Extract just the filename
        //

        filesystem::path fullPath (szPath);

        strImageName = fullPath.filename().wstring();
    }

Error:
    if (hParent != nullptr)
    {
        CloseHandle (hParent);
    }

    if (hSnapshot != INVALID_HANDLE_VALUE)
    {
        CloseHandle (hSnapshot);
    }

    return hr;
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
    HRESULT hr = S_OK;
    wstring strImageName;



    eVersion = EPowerShellVersion::Unknown;

    hr = GetParentProcessImageName (strImageName);
    CHR (hr);

    if (_wcsicmp (strImageName.c_str(), L"pwsh.exe") == 0)
    {
        eVersion = EPowerShellVersion::PS7Plus;
    }
    else if (_wcsicmp (strImageName.c_str(), L"powershell.exe") == 0)
    {
        eVersion = EPowerShellVersion::PS51;
    }
    else
    {
        eVersion = EPowerShellVersion::Unknown;
    }

Error:
    return hr;
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



    rgLocations.clear();

    //
    // Determine the PS subdirectory name
    //

    LPCWSTR pszPsDir = (eVersion == EPowerShellVersion::PS7Plus)
                      ? L"PowerShell"
                      : L"WindowsPowerShell";

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
    // Build the four profile locations
    //

    {
        struct SProfileDef
        {
            EProfileScope eScope;
            LPCWSTR       pszVarName;
            LPCWSTR       pszBaseDir;
            LPCWSTR       pszFilename;
            bool          fRequiresAdmin;
        };

        SProfileDef defs[] =
        {
            { EProfileScope::CurrentUserCurrentHost, L"$PROFILE.CurrentUserCurrentHost", pszDocuments,   L"Microsoft.PowerShell_profile.ps1", false },
            { EProfileScope::CurrentUserAllHosts,    L"$PROFILE.CurrentUserAllHosts",    pszDocuments,   L"profile.ps1",                      false },
            { EProfileScope::AllUsersCurrentHost,    L"$PROFILE.AllUsersCurrentHost",    pszProgramData, L"Microsoft.PowerShell_profile.ps1", true  },
            { EProfileScope::AllUsersAllHosts,       L"$PROFILE.AllUsersAllHosts",       pszProgramData, L"profile.ps1",                      true  },
        };

        for (const auto & def : defs)
        {
            filesystem::path profilePath = filesystem::path (def.pszBaseDir) / pszPsDir / def.pszFilename;

            SProfileLocation loc;

            loc.eScope          = def.eScope;
            loc.strVariableName = def.pszVarName;
            loc.strResolvedPath = profilePath.wstring();
            loc.fExists         = filesystem::exists (profilePath);
            loc.fRequiresAdmin  = def.fRequiresAdmin;
            loc.fHasAliasBlock  = false;

            rgLocations.push_back (std::move (loc));
        }
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
    HANDLE          hToken    = nullptr;
    TOKEN_ELEVATION elevation = {};
    DWORD           cbSize    = sizeof (elevation);



    fIsAdmin = false;

    BOOL fOk = OpenProcessToken (GetCurrentProcess(), TOKEN_QUERY, &hToken);
    CBRAEx (fOk, HRESULT_FROM_WIN32 (GetLastError()));

    fOk = GetTokenInformation (hToken, TokenElevation, &elevation, sizeof (elevation), &cbSize);
    CBRAEx (fOk, HRESULT_FROM_WIN32 (GetLastError()));

    fIsAdmin = (elevation.TokenIsElevated != 0);

Error:
    if (hToken != nullptr)
    {
        CloseHandle (hToken);
    }

    return hr;
}
