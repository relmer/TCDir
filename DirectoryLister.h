#pragma once

#include "FileInfo.h"
#include "DirectoryInfo.h"





class CCommandLine;
class CConfig;
class CConsole;





class CDirectoryLister
{
public:
    CDirectoryLister  (__in CCommandLine * pCmdLine, __in CConsole * pConsole, __in CConfig * pConfig);
    ~CDirectoryLister (void); 

    void List         (LPCWSTR pszMask);


protected:    
    enum class EDirectoryLevel
    {
        Initial,
        Subdirectory
    };


    HRESULT ProcessDirectory           (LPCWSTR pszPath, LPCWSTR pszFileSpec, EDirectoryLevel level);
    HRESULT RecurseIntoSubdirectories  (LPCWSTR pszPath, LPCWSTR pszFileSpec);
    HRESULT AddMatchToList             (__in CFileInfo * pwfd, __in CDirectoryInfo * pdi);
    void    DisplayResults             (__in CDirectoryInfo * pdi, EDirectoryLevel level);
    void    DisplayResultsWide         (__in CDirectoryInfo * pdi);
    HRESULT GetColumnInfo              (__in const CDirectoryInfo * pdi, __out UINT * pcColumns, __out UINT * pcxColumnWidth);
    HRESULT AddEmptyMatches            (__in CDirectoryInfo * pdi, size_t cColumns);
    HRESULT GetWideFormattedName       (__in const WIN32_FIND_DATA * pwfd, __deref_out_z LPCWSTR * ppszName);
    void    DisplayResultsNormal       (__in CDirectoryInfo * pdi);
    void    DisplayDriveHeader         (LPCWSTR pszPath);
    void    DisplayPathHeader          (LPCWSTR pszPath);
    void    DisplayDirectorySummary    (__in const CDirectoryInfo * pdi);
    void    DisplayListingSummary      (__in const CDirectoryInfo * pdi);
    void    DisplayVolumeFooter        (__in const CDirectoryInfo * pdi);
    void    DisplayFooterQuotaInfo     (__in const ULARGE_INTEGER * puliFreeBytesAvailable);
    UINT    GetMaxSize                 (__in const ULARGE_INTEGER * puli);
    LPCWSTR FormatNumberWithSeparators (ULONGLONG n);
    BOOL    IsDots                     (LPCWSTR pszFileName);


    
    CCommandLine   * m_pCmdLine; 
    CConsole       * m_pConsole;
    CConfig        * m_pConfig;
    ULARGE_INTEGER   m_uliSizeOfAllFilesFound;
    UINT             m_cFilesFound;
    UINT             m_cDirectoriesFound;
};                           






