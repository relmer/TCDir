#include "pch.h"
#include "DriveInfo.h"





CDriveInfo::CDriveInfo (const filesystem::path & dirPath) :
    m_szVolumeName     { 0 },
    m_szFileSystemName { 0 },
    m_nVolumeType      (DRIVE_UNKNOWN),
    m_fUncPath         (false)
{
    InitializeVolumeInfo (dirPath);
    InitializeUncInfo();
}





void CDriveInfo::InitializeVolumeInfo (const filesystem::path & dirPath)
{
    HRESULT hr       = S_OK;
    bool    fSuccess = false;



    // Ensure root path ends with a slash
    ASSERT (dirPath.has_root_path());
    m_rootPath = dirPath.root_path() / "";


    if (dirPath.has_root_name() &&
        dirPath.root_name().wstring().length() >= 2 &&
        dirPath.root_name().wstring()[1] == L':')
    {
        m_nVolumeType = GetDriveType (m_rootPath.c_str());
    }
    else
    {
        // If there's no drive letter, then this must be a UNC path
        m_fUncPath = true;
        m_uncPath  = dirPath;

        m_nVolumeType = DRIVE_REMOTE;
    }

    fSuccess = GetVolumeInformation (m_rootPath.c_str(),
                                     m_szVolumeName, ARRAYSIZE (m_szVolumeName),
                                     NULL,
                                     NULL,
                                     NULL,
                                     m_szFileSystemName, ARRAYSIZE (m_szFileSystemName));
    CWR (fSuccess);  // Worth logging, but expected to fail on bad paths, so don't assert.

Error:
    return;
}





void CDriveInfo::InitializeUncInfo()
{
    HRESULT hr            = S_OK;
    DWORD   dwResult      = 0;
    DWORD   cchRemoteName = MAX_PATH;



    BAIL_OUT_IF (!m_fUncPath,                   S_OK);
    BAIL_OUT_IF (m_nVolumeType != DRIVE_REMOTE, S_OK);

    //
    // If this is a mapped drive, get the remote name that it's mapped to
    //

    m_remoteName.resize (cchRemoteName);

    dwResult = WNetGetConnection (m_rootPath.root_name().c_str(), &m_remoteName[0], &cchRemoteName);
    if (dwResult == ERROR_MORE_DATA)
    {
        m_remoteName.resize (cchRemoteName);
        dwResult = WNetGetConnection (m_rootPath.root_name().c_str(), &m_remoteName[0], &cchRemoteName);
    }
    CBR (dwResult == NOERROR);  // Expected to fail on bad paths, so log but don't assert.

    // Trim the string to the actual length (remove null terminator and extra space)
    m_remoteName.resize (wcslen (m_remoteName.c_str()));

Error:
    if (FAILED (hr))
    {
        m_remoteName.clear();
    }

    return;
}

