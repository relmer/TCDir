#pragma once

#include "ResultsDisplayerBase.h"





class CResultsDisplayerWide : public CResultsDisplayerBase
{
public:
    CResultsDisplayerWide        (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr);

    void DisplayFileResults      (__in CDirectoryInfo * pdi) override;

protected:
    HRESULT GetColumnInfo        (__in const CDirectoryInfo * pdi, __out size_t * pcColumns, __out size_t * pcxColumnWidth);
    HRESULT GetWideFormattedName (__in const WIN32_FIND_DATA * pwfd, __deref_out_z LPCWSTR * ppszName);
};
