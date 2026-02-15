#pragma once

#include "ResultsDisplayerWithHeaderAndFooter.h"





class CResultsDisplayerWide : public CResultsDisplayerWithHeaderAndFooter
{
public:
    CResultsDisplayerWide        (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr, bool fIconsActive = false);

    void DisplayFileResults      (const CDirectoryInfo & di) override;

protected:
    HRESULT      DisplayFile          (const WIN32_FIND_DATA & wfd, size_t cxColumnWidth, bool fInSyncRoot);
    void         GetColumnInfo        (const CDirectoryInfo & di, bool fInSyncRoot, size_t & cColumns, size_t & cxColumnWidth);
    wstring_view GetWideFormattedName (const WIN32_FIND_DATA & wfd, LPWSTR pszBuffer, size_t cchBuffer);
};
