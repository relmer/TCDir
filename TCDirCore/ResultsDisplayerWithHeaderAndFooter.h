#pragma once

#include "IResultsDisplayer.h"





class CCommandLine;
class CConfig;
class CConsole;





class CResultsDisplayerWithHeaderAndFooter : public IResultsDisplayer
{
public:
    CResultsDisplayerWithHeaderAndFooter           (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr);
    virtual ~CResultsDisplayerWithHeaderAndFooter  (void);

    void    DisplayResults                         (const CDriveInfo & driveInfo, const CDirectoryInfo & di, EDirectoryLevel level) override;
    void    DisplayRecursiveSummary                (const CDirectoryInfo & diInitial, UINT cFilesFound, UINT cDirectoriesFound, const ULARGE_INTEGER & uliSizeOfAllFilesFound) override;
    
    // Pure virtual method - must be implemented by derived classes
    virtual void DisplayFileResults                (const CDirectoryInfo & di) = 0;


    
protected:
    void    DisplayDriveHeader                     (const CDriveInfo & driveInfo);
    void    DisplayPathHeader                      (const filesystem::path & dirPath);
    void    DisplayDirectorySummary                (const CDirectoryInfo & di);
    void    DisplayListingSummary                  (const CDirectoryInfo & di, UINT cFilesFound, UINT cDirectoriesFound, const ULARGE_INTEGER & uliSizeOfAllFilesFound);
    void    DisplayVolumeFooter                    (const CDirectoryInfo & di);
    void    DisplayFooterQuotaInfo                 (const ULARGE_INTEGER & uliFreeBytesAvailable);

    UINT    GetStringLengthOfMaxFileSize           (const ULARGE_INTEGER & uli);
    LPCWSTR FormatNumberWithSeparators             (ULONGLONG n);

    shared_ptr<CCommandLine> m_cmdLinePtr; 
    shared_ptr<CConsole>     m_consolePtr;
    shared_ptr<CConfig>      m_configPtr;
};
