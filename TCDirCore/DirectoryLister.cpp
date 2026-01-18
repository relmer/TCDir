#include "pch.h"
#include "DirectoryLister.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "FileComparator.h"
#include "Flag.h"
#include "MultiThreadedLister.h"
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

CDirectoryLister::CDirectoryLister (shared_ptr<CCommandLine> pCmdLine, shared_ptr<CConsole> pConsole, shared_ptr<CConfig> pConfig) :
    m_cmdLinePtr        (pCmdLine),
    m_consolePtr        (pConsole),
    m_configPtr         (pConfig),
    m_cFilesFound       (0),
    m_cDirectoriesFound (0)
{
    m_uliSizeOfAllFilesFound.QuadPart = 0;

    // Create the appropriate displayer based on wide listing flag
    if (m_cmdLinePtr->m_fWideListing)
    {
        m_displayer = make_unique<CResultsDisplayerWide>(m_cmdLinePtr, m_consolePtr, m_configPtr);
    }
    else
    {
        m_displayer = make_unique<CResultsDisplayerNormal>(m_cmdLinePtr, m_consolePtr, m_configPtr);
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
//  
//
////////////////////////////////////////////////////////////////////////////////  

void CDirectoryLister::List (const wstring & mask)
{
    HRESULT          hr           = S_OK;
    filesystem::path absolutePath   (filesystem::absolute(mask));
    bool             exists       = false;
    std::error_code  ec;
    filesystem::path dirPath;
    filesystem::path fileSpec;    
    
    

    //
    // If the mask is a directory, append the default mask to it
    // 

    exists = filesystem::exists (absolutePath, ec);
    if (exists && filesystem::is_directory (absolutePath))
    {
        absolutePath /= L"*";
    }

    dirPath  = absolutePath.parent_path() / "";
    fileSpec = absolutePath.filename();

    // 
    // At this point dirPath should be a directory, so we can validate it
    //

    exists = filesystem::exists (dirPath, ec);
    if (!exists || !filesystem::is_directory (dirPath)) 
    {
        m_consolePtr->Printf (CConfig::EAttribute::Error,                L"Error:  ");
        m_consolePtr->Printf (CConfig::EAttribute::InformationHighlight, L"%s", dirPath.c_str());
        m_consolePtr->Puts   (CConfig::EAttribute::Error,                L" does not exist");
        
        BAIL_OUT_IF (TRUE, HRESULT_FROM_WIN32 (ERROR_PATH_NOT_FOUND));
    }

    //
    // Process a directory
    //

    m_consolePtr->Puts (CConfig::EAttribute::Default, L"");

    {
        CDriveInfo driveInfo (dirPath);

        if (m_cmdLinePtr->m_fMultiThreaded && m_cmdLinePtr->m_fRecurse)
        {
            hr = ProcessDirectoryMultiThreaded (driveInfo, dirPath, fileSpec, IResultsDisplayer::EDirectoryLevel::Initial);
            CHR (hr);
        }
        else
        {
            hr = ProcessDirectory (driveInfo, dirPath, fileSpec, IResultsDisplayer::EDirectoryLevel::Initial);
            CHR (hr);
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

HRESULT CDirectoryLister::ProcessDirectory (const CDriveInfo & driveInfo, 
                                            const filesystem::path & dirPath, 
                                            const filesystem::path & fileSpec, 
                                            IResultsDisplayer::EDirectoryLevel level)
{
    HRESULT          hr = S_OK;
    CDirectoryInfo   di   (dirPath, fileSpec);



    //
    // Search for matching files and directories
    //     
    
    CollectMatchingFilesAndDirectories (dirPath, fileSpec, di);

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
            m_displayer->DisplayRecursiveSummary (di, m_cFilesFound, m_cDirectoriesFound, m_uliSizeOfAllFilesFound);
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

HRESULT CDirectoryLister::CollectMatchingFilesAndDirectories (const std::filesystem::path & dirPath, 
                                                              const std::filesystem::path & fileSpec, 
                                                              CDirectoryInfo & di)
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
                    hr = AddMatchToList (wfd, &di);
                    CHR (hr);
                }
            }

            fSuccess = FindNextFile (hFind.get(), &wfd);
        }
        while (fSuccess);
    }


    
Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::ProcessDirectoryMultiThreaded
//
//  Create multithreaded lister and process the directory tree
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CDirectoryLister::ProcessDirectoryMultiThreaded (const CDriveInfo & driveInfo,
                                                         const filesystem::path & dirPath,
                                                         const filesystem::path & fileSpec,
                                                         IResultsDisplayer::EDirectoryLevel level)
{
    HRESULT              hr             = S_OK;
    CMultiThreadedLister mtLister         (m_cmdLinePtr, m_consolePtr, m_configPtr);
    CDirectoryInfo       summaryDirInfo   (dirPath, fileSpec);
    


    hr = mtLister.ProcessDirectoryMultiThreaded (driveInfo, 
                                                 dirPath, 
                                                 fileSpec,
                                                 *m_displayer, 
                                                 level,
                                                 m_uliSizeOfAllFilesFound,
                                                 m_cFilesFound,
                                                 m_cDirectoriesFound);
    CHR (hr);

    m_displayer->DisplayRecursiveSummary (summaryDirInfo, m_cFilesFound, m_cDirectoriesFound, m_uliSizeOfAllFilesFound);

    

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

HRESULT CDirectoryLister::RecurseIntoSubdirectories (const CDriveInfo & driveInfo, const filesystem::path & dirPath, const filesystem::path & fileSpec)
{
    HRESULT          hr              = S_OK;
    filesystem::path pathAndFileSpec = dirPath / L"*";    
    BOOL             fSuccess;                    
    UniqueFindHandle hFind;
    WIN32_FIND_DATA  wfd;                         

    

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

HRESULT CDirectoryLister::AddMatchToList (const WIN32_FIND_DATA & wfd, __in CDirectoryInfo * pdi)
{
    HRESULT hr           = S_OK;
    size_t  cchFileName  = 0; 

    

    if (m_cmdLinePtr->m_fWideListing)
    {
        cchFileName = wcslen (wfd.cFileName);
    }
    
    if (CFlag::IsSet (wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
    {
        //
        // In wide directory listings, directories are shown inside brackets
        // so add space for them here.
        //
        if (m_cmdLinePtr->m_fWideListing)
        {                
            cchFileName += 2;  
        }
        
        ++pdi->m_cSubDirectories;
        ++m_cDirectoriesFound;
    }
    else
    {
        ULARGE_INTEGER uliFileSize = { 0 };
        
        //
        // Get the two 32-bit halves into a convenient 64-bit type
        //
        
        uliFileSize.LowPart  = wfd.nFileSizeLow;
        uliFileSize.HighPart = wfd.nFileSizeHigh;
        
        if (uliFileSize.QuadPart > pdi->m_uliLargestFileSize.QuadPart)
        {
            pdi->m_uliLargestFileSize = uliFileSize;
        }
        
        pdi->m_uliBytesUsed.QuadPart += uliFileSize.QuadPart;        
        ++pdi->m_cFiles;

        m_uliSizeOfAllFilesFound.QuadPart += uliFileSize.QuadPart;
        ++m_cFilesFound;
    }            

    if (m_cmdLinePtr->m_fWideListing)
    {
        if (cchFileName > pdi->m_cchLargestFileName)
        {
            pdi->m_cchLargestFileName = cchFileName;
        }
    }

    
    pdi->m_vMatches.push_back (wfd);



//Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::IsDots
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

BOOL CDirectoryLister::IsDots(LPCWSTR pszFileName)
{
    static const WCHAR kchDot = L'.';
    static const WCHAR kchNull = L'\0';
    BOOL               fDots = FALSE;

    

    if (pszFileName[0] == kchDot)
    {
        if (pszFileName[1] == kchNull)
        {
            fDots = TRUE;
        }
        else if (pszFileName[1] == kchDot)
        {
            if (pszFileName[2] == kchNull)
            {
                fDots = TRUE;
            }
        }
    }

    return fDots;
}
