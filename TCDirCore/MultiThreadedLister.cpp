#include "pch.h"
#include "MultiThreadedLister.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "DriveInfo.h"
#include "FileComparator.h"
#include "Flag.h"
#include "ResultsDisplayerTree.h"
#include "UniqueFindHandle.h"





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
    filesystem::path       pathAndFileSpec;
    UniqueFindHandle       hFind;
    WIN32_FIND_DATA        wfd              = { 0 };
    unordered_set<wstring> seenFilenames;
    DWORD                  dwError          = 0;




    //
    // Iterate through each file spec
    //

    for (const auto & fileSpec : pDirInfo->m_vFileSpecs)
    {
        // Build search path for files matching this pattern
        try
        {
            pathAndFileSpec = pDirInfo->m_dirPath / fileSpec;
        }
        catch (const filesystem::filesystem_error &)
        {
            continue;  // Skip this spec on error
        }

        hFind.reset (FindFirstFile (pathAndFileSpec.c_str(), &wfd));
        
        if (hFind.get() == INVALID_HANDLE_VALUE)
        {
            continue;  // No matches for this spec, try next
        }

        do
        {
            if (StopRequested())
            {
                break;
            }

            // Skip "." and ".."
            if (IsDots (wfd.cFileName))
            {
                continue;
            }

            //
            // Deduplicate: skip if we've already seen this filename
            //

            wstring lowerName = ToLower (wfd.cFileName);
            if (seenFilenames.contains (lowerName))
            {
                continue;
            }
            seenFilenames.insert (lowerName);

            // Check if this entry should be displayed based on attribute filters
            if (CFlag::IsSet    (wfd.dwFileAttributes, m_cmdLinePtr->m_dwAttributesRequired) &&
                CFlag::IsNotSet (wfd.dwFileAttributes, m_cmdLinePtr->m_dwAttributesExcluded))
            {
                lock_guard<mutex> lock (pDirInfo->m_mutex);

                AddMatchToList (wfd, *pDirInfo, nullptr);
            }

        } 
        while (FindNextFile (hFind.get(), &wfd));

        // Check if loop ended due to error or naturally
        dwError = GetLastError();
        if (dwError != ERROR_NO_MORE_FILES && dwError != ERROR_FILE_NOT_FOUND)
        {
            CHRA (HRESULT_FROM_WIN32 (dwError));
        }

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
    HRESULT          hr          = S_OK;
    filesystem::path pathForDirs;
    UniqueFindHandle hFind;
    WIN32_FIND_DATA  wfd         = { 0 };
    DWORD            dwError     = 0;

    //
    // In tree mode, directories must also appear in m_vMatches so they are
    // visible in the interleaved tree display. Build a set of directory names
    // already present (from EnumerateMatchingFiles) to avoid duplicates.
    //

    unordered_set<wstring> seenDirs;



    if (m_cmdLinePtr->m_fTree)
    {
        lock_guard<mutex> lock (pDirInfo->m_mutex);

        for (const auto & entry : pDirInfo->m_vMatches)
        {
            if (CFlag::IsSet (entry.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
            {
                seenDirs.insert (ToLower (entry.cFileName));
            }
        }
    }

    // Search using "*" to find all directories
    pathForDirs = pDirInfo->m_dirPath / L"*";
    hFind.reset (FindFirstFile (pathForDirs.c_str(), &wfd));
    BAIL_OUT_IF (hFind.get() == INVALID_HANDLE_VALUE, S_OK);

    do
    {
        if (StopRequested())
        {
            break;
        }

        if (IsDots (wfd.cFileName))
        {
            continue;
        }

        // Enqueue directories for recursion
        if (CFlag::IsSet (wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
        {
            lock_guard<mutex> lock (pDirInfo->m_mutex);

            EnqueueChildDirectory (wfd, pDirInfo);

            //
            // In tree mode, add every directory to m_vMatches so the tree
            // display can show it and recurse into it.  Skip if the directory
            // was already added by EnumerateMatchingFiles (e.g., a directory
            // whose name matched the file spec like *.cpp matching "foo.cpp/").
            //

            if (m_cmdLinePtr->m_fTree)
            {
                wstring lowerName = ToLower (wfd.cFileName);

                if (!seenDirs.contains (lowerName))
                {
                    seenDirs.insert (lowerName);
                    AddMatchToList (wfd, *pDirInfo, nullptr);
                }
            }
        }
    }
    while (FindNextFile (hFind.get(), &wfd));

    // Check if loop ended due to error or naturally
    dwError = GetLastError();
    if (dwError != ERROR_NO_MORE_FILES)
    {
        CHRA (HRESULT_FROM_WIN32 (dwError));
    }


Error:
    return hr;
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
        m_consolePtr->ColorPrintf (L"{Error}  Error accessing directory: {InformationHighlight}%s{Error}: HRESULT 0x%08X\n",
                                    pDirInfo->m_dirPath.c_str(), 
                                    pDirInfo->m_hr);
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

    std::sort (pDirInfo->m_vMatches.begin(), pDirInfo->m_vMatches.end(), 
               FileComparator (m_cmdLinePtr, m_cmdLinePtr->m_fTree));
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

    {
        //
        // Build lookup from lowercase filename → child CDirectoryInfo
        //

        unordered_map<wstring, shared_ptr<CDirectoryInfo>> childMap;

        for (const auto & pChild : pDirInfo->m_vChildren)
        {
            childMap[ToLower (pChild->m_dirPath.filename().wstring())] = pChild;
        }

        //
        // Display each entry interleaved with directory recursion
        //

        size_t cEntries = pDirInfo->m_vMatches.size();

        for (size_t i = 0; i < cEntries; ++i)
        {
            if (StopRequested())
            {
                CHR (E_ABORT);
            }

            const FileInfo & entry   = pDirInfo->m_vMatches[i];
            bool             fIsLast = (i == cEntries - 1);
            bool             fIsDir  = (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

            treeDisplayer.DisplaySingleEntry (entry, treeState, fIsLast, i);

            //
            // If the entry is a directory, find its child node and recurse
            // (unless depth limit has been reached)
            //

            if (fIsDir)
            {
                bool fDepthLimited = (m_cmdLinePtr->m_cMaxDepth > 0 &&
                                      treeState.Depth() + 1 >= m_cmdLinePtr->m_cMaxDepth);

                bool fIsReparse = CFlag::IsSet (entry.dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT);

                if (!fDepthLimited && !fIsReparse)
                {
                    auto it = childMap.find (ToLower (wstring (entry.cFileName)));

                    if (it != childMap.end())
                    {
                        //
                        // Flush everything displayed so far (including this
                        // directory entry) before recursing so the user sees
                        // output immediately rather than waiting for the
                        // entire subtree to finish.
                        //
                        // Save the parent's per-directory display state
                        // because BeginDirectory in the child will overwrite
                        // the displayer's member variables (field widths,
                        // sync root flag, owners).  Restore after returning
                        // so that remaining entries in this directory render
                        // with the correct column widths.
                        //

                        m_consolePtr->Flush();

                        auto savedState = treeDisplayer.SaveDirectoryState();

                        treeState.Push (!fIsLast);

                        hr = PrintDirectoryTreeMode (it->second,
                                                     driveInfo,
                                                     treeDisplayer,
                                                     IResultsDisplayer::EDirectoryLevel::Subdirectory,
                                                     totals,
                                                     treeState);
                        treeState.Pop();
                        IGNORE_RETURN_VALUE (hr, S_OK);

                        treeDisplayer.RestoreDirectoryState (move (savedState));
                    }
                }
            }
        }
    }

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
        treeDisplayer.DisplayTreeRootSummary (*pDirInfo);
    }



Error:
    return hr;
}