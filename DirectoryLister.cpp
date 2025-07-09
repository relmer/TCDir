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
//  CDirectoryLister::operator()
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

    hr = ProcessDirectory (szPath, szFileSpec, EDirectoryLevel::Initial);
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

HRESULT CDirectoryLister::ProcessDirectory (LPCWSTR pszPath, LPCWSTR pszFileSpec, EDirectoryLevel level)
{
    HRESULT                 hr                          = S_OK;
    BOOL                    fSuccess;                    
    WCHAR                   szPathAndFileSpec[MAX_PATH]; 
    UniqueFindHandle        hFind;
    CFileInfo               fileInfo;                         
    CDirectoryInfo          di;



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
    // Show the directory contents
    //
    
    DisplayResults (&di, level);           

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

        if (level == EDirectoryLevel::Initial)
        {
            DisplayListingSummary (&di);
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
            
                ProcessDirectory (szSubdirPath, pszFileSpec, EDirectoryLevel::Subdirectory);
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
//  CDirectoryLister::DisplayResults
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CDirectoryLister::DisplayResults (__in CDirectoryInfo * pdi, EDirectoryLevel level)
{
    // For subdirectories with no matches, skip displaying entirely
    if (level == EDirectoryLevel::Subdirectory && pdi->m_vMatches.size() == 0)
    {
        return;
    }

    if (level == EDirectoryLevel::Initial)
    {
        //
        // For the initial directory, we'll add the top separator.  
        // For recursed directories, we'll just add the bottom so that there's
        // only one separator between each subdirectory.
        // 
        
        m_pConsole->WriteSeparatorLine (m_pConfig->m_rgAttributes[CConfig::EAttribute::SeparatorLine]);

        //
        // We'll only show the drive header on the initial directory processed.
        // Any recursed subdirs won't show it.
        //

        DisplayDriveHeader(pdi->m_pszPath);
    }

    DisplayPathHeader (pdi->m_pszPath); 

    if (pdi->m_vMatches.size() == 0)
    {
        if (wcscmp(pdi->m_pszFileSpec, L"*") == 0)
        {
            m_pConsole->Puts(m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"Directory is empty.");
        }
        else
        {
            m_pConsole->Printf(m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"No files matching '%s' found.\n", pdi->m_pszFileSpec);
        }
    }
    else 
    {
        if (m_pCmdLine->m_fWideListing)
        {
            DisplayResultsWide (pdi);
        }
        else
        {
            DisplayResultsNormal (pdi);
        }

        DisplayDirectorySummary (pdi);

        //
        // Only show the volume footer here if we're not doing a recursive list.
        // Otherwise, it'll be shown after the recursive summary.
        //

        if (!m_pCmdLine->m_fRecurse)
        {
            DisplayVolumeFooter(pdi);
        }
    }
        
    m_pConsole->Puts (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"");
    m_pConsole->WriteSeparatorLine (m_pConfig->m_rgAttributes[CConfig::EAttribute::SeparatorLine]);
    m_pConsole->Puts (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"");

    m_pConsole->Flush();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::DisplayResultsWide
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CDirectoryLister::DisplayResultsWide (__in CDirectoryInfo * pdi)
{                                 
    HRESULT hr             = S_OK;
    size_t  cColumns;
    size_t  cxColumnWidth; 
    size_t  nRow;
    size_t  nCol;
    size_t  cRows;
    size_t  cItemsInLastRow;


    CBRA (pdi->m_cchLargestFileName > 0);

    hr = GetColumnInfo (pdi, &cColumns, &cxColumnWidth);
    CHR (hr);    

    cRows = (pdi->m_vMatches.size() + cColumns - 1) / cColumns;
    cItemsInLastRow = pdi->m_vMatches.size () % cColumns;
   
    //
    // Display the matches in columns
    //

    for (nRow = 0; nRow < cRows; ++nRow)
    {
        for (nCol = 0; nCol < cColumns; ++nCol)
        {   
            size_t            idx           = 0;
            size_t            fullRows      = cItemsInLastRow ? cRows - 1 : cRows;
            LPCWSTR           pszName       = NULL;    
            WIN32_FIND_DATA * pwfd          = NULL;       
            WORD              wAttr         = 0;      
            size_t            cSpacesNeeded = 0;



            if ((nRow * cColumns + nCol) >= pdi->m_vMatches.size ())
            {
                break;
            }

            // We print in column-major order, so skip over 
            // items in previous columns.  The last row
            // may not be full, so assume one fewer row for now.
            idx = nRow + (nCol * fullRows);

            // Skip past any items in the last row prior to this column
            if (nCol < cItemsInLastRow)
            {
                idx += nCol;
            }
            else
            {
                idx += cItemsInLastRow;
            }

            pwfd = &pdi->m_vMatches[idx];
            hr = GetWideFormattedName (pwfd, &pszName);
            CHR (hr);

            wAttr = m_pConfig->GetTextAttrForFile (pwfd);
            m_pConsole->Printf (wAttr, L"%s", pszName);

            for (cSpacesNeeded = cxColumnWidth - wcslen (pszName); cSpacesNeeded > 0; cSpacesNeeded--)
            {
                m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L" ");
            }
        }

        m_pConsole->Puts (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"");
    }


Error:
    return;
}






////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::GetColumnCount
//
//  Figure out how many columns fit on the screen
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CDirectoryLister::GetColumnInfo (__in const CDirectoryInfo * pdi, __out size_t * pcColumns, __out size_t * pcxColumnWidth)
{
    HRESULT                    hr              = S_OK;
    BOOL                       fSuccess;       
    CONSOLE_SCREEN_BUFFER_INFO csbi;           
    UINT                       cxConsoleWidth; 


    
    fSuccess = GetConsoleScreenBufferInfo (m_pConsole->m_hStdOut, &csbi);
    CBRA (fSuccess);

    cxConsoleWidth  = csbi.srWindow.Right - csbi.srWindow.Left + 1;

    if (pdi->m_cchLargestFileName + 1 > cxConsoleWidth)
    {
        *pcColumns = 1;
    }
    else
    {
        // 1 space between columns
        *pcColumns = cxConsoleWidth / (pdi->m_cchLargestFileName + 1);    
    }

    *pcxColumnWidth = cxConsoleWidth / *pcColumns;


Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::GetWideFormattedName
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CDirectoryLister::GetWideFormattedName (__in const WIN32_FIND_DATA * pwfd, __deref_out_z LPCWSTR * ppszName)
{
    HRESULT      hr                      = S_OK;
    static WCHAR szDirName[MAX_PATH + 2] = L"[";


    
    if (pwfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        LPWSTR pszBufEnd    = szDirName + 1;
        size_t cchRemaining = 0;


        
        StringCchCopyEx (szDirName + 1, ARRAYSIZE (szDirName) - 2, pwfd->cFileName, &pszBufEnd, &cchRemaining, 0);
        StringCchCat    (pszBufEnd,  cchRemaining, L"]");

        *ppszName = szDirName;
    }
    else
    {
        *ppszName = pwfd->cFileName;
    }

//Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::DisplayResultsNormal
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CDirectoryLister::DisplayResultsNormal (__in CDirectoryInfo * pdi)
{
    UINT               cchMaxSize   = 0;
    BOOL               fSuccess     = FALSE;   
    WCHAR              szDate[11]   = { 0 }; // "12/34/5678" + null = 11 characters
    WCHAR              szTime[9]    = { 0 };  // "12:34 PM"   + null = 9 characters
    FileInfoVectorIter iter;       
    static const WCHAR kchHyphen    = L'-';
    static const WCHAR kszDirSize[] = L"<DIR>";
    static const UINT  kcchDirSize  = ARRAYSIZE (kszDirSize) - 1;
    
    struct SAttrMap
    {
        DWORD m_dwAttr;
        WCHAR m_chAttr;
    };
    
    static const SAttrMap attrmap[] =
    {
        {  FILE_ATTRIBUTE_READONLY,      L'R' },
        {  FILE_ATTRIBUTE_HIDDEN,        L'H' },
        {  FILE_ATTRIBUTE_SYSTEM,        L'S' },
        {  FILE_ATTRIBUTE_ARCHIVE,       L'A' },
        {  FILE_ATTRIBUTE_TEMPORARY,     L'T' },
        {  FILE_ATTRIBUTE_ENCRYPTED,     L'E' },
        {  FILE_ATTRIBUTE_COMPRESSED,    L'C' },
        {  FILE_ATTRIBUTE_REPARSE_POINT, L'P' },
        {  FILE_ATTRIBUTE_SPARSE_FILE,   L'0' },
        {  0,                            0    }
    };      
    
    

    cchMaxSize = GetMaxSize (&pdi->m_uliLargestFileSize);
    cchMaxSize = max (cchMaxSize, kcchDirSize);
    
    iter = pdi->m_vMatches.begin();
    while (iter != pdi->m_vMatches.end())
    {                           
        SYSTEMTIME       st             = { 0 };
        SYSTEMTIME       stLocal        = { 0 };
        ULARGE_INTEGER   uliFileSize    = { 0 };
        const SAttrMap * pAttrMap       = attrmap;
        WORD             attr           = 0;


        fSuccess = FileTimeToSystemTime (&iter->ftLastWriteTime, &st);
        assert (fSuccess);
        
        fSuccess = SystemTimeToTzSpecificLocalTime (NULL, &st, &stLocal);
        assert (fSuccess);

        GetDateFormat (LOCALE_USER_DEFAULT, 0, &stLocal, L"MM/dd/yyyy", szDate, ARRAYSIZE (szDate));
        GetTimeFormat (LOCALE_USER_DEFAULT, 0, &stLocal, L"hh:mm tt",   szTime, ARRAYSIZE (szTime));


        //
        // Get the two 32-bit halves into a convenient 64-bit type
        //
        
        uliFileSize.LowPart  = iter->nFileSizeLow;
        uliFileSize.HighPart = iter->nFileSizeHigh;

        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Date],    L"%s", szDate);
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"  ");
        
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Time],    L"%s", szTime);
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L" ");

        while (pAttrMap->m_dwAttr != 0)
        {
            WCHAR chDisplay;
            
            if (iter->dwFileAttributes & pAttrMap->m_dwAttr)
            {
                attr = m_pConfig->m_rgAttributes[CConfig::EAttribute::FileAttributePresent];
                chDisplay = pAttrMap->m_chAttr;
            }
            else
            {
                attr = m_pConfig->m_rgAttributes[CConfig::EAttribute::FileAttributeNotPresent];
                chDisplay = kchHyphen;
            }

            m_pConsole->Printf (attr, L"%c", chDisplay);

            ++pAttrMap;
        }

        
        if ((iter->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Size], L" %*s ", cchMaxSize, FormatNumberWithSeparators (uliFileSize.QuadPart));
        }
        else
        {
            UINT cchLeftSidePadding = (cchMaxSize - kcchDirSize) / 2;            
            m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Directory], L" %*s%-*s ", cchLeftSidePadding, L"", cchMaxSize - cchLeftSidePadding, kszDirSize);
         }        

        attr = (m_pConfig->GetTextAttrForFile (&(*iter)));
        m_pConsole->Printf (attr, L"%s\n", iter->cFileName);

        ++iter;
    }
}






////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::DisplayDriveHeader
//
//  Displays information about the drive, eg: 
// 
//      Volume in drive C is a hard drive (NTFS)
//      Volume Name is "Maxtor 250GB"
//
////////////////////////////////////////////////////////////////////////////////

void CDirectoryLister::DisplayDriveHeader (LPCWSTR pszPath)
{
    static const LPCWSTR s_krgszVolumeDescription[] = 
    {
        L"an unknown type",
        L"an unknown type",
        L"a removable disk",
        L"a hard drive",
        L"a network drive",
        L"a CD/DVD",
        L"a RAM disk",
    };

    static WCHAR s_szPreviousDriveRoot[MAX_PATH] = { L'\0' };

    HRESULT hr                          = S_OK;;                         
    int     nDrive;                     
    WCHAR   szDriveRoot[MAX_PATH];      
    UINT    uiDriveType;                
    BOOL    fUncPath                    = FALSE;
    WCHAR   szVolumeName[MAX_PATH];     
    WCHAR   szFileSystemName[MAX_PATH]; 


    
    nDrive = PathGetDriveNumber (pszPath);
    if (nDrive >= 0)
    {
        StringCchCopy (szDriveRoot, ARRAYSIZE (szDriveRoot), L" :\\");
        szDriveRoot[0] = (WCHAR) ((int) L'A' + nDrive);
        uiDriveType = GetDriveType (szDriveRoot);
    }
    else
    {
        //
        // If there's no drive letter, then this must be a UNC path
        //
        
        assert (PathIsUNC (pszPath));
        uiDriveType = DRIVE_REMOTE;     
        fUncPath    = TRUE;

        StringCchCopy (szDriveRoot, ARRAYSIZE (szDriveRoot), pszPath);
        PathAddBackslash (szDriveRoot);
    }             


    //
    // Only show the drive header if it's different from the previous one
    // (prevents wasted screen real estate in /s listings)
    //
    
    CBREx (wcscmp (s_szPreviousDriveRoot, szDriveRoot) != 0, S_OK);

    StringCchCopy (s_szPreviousDriveRoot, ARRAYSIZE (s_szPreviousDriveRoot), szDriveRoot);


    GetVolumeInformation (szDriveRoot, 
                          szVolumeName, ARRAYSIZE (szVolumeName), 
                          NULL, 
                          NULL, 
                          NULL, 
                          szFileSystemName, ARRAYSIZE (szFileSystemName));

    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information], L" Volume ");

    if (fUncPath)
    {
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"%s", pszPath);
    }
    else
    {
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L"in drive ");
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"%c", szDriveRoot[0]);
    }        

    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L" is ");
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"%s", s_krgszVolumeDescription[uiDriveType]);


    //
    // If this is a mapped drive, get the remote name that it's mapped to
    //
    
    if ((fUncPath == FALSE) && (uiDriveType == DRIVE_REMOTE))
    {
        LPWSTR psz;                    
        WCHAR  szRemoteName[MAX_PATH]; 
        DWORD  cchRemoteName           = ARRAYSIZE (szRemoteName); 
        DWORD  dwResult;               
    
    
        
        psz = szDriveRoot + wcslen (szDriveRoot) - 1;
        if (*psz == L'\\')
        {
            *psz = L'\0';
        }
        
        dwResult = WNetGetConnection (szDriveRoot, szRemoteName, &cchRemoteName);              
        if (dwResult == NOERROR)
        {
            m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L" mapped to ");
            m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"%s", szRemoteName);
        }
    }

    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L" (");
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"%s", szFileSystemName);
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L")\n");

    if (szVolumeName[0] != L'\0')
    {
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L" Volume name is \"");
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"%s", szVolumeName);
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L"\"\n\n");
    }
    else
    {
        m_pConsole->Puts (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information], L" Volume has no name\n");
    }

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::DisplayPathHeader
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CDirectoryLister::DisplayPathHeader (LPCWSTR pszPath)
{
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L" Directory of ");
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"%s", pszPath);
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L"\n\n");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::DisplayListingSummary
//
//  Display full recursive summary information:
// 
//   Total files listed:
//         143 files using 123,456 bytes
//           7 subdirectories
// 
//   123,123,123,123 bytes free on volume
//   123,117,699,072 bytes available to user %s
//
////////////////////////////////////////////////////////////////////////////////

void CDirectoryLister::DisplayListingSummary (__in const CDirectoryInfo * pdi)
{
    int cMaxDigits = 1;


    if (m_cFilesFound > 0 || m_cDirectoriesFound > 0)
    { 
        cMaxDigits = (int) log10 (max (m_cFilesFound, m_cDirectoriesFound)) + 1;
        cMaxDigits += cMaxDigits / 3;  // add space for each comma
    }


    m_pConsole->Puts   (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L" Total files listed:");
    m_pConsole->Puts   (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default],              L"\n");

    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"    %*s", cMaxDigits, FormatNumberWithSeparators (m_cFilesFound));
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          m_cFilesFound == 1 ? L" file using " : L" files using ");
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], FormatNumberWithSeparators (m_uliSizeOfAllFilesFound.QuadPart));
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          m_uliSizeOfAllFilesFound.QuadPart == 1 ? L" byte\n" : L" bytes\n");

    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"    %*s", cMaxDigits, FormatNumberWithSeparators (m_cDirectoriesFound));
    m_pConsole->Puts   (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          m_cDirectoriesFound == 1 ? L" subdirectory" : L" subdirectories");

    m_pConsole->Puts   (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default],              L"");
    DisplayVolumeFooter (pdi);

    m_pConsole->WriteSeparatorLine (m_pConfig->m_rgAttributes[CConfig::EAttribute::SeparatorLine]);
    m_pConsole->Puts               (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::DisplayDirectorySummary
//
//  Display summary information for the directory:
//
//   4 directories, 27 files using 284,475 bytes
//
////////////////////////////////////////////////////////////////////////////////

void CDirectoryLister::DisplayDirectorySummary (__in const CDirectoryInfo * pdi)
{
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L"\n ");
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"%d", pdi->m_cSubDirectories);
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          pdi->m_cSubDirectories == 1 ? L" dir, " : L" dirs, ");

    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"%d", pdi->m_cFiles);
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          pdi->m_cFiles == 1 ? L" file using " : L" files using ");
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], FormatNumberWithSeparators (pdi->m_uliBytesUsed.QuadPart));
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          pdi->m_uliBytesUsed.QuadPart == 1 ? L" byte\n" : L" bytes\n");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::DisplayVolumeFooter
//
//  Display free space info for volume:
//
//   123,123,123,123 bytes free on volume
//   123,117,699,072 bytes available to user %s
//
////////////////////////////////////////////////////////////////////////////////

void CDirectoryLister::DisplayVolumeFooter (__in const CDirectoryInfo * pdi)
{
    HRESULT        hr                     = S_OK;
    BOOL           fSuccess;              
    ULARGE_INTEGER uliFreeBytesAvailable; 
    ULARGE_INTEGER uliTotalBytes;         
    ULARGE_INTEGER uliTotalFreeBytes;     


    
    fSuccess = GetDiskFreeSpaceEx (pdi->m_pszPath, &uliFreeBytesAvailable, &uliTotalBytes, &uliTotalFreeBytes);
    CBRA (fSuccess);

    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L" %s", FormatNumberWithSeparators (uliTotalFreeBytes.QuadPart));
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          uliTotalFreeBytes.QuadPart == 1 ? L" byte free on volume\n" : L" bytes free on volume\n");

    if (uliFreeBytesAvailable.QuadPart != uliTotalFreeBytes.QuadPart)
    {
        DisplayFooterQuotaInfo (&uliFreeBytesAvailable);
    }
    

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::DisplayFooterQuotaInfo
//
//  Display free space info for volume:
//
//   123,117,699,072 bytes available to user %s
//
////////////////////////////////////////////////////////////////////////////////

void CDirectoryLister::DisplayFooterQuotaInfo (__in const ULARGE_INTEGER * puliFreeBytesAvailable)
{
    BOOL    fSuccess    = FALSE;
    DWORD   cchUsername = UNLEN + 1;     // Use Windows constant for max username length
    wstring strUsername;            

    

    strUsername.resize (cchUsername);
    
    fSuccess = GetUserName (&strUsername[0], &cchUsername);
    if (fSuccess)
    {
        strUsername.resize (cchUsername - 1); // Remove null terminator from size
    }
    else
    {
        strUsername = L"<Unknown>";
    }

    m_pConsole->Printf(m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L" ");
    m_pConsole->Printf(m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], FormatNumberWithSeparators (puliFreeBytesAvailable->QuadPart));
    m_pConsole->Printf(m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          puliFreeBytesAvailable->QuadPart == 1 ? L" byte available to " : L" bytes available to ");
    m_pConsole->Printf(m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], strUsername.c_str());
    m_pConsole->Printf(m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L" due to quota\n");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::GetMaxSize
//
//  
//
////////////////////////////////////////////////////////////////////////////////

UINT CDirectoryLister::GetMaxSize (__in const ULARGE_INTEGER * puli)
{
    UINT cchDigits = 1;


    if (puli->QuadPart != 0)
    {
        cchDigits += (UINT) log10 ((double) puli->QuadPart);
    }

    //
    // add space for the comma digit group separators
    //

    cchDigits += (cchDigits - 1) / 3;

    return cchDigits;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectoryLister::FormatNumberWithSeparators
//
//  
//
////////////////////////////////////////////////////////////////////////////////

LPCWSTR CDirectoryLister::FormatNumberWithSeparators (ULONGLONG n)
{
    static WCHAR szFileSize[27];  // 2^64 = 1.84467440737096E+19 = 18,446,744,073,709,600,000 = 18 chars + 6 commas + 1 null
    LPWSTR       pszSize;
    UINT         nDigitPosition = 0;



    //
    // Point to the end of the size buffer
    // 
    
    pszSize = szFileSize + ARRAYSIZE (szFileSize) - 1;
    *pszSize = L'\0';
    


    //
    // Extract the digits, placing comma seperators appropriately
    //

    do
    {
        UINT uiDigit;


        //
        // Pick off the lowest remaining digit and shift the 
        // remaining number down by a factor of 10.
        //

        uiDigit = (UINT) (n % 10ull);
        n       = n / 10ull;

        //
        // Point to the next free spot in the write buffer
        //
        
        --pszSize;
        assert (pszSize >= szFileSize);

        //
        // Every 4th character written should be a comma
        //
        
        ++nDigitPosition;
        if ((nDigitPosition % 4) == 0)
        {
            *pszSize = L',';
            ++nDigitPosition;
            --pszSize;
            assert (pszSize >= szFileSize);
        }

        //
        // Write the digit to the buffer
        // 
        
        *pszSize = (WCHAR) (uiDigit + (UINT) L'0');
    }             
    while (n > 0ull);

    return pszSize;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::IsDots
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
