#pragma once

#include "ResultsDisplayerWithHeaderAndFooter.h"





// Cloud sync status (computed from file attributes at display time)
enum class ECloudStatus
{
    CS_NONE,        // Not a cloud file
    CS_CLOUD_ONLY,  // Placeholder, not locally available
    CS_LOCAL,       // Available locally, can be dehydrated
    CS_PINNED       // Pinned, always available locally
};





class CResultsDisplayerNormal : public CResultsDisplayerWithHeaderAndFooter
{
public:
    CResultsDisplayerNormal (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr);

    void DisplayFileResults (const CDirectoryInfo & di) override;

protected:
    static bool         IsUnderSyncRoot (LPCWSTR pszPath);
    static ECloudStatus GetCloudStatus  (const WIN32_FIND_DATA & wfd, bool fInSyncRoot);

    const FILETIME & GetTimeFieldForDisplay          (const WIN32_FIND_DATA & wfd) const;
    HRESULT          DisplayResultsNormalDateAndTime (const FILETIME & ftLastWriteTime);
    void             DisplayResultsNormalAttributes  (DWORD dwFileAttributes);
    void             DisplayResultsNormalFileSize    (const WIN32_FIND_DATA & fileInfo, size_t cchStringLengthOfMaxFileSize);
    void             DisplayCloudStatusSymbol        (ECloudStatus status);
    void             DisplayRawAttributes            (const WIN32_FIND_DATA & wfd);
    void             DisplayFileOwner                (const wstring & owner, size_t cchColumnWidth);
    static wstring   GetFileOwner                    (LPCWSTR pszFilePath);
    void             GetFileOwners                   (const CDirectoryInfo & di, vector<wstring> & owners, size_t & cchMaxOwnerLength);
    void             DisplayFileStreams              (const FileInfo & fileEntry, size_t cchStringLengthOfMaxFileSize, size_t cchOwnerWidth);
};
