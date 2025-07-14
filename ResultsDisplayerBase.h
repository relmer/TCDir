#pragma once

#include "DirectoryInfo.h"
#include "DriveInfo.h"

class CCommandLine;
class CConfig;
class CConsole;

class CResultsDisplayerBase
{
public:
    enum class EDirectoryLevel
    {
        Initial,
        Subdirectory
    };

    CResultsDisplayerBase                   (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr);
    virtual ~CResultsDisplayerBase          (void);

    void    DisplayResults                  (const CDriveInfo & driveInfo, __in CDirectoryInfo * pdi, EDirectoryLevel level);
    
    // Pure virtual method - must be implemented by derived classes
    virtual void DisplayFileResults         (__in CDirectoryInfo * pdi) = 0;
    void    DisplayDriveHeader              (const CDriveInfo & driveInfo);
    void    DisplayPathHeader               (const filesystem::path & dirPath);
    void    DisplayDirectorySummary         (__in const CDirectoryInfo * pdi);
    void    DisplayListingSummary           (__in const CDirectoryInfo * pdi, UINT cFilesFound, UINT cDirectoriesFound, const ULARGE_INTEGER& uliSizeOfAllFilesFound);
    void    DisplayVolumeFooter             (__in const CDirectoryInfo * pdi);
    void    DisplayFooterQuotaInfo          (__in const ULARGE_INTEGER * puliFreeBytesAvailable);

protected:
    UINT    GetStringLengthOfMaxFileSize    (__in const ULARGE_INTEGER * puli);
    LPCWSTR FormatNumberWithSeparators      (ULONGLONG n);

    shared_ptr<CCommandLine> m_cmdLinePtr; 
    shared_ptr<CConsole>     m_consolePtr;
    shared_ptr<CConfig>      m_configPtr;
};
