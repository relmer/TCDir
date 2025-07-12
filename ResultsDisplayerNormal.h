#pragma once

#include "ResultsDisplayerBase.h"





class CResultsDisplayerNormal : public CResultsDisplayerBase
{
public:
    CResultsDisplayerNormal                 (__in CCommandLine * pCmdLine, __in CConsole * pConsole, __in CConfig * pConfig);

    void DisplayFileResults                 (__in CDirectoryInfo * pdi) override;

protected:
    HRESULT DisplayResultsNormalDateAndTime (const FILETIME & ftLastWriteTime);
    void    DisplayResultsNormalAttributes  (DWORD dwFileAttributes);
    void    DisplayResultsNormalFileSize    (const CFileInfo & fileInfo, size_t cchStringLengthOfMaxFileSize);
};
