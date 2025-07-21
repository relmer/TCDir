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

    void    DisplayResults                  (const CDriveInfo & driveInfo, const CDirectoryInfo & di, EDirectoryLevel level);
    
    // Pure virtual method - must be implemented by derived classes
    virtual void DisplayFileResults         (const CDirectoryInfo & di) = 0;
    void    DisplayDriveHeader              (const CDriveInfo & driveInfo);
    void    DisplayPathHeader               (const filesystem::path & dirPath);
    void    DisplayDirectorySummary         (const CDirectoryInfo & di);
    void    DisplayListingSummary           (const CDirectoryInfo & di, UINT cFilesFound, UINT cDirectoriesFound, const ULARGE_INTEGER & uliSizeOfAllFilesFound);
    void    DisplayVolumeFooter             (const CDirectoryInfo & di);
    void    DisplayFooterQuotaInfo          (const ULARGE_INTEGER & uliFreeBytesAvailable);

protected:
    UINT    GetStringLengthOfMaxFileSize    (const ULARGE_INTEGER & uli);
    LPCWSTR FormatNumberWithSeparators      (ULONGLONG n);

    shared_ptr<CCommandLine> m_cmdLinePtr; 
    shared_ptr<CConsole>     m_consolePtr;
    shared_ptr<CConfig>      m_configPtr;
};
