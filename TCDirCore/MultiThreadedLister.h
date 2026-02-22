#pragma once

#include "DirectoryLister.h"
#include "TreeConnectorState.h"
#include "WorkQueue.h"





class CDriveInfo;
class CResultsDisplayerTree;





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
                                           const vector<filesystem::path> & fileSpecs,
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
    static inline wstring ToLower (const wstring & s)
    {
        wstring lower = s;



        transform (lower.begin(), lower.end(), lower.begin(), towlower);
        return lower;
    }



    HRESULT PerformEnumeration            (shared_ptr<CDirectoryInfo> pDirInfo);
    HRESULT EnumerateMatchingFiles        (shared_ptr<CDirectoryInfo> pDirInfo);
    HRESULT EnumerateSubdirectories       (shared_ptr<CDirectoryInfo> pDirInfo);
    void    EnqueueChildDirectory         (const WIN32_FIND_DATA & wfd, shared_ptr<CDirectoryInfo> pDirInfo);
    void    StopWorkers                   ();

    HRESULT WaitForNodeCompletion         (shared_ptr<CDirectoryInfo> pDirInfo);
    void    SortResults                   (shared_ptr<CDirectoryInfo> pDirInfo);
    void    AccumulateTotals              (shared_ptr<CDirectoryInfo> pDirInfo, SListingTotals & totals);
    HRESULT ProcessChildren               (shared_ptr<CDirectoryInfo> pDirInfo,
                                           const CDriveInfo & driveInfo,
                                           IResultsDisplayer & displayer,
                                           SListingTotals & totals);

    using ChildMap = unordered_map<wstring, shared_ptr<CDirectoryInfo>>;

    HRESULT PrintDirectoryTreeMode        (shared_ptr<CDirectoryInfo>           pDirInfo,
                                           const CDriveInfo                   & driveInfo,
                                           CResultsDisplayerTree              & treeDisplayer,
                                           IResultsDisplayer::EDirectoryLevel   level,
                                           SListingTotals                     & totals,
                                           STreeConnectorState                & treeState);
    HRESULT DisplayTreeEntries            (shared_ptr<CDirectoryInfo> pDirInfo,
                                           const CDriveInfo & driveInfo,
                                           CResultsDisplayerTree & treeDisplayer,
                                           SListingTotals & totals,
                                           STreeConnectorState & treeState);
    bool    IsLastVisibleEntry            (const vector<FileInfo> & vMatches,
                                           size_t iCurrent,
                                           const ChildMap & childMap);
    HRESULT RecurseIntoChildDirectory     (shared_ptr<CDirectoryInfo> pChild,
                                           const FileInfo & parentEntry,
                                           bool fIsLast,
                                           const CDriveInfo & driveInfo,
                                           CResultsDisplayerTree & treeDisplayer,
                                           SListingTotals & totals,
                                           STreeConnectorState & treeState);

    void    PropagateDescendantMatch      (shared_ptr<CDirectoryInfo> pDirInfo);
    void    TrySignalParentSubtreeComplete (shared_ptr<CDirectoryInfo> pParent);
    bool    WaitForTreeVisibility         (shared_ptr<CDirectoryInfo> pDirInfo);

    bool    StopRequested                 () const { return m_stopSource.stop_requested(); }

    stop_source               m_stopSource;
    CWorkQueue<WorkItem>      m_workQueue;
    vector<jthread>           m_workers;
    bool                      m_fTreePruningActive = false;
};
