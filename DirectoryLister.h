#pragma once

#include "DirectoryInfo.h"
#include "ResultsDisplayerBase.h"



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
    HRESULT ProcessDirectory           (const CDriveInfo & driveInfo, 
                                        filesystem::path dirPath, 
                                        filesystem::path fileSpec, 
                                        CResultsDisplayerBase::EDirectoryLevel level);
    
    HRESULT RecurseIntoSubdirectories  (const CDriveInfo & driveInfo,
                                        filesystem::path dirPath, 
                                        filesystem::path fileSpec);

    HRESULT AddMatchToList             (__in WIN32_FIND_DATA * pwfd, __in CDirectoryInfo * pdi);
    BOOL    IsDots                     (LPCWSTR pszFileName);

    
    CCommandLine                        * m_pCmdLine; 
    CConsole                            * m_pConsole;
    CConfig                             * m_pConfig;
    unique_ptr<CResultsDisplayerBase>     m_displayer;
    ULARGE_INTEGER                        m_uliSizeOfAllFilesFound;
    UINT                                  m_cFilesFound;
    UINT                                  m_cDirectoriesFound;
};
