#include "pch.h"
#include "ResultsDisplayerNormal.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "FileAttributeMap.h"
#include "IconMapping.h"
#include "UnicodeSymbols.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerNormal::CResultsDisplayerNormal
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

CResultsDisplayerNormal::CResultsDisplayerNormal (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr, bool fIconsActive) :
    CResultsDisplayerWithHeaderAndFooter (cmdLinePtr, consolePtr, configPtr, fIconsActive)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerNormal::DisplayFileResults
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerNormal::DisplayFileResults (const CDirectoryInfo & di)
{
    HRESULT         hr                           = S_OK;
    size_t          cchStringLengthOfMaxFileSize = GetStringLengthOfMaxFileSize (di.m_uliLargestFileSize);
    bool            fInSyncRoot                  = IsUnderSyncRoot (di.m_dirPath.c_str());
    vector<wstring> owners;
    size_t          cchMaxOwnerLength            = 0;
    


    //
    // If showing owners, pre-calculate all owner strings to determine column width
    //

    if (m_cmdLinePtr->m_fShowOwner)
    {
        GetFileOwners (di, owners, cchMaxOwnerLength);
    }

    //
    // Display each file
    //

    for (auto && [idxFile, fileInfo] : views::enumerate (di.m_vMatches))
    {
        CConfig::SFileDisplayStyle   style       = m_configPtr->GetDisplayStyleForFile (fileInfo);
        WORD                         textAttr    = style.m_wTextAttr;
        ECloudStatus                 cloudStatus = GetCloudStatus (fileInfo, fInSyncRoot);
        const FILETIME             & ftDisplay   = GetTimeFieldForDisplay (fileInfo);



        hr = DisplayResultsNormalDateAndTime (ftDisplay);
        CHR (hr);

        DisplayResultsNormalAttributes  (fileInfo.dwFileAttributes);
        DisplayResultsNormalFileSize    (fileInfo, cchStringLengthOfMaxFileSize);
        DisplayCloudStatusSymbol        (cloudStatus);

        if (m_cmdLinePtr->m_fDebug)
        {
            DisplayRawAttributes (fileInfo);
        }

        if (m_cmdLinePtr->m_fShowOwner)
        {
            DisplayFileOwner (owners[idxFile], cchMaxOwnerLength);
        }

        //
        // Display icon glyph before filename (when icons are active)
        //

        if (m_fIconsActive && style.m_iconCodePoint != 0 && !style.m_fIconSuppressed)
        {
            WideCharPair pair      = CodePointToWideChars (style.m_iconCodePoint);
            wchar_t      szIcon[3] = { pair.chars[0], pair.chars[1], L'\0' };

            m_consolePtr->Printf (textAttr, L"%s ", szIcon);
        }

        m_consolePtr->Printf (textAttr, L"%s\n", fileInfo.cFileName);

        //
        // If showing streams and this is a file (not a directory), display any alternate data streams
        //

        if (m_cmdLinePtr->m_fShowStreams && !(fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            DisplayFileStreams (fileInfo, cchStringLengthOfMaxFileSize, cchMaxOwnerLength);
        }
    }
    


Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerNormal::DisplayResultsNormalDateAndTime
//
//  Displays the date and time from the given FILETIME 
// 
////////////////////////////////////////////////////////////////////////////////  

HRESULT CResultsDisplayerNormal::DisplayResultsNormalDateAndTime (const FILETIME & ftLastWriteTime)
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

    m_consolePtr->ColorPrintf (L"{Date}%s  {Time}%s{Default} ", szDate, szTime);


    
Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerNormal::GetTimeFieldForDisplay
//
//  Returns the appropriate FILETIME based on the /T: switch setting
// 
////////////////////////////////////////////////////////////////////////////////  

const FILETIME & CResultsDisplayerNormal::GetTimeFieldForDisplay (const WIN32_FIND_DATA & wfd) const
{
    switch (m_cmdLinePtr->m_timeField)
    {
        case CCommandLine::ETimeField::TF_CREATION:
            return wfd.ftCreationTime;

        case CCommandLine::ETimeField::TF_ACCESS:
            return wfd.ftLastAccessTime;

        case CCommandLine::ETimeField::TF_WRITTEN:
        default:
            return wfd.ftLastWriteTime;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerNormal::DisplayResultsNormalAttributes
//
//  Displays the file attributes
// 
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerNormal::DisplayResultsNormalAttributes  (DWORD dwFileAttributes)
{
    for (const SFileAttributeMap fileAttributeEntry : k_rgFileAttributeMap)
    {
        CConfig::EAttribute attr      = CConfig::EAttribute::FileAttributeNotPresent;
        WCHAR               chDisplay = L'-';
        

        if (dwFileAttributes & fileAttributeEntry.m_dwAttribute)
        {
            attr = CConfig::EAttribute::FileAttributePresent;
            chDisplay = fileAttributeEntry.m_chKey;
        }
        
        m_consolePtr->Printf (attr, L"%c", chDisplay);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerNormal::DisplayResultsNormalFileSize
//
//  Displays the file size
//  
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerNormal::DisplayResultsNormalFileSize (const WIN32_FIND_DATA & fileInfo, size_t cchStringLengthOfMaxFileSize)
{
    static constexpr WCHAR  kszDirSize[]    = L"<DIR>";
    static constexpr size_t kcchDirSize     = ARRAYSIZE (kszDirSize) - 1;
    static constexpr size_t kcchAbbreviated = 7;

    ULARGE_INTEGER uliFileSize;



    uliFileSize.LowPart  = fileInfo.nFileSizeLow;
    uliFileSize.HighPart = fileInfo.nFileSizeHigh;

    //
    // Abbreviated size mode (Auto): fixed 7-character field, Explorer-style
    //

    if (m_cmdLinePtr->m_eSizeFormat == ESizeFormat::Auto)
    {
        if ((fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        {
            m_consolePtr->Printf (CConfig::EAttribute::Size, L"  %7s", FormatAbbreviatedSize (uliFileSize.QuadPart).c_str());
        }
        else
        {
            m_consolePtr->Printf (CConfig::EAttribute::Directory, L" <DIR>   ");
        }

        return;
    }

    //
    // Bytes mode (comma-separated exact size): variable-width column
    //

    size_t cchMaxFileSize = max (cchStringLengthOfMaxFileSize, kcchDirSize);

    if ((fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        m_consolePtr->Printf (CConfig::EAttribute::Size, 
                              L"  %*s", 
                              cchMaxFileSize, 
                              FormatNumberWithSeparators (uliFileSize.QuadPart).c_str());
    }
    else
    {
        size_t cchLeftSidePadding = (cchMaxFileSize - kcchDirSize) / 2;            
        m_consolePtr->Printf (CConfig::EAttribute::Directory, 
                              L"  %*s%-*s", 
                              cchLeftSidePadding, 
                              L"", 
                              cchMaxFileSize - cchLeftSidePadding, 
                              kszDirSize);
    }        
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerNormal::FormatAbbreviatedSize
//
//  Explorer-style abbreviated file size: 1024-based division with 3
//  significant digits and a fixed 7-character width.  The numeric
//  portion is right-justified in a 4-character field, followed by a
//  space separator, followed by the unit label left-justified in a
//  2-character field.  This ensures numbers and suffixes each align
//  in their own sub-column.
//
//  Range                 Format       Example
//  0                     0 B          "   0 B "
//  1-999                 ### B        " 426 B "
//  1000-1023             1 KB         "   1 KB"      (matches Explorer rounding)
//  1024-10239            X.XX KB      "4.61 KB"
//  10240-102399          XX.X KB      "17.1 KB"
//  102400-1048575        ### KB       " 976 KB"
//  1 MB+                 same 3-sig   "16.7 MB"
//  1 GB+                 same         "1.39 GB"
//  1 TB+                 same         "1.00 TB"
//
////////////////////////////////////////////////////////////////////////////////

wstring CResultsDisplayerNormal::FormatAbbreviatedSize (ULONGLONG cbSize)
{
    static constexpr LPCWSTR s_krgSuffixes[] = { L"B", L"KB", L"MB", L"GB", L"TB", L"PB", L"EB" };
    static constexpr size_t  s_kcSuffixes    = ARRAYSIZE (s_krgSuffixes);

    WCHAR szBuf[16] = {};



    //
    // Bytes range: 0-999 displayed as integer bytes
    //

    if (cbSize < 1000)
    {
        swprintf_s (szBuf, L"%4llu %-2s", cbSize, L"B");
        return szBuf;
    }

    //
    // 1000-1023 bytes: Explorer shows "1 KB" (rounds up)
    //

    if (cbSize < 1024)
    {
        return L"   1 KB";
    }

    //
    // Divide by 1024 repeatedly until value fits in 3 significant digits.
    // Use double for fractional display.
    //

    double   dValue      = static_cast<double>(cbSize);
    size_t   idxSuffix   = 0;

    while (dValue >= 1024.0 && idxSuffix + 1 < s_kcSuffixes)
    {
        dValue /= 1024.0;
        ++idxSuffix;
    }

    //
    // Three-significant-digit formatting:
    //   <10    → X.XX  (e.g., "4.61 KB")
    //   <100   → XX.X  (e.g., "17.1 KB")
    //   >=100  → ###   (e.g., " 976 KB")
    //

    if (dValue < 10.0)
    {
        swprintf_s (szBuf, L"%4.2f %-2s", dValue, s_krgSuffixes[idxSuffix]);
    }
    else if (dValue < 100.0)
    {
        swprintf_s (szBuf, L"%4.1f %-2s", dValue, s_krgSuffixes[idxSuffix]);
    }
    else
    {
        swprintf_s (szBuf, L"%4.0f %-2s", dValue, s_krgSuffixes[idxSuffix]);
    }

    return szBuf;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerNormal::DisplayCloudStatusSymbol
//
//  Displays cloud status symbol with appropriate color
// 
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerNormal::DisplayCloudStatusSymbol (ECloudStatus status)
{
    struct SCloudStatusEntry
    {
        LPCWSTR pszColorMarker;
        WCHAR   chSymbol;
    };

    static constexpr SCloudStatusEntry s_krgCloudStatusMap[] =
    {
        { L"{Default}",                            L' '                             },  // CS_NONE
        { L"{CloudStatusCloudOnly}",               UnicodeSymbols::CircleHollow     },  // CS_CLOUD_ONLY
        { L"{CloudStatusLocallyAvailable}",        UnicodeSymbols::CircleHalfFilled },  // CS_LOCAL
        { L"{CloudStatusAlwaysLocallyAvailable}",  UnicodeSymbols::CircleFilled     },  // CS_PINNED
    };

    static constexpr LPCWSTR s_krgCloudColorMarkers[] =
    {
        L"{Default}",
        L"{CloudStatusCloudOnly}",
        L"{CloudStatusLocallyAvailable}",
        L"{CloudStatusAlwaysLocallyAvailable}",
    };



    if (m_fIconsActive)
    {
        size_t   idx    = static_cast<size_t>(status);
        char32_t iconCP = m_configPtr->GetCloudStatusIcon (static_cast<DWORD>(status));



        if (iconCP != 0)
        {
            WideCharPair pair   = CodePointToWideChars (iconCP);
            wchar_t      szIcon[3] = { pair.chars[0], pair.chars[1], L'\0' };


            
            m_consolePtr->ColorPrintf (L"%s%s ", s_krgCloudColorMarkers[idx], szIcon);
        }
        else
        {
            m_consolePtr->ColorPrintf (L"{Default}  ");
        }
    }
    else
    {
        const SCloudStatusEntry & entry = s_krgCloudStatusMap[static_cast<size_t>(status)];



        m_consolePtr->ColorPrintf (L"%s%c ", entry.pszColorMarker, entry.chSymbol);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerNormal::DisplayRawAttributes
//
//  Displays raw file attributes and cfapi state in hex format for debugging.
//  Format: [XXXXXXXX:YY] where X = file attributes, Y = cfapi placeholder state
// 
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerNormal::DisplayRawAttributes (const WIN32_FIND_DATA & wfd)
{
    CF_PLACEHOLDER_STATE cfState = CfGetPlaceholderStateFromFindData (&wfd);

    m_consolePtr->Printf (CConfig::EAttribute::Information, L"[%08X:%02X] ", 
                          wfd.dwFileAttributes, 
                          static_cast<DWORD>(cfState));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerNormal::GetFileOwner
//
//  Retrieves the file owner in DOMAIN\User format.
//  Returns "Unknown" if access is denied or the owner cannot be determined.
// 
////////////////////////////////////////////////////////////////////////////////  

wstring CResultsDisplayerNormal::GetFileOwner (LPCWSTR pszFilePath)
{
    static constexpr LPCWSTR kszUnknown = L"Unknown";
    
    HRESULT               hr            = S_OK;
    PSID                  pSidOwner     = nullptr;
    PSECURITY_DESCRIPTOR  pSD           = nullptr;
    DWORD                 dwResult      = ERROR_SUCCESS;
    BOOL                  fSuccess      = FALSE;
    WCHAR                 szName[256]   = { 0 };
    WCHAR                 szDomain[256] = { 0 };
    DWORD                 cchName       = ARRAYSIZE (szName);
    DWORD                 cchDomain     = ARRAYSIZE (szDomain);
    SID_NAME_USE          sidUse        = SidTypeUnknown;
    wstring               result          (kszUnknown);



    //
    // Get the owner SID from the file's security descriptor
    //
    dwResult = GetNamedSecurityInfoW (pszFilePath, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION,
                                      &pSidOwner, nullptr, nullptr, nullptr, &pSD);
    CHRA (HRESULT_FROM_WIN32 (dwResult));

    //
    // Look up the account name for the SID
    //
    fSuccess = LookupAccountSidW (nullptr, pSidOwner, szName, &cchName, szDomain, &cchDomain, &sidUse);
    CWRA (fSuccess);

    //
    // Format as DOMAIN\User or just User if domain is empty
    //
    if (cchDomain > 1 && szDomain[0] != L'\0')
    {
        result = format (L"{}\\{}", szDomain, szName);
    }
    else
    {
        result = szName;
    }


Error:
    if (pSD != nullptr)
    {
        LocalFree (pSD);
    }

    return result;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerNormal::DisplayFileOwner
//
//  Displays the file owner column with the specified width
// 
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerNormal::DisplayFileOwner (const wstring & owner, size_t cchColumnWidth)
{
    int cchPadding = static_cast<int>(cchColumnWidth - owner.length() + 1);

    m_consolePtr->ColorPrintf (L"{Owner}%s{Default}%*s", owner.c_str(), cchPadding, L"");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerNormal::GetFileOwners
//
//  Pre-calculates owner strings for all files in the directory.
//  Returns the owners vector and the maximum owner string length for column sizing.
// 
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerNormal::GetFileOwners (const CDirectoryInfo & di, vector<wstring> & owners, size_t & cchMaxOwnerLength)
{
    owners.reserve (di.m_vMatches.size ());
    cchMaxOwnerLength = 0;

    for (const auto & fileInfo : di.m_vMatches)
    {
        filesystem::path fullPath = di.m_dirPath / fileInfo.cFileName;
        wstring          owner    = GetFileOwner (fullPath.c_str ());
        
        cchMaxOwnerLength = max (cchMaxOwnerLength, owner.length ());
        owners.push_back (move (owner));
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerNormal::DisplayFileStreams
//
//  Displays pre-collected alternate data streams for a file.
//  Format matches CMD.exe /R output: indented stream size and name.
// 
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerNormal::DisplayFileStreams (const FileInfo & fileEntry, size_t cchStringLengthOfMaxFileSize, size_t cchOwnerWidth)
{
    //
    // Format: indentation + size + filename:streamname
    //
    // Match normal file output format:
    //   Date/time: 21 chars (10 date + 2 spaces + 8 time + 1 space)
    //   Attributes: 9 chars
    //   Size: "  %*s" (2 leading spaces + width, no trailing space)
    //   Cloud status: 2 chars (symbol + space)
    //   Owner: cchOwnerWidth + 1 for trailing space (if showing owner)
    // Use same width calculation as DisplayResultsNormalFileSize (max of file size or 5 for "<DIR>")
    //

    size_t cchMaxFileSize = max (cchStringLengthOfMaxFileSize, size_t (5));

    for (const SStreamInfo & si : fileEntry.m_vStreams)
    {
        wstring pszStreamSize   = FormatNumberWithSeparators (si.m_liSize.QuadPart);
        int     cchOwnerPadding = (cchOwnerWidth > 0) ? static_cast<int>(cchOwnerWidth + 1) : 0;

        m_consolePtr->ColorPrintf (L"{Default}%*c{Size}  %*s{Default}  %*s{Stream}%s%s\n",
                                   30, L' ',
                                   static_cast<int>(cchMaxFileSize), pszStreamSize.c_str(),
                                   cchOwnerPadding, L"",
                                   fileEntry.cFileName, 
                                   si.m_strName.c_str());
    }
}
