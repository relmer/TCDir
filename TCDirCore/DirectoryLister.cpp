#include "pch.h"
#include "DirectoryLister.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "FileComparator.h"
#include "Flag.h"
#include "MultiThreadedLister.h"
#include "ResultsDisplayerBare.h"
#include "ResultsDisplayerNormal.h"
#include "ResultsDisplayerWide.h"
#include "UniqueFindHandle.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::CDirectoryLister
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

CDirectoryLister::CDirectoryLister (shared_ptr<CCommandLine> pCmdLine, shared_ptr<CConsole> pConsole, shared_ptr<CConfig> pConfig, bool fIconsActive) :
    m_cmdLinePtr        (pCmdLine),
    m_consolePtr        (pConsole),
    m_configPtr         (pConfig)
{
    // Create the appropriate displayer based on listing mode flags
    // Bare mode takes precedence over wide mode
    if (m_cmdLinePtr->m_fBareListing)
    {
        m_displayer = make_unique<CResultsDisplayerBare>(m_cmdLinePtr, m_consolePtr, m_configPtr, fIconsActive);
    }
    else if (m_cmdLinePtr->m_fWideListing)
    {
        m_displayer = make_unique<CResultsDisplayerWide>(m_cmdLinePtr, m_consolePtr, m_configPtr, fIconsActive);
    }
    else
    {
        m_displayer = make_unique<CResultsDisplayerNormal>(m_cmdLinePtr, m_consolePtr, m_configPtr, fIconsActive);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::~CDirectoryLister
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

CDirectoryLister::~CDirectoryLister (void)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::List
//
//  Entry point for listing a directory with file specs applied.
//  For multithreaded recursive mode, all specs are processed together in a
//  single pass with deduplication. Otherwise falls back to sequential
//  processing per spec.
//
////////////////////////////////////////////////////////////////////////////////

void CDirectoryLister::List (const MaskGroup & group)
{
    HRESULT                          hr = S_OK;
    std::error_code                  ec;
    const filesystem::path         & dirPath   = group.first;
    const vector<filesystem::path> & fileSpecs = group.second;



    //
    // Validate the directory exists
    //

    if (!filesystem::exists (dirPath, ec) || !filesystem::is_directory (dirPath, ec)) 
    {
        m_consolePtr->ColorPrintf (L"{Error}Error:   {InformationHighlight}%s{Error} does not exist\n", 
                                   dirPath.c_str());
        BAIL_OUT_IF (TRUE, HRESULT_FROM_WIN32 (ERROR_PATH_NOT_FOUND));
    }

    m_consolePtr->Puts (CConfig::EAttribute::Default, L"");

    {
        CDriveInfo driveInfo (dirPath);

        if (m_cmdLinePtr->m_fMultiThreaded && m_cmdLinePtr->m_fRecurse)
        {
            hr = ProcessDirectoryMultiThreaded (driveInfo, dirPath, fileSpecs, IResultsDisplayer::EDirectoryLevel::Initial);
            CHR (hr);
        }
        else
        {
            for (const auto & fileSpec : fileSpecs)
            {
                hr = ProcessDirectory (driveInfo, dirPath, fileSpec, IResultsDisplayer::EDirectoryLevel::Initial);
                IGNORE_RETURN_VALUE (hr, S_OK);
            }
        }
    }



Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::ProcessDirectory
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

HRESULT CDirectoryLister::ProcessDirectory (
    const CDriveInfo                   & driveInfo, 
    const filesystem::path             & dirPath, 
    const filesystem::path             & fileSpec, 
    IResultsDisplayer::EDirectoryLevel   level)
{
    HRESULT          hr = S_OK;
    CDirectoryInfo   di   (dirPath, fileSpec);



    //
    // Search for matching files and directories
    //     
    
    CollectMatchingFilesAndDirectories (dirPath, fileSpec, di);

    //
    // Count directories whose names matched the mask
    //

    m_totals.m_cDirectories += di.m_cSubDirectories;

    //
    // Sort the results using FileComparator
    //

    std::sort (di.m_vMatches.begin(), di.m_vMatches.end(), FileComparator (m_cmdLinePtr));

    //
    // Show the directory contents using the displayer
    //
    
    m_displayer->DisplayResults (driveInfo, di, level);

    //
    // Recurse into subdirectories 
    //

    if (m_cmdLinePtr->m_fRecurse)
    {
        hr = RecurseIntoSubdirectories (driveInfo, dirPath, fileSpec);
        CHR (hr);

        //
        // If this is the end of the initial directory, show the summary too
        //

        if (level == IResultsDisplayer::EDirectoryLevel::Initial)
        {
            m_displayer->DisplayRecursiveSummary (di, m_totals);
        }
    }



Error:    
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::CollectMatchingFilesAndDirectories
//
//  
// 
////////////////////////////////////////////////////////////////////////////////

HRESULT CDirectoryLister::CollectMatchingFilesAndDirectories (
    const std::filesystem::path & dirPath, 
    const std::filesystem::path & fileSpec, 
    CDirectoryInfo              & di)
{
    HRESULT          hr              = S_OK;
    filesystem::path pathAndFileSpec = dirPath / fileSpec;
    BOOL             fSuccess        = FALSE;
    UniqueFindHandle hFind;
    WIN32_FIND_DATA  wfd             = { 0 };



    hFind.reset (FindFirstFile (pathAndFileSpec.c_str(), &wfd));
    if (hFind.get() != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (!IsDots (wfd.cFileName))
            {
                //
                // If the required attributes are present and the excluded attributes are not 
                // then add the file to the list of matches for this directory
                //

                if (CFlag::IsSet (wfd.dwFileAttributes, m_cmdLinePtr->m_dwAttributesRequired) &&
                    CFlag::IsNotSet (wfd.dwFileAttributes, m_cmdLinePtr->m_dwAttributesExcluded))
                {
                    AddMatchToList (wfd, di, &m_totals);
                }
            }

            fSuccess = FindNextFile (hFind.get(), &wfd);
        }
        while (fSuccess);
    }


    
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::ProcessDirectoryMultiThreaded
//
//  Create multithreaded lister and process the directory tree with multiple
//  file specs applied together. Results from all specs are combined and
//  deduplicated in a single listing per directory.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CDirectoryLister::ProcessDirectoryMultiThreaded (
    const CDriveInfo                   & driveInfo,
    const filesystem::path             & dirPath,
    const vector<filesystem::path>     & fileSpecs,
    IResultsDisplayer::EDirectoryLevel level)
{
    HRESULT              hr             = S_OK;
    CMultiThreadedLister mtLister         (m_cmdLinePtr, m_consolePtr, m_configPtr);
    CDirectoryInfo       summaryDirInfo   (dirPath, fileSpecs);
    


    hr = mtLister.ProcessDirectoryMultiThreaded (driveInfo, 
                                                 dirPath, 
                                                 fileSpecs,
                                                 *m_displayer, 
                                                 level,
                                                 m_totals);
    CHR (hr);

    m_displayer->DisplayRecursiveSummary (summaryDirInfo, m_totals);

    

Error:
    return hr;
}




////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::RecurseIntoSubdirectories
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

HRESULT CDirectoryLister::RecurseIntoSubdirectories (
    const CDriveInfo       & driveInfo, 
    const filesystem::path & dirPath, 
    const filesystem::path & fileSpec)
{
    HRESULT          hr              = S_OK;
    filesystem::path pathAndFileSpec = dirPath / L"*";    
    BOOL             fSuccess        = FALSE;                    
    UniqueFindHandle hFind;
    WIN32_FIND_DATA  wfd             = { };                         

    

    //
    // Search for subdirectories to recurse into
    // 
    
    hFind.reset (FindFirstFile (pathAndFileSpec.c_str(), &wfd));
    CBR (hFind.get() != INVALID_HANDLE_VALUE);

    do 
    {
        //
        // Add directories, except "." and "..", to a list
        // of subdirectories we'll recurse into
        //
                
        if (!IsDots (wfd.cFileName))
        {
            if (CFlag::IsSet (wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
            {
                filesystem::path subdirPath = dirPath / wfd.cFileName;            
                hr = ProcessDirectory (driveInfo, subdirPath, fileSpec, IResultsDisplayer::EDirectoryLevel::Subdirectory);
                IGNORE_RETURN_VALUE (hr, S_OK);
            }
        }
            
        fSuccess = FindNextFile (hFind.get(), &wfd);
    }
    while (fSuccess);



Error:
    return hr;
}    





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::AddMatchToList
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

void CDirectoryLister::AddMatchToList (
    const WIN32_FIND_DATA & wfd, 
    CDirectoryInfo        & di, 
    SListingTotals        * pTotals)
{
    size_t   cchFileName = 0; 
    FileInfo fileEntry     (wfd);

    

    if (m_cmdLinePtr->m_fWideListing)
    {
        cchFileName = wcslen (wfd.cFileName);
    }
    
    if (CFlag::IsSet (wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
    {
        HandleDirectoryMatch (cchFileName, di);
    }
    else
    {
        HandleFileMatch (wfd, fileEntry, di, pTotals);
    }            

    if (m_cmdLinePtr->m_fWideListing)
    {
        if (cchFileName > di.m_cchLargestFileName)
        {
            di.m_cchLargestFileName = cchFileName;
        }
    }
    
    di.m_vMatches.push_back (move (fileEntry));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::HandleDirectoryMatch
//
//  Process a directory entry - update counters and handle wide listing format.
//
////////////////////////////////////////////////////////////////////////////////  

void CDirectoryLister::HandleDirectoryMatch (size_t & cchFileName, CDirectoryInfo & di)
{
    //
    // In wide directory listings, directories are shown inside brackets
    // so add space for them here.
    //

    if (m_cmdLinePtr->m_fWideListing)
    {                
        cchFileName += 2;  
    }
    
    ++di.m_cSubDirectories;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::HandleFileMatch
//
//  Process a file entry - update size counters and collect streams if needed.
//
////////////////////////////////////////////////////////////////////////////////  

void CDirectoryLister::HandleFileMatch (const WIN32_FIND_DATA & wfd, FileInfo & fileEntry, CDirectoryInfo & di, SListingTotals * pTotals)
{
    HRESULT        hr          = S_OK;
    ULARGE_INTEGER uliFileSize = { 0 };
    


    //
    // Get the two 32-bit halves into a convenient 64-bit type
    //
    
    uliFileSize.LowPart  = wfd.nFileSizeLow;
    uliFileSize.HighPart = wfd.nFileSizeHigh;
    
    if (uliFileSize.QuadPart > di.m_uliLargestFileSize.QuadPart)
    {
        di.m_uliLargestFileSize = uliFileSize;
    }
    
    di.m_uliBytesUsed.QuadPart += uliFileSize.QuadPart;        
    ++di.m_cFiles;

    if (pTotals)
    {
        pTotals->m_uliFileBytes.QuadPart += uliFileSize.QuadPart;
        ++pTotals->m_cFiles;
    }

    //
    // If showing streams, enumerate and collect alternate data streams
    //

    if (m_cmdLinePtr->m_fShowStreams)
    {
        hr = HandleFileMatchStreams (wfd, fileEntry, di, pTotals);
        IGNORE_RETURN_VALUE (hr, S_OK);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::HandleFileMatchStreams
//
//  Enumerate and collect alternate data streams for a file.
//
////////////////////////////////////////////////////////////////////////////////  

HRESULT CDirectoryLister::HandleFileMatchStreams (const WIN32_FIND_DATA & wfd, FileInfo & fileEntry, CDirectoryInfo & di, SListingTotals * pTotals)
{
    HRESULT                hr         = S_OK;
    filesystem::path       fullPath   = di.m_dirPath / wfd.cFileName;
    WIN32_FIND_STREAM_DATA streamData = {};
    UniqueFindHandle       hFind;



    hFind.reset (FindFirstStreamW (fullPath.c_str(), FindStreamInfoStandard, &streamData, 0));
    CWR (hFind.get() != INVALID_HANDLE_VALUE);

    do
    {
        SStreamInfo si;



        // Skip the default unnamed data stream (::$DATA)
        if (_wcsicmp (streamData.cStreamName, L"::$DATA") == 0)
        {
            continue;
        }

        if (static_cast<ULONGLONG>(streamData.StreamSize.QuadPart) > di.m_uliLargestFileSize.QuadPart)
        {
            di.m_uliLargestFileSize.QuadPart = streamData.StreamSize.QuadPart;
        }

        ++di.m_cStreams;
        di.m_uliStreamBytesUsed.QuadPart += streamData.StreamSize.QuadPart;

        // Track global stream totals for recursive summary
        if (pTotals)
        {
            ++pTotals->m_cStreams;
            pTotals->m_uliStreamBytes.QuadPart += streamData.StreamSize.QuadPart;
        }

        // Store stream info for display phase - strip ":$DATA" suffix
        si.m_strName = streamData.cStreamName;
        si.m_liSize  = streamData.StreamSize;

        if (si.m_strName.length () > 6 && 
            _wcsicmp (si.m_strName.c_str () + si.m_strName.length () - 6, L":$DATA") == 0)
        {
            si.m_strName.resize (si.m_strName.length () - 6);
        }

        fileEntry.m_vStreams.push_back (move (si));
    }
    while (FindNextStreamW (hFind.get(), &streamData));



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::IsDots
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

/*static*/ bool CDirectoryLister::IsDots (LPCWSTR pszFileName)
{
    static const WCHAR kchDot  = L'.';
    static const WCHAR kchNull = L'\0';
    bool               fDots   = false;

    

    if (pszFileName[0] == kchDot)
    {
        if (pszFileName[1] == kchNull)
        {
            fDots = true;
        }
        else if (pszFileName[1] == kchDot)
        {
            if (pszFileName[2] == kchNull)
            {
                fDots = true;
            }
        }
    }

    return fDots;
}
