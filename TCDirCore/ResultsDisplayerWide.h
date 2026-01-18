#pragma once

#include "ResultsDisplayerWithHeaderAndFooter.h"





class CResultsDisplayerWide : public CResultsDisplayerWithHeaderAndFooter
{
public:
    CResultsDisplayerWide        (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr);

    void DisplayFileResults      (const CDirectoryInfo & di) override;

protected:
    HRESULT DisplayFile          (const WIN32_FIND_DATA & wfd, size_t cxColumnWidth);
    void    GetColumnInfo        (const CDirectoryInfo & di, size_t & cColumns, size_t & cxColumnWidth);
    HRESULT GetWideFormattedName (const WIN32_FIND_DATA & wfd, __deref_out_z LPCWSTR * ppszName);
};
