#pragma once

#include "IResultsDisplayer.h"





class CCommandLine;
class CConfig;
class CConsole;





class CResultsDisplayerBare : public IResultsDisplayer
{
public:
    CResultsDisplayerBare        (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr);

    void DisplayResults          (const CDriveInfo & driveInfo, const CDirectoryInfo & di, EDirectoryLevel level) override;
    void DisplayRecursiveSummary (const CDirectoryInfo & diInitial, UINT cFilesFound, UINT cDirectoriesFound, const ULARGE_INTEGER & uliSizeOfAllFilesFound) override;

protected:
    shared_ptr<CCommandLine> m_cmdLinePtr; 
    shared_ptr<CConsole>     m_consolePtr;
    shared_ptr<CConfig>      m_configPtr;
};

