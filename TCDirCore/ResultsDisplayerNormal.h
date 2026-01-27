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

    HRESULT DisplayResultsNormalDateAndTime (const FILETIME & ftLastWriteTime);
    void    DisplayResultsNormalAttributes  (DWORD dwFileAttributes);
    void    DisplayResultsNormalFileSize    (const WIN32_FIND_DATA & fileInfo, size_t cchStringLengthOfMaxFileSize);
    void    DisplayCloudStatusSymbol        (ECloudStatus status);
    void    DisplayRawAttributes            (const WIN32_FIND_DATA & wfd);
};
