#pragma once

#include "DirectoryInfo.h"
#include "ResultsDisplayerBase.h"





class CCommandLine;
class CConfig;
class CConsole;





class CDirectoryLister
{
public:
    CDirectoryLister  (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr);
    ~CDirectoryLister (void); 

    void List         (const wstring & mask);

protected:
    HRESULT ProcessDirectory                   (const CDriveInfo & driveInfo, 
                                                const filesystem::path & dirPath, 
                                                const filesystem::path & fileSpec, 
                                                CResultsDisplayerBase::EDirectoryLevel level);
    
    HRESULT CollectMatchingFilesAndDirectories (const std::filesystem::path & dirPath,
                                                const std::filesystem::path & fileSpec,
                                                CDirectoryInfo & di);

    HRESULT ProcessDirectoryMultiThreaded      (const CDriveInfo & driveInfo, 
                                                const filesystem::path & dirPath, 
                                                const filesystem::path & fileSpec,
                                                CResultsDisplayerBase::EDirectoryLevel level);
    
    HRESULT RecurseIntoSubdirectories          (const CDriveInfo & driveInfo,
                                                const filesystem::path & dirPath, 
                                                const filesystem::path & fileSpec);

    HRESULT AddMatchToList                     (const WIN32_FIND_DATA & wfd, __in CDirectoryInfo * pdi);
    BOOL    IsDots                             (LPCWSTR pszFileName);


    
    shared_ptr<CCommandLine>              m_cmdLinePtr; 
    shared_ptr<CConsole>                  m_consolePtr;
    shared_ptr<CConfig>                   m_configPtr;
    unique_ptr<CResultsDisplayerBase>     m_displayer;
    ULARGE_INTEGER                        m_uliSizeOfAllFilesFound;
    UINT                                  m_cFilesFound;
    UINT                                  m_cDirectoriesFound;
};
