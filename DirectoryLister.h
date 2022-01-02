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

    void operator() (LPCWSTR pszMask);


protected:    
    HRESULT ProcessDirectory          (LPCWSTR pszPath, LPCWSTR pszFileSpec);
    HRESULT RecurseIntoSubdirectories (LPCWSTR pszPath, LPCWSTR pszFileSpec);
    HRESULT AddMatchToList            (CFileInfo * pwfd, CDirectoryInfo * pdi);
    void    Scroll                    (CDirectoryInfo * pdi);
    HRESULT CalculateLinesNeeded      (__in CDirectoryInfo * pdi, __out int * pcLinesNeeded);
    void    DisplayResults            (CDirectoryInfo * pdi);
    void    DisplayResultsWide        (CDirectoryInfo * pdi);
    HRESULT GetColumnInfo             (CDirectoryInfo * pdi, UINT * pcColumns, UINT * pcxColumnWidth);
    HRESULT AddEmptyMatches           (CDirectoryInfo * pdi, size_t cColumns);
    HRESULT GetWideFormattedName      (WIN32_FIND_DATA * pwfd, LPWSTR * ppszName);
    void    DisplayResultsNormal      (CDirectoryInfo * pdi);
    void    DisplayDriveHeader        (LPCWSTR pszPath);
    void    DisplayPathHeader         (LPCWSTR pszPath);
    void    DisplayFooter             (CDirectoryInfo * pdi);
    void    DisplayFooterQuotaInfo    (const ULARGE_INTEGER * puliFreeBytesAvailable);
    UINT    GetMaxSize                (ULARGE_INTEGER * puli);
    LPCWSTR FormatFileSize            (ULARGE_INTEGER uli);
    BOOL    IsDots                    (LPCWSTR pszFileName);


    
    CCommandLine   * m_pCmdLine; 
    CConsole       * m_pConsole;
    CConfig        * m_pConfig;
    ULARGE_INTEGER   m_uliSizeOfAllFilesFound;
    UINT             m_cFilesFound;
    UINT             m_cDirectoriesFound;
};                           






