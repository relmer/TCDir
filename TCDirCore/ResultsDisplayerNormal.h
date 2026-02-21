#pragma once

#include "ResultsDisplayerWithHeaderAndFooter.h"






class CResultsDisplayerNormal : public CResultsDisplayerWithHeaderAndFooter
{
public:
    CResultsDisplayerNormal (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr, bool fIconsActive);

    void DisplayFileResults (const CDirectoryInfo & di) override;

    static wstring   FormatAbbreviatedSize           (ULONGLONG cbSize);

protected:
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
