#include "pch.h"
#include "MultiThreadedLister.h"

#include "AutoHandle.h"
#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "DriveInfo.h"
#include "FileComparator.h"
#include "Flag.h"
#include "ResultsDisplayerTree.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::CMultiThreadedLister
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CMultiThreadedLister::CMultiThreadedLister (shared_ptr<CCommandLine> pCmdLine, shared_ptr<CConsole> pConsole, shared_ptr<CConfig> pConfig) :
    CDirectoryLister (pCmdLine, pConsole, pConfig)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::~CMultiThreadedLister
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CMultiThreadedLister::~CMultiThreadedLister()
{
    StopWorkers();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::StopWorkers
//
//  Ensures worker threads are stopped and joined
//
////////////////////////////////////////////////////////////////////////////////

void CMultiThreadedLister::StopWorkers()
{
    m_stopSource.request_stop();
    m_workQueue.SetDone();
    m_workers.clear();  // jthreads auto-join on destruction
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::ProcessDirectoryMultiThreaded
//
//  Main entry point for multithreaded directory enumeration.  Takes multiple
//  file specs and applies them to each directory, deduplicating results when
//  a file matches more than one spec.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CMultiThreadedLister::ProcessDirectoryMultiThreaded (
    const CDriveInfo                     & driveInfo,
    const filesystem::path               & dirPath,
    const vector<filesystem::path>       & fileSpecs,
    IResultsDisplayer                    & displayer,
    IResultsDisplayer::EDirectoryLevel     level,
    SListingTotals                       & totals)
{
    HRESULT hr = S_OK;

    

    // Create root directory info node with multiple file specs
    auto pRootDirInfo = make_shared<CDirectoryInfo> (dirPath, fileSpecs);

    pRootDirInfo->m_fInArchive = m_enumerator.IsArchiveLocation (dirPath);

    //
    // Determine whether tree-pruning is active.  This is true only in tree
    // mode with a non-"*" file mask — i.e., when empty subdirectories should
    // be hidden.  When all specs are "*", every directory is visible and no
    // pruning logic runs.  The /S recursive path is never affected.
    //

    if (m_cmdLinePtr->m_fTree)
    {
        bool fAllStar = all_of (fileSpecs.begin(), fileSpecs.end(),
                                [](const auto & spec) { return spec == L"*"; });

        m_fTreePruningActive = !fAllStar;
    }

    // Initialize work queue with root
    m_workQueue.Push (WorkItem { pRootDirInfo });

    // Create worker threads
    const size_t numThreads = max(1u, jthread::hardware_concurrency());

    for (size_t i = 0; i < numThreads; ++i)
    {
        m_workers.emplace_back ([this](stop_token st) { WorkerThreadFunc (st); });
    }

    // Start consuming immediately (streaming output)
    if (m_cmdLinePtr->m_fTree)
    {
        CResultsDisplayerTree & treeDisplayer = static_cast<CResultsDisplayerTree &>(displayer);
        STreeConnectorState     treeState       (m_cmdLinePtr->m_cTreeIndent);

        hr = PrintDirectoryTreeMode (pRootDirInfo,
                                     driveInfo,
                                     treeDisplayer,
                                     level,
                                     totals,
                                     treeState);
        CHR (hr);
    }
    else
    {
        hr = PrintDirectoryTree (pRootDirInfo, 
                                 driveInfo, 
                                 displayer, 
                                 level,
                                 totals);
        CHR (hr);
    }



Error:
    StopWorkers();
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::EnumerateDirectoryNode
//
//  Enumerates a single directory node using Win32 API (producer function)
//
////////////////////////////////////////////////////////////////////////////////

void CMultiThreadedLister::EnumerateDirectoryNode (shared_ptr<CDirectoryInfo> pDirInfo)
{
    HRESULT hr = S_OK;



    {
        lock_guard<mutex> lock (pDirInfo->m_mutex);
        pDirInfo->m_status = CDirectoryInfo::Status::InProgress;
    }

    

    hr = PerformEnumeration (pDirInfo);



    {
        lock_guard<mutex> lock (pDirInfo->m_mutex);
        pDirInfo->m_status = FAILED (hr) ? CDirectoryInfo::Status::Error
                                         : CDirectoryInfo::Status::Done;
        pDirInfo->m_hr = hr;
    }
    
    pDirInfo->m_cvStatusChanged.notify_one();

    //
    // Tree-pruning propagation (only when m_fTreePruningActive).
    //
    // 1. If this node has matching files, propagate the "descendant match"
    //    signal upward through all ancestors so the display thread knows
    //    they are visible.
    //
    // 2. If this node is a leaf (no children), mark its subtree as complete
    //    and try to propagate subtree-completion upward.
    //

    if (m_fTreePruningActive)
    {
        if (pDirInfo->m_cFiles > 0)
        {
            pDirInfo->m_fDescendantMatchFound.store (true, memory_order_release);
            pDirInfo->m_cvStatusChanged.notify_all();

            PropagateDescendantMatch (pDirInfo);
        }

        bool fIsLeaf;

        {
            lock_guard<mutex> lock (pDirInfo->m_mutex);
            fIsLeaf = pDirInfo->m_vChildren.empty();
        }

        if (fIsLeaf)
        {
            pDirInfo->m_fSubtreeComplete.store (true, memory_order_release);
            pDirInfo->m_cvStatusChanged.notify_all();

            auto pParent = pDirInfo->m_wpParent.lock();

            if (pParent)
            {
                TrySignalParentSubtreeComplete (pParent);
            }
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::PerformEnumeration
//
//  Performs the actual directory enumeration
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CMultiThreadedLister::PerformEnumeration (shared_ptr<CDirectoryInfo> pDirInfo)
{
    HRESULT hr = S_OK;

    

    hr = EnumerateMatchingFiles (pDirInfo);
    CHR (hr);

    if (m_cmdLinePtr->m_fRecurse || m_cmdLinePtr->m_fTree)
    {
        hr = EnumerateSubdirectories (pDirInfo);
        CHR (hr);
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::EnumerateMatchingFiles
//
//  Searches for files matching the file spec pattern(s).
//  If multiple file specs are provided, deduplicates results by filename
//  (case-insensitive on Windows).
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CMultiThreadedLister::EnumerateMatchingFiles (shared_ptr<CDirectoryInfo> pDirInfo)
{
    HRESULT                hr               = S_OK;
    NameSet                seenFilenames;
    SEnumerationError      error;




    //
    // Iterate through each file spec
    //

    for (const auto & fileSpec : pDirInfo->m_vFileSpecs)
    {
        hr = m_enumerator.Enumerate (pDirInfo->m_dirPath, fileSpec, [&] (const WIN32_FIND_DATA & wfd)
        {
            // Skip "." and ".."
            if (IsDots (wfd.cFileName))
            {
                return !StopRequested();
            }

            //
            // Deduplicate across file specs: skip if we've already emitted this
            // entry.  Names come from FindFirstFile on this one directory, so
            // they are the canonical on-disk bytes and compare directly.
            //

            if (seenFilenames.contains (wfd.cFileName))
            {
                return !StopRequested();
            }
            seenFilenames.insert (wfd.cFileName);

            // Check if this entry should be displayed based on attribute filters
            if (CFlag::IsSet    (wfd.dwFileAttributes, m_cmdLinePtr->m_dwAttributesRequired) &&
                CFlag::IsNotSet (wfd.dwFileAttributes, m_cmdLinePtr->m_dwAttributesExcluded))
            {
                lock_guard<mutex> lock (pDirInfo->m_mutex);

                AddMatchToList (wfd, *pDirInfo, nullptr);
            }

            return !StopRequested();
        },
        error);

        //
        // A spec with no matches is not an error; an unreadable archive is,
        // and so is an enumeration that fails part way through.
        //

        if (!error.m_strMessage.empty())
        {
            pDirInfo->m_archiveErrorPath = error.m_archivePath;
            pDirInfo->m_strArchiveError  = error.m_strMessage;
            CHR (hr);
        }

        if (IsBenignEnumerationError (hr))
        {
            hr = S_OK;
        }

        CHR (hr);

        if (StopRequested())
        {
            break;
        }
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::EnumerateSubdirectories
//
//  Searches for subdirectories and enqueues them for processing
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CMultiThreadedLister::EnumerateSubdirectories (shared_ptr<CDirectoryInfo> pDirInfo)
{
    HRESULT             hr    = S_OK;
    SEnumerationError   error;

    //
    // In tree mode, directories and archive files must also appear in
    // m_vMatches so they are visible in the interleaved tree display. Build a
    // set of the entries already present (from EnumerateMatchingFiles) to
    // avoid duplicates.
    //

    NameSet seenDirs;



    if (m_cmdLinePtr->m_fTree)
    {
        lock_guard<mutex> lock (pDirInfo->m_mutex);

        for (const auto & entry : pDirInfo->m_vMatches)
        {
            if (CFlag::IsSet (entry.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) ||
                m_enumerator.IsArchiveFile (pDirInfo->m_fInArchive, entry))
            {
                seenDirs.insert (entry.cFileName);
            }
        }
    }

    // Search using "*" to find all directories and archive files
    hr = m_enumerator.Enumerate (pDirInfo->m_dirPath, L"*", [&] (const WIN32_FIND_DATA & wfd)
    {
        bool fDirectory = !IsDots (wfd.cFileName) && CFlag::IsSet (wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
        bool fArchive   = m_enumerator.IsArchiveFile (pDirInfo->m_fInArchive, wfd);



        if (StopRequested())
        {
            return false;
        }

        // Enqueue directories and archive files for recursion
        if (fDirectory || fArchive)
        {
            lock_guard<mutex> lock (pDirInfo->m_mutex);

            EnqueueChildDirectory (wfd, pDirInfo);

            //
            // In tree mode, add every directory and archive file to m_vMatches
            // so that it is displayed in the tree and recursed into.  Skip if the
            // entry was already added by EnumerateMatchingFiles (e.g., a
            // directory whose name matched the file spec like *.cpp matching
            // "foo.cpp/").  An archive file that did not match a file spec is
            // added as a container only, and is not counted as a file.
            //

            if (m_cmdLinePtr->m_fTree)
            {
                if (!seenDirs.contains (wfd.cFileName))
                {
                    seenDirs.insert (wfd.cFileName);

                    if (fDirectory)
                    {
                        AddMatchToList (wfd, *pDirInfo, nullptr);
                    }
                    else
                    {
                        FileInfo container (wfd);

                        container.m_fArchiveContainer = true;
                        pDirInfo->m_vMatches.push_back (move (container));
                    }
                }
            }
        }

        return true;
    },
    error);

    //
    // The directory was already enumerated for matching files, so no entries,
    // or an archive error that is already in pDirInfo, is not an error here.
    // Any other failure part way through the enumeration is.
    //

    if (!error.m_strMessage.empty() || IsBenignEnumerationError (hr))
    {
        hr = S_OK;
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::IsBenignEnumerationError
//
//  True for the results of an enumeration with nothing to list: no entries
//  match, the path is missing, or the directory is not readable (an
//  unreadable directory lists as empty, as it did before the enumerator).
//
////////////////////////////////////////////////////////////////////////////////

bool CMultiThreadedLister::IsBenignEnumerationError (HRESULT hr)
{
    return SUCCEEDED (hr)                                   ||
           hr == HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND)  ||
           hr == HRESULT_FROM_WIN32 (ERROR_PATH_NOT_FOUND)  ||
           hr == HRESULT_FROM_WIN32 (ERROR_NO_MORE_FILES)   ||
           hr == HRESULT_FROM_WIN32 (ERROR_ACCESS_DENIED);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::EnqueueChildDirectory
//
//  Creates and enqueues a child directory for processing
//
////////////////////////////////////////////////////////////////////////////////

void CMultiThreadedLister::EnqueueChildDirectory (const WIN32_FIND_DATA & wfd, shared_ptr<CDirectoryInfo> pDirInfo)
{
    filesystem::path subdirPath = pDirInfo->m_dirPath / wfd.cFileName;
    auto             pChild     = make_shared<CDirectoryInfo> (subdirPath, pDirInfo->m_vFileSpecs);



    pChild->m_fInArchive = pDirInfo->m_fInArchive || m_enumerator.IsArchiveLocation (subdirPath);

    //
    // Set parent back-pointer for tree-pruning propagation.  Only set when
    // pruning is active so the /S path and tree-without-mask path are
    // completely unaffected.
    //

    if (m_fTreePruningActive)
    {
        pChild->m_wpParent = pDirInfo;
    }

    pDirInfo->m_vChildren.push_back (pChild);

    //
    // Count this subdirectory only if it won't be counted during file enumeration.
    // When a file spec matches this directory name, HandleDirectoryMatch will
    // count it. Avoid double-counting by checking if any pattern matches.
    //
    // Note: We don't increment m_cSubDirectories here. That counter tracks
    // directories that matched the file pattern (e.g., lib.cpp matching *.cpp),
    // which is handled in HandleDirectoryMatch when the dir is added to results.
    //

    m_workQueue.Push (WorkItem { pChild });
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::WorkerThreadFunc
//
//  Worker thread function - processes items from work queue
//
////////////////////////////////////////////////////////////////////////////////

void CMultiThreadedLister::WorkerThreadFunc (stop_token stopToken)
{
    while (!stopToken.stop_requested())
    {
        WorkItem item;

        if (m_workQueue.Pop (item))
        {
            EnumerateDirectoryNode (item.m_pDirInfo);
        }
        else
        {
            // Queue is done
            break;
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::PrintDirectoryTree
//
//  Consumer function - prints directory tree (runs on main thread)
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CMultiThreadedLister::PrintDirectoryTree (
    shared_ptr<CDirectoryInfo>           pDirInfo,
    const CDriveInfo                   & driveInfo,
    IResultsDisplayer                  & displayer,
    IResultsDisplayer::EDirectoryLevel   level,
    SListingTotals                     & totals)
{
    HRESULT hr = S_OK;

    

    hr = WaitForNodeCompletion (pDirInfo);
    CHR (hr);

    SortResults (pDirInfo);

    displayer.DisplayResults (driveInfo, *pDirInfo, level);

    AccumulateTotals (pDirInfo, totals);

    hr = ProcessChildren (pDirInfo, driveInfo, displayer, totals);
    CHR (hr);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::WaitForNodeCompletion
//
//  Waits for a directory node to complete enumeration
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CMultiThreadedLister::WaitForNodeCompletion (shared_ptr<CDirectoryInfo> pDirInfo)
{
    HRESULT hr = S_OK;

    unique_lock<mutex> lock (pDirInfo->m_mutex);

    pDirInfo->m_cvStatusChanged.wait (lock, [&]() {
        return pDirInfo->m_status == CDirectoryInfo::Status::Done ||
               pDirInfo->m_status == CDirectoryInfo::Status::Error ||
               StopRequested();
    });

    // Check for cancellation
    if (StopRequested())
    {
        CHR (E_ABORT);
    }

    // Check for error
    if (pDirInfo->m_status == CDirectoryInfo::Status::Error)
    {
        if (!pDirInfo->m_strArchiveError.empty())
        {
            PrintArchiveError (*m_consolePtr, pDirInfo->m_archiveErrorPath, pDirInfo->m_strArchiveError);
        }
        else
        {
            m_consolePtr->ColorPrintf (L"{Error}  Error accessing directory: {InformationHighlight}%s{Error}: HRESULT 0x%08X\n",
                                        pDirInfo->m_dirPath.c_str(),
                                        pDirInfo->m_hr);
        }

        CHR (pDirInfo->m_hr);
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::SortResults
//
//  Sorts the file matches using FileComparator
//
////////////////////////////////////////////////////////////////////////////////

void CMultiThreadedLister::SortResults (shared_ptr<CDirectoryInfo> pDirInfo)
{
    lock_guard<mutex> lock (pDirInfo->m_mutex);

    SortMatches (pDirInfo->m_vMatches, FileComparator (m_cmdLinePtr, m_cmdLinePtr->m_fTree));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::AccumulateTotals
//
//  Updates global counters from a directory node
//
////////////////////////////////////////////////////////////////////////////////

void CMultiThreadedLister::AccumulateTotals (shared_ptr<CDirectoryInfo> pDirInfo, SListingTotals & totals)
{
    lock_guard<mutex> lock (pDirInfo->m_mutex);

    totals.m_cFiles                  += pDirInfo->m_cFiles;
    totals.m_uliFileBytes.QuadPart   += pDirInfo->m_uliBytesUsed.QuadPart;
    totals.m_cStreams                += pDirInfo->m_cStreams;
    totals.m_uliStreamBytes.QuadPart += pDirInfo->m_uliStreamBytesUsed.QuadPart;

    //
    // Count directories whose names matched the mask
    //

    totals.m_cDirectories += pDirInfo->m_cSubDirectories;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::ProcessChildren
//
//  Recursively processes child directories
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CMultiThreadedLister::ProcessChildren (shared_ptr<CDirectoryInfo> pDirInfo,
                                               const CDriveInfo & driveInfo,
                                               IResultsDisplayer & displayer,
                                               SListingTotals & totals)
{
    HRESULT hr = S_OK;

    for (const auto & pChild : pDirInfo->m_vChildren)
    {
        if (StopRequested())
        {
            CHR (E_ABORT);
        }

        hr = PrintDirectoryTree (pChild, 
                                 driveInfo, 
                                 displayer, 
                                 IResultsDisplayer::EDirectoryLevel::Subdirectory,
                                 totals);
        IGNORE_RETURN_VALUE (hr, S_OK);
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::PropagateDescendantMatch
//
//  Walks up the parent chain via m_wpParent, setting
//  m_fDescendantMatchFound = true and notifying m_cvStatusChanged on each
//  ancestor.  Stops when the parent is null (root reached) or the flag is
//  already set (an earlier producer already propagated through this path).
//
////////////////////////////////////////////////////////////////////////////////

void CMultiThreadedLister::PropagateDescendantMatch (shared_ptr<CDirectoryInfo> pDirInfo)
{
    auto pParent = pDirInfo->m_wpParent.lock();



    while (pParent)
    {
        //
        // If already flagged, every ancestor above is also flagged (or will
        // be by the thread that set this one).  Stop to avoid redundant work.
        //

        if (pParent->m_fDescendantMatchFound.exchange (true, memory_order_acq_rel))
        {
            break;
        }

        pParent->m_cvStatusChanged.notify_all();

        pParent = pParent->m_wpParent.lock();
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::TrySignalParentSubtreeComplete
//
//  Checks whether ALL of pParent's children have m_fSubtreeComplete == true.
//  If so, sets pParent->m_fSubtreeComplete, notifies, and recurses upward
//  to the grandparent.
//
////////////////////////////////////////////////////////////////////////////////

void CMultiThreadedLister::TrySignalParentSubtreeComplete (shared_ptr<CDirectoryInfo> pParent)
{
    {
        lock_guard<mutex> lock (pParent->m_mutex);

        for (const auto & pSibling : pParent->m_vChildren)
        {
            if (!pSibling->m_fSubtreeComplete.load (memory_order_acquire))
            {
                return;   // At least one sibling subtree is still in progress
            }
        }
    }



    //
    // All children are subtree-complete.  Mark this parent as well.
    //

    pParent->m_fSubtreeComplete.store (true, memory_order_release);
    pParent->m_cvStatusChanged.notify_all();

    auto pGrandparent = pParent->m_wpParent.lock();

    if (pGrandparent)
    {
        TrySignalParentSubtreeComplete (pGrandparent);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::WaitForTreeVisibility
//
//  Blocks until the visibility of the given directory node is determined.
//  Returns true if the directory should be displayed (it or a descendant
//  has matching files), false if it should be pruned.
//
//  Waits on m_cvStatusChanged until either m_fDescendantMatchFound or
//  m_fSubtreeComplete becomes true.  Also wakes on stop-requested.
//
////////////////////////////////////////////////////////////////////////////////

bool CMultiThreadedLister::WaitForTreeVisibility (shared_ptr<CDirectoryInfo> pDirInfo)
{
    //
    // Fast path: check atomics before taking the lock.
    //

    if (pDirInfo->m_fDescendantMatchFound.load (memory_order_acquire))
    {
        return true;
    }

    if (pDirInfo->m_fSubtreeComplete.load (memory_order_acquire))
    {
        return false;
    }



    //
    // Slow path: wait for a signal.
    //

    unique_lock<mutex> lock (pDirInfo->m_mutex);

    pDirInfo->m_cvStatusChanged.wait (lock, [&]() {
        return pDirInfo->m_fDescendantMatchFound.load (memory_order_acquire) ||
               pDirInfo->m_fSubtreeComplete.load (memory_order_acquire)     ||
               StopRequested();
    });

    return pDirInfo->m_fDescendantMatchFound.load (memory_order_acquire);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::PrintDirectoryTreeMode
//
//  Tree-mode consumer.  Displays entries for one directory node with tree
//  connector prefixes and immediately recurses into child directories
//  (interleaved display) so that the output forms a proper DFS tree.
//
//  Root level gets drive header, path header, and per-dir summary.
//  Subdirectories have no headers — tree connectors replace path headers.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CMultiThreadedLister::PrintDirectoryTreeMode (
    shared_ptr<CDirectoryInfo>           pDirInfo,
    const CDriveInfo                   & driveInfo,
    CResultsDisplayerTree              & treeDisplayer,
    IResultsDisplayer::EDirectoryLevel   level,
    SListingTotals                     & totals,
    STreeConnectorState                & treeState)
{
    HRESULT hr = S_OK;



    hr = WaitForNodeCompletion (pDirInfo);
    CHR (hr);

    SortResults (pDirInfo);

    //
    // Root directory: show drive header, path header, empty-dir message.
    //

    if (level == IResultsDisplayer::EDirectoryLevel::Initial)
    {
        treeDisplayer.DisplayTreeRootHeader (driveInfo, *pDirInfo);

        if (pDirInfo->m_vMatches.empty() && pDirInfo->m_vChildren.empty())
        {
            treeDisplayer.DisplayTreeEmptyRootMessage (*pDirInfo);
            goto Error;
        }
    }

    //
    // Compute per-directory display state (max file size width, owners, etc.)
    //

    treeDisplayer.BeginDirectory (*pDirInfo);

    hr = DisplayTreeEntries (pDirInfo, driveInfo, treeDisplayer, totals, treeState);
    CHR (hr);

    //
    // Flush any trailing file entries that followed the last subdirectory
    // (or all entries if this directory had no child directories).
    //

    m_consolePtr->Flush();

    AccumulateTotals (pDirInfo, totals);

    //
    // Root directory: show per-dir summary + separator
    //

    if (level == IResultsDisplayer::EDirectoryLevel::Initial)
    {
        treeDisplayer.DisplayTreeRootSummary();
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::DisplayTreeEntries
//
//  Iterates through a directory node's sorted entries, displaying each
//  visible item (pruning invisible directories when a file mask is active)
//  and recursing into child directories for interleaved DFS output.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CMultiThreadedLister::DisplayTreeEntries (
    shared_ptr<CDirectoryInfo>   pDirInfo,
    const CDriveInfo           & driveInfo,
    CResultsDisplayerTree      & treeDisplayer,
    SListingTotals             & totals,
    STreeConnectorState        & treeState)
{
    HRESULT hr = S_OK;



    //
    // Build lookup from child directory name -> child CDirectoryInfo.  Both the
    // keys here and the entry names probed below come from FindFirstFile on this
    // directory, so they are the canonical on-disk bytes and compare directly.
    //

    ChildMap childMap;

    for (const auto & pChild : pDirInfo->m_vChildren)
    {
        childMap[pChild->m_dirPath.filename().wstring()] = pChild;
    }

    size_t cEntries = pDirInfo->m_vMatches.size();



    //
    // Display each visible entry interleaved with directory recursion.
    //
    // When a file mask is active, directories with no matching files
    // anywhere in their subtree are pruned from the display.  Use
    // a look-ahead pattern: for each entry, determine its visibility;
    // then peek forward to find the next visible entry to decide
    // whether this entry is last (└──) or middle (├──).
    //
    // When m_fTreePruningActive is false (no file mask, or all specs
    // are "*"), every directory is visible — no waiting needed.
    //

    for (size_t i = 0; i < cEntries; ++i)
    {
        if (StopRequested())
        {
            CHR (E_ABORT);
        }

        const FileInfo & entry   = pDirInfo->m_vMatches[i];
        bool             fIsDir  = (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        auto             itChild = childMap.find (entry.cFileName);

        //
        // Determine visibility of this entry.
        // Files are always visible, including archive files that matched a
        // file spec.  Directories and container-only archive files are
        // visible only if they (or a descendant) have matching files, or if
        // pruning is not active.
        //

        if (IsPrunable (entry) && m_fTreePruningActive)
        {
            if (itChild != childMap.end() && !WaitForTreeVisibility (itChild->second))
            {
                //
                // This entry was pruned (no matching descendants).  Adjust
                // the parent's subdirectory count so that AccumulateTotals
                // only counts displayed directories.  A container-only
                // archive file was never counted.
                //

                if (fIsDir)
                {
                    --pDirInfo->m_cSubDirectories;
                }

                continue;
            }
        }

        bool fIsLast = IsLastVisibleEntry (pDirInfo->m_vMatches, i, childMap);

        treeDisplayer.DisplaySingleEntry (entry, treeState, fIsLast, i);

        //
        // If the entry is a directory or an archive file, recurse into its
        // child node.
        //

        if (itChild != childMap.end())
        {
            hr = RecurseIntoChildDirectory (itChild->second, entry, fIsLast, driveInfo, treeDisplayer, totals, treeState);
            IGNORE_RETURN_VALUE (hr, S_OK);
        }
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::IsLastVisibleEntry
//
//  Looks ahead from the current index to determine whether the entry is
//  the last visible one in the list.  Files are always visible; directories
//  are visible only when pruning is inactive or the directory has matching
//  descendants.
//
////////////////////////////////////////////////////////////////////////////////

bool CMultiThreadedLister::IsLastVisibleEntry (
    const vector<FileInfo> & vMatches,
    size_t                   iCurrent,
    const ChildMap         & childMap)
{
    size_t cEntries = vMatches.size();



    for (size_t j = iCurrent + 1; j < cEntries; ++j)
    {
        const FileInfo & nextEntry = vMatches[j];

        if (!IsPrunable (nextEntry))
        {
            return false;   // A file follows — always visible
        }

        //
        // Next entry is a directory or a container-only archive file.  If
        // pruning is not active, it's visible.  Otherwise wait for its
        // visibility.
        //

        if (!m_fTreePruningActive)
        {
            return false;
        }

        auto itNext = childMap.find (nextEntry.cFileName);

        if (itNext != childMap.end() && WaitForTreeVisibility (itNext->second))
        {
            return false;
        }

        //
        // That directory was pruned — keep looking.
        //
    }

    return true;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::IsPrunable
//
//  Returns true for a tree entry that is hidden when tree pruning is active
//  and nothing below it matches: a directory, or an archive file that was
//  added only so the tree can descend into it.
//
////////////////////////////////////////////////////////////////////////////////

bool CMultiThreadedLister::IsPrunable (const FileInfo & entry)
{
    return CFlag::IsSet (entry.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) || entry.m_fArchiveContainer;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMultiThreadedLister::RecurseIntoChildDirectory
//
//  Recurses into a child directory during tree-mode display.  Checks
//  depth limit and reparse-point status before recursing.  Flushes the
//  console and saves/restores the displayer's per-directory state so that
//  column widths are correct across nesting levels.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CMultiThreadedLister::RecurseIntoChildDirectory (
    shared_ptr<CDirectoryInfo>   pChild,
    const FileInfo             & parentEntry,
    bool                         fIsLast,
    const CDriveInfo           & driveInfo,
    CResultsDisplayerTree      & treeDisplayer,
    SListingTotals             & totals,
    STreeConnectorState        & treeState)
{
    HRESULT                hr             = S_OK;

    bool                   fDepthLimited  = (m_cmdLinePtr->m_cMaxDepth > 0 &&
                                             treeState.Depth() + 1 >= m_cmdLinePtr->m_cMaxDepth);
    bool                   fIsReparse     = CFlag::IsSet (parentEntry.dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT) &&
                                            !pChild->m_fInArchive;

    CResultsDisplayerTree::SDirectoryDisplayState savedState;



    BAIL_OUT_IF (fDepthLimited || fIsReparse, S_OK);

    //
    // Flush everything displayed so far (including this directory entry)
    // before recursing so the user sees output immediately rather than
    // waiting for the entire subtree to finish.
    //
    // Save the parent's per-directory display state because
    // BeginDirectory in the child will overwrite the displayer's member
    // variables (field widths, sync root flag, owners).  Restore after
    // returning so that remaining entries in this directory render with
    // the correct column widths.
    //

    m_consolePtr->Flush();

    savedState = treeDisplayer.SaveDirectoryState();

    treeState.Push (!fIsLast);

    hr = PrintDirectoryTreeMode (pChild,
                                 driveInfo,
                                 treeDisplayer,
                                 IResultsDisplayer::EDirectoryLevel::Subdirectory,
                                 totals,
                                 treeState);
    treeState.Pop();
    IGNORE_RETURN_VALUE (hr, S_OK);

    treeDisplayer.RestoreDirectoryState (move (savedState));

Error:
    return hr;
}