#include "pch.h"
#include "MultiThreadedLister.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "DriveInfo.h"
#include "FileComparator.h"
#include "Flag.h"
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
//  Main entry point for multithreaded directory enumeration
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CMultiThreadedLister::ProcessDirectoryMultiThreaded (const CDriveInfo & driveInfo,
                                                             const filesystem::path & dirPath,
                                                             const filesystem::path & fileSpec,
                                                             IResultsDisplayer & displayer,
                                                             IResultsDisplayer::EDirectoryLevel level,
                                                             SListingTotals & totals)
{
    HRESULT hr = S_OK;

    

    // Create root directory info node
    auto pRootDirInfo = make_shared<CDirectoryInfo> (dirPath, fileSpec);

    // Initialize work queue with root
    m_workQueue.Push (WorkItem { pRootDirInfo });

    // Create worker threads
    const size_t numThreads = max(1u, jthread::hardware_concurrency());

    for (size_t i = 0; i < numThreads; ++i)
    {
        m_workers.emplace_back ([this](stop_token st) { WorkerThreadFunc (st); });
    }

    // Start consuming immediately (streaming output)
    hr = PrintDirectoryTree (pRootDirInfo, 
                             driveInfo, 
                             displayer, 
                             level,
                             totals);
    CHR (hr);



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
    HRESULT          hr               = S_OK;
    WIN32_FIND_DATA  wfd              = { 0 };
    UniqueFindHandle hFind;
    filesystem::path pathAndFileSpec;
    DWORD            dwError          = 0;
    
    

    // Build search path for files matching the pattern
    try
    {
        pathAndFileSpec = pDirInfo->m_dirPath / pDirInfo->m_fileSpec;
    }
    catch (const filesystem::filesystem_error &)
    {
        return E_INVALIDARG;
    }

    // Search for files matching the file spec
    hFind.reset (FindFirstFile (pathAndFileSpec.c_str(), &wfd));
    
    if (hFind.get() != INVALID_HANDLE_VALUE)
    {
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

            // Check if this entry should be displayed based on attribute filters
            if (CFlag::IsSet    (wfd.dwFileAttributes, m_cmdLinePtr->m_dwAttributesRequired) &&
                CFlag::IsNotSet (wfd.dwFileAttributes, m_cmdLinePtr->m_dwAttributesExcluded))
            {
                lock_guard<mutex> lock (pDirInfo->m_mutex);
                AddMatchToList (wfd, pDirInfo.get(), nullptr);
            }

        } 
        while (FindNextFile (hFind.get(), &wfd));

        // Check if loop ended due to error or naturally
        dwError = GetLastError();
        if (dwError != ERROR_NO_MORE_FILES)
        {
            CHRA (HRESULT_FROM_WIN32 (dwError));
        }
    }

    // Now search for subdirectories if recursion is enabled
    // This is a separate search using "*" to find all directories
    if (m_cmdLinePtr->m_fRecurse)
    {
        filesystem::path pathForDirs = pDirInfo->m_dirPath / L"*";
        hFind.reset (FindFirstFile (pathForDirs.c_str(), &wfd));
        
        if (hFind.get() != INVALID_HANDLE_VALUE)
        {
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

                // Enqueue all directories for recursion
                if (CFlag::IsSet (wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
                {
                    lock_guard<mutex> lock (pDirInfo->m_mutex);
                    EnqueueChildDirectory (wfd, pDirInfo);
                }
            }
            while (FindNextFile (hFind.get(), &wfd));
        }
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
    auto             pChild     = make_shared<CDirectoryInfo> (subdirPath, pDirInfo->m_fileSpec);

    pDirInfo->m_vChildren.push_back (pChild);

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

HRESULT CMultiThreadedLister::PrintDirectoryTree (shared_ptr<CDirectoryInfo> pDirInfo,
                                                  const CDriveInfo & driveInfo,
                                                  IResultsDisplayer & displayer,
                                                  IResultsDisplayer::EDirectoryLevel level,
                                                  SListingTotals & totals)
{
    HRESULT hr = S_OK;

    

    // Wait for this node to complete enumeration
    {
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
            m_consolePtr->Printf (CConfig::EAttribute::Error,
                                  L"  Error accessing directory: %s: HRESULT 0x%08X\n",
                                  pDirInfo->m_dirPath.c_str(), 
                                  pDirInfo->m_hr);
            CHR (pDirInfo->m_hr);
        }
    }

    // Sort the results using FileComparator
    {
        lock_guard<mutex> lock (pDirInfo->m_mutex);

        std::sort (pDirInfo->m_vMatches.begin(), pDirInfo->m_vMatches.end(), 
                   FileComparator (m_cmdLinePtr));
    }

    // Display the results
    displayer.DisplayResults (driveInfo, *pDirInfo, level);

    // Update global counters
    {
        lock_guard<mutex> lock (pDirInfo->m_mutex);

        totals.m_cFiles                  += pDirInfo->m_cFiles;
        totals.m_cDirectories            += pDirInfo->m_cSubDirectories;
        totals.m_uliFileBytes.QuadPart   += pDirInfo->m_uliBytesUsed.QuadPart;
        totals.m_cStreams                += pDirInfo->m_cStreams;
        totals.m_uliStreamBytes.QuadPart += pDirInfo->m_uliStreamBytesUsed.QuadPart;
    }

    // Recurse into children
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
