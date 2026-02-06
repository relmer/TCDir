#pragma once

#include "DirectoryInfo.h"
#include "IResultsDisplayer.h"
#include "ListingTotals.h"
#include "MaskGrouper.h"





class CCommandLine;
class CConfig;
class CConsole;





class CDirectoryLister
{
public:
    CDirectoryLister  (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr);
    ~CDirectoryLister (void); 

    void List         (const MaskGroup & group);
    
    static bool IsDots (LPCWSTR pszFileName);

protected:
    HRESULT ProcessDirectory                   (const CDriveInfo                   & driveInfo, 
                                                const filesystem::path             & dirPath, 
                                                const filesystem::path             & fileSpec, 
                                                IResultsDisplayer::EDirectoryLevel   level);
    
    HRESULT CollectMatchingFilesAndDirectories (const std::filesystem::path & dirPath,
                                                const std::filesystem::path & fileSpec,
                                                CDirectoryInfo              & di);

    HRESULT ProcessDirectoryMultiThreaded      (const CDriveInfo                   & driveInfo, 
                                                const filesystem::path             & dirPath, 
                                                const vector<filesystem::path>     & fileSpecs,
                                                IResultsDisplayer::EDirectoryLevel   level);
    
    HRESULT RecurseIntoSubdirectories          (const CDriveInfo       & driveInfo,
                                                const filesystem::path & dirPath, 
                                                const filesystem::path & fileSpec);

    void    AddMatchToList                     (const WIN32_FIND_DATA & wfd, CDirectoryInfo & di, SListingTotals * pTotals);
    void    HandleDirectoryMatch               (size_t & cchFileName, CDirectoryInfo & di);
    void    HandleFileMatch                    (const WIN32_FIND_DATA & wfd, FileInfo & fileEntry, CDirectoryInfo & di, SListingTotals * pTotals);
    HRESULT HandleFileMatchStreams             (const WIN32_FIND_DATA & wfd, FileInfo & fileEntry, CDirectoryInfo & di, SListingTotals * pTotals);



    shared_ptr<CCommandLine>              m_cmdLinePtr; 
    shared_ptr<CConsole>                  m_consolePtr;
    shared_ptr<CConfig>                   m_configPtr;
    unique_ptr<IResultsDisplayer>         m_displayer;
    SListingTotals                        m_totals;
};
