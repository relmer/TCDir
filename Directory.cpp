#include "StdAfx.h"
#include "Directory.h"
#include "CommandLine.h"
#include "Utils.h"




////////////////////////////////////////////////////////////////////////////////
//
//  Define class statics
//
////////////////////////////////////////////////////////////////////////////////

CCommandLine * CDirectory::CFileInfo::s_pCmdLine = NULL;





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::CFileInfo::operator< 
//
//  Define a compoarison operator for use by std::sort
//
////////////////////////////////////////////////////////////////////////////////

CDirectory::CFileInfo::CFileInfo (void)
{
    cAlternateFileName[0] = L'\0';
    cFileName[0] = L'\0';
    dwFileAttributes = 0;
    dwReserved0 = 0;
    dwReserved1 = 0;
    ftCreationTime.dwHighDateTime = 0;
    ftCreationTime.dwLowDateTime = 0;
    ftLastAccessTime.dwHighDateTime = 0;
    ftLastAccessTime.dwLowDateTime = 0;
    ftLastWriteTime.dwHighDateTime = 0;
    ftLastWriteTime.dwLowDateTime = 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::CFileInfo::operator< 
//
//  Define a compoarison operator for use by std::sort
//
////////////////////////////////////////////////////////////////////////////////

bool CDirectory::CFileInfo::operator< (const CFileInfo & rhs) const
{
    bool                       comesBeforeRhs = false;
    bool                       isDirectory    = false;
    bool                       isDirectoryRhs = false;
    CCommandLine::ESortOrder * pSortAttribute;
    CCommandLine::ESortOrder * pLastAttribute = &s_pCmdLine->m_rgSortPreference[ARRAYSIZE(s_pCmdLine->m_rgSortPreference)];
    LONGLONG                   cmp            = 0;


    //
    // If only one of the operands is a directory, it should be sorted first
    //

    isDirectory    = !!(dwFileAttributes     & FILE_ATTRIBUTE_DIRECTORY);
    isDirectoryRhs = !!(rhs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

    if (isDirectory ^ isDirectoryRhs)
    {
        return isDirectory;
    }

    //
    // Compare based on requested sort attribute with fallbacks
    //

    for (pSortAttribute = s_pCmdLine->m_rgSortPreference; 
         pSortAttribute < pLastAttribute; 
         ++pSortAttribute)
    {
        switch (*pSortAttribute)
        {
            case CCommandLine::ESortOrder::SO_DEFAULT:
            case CCommandLine::ESortOrder::SO_NAME:
                cmp = lstrcmpiW (cFileName, rhs.cFileName);
                break;

            case CCommandLine::ESortOrder::SO_DATE:
                cmp = CompareFileTime (&ftLastWriteTime, &rhs.ftLastWriteTime);
                break;

            case CCommandLine::ESortOrder::SO_EXTENSION:
                cmp = lstrcmpiW (PathFindExtension (cFileName), PathFindExtension (rhs.cFileName));
                break;

            case CCommandLine::ESortOrder::SO_SIZE:
            {
                ULARGE_INTEGER uliFileSize;
                ULARGE_INTEGER uliRhsFileSize;

                uliFileSize.HighPart = nFileSizeHigh;
                uliFileSize.LowPart = nFileSizeLow;

                uliRhsFileSize.HighPart = rhs.nFileSizeHigh;
                uliRhsFileSize.LowPart = rhs.nFileSizeLow;

                cmp = uliFileSize.QuadPart - uliRhsFileSize.QuadPart;
                break;
            }
        }

        //
        // If we didn't differ from rhs in this attribute, move on to the next one
        //

        if (cmp == 0)
        {
            continue;
        }

        comesBeforeRhs = cmp < 0;

        //
        // We found an attribute where we're different from the rhs, so break out of the sort loop
        //

        break;
    }

    //
    // Only respect reverse sorting if the comparison was against the requested attribute.  
    // If we had to fall back to another attribute because we did not differ from rhs in the
    // requested attribute, *ignore* reverse sorting for this attribute.
    //

    if (pSortAttribute == s_pCmdLine->m_rgSortPreference && 
        s_pCmdLine->m_sortdirection == CCommandLine::ESortDirection::SD_DESCENDING)
    {
        comesBeforeRhs = !comesBeforeRhs;
    }
    
    return comesBeforeRhs;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::CDirectoryInfo::CDirectoryInfo
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CDirectory::CDirectoryInfo::CDirectoryInfo (void) :
    m_pszPath            (NULL), 
    m_pszFileSpec        (NULL), 
    m_cchLargestFileName (0),    
    m_cFiles             (0),    
    m_cSubDirectories    (0)     

{    
    m_uliLargestFileSize.QuadPart = 0;
    m_uliBytesUsed.QuadPart       = 0;
}                               





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::CDirectory
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CDirectory::CDirectory (CCommandLine * pCmdLine) :
    m_pCmdLine (pCmdLine),
    m_cFilesFound (0),
    m_cDirectoriesFound (0)
{
    m_uliSizeOfAllFilesFound.QuadPart = 0;

    CFileInfo::s_pCmdLine = m_pCmdLine;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::~CDirectory
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CDirectory::~CDirectory (void)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::operator()
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CDirectory::operator() (LPCWSTR pszMask)
{
    HRESULT hr                          = S_OK;
    int     nDrive;    
    BOOL    fSuccess;                   
    WCHAR   szWellFormedPath[MAX_PATH]; 
    WCHAR   szPath[MAX_PATH];           
    WCHAR   szFileSpec[MAX_PATH];       



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
        PathAppend (szWellFormedPath, g_kszDefaultMask);
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

    hr = ProcessDirectory (szPath, szFileSpec);
    CHR (hr);

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::CollectDirectoryFiles
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CDirectory::ProcessDirectory (LPCWSTR pszPath, LPCWSTR pszFileSpec)
{
    HRESULT         hr = S_OK;
    BOOL            fSuccess;                    
    WCHAR           szPathAndFileSpec[MAX_PATH]; 
    HANDLE          hFind = INVALID_HANDLE_VALUE;
    CFileInfo       fileInfo;                         
    CDirectoryInfo  di;
    StringList      listSubdirs;                 
    StringListIter  iter;                        



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
    
    hFind = FindFirstFile (szPathAndFileSpec, &fileInfo);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do 
        {
            if (!g_util.IsDots (fileInfo.cFileName))
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
                
            fSuccess = FindNextFile (hFind, &fileInfo);
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
    
    DisplayResults (&di);           

    //
    // Recurse into subdirectories 
    //

    if (m_pCmdLine->m_fRecurse)
    {
        hr = RecurseIntoSubdirectories (pszPath, pszFileSpec);
        CHR (hr);
    }


Error:    
    FindClose (hFind);
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::RecurseIntoSubdirectories
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CDirectory::RecurseIntoSubdirectories (LPCWSTR pszPath, LPCWSTR pszFileSpec)
{
    HRESULT         hr                           = S_OK;
    BOOL            fSuccess;                    
    WCHAR           szPathAndFileSpec[MAX_PATH]; 
    HANDLE          hFind                        = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA wfd;                         

   

    //
    // Create the full path + filespec
    //

    StringCchCopy (szPathAndFileSpec, ARRAYSIZE (szPathAndFileSpec), pszPath);
    fSuccess = PathAppend (szPathAndFileSpec, L"*");
    CBRA (fSuccess);
    

    //
    // Search for subdirectories to recurse into
    // 
    
    hFind = FindFirstFile (szPathAndFileSpec, &wfd);
    CBR (hFind != INVALID_HANDLE_VALUE);

    do 
    {
        //
        // Add directories, except "." and "..", to a list
        // of subdirectories we'll recurse into
        //
                
        if (!g_util.IsDots (wfd.cFileName))
        {
            if (CFlag::IsSet (wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY))
            {
                WCHAR szSubdirPath[MAX_PATH];
            
            
                StringCchCopy (szSubdirPath, ARRAYSIZE (szSubdirPath), pszPath);
            
                fSuccess = PathAppend (szSubdirPath, wfd.cFileName);
                CBRA (fSuccess);
            
                ProcessDirectory (szSubdirPath, pszFileSpec);
            }
        }
            
        fSuccess = FindNextFile (hFind, &wfd);
    }
    while (fSuccess);



Error:
    FindClose (hFind);
    
    return hr;
}    

    



////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::AddMatchToList
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CDirectory::AddMatchToList (CFileInfo * pwfd, CDirectoryInfo * pdi)
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
    }
    else
    {
        ULARGE_INTEGER uliFileSize;            
        
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
//  CDirectory::Scroll
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CDirectory::Scroll (CDirectoryInfo * pdi)
{
    HRESULT                    hr                    = S_OK;
    BOOL                       fSuccess              = FALSE;
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo              = { 0 };
    UINT                       cWindowLinesTotal     = 0;
    UINT                       cBufferLinesTotal     = 0;
    UINT                       cBufferLinesRemaining = 0;
    UINT                       cLinesNeeded          = 0;



    //
    // Get the screen buffer size. 
    //
    
    fSuccess = GetConsoleScreenBufferInfo (g_util.m_hStdOut, &csbiInfo); 
    CBRA (fSuccess);

    cWindowLinesTotal = csbiInfo.srWindow.Bottom - csbiInfo.srWindow.Top + 1;
    cBufferLinesTotal = csbiInfo.dwSize.Y;    

    //
    // Calculate the number of rows to scroll
    //

    hr = CalculateLinesNeeded (pdi, &cLinesNeeded);
    CHR (hr);

    //
    // If there are enough lines left in the current window, no need to scroll
    //

    BAIL_OUT_IF ((cLinesNeeded + csbiInfo.dwCursorPosition.Y) <= cWindowLinesTotal, S_OK);

    //
    // If we need more lines than are free in the buffer, scroll the buffer up to make room
    //

    cBufferLinesRemaining = cBufferLinesTotal - csbiInfo.dwCursorPosition.Y;
    if (cLinesNeeded > cBufferLinesRemaining)
    {
        SMALL_RECT srcScrollRect  = { 0 };
        CHAR_INFO  chiFill        = { 0 };
        COORD      coordDest      = { 0 };
        COORD      coordCursorPos = { 0 };

        srcScrollRect.Top    = (short) cLinesNeeded - (short) cBufferLinesRemaining;
        srcScrollRect.Bottom = csbiInfo.dwSize.Y - 1;
        srcScrollRect.Left   = 0;
        srcScrollRect.Right  = csbiInfo.dwSize.X - 1;

        chiFill.Attributes       = csbiInfo.wAttributes;
        chiFill.Char.UnicodeChar = L' ';

        fSuccess = ScrollConsoleScreenBuffer (g_util.m_hStdOut, &srcScrollRect, NULL, coordDest, &chiFill);
        CWRA (fSuccess);

        coordCursorPos.X = 0;
        coordCursorPos.Y = srcScrollRect.Top;
        fSuccess = SetConsoleCursorPosition (g_util.m_hStdOut, coordCursorPos);
        CWRA (fSuccess);
    }

    //
    // If we're going to write past the bottom of the window, reposition the window 
    // to the bottom of the area we need
    //
        
    if (csbiInfo.dwCursorPosition.Y + cLinesNeeded > (UINT) csbiInfo.srWindow.Bottom)
    {
        SHORT      cLinesToMoveWindow = 0;
        SMALL_RECT srcWindow          = { 0 };



        cLinesToMoveWindow = (SHORT) csbiInfo.dwCursorPosition.Y + (SHORT) cLinesNeeded - csbiInfo.srWindow.Bottom;

        // relative coords
        srcWindow.Top    += cLinesToMoveWindow;
        srcWindow.Bottom += cLinesToMoveWindow;

        fSuccess = SetConsoleWindowInfo (g_util.m_hStdOut, FALSE, &srcWindow);
        CWRA (fSuccess);
    }
  


Error:
    return;    
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::CalculateLinesNeeded
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CDirectory::CalculateLinesNeeded (__in CDirectoryInfo * pdi, __out UINT * pcLinesNeeded)
{
    HRESULT hr           = S_OK;
    UINT    cLinesNeeded = 0;



    ++cLinesNeeded;     // Top separator line
    ++cLinesNeeded;     // Volume in drive _ is _____
    ++cLinesNeeded;     // Volume name is ______
    ++cLinesNeeded;     // Blank line

    if (pdi->m_cFiles > 0)
    {
        ++cLinesNeeded;     // Directory of ___________________
        ++cLinesNeeded;     // Blank line

        if (m_pCmdLine->m_fWideListing)
        {
            UINT cColumns;
            UINT cxColumnWidth;



            assert (pdi->m_cchLargestFileName > 0);

            hr = GetColumnInfo (pdi, &cColumns, &cxColumnWidth);
            CHR (hr);

            cLinesNeeded += ((UINT) pdi->m_vMatches.size() + (cColumns - 1)) / cColumns;
        }
        else
        {
            cLinesNeeded += (UINT) pdi->m_vMatches.size();
        }

        ++cLinesNeeded;     // Blank line
        ++cLinesNeeded;     // __ dirs, __ files using ____ bytes
        ++cLinesNeeded;     // _____________ bytes free on volume
    }

    ++cLinesNeeded;     // Bottom separator line
    ++cLinesNeeded;     // Blank line

    *pcLinesNeeded = cLinesNeeded;

Error:
    return hr;
}




////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::DisplayResults
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CDirectory::DisplayResults (CDirectoryInfo * pdi)
{
#ifdef _DEBUG
    int i;
    for (i = 1; i <= 100; ++i)
    {
        //g_util.ConsolePrintf (L"%d\n", i);
    }
#endif

    Scroll (pdi);
    
    g_util.ConsoleDrawSeparator();

    DisplayDriveHeader (pdi->m_pszPath); 

    if (pdi->m_cFiles > 0)
    {
        DisplayPathHeader (pdi->m_pszPath); 
    }

    
    if (m_pCmdLine->m_fWideListing)
    {
        DisplayResultsWide (pdi);
    }
    else
    {
        DisplayResultsNormal (pdi);
    }

    if (pdi->m_cFiles > 0 || pdi->m_cSubDirectories > 0)
    {
        DisplayFooter (pdi);    
    }
        
    g_util.ConsoleDrawSeparator ();
    g_util.ConsolePrintf (L"\n");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::DisplayResultsWide
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CDirectory::DisplayResultsWide (CDirectoryInfo * pdi)
{                                 
    HRESULT hr             = S_OK;
    UINT    cColumns;
    UINT    cxColumnWidth; 
    UINT    nRow;
    UINT    nCol;
    UINT    cRows;



    CBR (pdi->m_cchLargestFileName > 0);

    hr = GetColumnInfo (pdi, &cColumns, &cxColumnWidth);
    CHR (hr);    

    hr = AddEmptyMatches (pdi, cColumns);
    CHR (hr);

    cRows = (UINT) pdi->m_vMatches.size() / cColumns;
   
    //
    // Display the matches in columns
    //

    for (nRow = 0; nRow < cRows; ++nRow)
    {
        for (nCol = 0; nCol < cColumns; ++nCol)
        {                   
            LPWSTR            pszName;    
            int               cchPrinted; 
            WIN32_FIND_DATA * pwfd;       
            WORD              wAttr;      



            pwfd = &pdi->m_vMatches[nRow + ((size_t) nCol * (size_t) cRows)];
            hr = GetWideFormattedName (pwfd, &pszName);
            CHR (hr);

            wAttr = g_util.GetTextAttrForFile (pwfd);
            g_util.SetTextAttr (wAttr);
            
            cchPrinted = g_util.ConsolePrintf (L"%s", pszName);

            g_util.ResetTextAttr();
            g_util.ConsolePrintf (L"%*s", cxColumnWidth - cchPrinted, L"");
        }

        g_util.ConsolePrintf (L"\n");
    }

Error:
    return;
}






////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::GetColumnCount
//
//  Figure out how many columns fit on the screen
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CDirectory::GetColumnInfo (CDirectoryInfo * pdi, UINT * pcColumns, UINT * pcxColumnWidth)
{
    HRESULT                    hr              = S_OK;
    BOOL                       fSuccess;       
    CONSOLE_SCREEN_BUFFER_INFO csbi;           
	UINT                       cxConsoleWidth; 


    
    fSuccess = GetConsoleScreenBufferInfo (g_util.m_hStdOut, &csbi);
    CBRA (fSuccess);

    cxConsoleWidth  = csbi.srWindow.Right - csbi.srWindow.Left;
    *pcColumns      = cxConsoleWidth / (UINT) (pdi->m_cchLargestFileName + 1);    // 1 space between columns
	*pcxColumnWidth = cxConsoleWidth / *pcColumns;

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::AddEmptyMatches
//
//  Add empty matches to the end of the match list
//  to ensure that there are an exact multiple of cColumns.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CDirectory::AddEmptyMatches (CDirectoryInfo * pdi, size_t cColumns)
{
	HRESULT   hr = S_OK;
    size_t    i;       
    CFileInfo emptyFileInfo;



    for (i = pdi->m_vMatches.size() % cColumns; i > 0; --i)
    {
        pdi->m_vMatches.push_back (emptyFileInfo);
    }

//Error:
	return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::GetWideFormattedName
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CDirectory::GetWideFormattedName (WIN32_FIND_DATA * pwfd, LPWSTR * ppszName)
{
    HRESULT      hr                  = S_OK;
    static WCHAR szDirName[MAX_PATH] = L"[";


    
    if (pwfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        size_t cchRemaining;


        
        StringCchCopyEx (szDirName + 1, ARRAYSIZE (szDirName) - 1, pwfd->cFileName, ppszName, &cchRemaining, 0);
        StringCchCat    (*ppszName,  cchRemaining, L"]");

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
//  CDirectory::DisplayResultsNormal
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CDirectory::DisplayResultsNormal (CDirectoryInfo * pdi)
{
    UINT               cchMaxSize;
    BOOL               fSuccess;   
    WCHAR              szDate[11]; // "12/34/5678" + null = 11 characters
    WCHAR              szTime[9];  // "12:34 PM"   + null = 9 characters
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
        SYSTEMTIME       st;          
        SYSTEMTIME       stLocal;     
        ULARGE_INTEGER   uliFileSize; 
        const SAttrMap * pAttrMap     = attrmap;



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

        g_util.SetTextAttr (g_util.GetDateAttr());
        g_util.ConsolePrintf (L"%s", szDate);
        g_util.ResetTextAttr();        
        g_util.ConsolePrintf (L"  ");
        
        g_util.SetTextAttr (g_util.GetTimeAttr());
        g_util.ConsolePrintf (L"%s", szTime);
        g_util.ResetTextAttr();        
        g_util.ConsolePrintf (L" ");

        while (pAttrMap->m_dwAttr != 0)
        {
            WCHAR chDisplay;
            
            if (iter->dwFileAttributes & pAttrMap->m_dwAttr)
            {
                g_util.SetTextAttr (g_util.GetAttributePresentAttr());
                chDisplay = pAttrMap->m_chAttr;
            }
            else
            {
                g_util.SetTextAttr (g_util.GetAttributeNotPresentAttr());
                chDisplay = kchHyphen;
            }

            g_util.ConsolePrintf (L"%c", chDisplay);
            g_util.ResetTextAttr();

            ++pAttrMap;
        }

        
        if ((iter->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            g_util.SetTextAttr (g_util.GetSizeAttr());
            g_util.ConsolePrintf (L" %*s ", cchMaxSize, FormatFileSize (uliFileSize));
        }
        else
        {
            UINT cchLeftSidePadding;

            cchLeftSidePadding = (cchMaxSize - kcchDirSize) / 2;            
            
            g_util.SetTextAttr (g_util.GetDirAttr());
            g_util.ConsolePrintf (L" %*s", cchLeftSidePadding, L"");
            g_util.ConsolePrintf (L"%-*s ", cchMaxSize - cchLeftSidePadding, kszDirSize);
         }        

        g_util.SetTextAttr (g_util.GetTextAttrForFile (&(*iter)));
        g_util.ConsolePrintf (L"%s", iter->cFileName);
        g_util.ResetTextAttr();        
        g_util.ConsolePrintf (L"\n");
        
        ++iter;
    }
}






////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::DisplayDriveHeader
//
//  Displays information about the drive, eg: 
// 
//      Volume in drive C is a hard drive (NTFS)
//      Volume Name is "Maxtor 250GB"
//
////////////////////////////////////////////////////////////////////////////////

void CDirectory::DisplayDriveHeader (LPCWSTR pszPath)
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

    g_util.SetTextAttr (g_util.GetInformationStandardAttr());
    g_util.ConsolePrintf (L" Volume ");

    if (fUncPath)
    {
        g_util.SetTextAttr (g_util.GetInformationHighlightAttr());
        g_util.ConsolePrintf (L"%s", pszPath);
    }
    else
    {
        g_util.ConsolePrintf (L"in drive ");
        
        g_util.SetTextAttr (g_util.GetInformationHighlightAttr());
        g_util.ConsolePrintf (L"%c", szDriveRoot[0]);        
    }        

    g_util.SetTextAttr (g_util.GetInformationStandardAttr());
    g_util.ConsolePrintf (L" is ");

    g_util.SetTextAttr (g_util.GetInformationHighlightAttr());
    g_util.ConsolePrintf (L"%s", s_krgszVolumeDescription[uiDriveType]);


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
            g_util.SetTextAttr (g_util.GetInformationStandardAttr());
            g_util.ConsolePrintf (L" mapped to ");
            
            g_util.SetTextAttr (g_util.GetInformationHighlightAttr());
            g_util.ConsolePrintf (L"%s", szRemoteName);
        }
    }


    g_util.SetTextAttr (g_util.GetInformationStandardAttr());
    g_util.ConsolePrintf (L" (");
    
    g_util.SetTextAttr (g_util.GetInformationHighlightAttr());
    g_util.ConsolePrintf (L"%s", szFileSystemName);
    
    g_util.SetTextAttr (g_util.GetInformationStandardAttr());
    g_util.ConsolePrintf (L")\n");

    

    if (szVolumeName[0] != L'\0')
    {
        g_util.SetTextAttr (g_util.GetInformationStandardAttr());
        g_util.ConsolePrintf (L" Volume name is \"");

        g_util.SetTextAttr (g_util.GetInformationHighlightAttr());
        g_util.ConsolePrintf (L"%s", szVolumeName);

        g_util.SetTextAttr (g_util.GetInformationStandardAttr());
        g_util.ConsolePrintf (L"\"\n\n");
    }
    else
    {
        g_util.SetTextAttr (g_util.GetInformationStandardAttr());
        g_util.ConsolePrintf (L" Volume has no name\n\n", szVolumeName);
    }

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::DisplayPathHeader
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CDirectory::DisplayPathHeader (LPCWSTR pszPath)
{
    g_util.SetTextAttr (g_util.GetInformationStandardAttr());
    g_util.ConsolePrintf (L" Directory of ");

    g_util.SetTextAttr (g_util.GetInformationHighlightAttr());
    g_util.ConsolePrintf (L"%s", pszPath);
        
    g_util.SetTextAttr (g_util.GetInformationStandardAttr());
    g_util.ConsolePrintf (L"\n\n");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::DisplayFooter
//
//  Display summary information for the directory:
//
//   4 directories, 27 files using 284,475 bytes
//   123,123,123,123 bytes free on volume
//   123,117,699,072 bytes available to user %s
//
////////////////////////////////////////////////////////////////////////////////

void CDirectory::DisplayFooter (CDirectoryInfo * pdi)
{
    HRESULT        hr                     = S_OK;
    BOOL           fSuccess;              
    ULARGE_INTEGER uliFreeBytesAvailable; 
    ULARGE_INTEGER uliTotalBytes;         
    ULARGE_INTEGER uliTotalFreeBytes;     

    
    fSuccess = GetDiskFreeSpaceEx (pdi->m_pszPath, &uliFreeBytesAvailable, &uliTotalBytes, &uliTotalFreeBytes);
    CBRA (fSuccess);


    g_util.SetTextAttr (g_util.GetInformationStandardAttr());
    g_util.ConsolePrintf (L"\n ");

    g_util.SetTextAttr (g_util.GetInformationHighlightAttr());
    g_util.ConsolePrintf (L"%d", pdi->m_cSubDirectories);

    g_util.SetTextAttr (g_util.GetInformationStandardAttr());
    g_util.ConsolePrintf (L" dir%s, ",pdi->m_cSubDirectories == 1 ? L"" : L"s");

    g_util.SetTextAttr (g_util.GetInformationHighlightAttr());
    g_util.ConsolePrintf (L"%d", pdi->m_cFiles);
    
    g_util.SetTextAttr (g_util.GetInformationStandardAttr());
    g_util.ConsolePrintf (L" file%s using ", pdi->m_cFiles == 1 ? L"" : L"s");
    
    g_util.SetTextAttr (g_util.GetInformationHighlightAttr());
    g_util.ConsolePrintf (L"%s", FormatFileSize (pdi->m_uliBytesUsed));

    g_util.SetTextAttr (g_util.GetInformationStandardAttr());
    g_util.ConsolePrintf (L" byte%s\n", pdi->m_uliBytesUsed.QuadPart == 1 ? L"" : L"s");



    g_util.SetTextAttr (g_util.GetInformationStandardAttr());
    g_util.ConsolePrintf (L" ");

    g_util.SetTextAttr (g_util.GetInformationHighlightAttr());
    g_util.ConsolePrintf (L"%s", FormatFileSize (uliTotalFreeBytes));

    g_util.SetTextAttr (g_util.GetInformationStandardAttr());
    g_util.ConsolePrintf (L" byte%s free on volume\n", uliTotalFreeBytes.QuadPart == 1 ? L"" : L"s");


    if (uliFreeBytesAvailable.QuadPart != uliTotalFreeBytes.QuadPart)
    {
        DisplayFooterQuotaInfo (&uliFreeBytesAvailable);
    }
    

Error:
    return;
}





void CDirectory::DisplayFooterQuotaInfo (const ULARGE_INTEGER * puliFreeBytesAvailable)
{
    DWORD cchMaxUsername = 1 << 15;
    LPWSTR pszUsername = new WCHAR[cchMaxUsername];   // Max size for an environment variable is 32k



    GetEnvironmentVariable (L"username", pszUsername, cchMaxUsername);


    g_util.SetTextAttr (g_util.GetInformationStandardAttr ());
    g_util.ConsolePrintf (L" ");

    g_util.SetTextAttr (g_util.GetInformationHighlightAttr ());
    g_util.ConsolePrintf (L"%s", FormatFileSize (*puliFreeBytesAvailable));

    g_util.SetTextAttr (g_util.GetInformationStandardAttr ());
    g_util.ConsolePrintf (L" byte%s available to ", puliFreeBytesAvailable->QuadPart == 1 ? L"" : L"s");

    g_util.SetTextAttr (g_util.GetInformationHighlightAttr ());
    g_util.ConsolePrintf (L"%s", pszUsername);

    g_util.SetTextAttr (g_util.GetInformationStandardAttr ());
    g_util.ConsolePrintf (L" due to quota\n");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::GetMaxSize
//
//  
//
////////////////////////////////////////////////////////////////////////////////

UINT CDirectory::GetMaxSize (ULARGE_INTEGER * puli)
{
    UINT cchDigits;


	if (puli->QuadPart != 0)
	{
		cchDigits = (UINT) log10 ((float) puli->QuadPart);
	}
	else
	{
		cchDigits = 0;
	}

    return cchDigits + (cchDigits / 3) + 1;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDirectory::FormatFileSize
//
//  
//
////////////////////////////////////////////////////////////////////////////////

LPCWSTR CDirectory::FormatFileSize (ULARGE_INTEGER uli)
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

        uiDigit      = (UINT) uli.QuadPart % 10u;    
        uli.QuadPart = uli.QuadPart / 10u;

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
	while (uli.QuadPart > 0);

    return pszSize;
}






