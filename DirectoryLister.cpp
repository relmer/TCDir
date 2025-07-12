#include "pch.h"
#include "DirectoryLister.h"

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

    CFileInfo::s_pCmdLine = m_pCmdLine;
    
    m_displayer = make_unique<CResultsDisplayerBase>(m_pCmdLine, m_pConsole, m_pConfig);
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
    HRESULT hr                          = S_OK;
    int     nDrive                      = 0;    
    BOOL    fSuccess                    = TRUE;                   
    WCHAR   szWellFormedPath[MAX_PATH]  = { 0 };
    WCHAR   szPath[MAX_PATH]            = { 0 };           
    WCHAR   szFileSpec[MAX_PATH]        = { 0 };     
  


    //
    // If the path does not begin with a drive letter, we'll use
    // the current drive.
    //
    
    nDrive = PathGetDriveNumber (pszMask);
    if (nDrive == -1)
    {
        GetCurrentDirectory (ARRAYSIZE (szWellFormedPath), szWellFormedPath);

        //
        // If the path begins with a backslash, it's an absolute path.
        // We'll prepend only the drive letter, colon, and root dir to it.
        //
        
        if (*pszMask == L'\\')
        {
            PathStripToRoot (szWellFormedPath);
        }             
        
        PathAppend (szWellFormedPath, pszMask);
    }                                
    else                             
    {                                
        StringCchCopy (szWellFormedPath, ARRAYSIZE (szWellFormedPath), pszMask);
    }                                
    
    //
    // If the mask is just a path, append the default mask to it
    // 

    if (PathIsDirectory (szWellFormedPath))
    {
        PathAppend (szWellFormedPath, L"*");
    }

    //
    // Get rid of any relative path references
    //

    fSuccess = PathCanonicalize (szPath, szWellFormedPath);
    CBRA (fSuccess);

    //
    // For recursive searches, we'll need to separate the path and mask portions
    //

    StringCchCopy      (szFileSpec,  ARRAYSIZE (szFileSpec), szPath);
    PathStripPath      (szFileSpec); 
    PathRemoveFileSpec (szPath);     

    //
    // Process a directory
    //

    m_pConsole->Puts (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"");

    hr = ProcessDirectory (szPath, szFileSpec, CResultsDisplayerBase::EDirectoryLevel::Initial);
    CHR (hr);

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

HRESULT CDirectoryLister::ProcessDirectory (LPCWSTR pszPath, LPCWSTR pszFileSpec, CResultsDisplayerBase::EDirectoryLevel level)
{
    HRESULT          hr                          = S_OK;
    BOOL             fSuccess;                    
    WCHAR            szPathAndFileSpec[MAX_PATH]; 
    UniqueFindHandle hFind;
    CFileInfo        fileInfo;                         
    CDirectoryInfo   di;



    //
    // Store the path and filespec
    //

    di.m_pszPath     = pszPath;
    di.m_pszFileSpec = pszFileSpec;
    

    //
    // Create the full path + filespec
    //

    StringCchCopy (szPathAndFileSpec, ARRAYSIZE (szPathAndFileSpec), pszPath);
    fSuccess = PathAppend (szPathAndFileSpec, pszFileSpec);
    CBRA (fSuccess);
    

    //
    // Search for matching files and directories
    // 
    
    hFind.reset (FindFirstFile (szPathAndFileSpec, &fileInfo));
    if (hFind.get() != INVALID_HANDLE_VALUE)
    {
        do 
        {
            if (!IsDots (fileInfo.cFileName))
            {
                //
                // If the required attributes are present and the excluded attributes are not 
                // then add the file to the list of matches for this directory
                //
                
                if (CFlag::IsSet    (fileInfo.dwFileAttributes, m_pCmdLine->m_dwAttributesRequired) && 
                    CFlag::IsNotSet (fileInfo.dwFileAttributes, m_pCmdLine->m_dwAttributesExcluded))
                {
                    hr = AddMatchToList (&fileInfo, &di);
                    CHR (hr);
                }
            }
                
            fSuccess = FindNextFile (hFind.get(), &fileInfo);
        }
        while (fSuccess);        
    }

    //
    // Sort the results
    //

    std::sort (di.m_vMatches.begin(), di.m_vMatches.end());

    //
    // Show the directory contents using the displayer
    //
    
    m_displayer->DisplayResults (&di, level);

    //
    // Recurse into subdirectories 
    //

    if (m_pCmdLine->m_fRecurse)
    {
        hr = RecurseIntoSubdirectories (pszPath, pszFileSpec);
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

HRESULT CDirectoryLister::RecurseIntoSubdirectories (LPCWSTR pszPath, LPCWSTR pszFileSpec)
{
    HRESULT          hr                           = S_OK;
    BOOL             fSuccess;                    
    WCHAR            szPathAndFileSpec[MAX_PATH]; 
    UniqueFindHandle hFind;
    WIN32_FIND_DATA  wfd;                         

   

    //
    // Create the full path + filespec
    //

    StringCchCopy (szPathAndFileSpec, ARRAYSIZE (szPathAndFileSpec), pszPath);
    fSuccess = PathAppend (szPathAndFileSpec, L"*");
    CBRA (fSuccess);
    

    //
    // Search for subdirectories to recurse into
    // 
    
    hFind.reset (FindFirstFile (szPathAndFileSpec, &wfd));
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
                WCHAR szSubdirPath[MAX_PATH];
            
            
                StringCchCopy (szSubdirPath, ARRAYSIZE (szSubdirPath), pszPath);
            
                fSuccess = PathAppend (szSubdirPath, wfd.cFileName);
                CBRA (fSuccess);
            
                ProcessDirectory (szSubdirPath, pszFileSpec, CResultsDisplayerBase::EDirectoryLevel::Subdirectory);
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

HRESULT CDirectoryLister::AddMatchToList (__in CFileInfo * pwfd, __in CDirectoryInfo * pdi)
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
