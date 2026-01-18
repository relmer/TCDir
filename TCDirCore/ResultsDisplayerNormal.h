#pragma once

#include "ResultsDisplayerWithHeaderAndFooter.h"





class CResultsDisplayerNormal : public CResultsDisplayerWithHeaderAndFooter
{
public:
    CResultsDisplayerNormal                 (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr);

    void DisplayFileResults                 (const CDirectoryInfo & di) override;

protected:
    HRESULT DisplayResultsNormalDateAndTime (const FILETIME & ftLastWriteTime);
    void    DisplayResultsNormalAttributes  (DWORD dwFileAttributes);
    void    DisplayResultsNormalFileSize    (const WIN32_FIND_DATA & fileInfo, size_t cchStringLengthOfMaxFileSize);
};
