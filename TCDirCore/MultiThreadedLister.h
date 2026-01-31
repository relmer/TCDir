#pragma once

#include "DirectoryLister.h"
#include "WorkQueue.h"





class CDriveInfo;





struct WorkItem
{
    shared_ptr<CDirectoryInfo> m_pDirInfo;
};





class CMultiThreadedLister : public CDirectoryLister
{
public:
    CMultiThreadedLister  (shared_ptr<CCommandLine> cmdLinePtr, 
                           shared_ptr<CConsole> consolePtr, 
                           shared_ptr<CConfig> configPtr);
    ~CMultiThreadedLister ();

    HRESULT ProcessDirectoryMultiThreaded (const CDriveInfo & driveInfo, 
                                           const filesystem::path & dirPath, 
                                           const filesystem::path & fileSpec,
                                           IResultsDisplayer & displayer,
                                           IResultsDisplayer::EDirectoryLevel level,
                                           SListingTotals & totals);

protected:
    void    EnumerateDirectoryNode        (shared_ptr<CDirectoryInfo> pDirInfo);
    void    WorkerThreadFunc              (stop_token stopToken);
    HRESULT PrintDirectoryTree            (shared_ptr<CDirectoryInfo> pDirInfo, 
                                           const CDriveInfo & driveInfo,
                                           IResultsDisplayer & displayer,
                                           IResultsDisplayer::EDirectoryLevel level,
                                           SListingTotals & totals);

private:
    HRESULT PerformEnumeration            (shared_ptr<CDirectoryInfo> pDirInfo);
    void    EnqueueChildDirectory         (const WIN32_FIND_DATA & wfd, shared_ptr<CDirectoryInfo> pDirInfo);
    void    StopWorkers                   ();

    bool    StopRequested                 () const { return m_stopSource.stop_requested(); }

    stop_source               m_stopSource;
    CWorkQueue<WorkItem>      m_workQueue;
    vector<jthread>           m_workers;
};
