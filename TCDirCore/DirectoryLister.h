#pragma once

#include "DirectoryInfo.h"
#include "IResultsDisplayer.h"
#include "ListingTotals.h"





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
                                                IResultsDisplayer::EDirectoryLevel level);
    
    HRESULT CollectMatchingFilesAndDirectories (const std::filesystem::path & dirPath,
                                                const std::filesystem::path & fileSpec,
                                                CDirectoryInfo & di);

    HRESULT ProcessDirectoryMultiThreaded      (const CDriveInfo & driveInfo, 
                                                const filesystem::path & dirPath, 
                                                const filesystem::path & fileSpec,
                                                IResultsDisplayer::EDirectoryLevel level);
    
    HRESULT RecurseIntoSubdirectories          (const CDriveInfo & driveInfo,
                                                const filesystem::path & dirPath, 
                                                const filesystem::path & fileSpec);

    HRESULT AddMatchToList                     (const WIN32_FIND_DATA & wfd, CDirectoryInfo * pdi);
    void    HandleDirectoryMatch               (size_t & cchFileName, CDirectoryInfo * pdi);
    void    HandleFileMatch                    (const WIN32_FIND_DATA & wfd, FileInfo & fileEntry, CDirectoryInfo * pdi);
    HRESULT HandleFileMatchStreams             (const WIN32_FIND_DATA & wfd, FileInfo & fileEntry, CDirectoryInfo * pdi);
    BOOL    IsDots                             (LPCWSTR pszFileName);


    
    shared_ptr<CCommandLine>              m_cmdLinePtr; 
    shared_ptr<CConsole>                  m_consolePtr;
    shared_ptr<CConfig>                   m_configPtr;
    unique_ptr<IResultsDisplayer>         m_displayer;
    SListingTotals                        m_totals;
};
