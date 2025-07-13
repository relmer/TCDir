#include "pch.h"
#include "DirectoryLister.h"
#include "ResultsDisplayerNormal.h"
#include "ResultsDisplayerWide.h"
#include "FileComparator.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "Flag.h"
#include "UniqueFindHandle.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::CDirectoryLister
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CDirectoryLister::CDirectoryLister (__in CCommandLine * pCmdLine, __in CConsole * pConsole, __in CConfig * pConfig) :
    m_pCmdLine          (pCmdLine),
    m_pConsole          (pConsole),
    m_pConfig           (pConfig),
    m_cFilesFound       (0),
    m_cDirectoriesFound (0)
{
    m_uliSizeOfAllFilesFound.QuadPart = 0;

    // Create the appropriate displayer based on wide listing flag
    if (m_pCmdLine->m_fWideListing)
    {
        m_displayer = make_unique<CResultsDisplayerWide>(m_pCmdLine, m_pConsole, m_pConfig);
    }
    else
    {
        m_displayer = make_unique<CResultsDisplayerNormal>(m_pCmdLine, m_pConsole, m_pConfig);
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

void CDirectoryLister::List (LPCWSTR pszMask)
{
    HRESULT          hr             = S_OK;
    filesystem::path inputPath        (pszMask);
    filesystem::path wellFormedPath;
    


    if (!inputPath.has_root_name())
    {
        // No drive letter, use current directory
        wellFormedPath = filesystem::current_path();
        
        //
        // If the path begins with a backslash, it's an absolute path.
        // We'll use only the root path portion.
        //
        
        if (*pszMask == L'\\')
        {
            wellFormedPath = wellFormedPath.root_path();
        }             
        
        wellFormedPath /= inputPath;
    }                                
    else                             
    {                                
        wellFormedPath = inputPath;
    }
    
    //
    // If the mask is just a path, append the default mask to it
    // 

    if (filesystem::is_directory(wellFormedPath))
    {
        wellFormedPath /= L"*";
    }

    //
    // Get rid of any relative path references and separate path and mask portions
    //

    filesystem::path canonicalPath = filesystem::weakly_canonical (wellFormedPath);
    filesystem::path dirPath       = canonicalPath.parent_path();
    filesystem::path fileSpec      = canonicalPath.filename();    

    //
    // Process a directory
    //

    m_pConsole->Puts (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"");

    {
        CDriveInfo driveInfo (dirPath);

        hr = ProcessDirectory (driveInfo, dirPath, fileSpec, CResultsDisplayerBase::EDirectoryLevel::Initial);
        CHR (hr);
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

HRESULT CDirectoryLister::ProcessDirectory (const CDriveInfo & driveInfo, filesystem::path dirPath, filesystem::path fileSpec, CResultsDisplayerBase::EDirectoryLevel level)
{
    HRESULT          hr              = S_OK;
    CDirectoryInfo   di                (dirPath, fileSpec);
    filesystem::path pathAndFileSpec = dirPath / fileSpec;
    BOOL             fSuccess;                    
    UniqueFindHandle hFind;
    WIN32_FIND_DATA  wfd;                         



    //
    // Search for matching files and directories
    // 
    
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
                
                if (CFlag::IsSet    (wfd.dwFileAttributes, m_pCmdLine->m_dwAttributesRequired) && 
                    CFlag::IsNotSet (wfd.dwFileAttributes, m_pCmdLine->m_dwAttributesExcluded))
                {
                    hr = AddMatchToList (&wfd, &di);
                    CHR (hr);
                }
            }
                
            fSuccess = FindNextFile (hFind.get(), &wfd);
        }
        while (fSuccess);        
    }

    //
    // Sort the results using FileComparator
    //

    std::sort (di.m_vMatches.begin(), di.m_vMatches.end(), FileComparator (m_pCmdLine));

    //
    // Show the directory contents using the displayer
    //
    
    m_displayer->DisplayResults (driveInfo, &di, level);

    //
    // Recurse into subdirectories 
    //

    if (m_pCmdLine->m_fRecurse)
    {
        hr = RecurseIntoSubdirectories (driveInfo, dirPath, fileSpec);
        CHR (hr);

        //
        // If this is the end of the initial directory, show the summary too
        //

        if (level == CResultsDisplayerBase::EDirectoryLevel::Initial)
        {
            m_displayer->DisplayListingSummary (&di, m_cFilesFound, m_cDirectoriesFound, m_uliSizeOfAllFilesFound);
        }
    }

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

HRESULT CDirectoryLister::RecurseIntoSubdirectories (const CDriveInfo & driveInfo, filesystem::path dirPath, filesystem::path fileSpec)
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
                ProcessDirectory (driveInfo, subdirPath, fileSpec, CResultsDisplayerBase::EDirectoryLevel::Subdirectory);
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

HRESULT CDirectoryLister::AddMatchToList (__in WIN32_FIND_DATA * pwfd, __in CDirectoryInfo * pdi)
{
    HRESULT hr           = S_OK;
    size_t  cchFileName  = 0; 



    if (m_pCmdLine->m_fWideListing)
    {
        cchFileName = wcslen (pwfd->cFileName);
    }
    
    if (CFlag::IsSet (pwfd->dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
    {
        //
        // In wide directory listings, directories are shown inside brackets
        // so add space for them here.
        //
        if (m_pCmdLine->m_fWideListing)
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
        
        uliFileSize.LowPart  = pwfd->nFileSizeLow;
        uliFileSize.HighPart = pwfd->nFileSizeHigh;
        
        if (uliFileSize.QuadPart > pdi->m_uliLargestFileSize.QuadPart)
        {
            pdi->m_uliLargestFileSize = uliFileSize;
        }
        
        pdi->m_uliBytesUsed.QuadPart += uliFileSize.QuadPart;        
        ++pdi->m_cFiles;

        m_uliSizeOfAllFilesFound.QuadPart += uliFileSize.QuadPart;
        ++m_cFilesFound;
    }            

    if (m_pCmdLine->m_fWideListing)
    {
        if (cchFileName > pdi->m_cchLargestFileName)
        {
            pdi->m_cchLargestFileName = cchFileName;
        }
    }

    
    pdi->m_vMatches.push_back (*pwfd);

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
