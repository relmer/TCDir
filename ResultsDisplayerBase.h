#pragma once

#include "DirectoryInfo.h"

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

    CResultsDisplayerBase                   (__in CCommandLine * pCmdLine, __in CConsole * pConsole, __in CConfig * pConfig);
    virtual ~CResultsDisplayerBase          (void);

    void    DisplayResults                  (__in CDirectoryInfo * pdi, EDirectoryLevel level);
    
    // Pure virtual method - must be implemented by derived classes
    virtual void DisplayFileResults         (__in CDirectoryInfo * pdi) = 0;
    void    DisplayDriveHeader              (LPCWSTR pszPath);
    void    DisplayPathHeader               (LPCWSTR pszPath);
    void    DisplayDirectorySummary         (__in const CDirectoryInfo * pdi);
    void    DisplayListingSummary           (__in const CDirectoryInfo * pdi, UINT cFilesFound, UINT cDirectoriesFound, const ULARGE_INTEGER& uliSizeOfAllFilesFound);
    void    DisplayVolumeFooter             (__in const CDirectoryInfo * pdi);
    void    DisplayFooterQuotaInfo          (__in const ULARGE_INTEGER * puliFreeBytesAvailable);

protected:
    UINT    GetStringLengthOfMaxFileSize    (__in const ULARGE_INTEGER * puli);
    LPCWSTR FormatNumberWithSeparators      (ULONGLONG n);

    CCommandLine   * m_pCmdLine; 
    CConsole       * m_pConsole;
    CConfig        * m_pConfig;
};
