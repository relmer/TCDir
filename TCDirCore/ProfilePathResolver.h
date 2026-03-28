#pragma once





////////////////////////////////////////////////////////////////////////////////
//
//  EPowerShellVersion
//
//  Detected PowerShell version of the calling shell.
//
////////////////////////////////////////////////////////////////////////////////

enum class EPowerShellVersion
{
    PowerShell,         // PowerShell 7+ (pwsh.exe). Profile dir: "PowerShell\"
    WindowsPowerShell,  // Windows PowerShell 5.1 (powershell.exe). Profile dir: "WindowsPowerShell\"
    Unknown             // Parent process is neither pwsh.exe nor powershell.exe
};





////////////////////////////////////////////////////////////////////////////////
//
//  EProfileScope
//
//  One of the four standard PowerShell profile scopes.
//
////////////////////////////////////////////////////////////////////////////////

enum class EProfileScope
{
    CurrentUserCurrentHost,
    CurrentUserAllHosts,
    AllUsersCurrentHost,
    AllUsersAllHosts,
    SessionOnly,             // Sentinel: output to console, no file write

    __COUNT
};





////////////////////////////////////////////////////////////////////////////////
//
//  SProfileLocation
//
//  A resolved profile file path with metadata.
//
////////////////////////////////////////////////////////////////////////////////

struct SProfileLocation
{
    EProfileScope eScope          = EProfileScope::CurrentUserAllHosts;
    wstring       strVariableName;     // Display name (e.g., "$PROFILE.CurrentUserAllHosts")
    wstring       strResolvedPath;     // Full filesystem path
    bool          fExists         = false;
    bool          fRequiresAdmin  = false;
    bool          fHasAliasBlock  = false;
};





////////////////////////////////////////////////////////////////////////////////
//
//  CProfilePathResolver
//
//  Detects the calling PowerShell version via parent process inspection,
//  and resolves the four profile paths for that version.
//
////////////////////////////////////////////////////////////////////////////////

class CProfilePathResolver
{
public:
    HRESULT DetectPowerShellVersion     (EPowerShellVersion & eVersion);
    HRESULT ResolveProfilePaths         (EPowerShellVersion eVersion, vector<SProfileLocation> & rgLocations);
    HRESULT IsRunningAsAdmin            (bool & fIsAdmin);

    //
    // Pure-logic helpers — no system calls, directly unit-testable
    //

    static EPowerShellVersion MapImageNameToVersion   (const wstring & strImageName);
    static void               BuildProfileLocations   (const wstring & strDocuments, const wstring & strProgramData,
                                                       EPowerShellVersion eVersion, vector<SProfileLocation> & rgLocations);

private:
    HRESULT GetParentProcessImageName    (wstring & strImageName);

    static HRESULT FindParentPid         (DWORD & dwParentPid);
    static HRESULT QueryProcessImageName (DWORD dwPid, wstring & strImageName);
};
