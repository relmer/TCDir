#pragma once

#include "pch.h"
#include "DirectoryInfo.h"
#include "ResultsDisplayerBase.h"
#include "WorkQueue.h"

class CCommandLine;
class CConfig;
class CConsole;
class CDriveInfo;


struct WorkItem
{
    shared_ptr<CDirectoryInfo> m_pDirInfo;
};


class CMultiThreadedLister
{
public:
    CMultiThreadedLister  (shared_ptr<CCommandLine> cmdLinePtr, 
                          shared_ptr<CConsole> consolePtr, 
                          shared_ptr<CConfig> configPtr);
    ~CMultiThreadedLister ();

    HRESULT ProcessDirectoryMultiThreaded (const CDriveInfo & driveInfo, 
                                          filesystem::path dirPath, 
                                          filesystem::path fileSpec,
                                          CResultsDisplayerBase * pDisplayer,
                                          CResultsDisplayerBase::EDirectoryLevel level,
                                          ULARGE_INTEGER & uliSizeOfAllFilesFound,
                                          UINT & cFilesFound,
                                          UINT & cDirectoriesFound);

protected:
    void    EnumerateDirectoryNode        (shared_ptr<CDirectoryInfo> pDirInfo);
    void    WorkerThreadFunc              ();
    HRESULT PrintDirectoryTree            (shared_ptr<CDirectoryInfo> pDirInfo, 
                                          const CDriveInfo & driveInfo,
                                          CResultsDisplayerBase * pDisplayer,
                                          CResultsDisplayerBase::EDirectoryLevel level,
                                          ULARGE_INTEGER & uliSizeOfAllFilesFound,
                                          UINT & cFilesFound,
                                          UINT & cDirectoriesFound);
    HRESULT AddMatchToList                (__in WIN32_FIND_DATA * pwfd, 
                                          __in CDirectoryInfo * pdi);
    BOOL    IsDots                        (LPCWSTR pszFileName);


    shared_ptr<CCommandLine>  m_cmdLinePtr;
    shared_ptr<CConsole>      m_consolePtr;
    shared_ptr<CConfig>       m_configPtr;
    atomic<bool>              m_fCancelRequested;
    CWorkQueue<WorkItem>      m_workQueue;
};
