#pragma once

#include "FileInfo.h"
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
    ~CResultsDisplayerBase                  (void);

    void    DisplayResults                  (__in CDirectoryInfo * pdi, EDirectoryLevel level);
    void    DisplayResultsWide              (__in CDirectoryInfo * pdi);
    void    DisplayResultsNormal            (__in CDirectoryInfo * pdi);
    void    DisplayDriveHeader              (LPCWSTR pszPath);
    void    DisplayPathHeader               (LPCWSTR pszPath);
    void    DisplayDirectorySummary         (__in const CDirectoryInfo * pdi);
    void    DisplayListingSummary           (__in const CDirectoryInfo * pdi, UINT cFilesFound, UINT cDirectoriesFound, const ULARGE_INTEGER& uliSizeOfAllFilesFound);
    void    DisplayVolumeFooter             (__in const CDirectoryInfo * pdi);
    void    DisplayFooterQuotaInfo          (__in const ULARGE_INTEGER * puliFreeBytesAvailable);

protected:
    HRESULT GetColumnInfo                   (__in const CDirectoryInfo * pdi, __out size_t * pcColumns, __out size_t * pcxColumnWidth);
    HRESULT GetWideFormattedName            (__in const WIN32_FIND_DATA * pwfd, __deref_out_z LPCWSTR * ppszName);
    HRESULT DisplayResultsNormalDateAndTime (const FILETIME & ftLastWriteTime);
    void    DisplayResultsNormalAttributes  (DWORD dwFileAttributes);
    void    DisplayResultsNormalFileSize    (const CFileInfo & fileInfo, size_t cchStringLengthOfMaxFileSize);
    UINT    GetStringLengthOfMaxFileSize    (__in const ULARGE_INTEGER * puli);
    LPCWSTR FormatNumberWithSeparators      (ULONGLONG n);

    CCommandLine   * m_pCmdLine; 
    CConsole       * m_pConsole;
    CConfig        * m_pConfig;
};
