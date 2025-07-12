#include "pch.h"
#include "ResultsDisplayerBase.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "Flag.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerBase::CResultsDisplayerBase
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CResultsDisplayerBase::CResultsDisplayerBase (__in CCommandLine * pCmdLine, __in CConsole * pConsole, __in CConfig * pConfig) :
    m_pCmdLine          (pCmdLine),
    m_pConsole          (pConsole),
    m_pConfig           (pConfig)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerBase::~CResultsDisplayerBase
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CResultsDisplayerBase::~CResultsDisplayerBase (void)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerBase::DisplayResults
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerBase::DisplayResults (__in CDirectoryInfo * pdi, EDirectoryLevel level)
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
//  CResultsDisplayerBase::DisplayResultsWide
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerBase::DisplayResultsWide (__in CDirectoryInfo * pdi)
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
            size_t            cchName       = 0;
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
            m_pConsole->Printf (wAttr, L"%.*s", cxColumnWidth, pszName);

            cchName = wcslen (pszName);
            if (cxColumnWidth > cchName)
            {
                for (cSpacesNeeded = cxColumnWidth - wcslen (pszName); cSpacesNeeded > 0; cSpacesNeeded--)
                {
                    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L" ");
                }
            }
        }

        m_pConsole->Puts (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"");
    }

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerBase::GetColumnInfo
//
//  Figure out how many columns fit on the screen
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CResultsDisplayerBase::GetColumnInfo (__in const CDirectoryInfo * pdi, __out size_t * pcColumns, __out size_t * pcxColumnWidth)
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
//  CResultsDisplayerBase::GetWideFormattedName
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CResultsDisplayerBase::GetWideFormattedName (__in const WIN32_FIND_DATA * pwfd, __deref_out_z LPCWSTR * ppszName)
{
    HRESULT      hr                      = S_OK;
    static WCHAR szDirName[MAX_PATH + 2] = L"[";

    
    if (pwfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        LPWSTR pszBufEnd    = szDirName + 1;
        size_t cchRemaining = 0;

        
        hr = StringCchCopyEx (szDirName + 1, ARRAYSIZE (szDirName) - 2, pwfd->cFileName, &pszBufEnd, &cchRemaining, 0);
        CHRA (hr);

        hr = StringCchCat (pszBufEnd,  cchRemaining, L"]");
        CHRA (hr);

        *ppszName = szDirName;
    }
    else
    {
        *ppszName = pwfd->cFileName;
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerBase::DisplayResultsNormal
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerBase::DisplayResultsNormal (__in CDirectoryInfo * pdi)
{
    HRESULT hr                           = S_OK;
    size_t  cchStringLengthOfMaxFileSize = GetStringLengthOfMaxFileSize (&pdi->m_uliLargestFileSize);

    

    for (const CFileInfo & fileInfo : pdi->m_vMatches)
    {
        WORD textAttr = m_pConfig->GetTextAttrForFile (&fileInfo);

        hr = DisplayResultsNormalDateAndTime (fileInfo.ftLastWriteTime);
        CHR (hr);

        DisplayResultsNormalAttributes (fileInfo.dwFileAttributes);
        DisplayResultsNormalFileSize   (fileInfo, cchStringLengthOfMaxFileSize);

        m_pConsole->Printf (textAttr, L"%s\n", fileInfo.cFileName);
    }
    

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerBase::DisplayResultsNormalDateAndTime
//
//  Displays the date and time from the given FILETIME 
// 
////////////////////////////////////////////////////////////////////////////////

HRESULT CResultsDisplayerBase::DisplayResultsNormalDateAndTime (const FILETIME & ftLastWriteTime)
{
    HRESULT    hr         = S_OK;
    BOOL       fSuccess   = FALSE;
    WCHAR      szDate[11] = { 0 }; // "12/34/5678" + null = 11 characters
    WCHAR      szTime[9]  = { 0 };  // "12:34 PM"   + null = 9 characters
    SYSTEMTIME st         = { 0 };
    SYSTEMTIME stLocal    = { 0 };



    fSuccess = FileTimeToSystemTime (&ftLastWriteTime, &st);
    CWRA (fSuccess);
    
    fSuccess = SystemTimeToTzSpecificLocalTime (NULL, &st, &stLocal);
    CWRA (fSuccess);

    fSuccess = GetDateFormatEx (LOCALE_NAME_USER_DEFAULT, 0, &stLocal, L"MM/dd/yyyy", szDate, ARRAYSIZE (szDate), NULL);
    CWRA (fSuccess);

    fSuccess = GetTimeFormatEx (LOCALE_NAME_USER_DEFAULT, 0, &stLocal, L"hh:mm tt",   szTime, ARRAYSIZE (szTime));
    CWRA (fSuccess);

    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Date],    L"%s", szDate);
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"  ");
    
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Time],    L"%s", szTime);
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L" ");

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerBase::DisplayResultsNormalAttributes
//
//  Displays the file attributes
// 
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerBase::DisplayResultsNormalAttributes  (DWORD dwFileAttributes)
{
    struct SFileAttributeEntry
    {
        DWORD m_dwAttr;
        WCHAR m_chAttr;
    };
    
    static const SFileAttributeEntry s_krgAttrMap[] =
    {
        {  FILE_ATTRIBUTE_READONLY,      L'R' },
        {  FILE_ATTRIBUTE_HIDDEN,        L'H' },
        {  FILE_ATTRIBUTE_SYSTEM,        L'S' },
        {  FILE_ATTRIBUTE_ARCHIVE,       L'A' },
        {  FILE_ATTRIBUTE_TEMPORARY,     L'T' },
        {  FILE_ATTRIBUTE_ENCRYPTED,     L'E' },
        {  FILE_ATTRIBUTE_COMPRESSED,    L'C' },
        {  FILE_ATTRIBUTE_REPARSE_POINT, L'P' },
        {  FILE_ATTRIBUTE_SPARSE_FILE,   L'0' }
    };      

    for (const SFileAttributeEntry fileAttributeEntry : s_krgAttrMap)
    {
        WORD  attrTextColor = m_pConfig->m_rgAttributes[CConfig::EAttribute::FileAttributeNotPresent];
        WCHAR chDisplay     = L'-';
        

        if (dwFileAttributes & fileAttributeEntry.m_dwAttr)
        {
            attrTextColor = m_pConfig->m_rgAttributes[CConfig::EAttribute::FileAttributePresent];
            chDisplay = fileAttributeEntry.m_chAttr;
        }
        
        m_pConsole->Printf (attrTextColor, L"%c", chDisplay);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerBase::DisplayResultsNormalFileSize
//
//  Displays the file size
//  
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerBase::DisplayResultsNormalFileSize (const CFileInfo & fileInfo, size_t cchStringLengthOfMaxFileSize)
{
    static const WCHAR  kszDirSize[] = L"<DIR>";
    static const size_t kcchDirSize  = ARRAYSIZE (kszDirSize) - 1;

    size_t         cchMaxFileSize = max (cchStringLengthOfMaxFileSize, kcchDirSize);
    ULARGE_INTEGER uliFileSize;


    
    uliFileSize.LowPart  = fileInfo.nFileSizeLow;
    uliFileSize.HighPart = fileInfo.nFileSizeHigh;

    if ((fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Size], 
                            L" %*s ", 
                            cchMaxFileSize, 
                            FormatNumberWithSeparators (uliFileSize.QuadPart));
    }
    else
    {
        size_t cchLeftSidePadding = (cchMaxFileSize - kcchDirSize) / 2;            
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Directory], 
                            L" %*s%-*s ", 
                            cchLeftSidePadding, 
                            L"", 
                            cchMaxFileSize - cchLeftSidePadding, 
                            kszDirSize);
    }        
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerBase::DisplayDriveHeader
//
//  Displays information about the drive, eg: 
// 
//      Volume in drive C is a hard drive (NTFS)
//      Volume Name is "Maxtor 250GB"
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerBase::DisplayDriveHeader (LPCWSTR pszPath)
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
//  CResultsDisplayerBase::DisplayPathHeader
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerBase::DisplayPathHeader (LPCWSTR pszPath)
{
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L" Directory of ");
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"%s", pszPath);
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L"\n\n");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerBase::DisplayListingSummary
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

void CResultsDisplayerBase::DisplayListingSummary (__in const CDirectoryInfo * pdi, UINT cFilesFound, UINT cDirectoriesFound, const ULARGE_INTEGER& uliSizeOfAllFilesFound)
{
    int cMaxDigits = 1;

    if (cFilesFound > 0 || cDirectoriesFound > 0)
    { 
        cMaxDigits = (int) log10 (max (cFilesFound, cDirectoriesFound)) + 1;
        cMaxDigits += cMaxDigits / 3;  // add space for each comma
    }

    m_pConsole->Puts   (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L" Total files listed:");
    m_pConsole->Puts   (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default],              L"\n");

    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"    %*s", cMaxDigits, FormatNumberWithSeparators (cFilesFound));
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          cFilesFound == 1 ? L" file using " : L" files using ");
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], FormatNumberWithSeparators (uliSizeOfAllFilesFound.QuadPart));
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          uliSizeOfAllFilesFound.QuadPart == 1 ? L" byte\n" : L" bytes\n");

    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"    %*s", cMaxDigits, FormatNumberWithSeparators (cDirectoriesFound));
    m_pConsole->Puts   (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          cDirectoriesFound == 1 ? L" subdirectory" : L" subdirectories");

    m_pConsole->Puts   (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default],              L"");
    DisplayVolumeFooter (pdi);

    m_pConsole->WriteSeparatorLine (m_pConfig->m_rgAttributes[CConfig::EAttribute::SeparatorLine]);
    m_pConsole->Puts               (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerBase::DisplayDirectorySummary
//
//  Display summary information for the directory:
//
//   4 directories, 27 files using 284,475 bytes
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerBase::DisplayDirectorySummary (__in const CDirectoryInfo * pdi)
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
//  CResultsDisplayerBase::DisplayVolumeFooter
//
//  Display free space info for volume:
//
//   123,123,123,123 bytes free on volume
//   123,117,699,072 bytes available to user %s
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerBase::DisplayVolumeFooter (__in const CDirectoryInfo * pdi)
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
//  CResultsDisplayerBase::DisplayFooterQuotaInfo
//
//  Display free space info for volume:
//
//   123,117,699,072 bytes available to user %s
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerBase::DisplayFooterQuotaInfo (__in const ULARGE_INTEGER * puliFreeBytesAvailable)
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
//  CResultsDisplayerBase::GetStringLengthOfMaxFileSize
//
//  
//
////////////////////////////////////////////////////////////////////////////////

UINT CResultsDisplayerBase::GetStringLengthOfMaxFileSize (__in const ULARGE_INTEGER * puli)
{
    UINT cch = 1;

    if (puli->QuadPart != 0)
    {
        cch += (UINT) log10 ((double) puli->QuadPart);
    }

    //
    // add space for the comma digit group separators
    //

    cch += (cch - 1) / 3;

    return cch;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerBase::FormatNumberWithSeparators
//
//  
//
////////////////////////////////////////////////////////////////////////////////

LPCWSTR CResultsDisplayerBase::FormatNumberWithSeparators (ULONGLONG n)
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
