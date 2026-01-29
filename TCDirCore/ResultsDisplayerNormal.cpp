#include "pch.h"
#include "ResultsDisplayerNormal.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "Flag.h"
#include "UnicodeSymbols.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerNormal::CResultsDisplayerNormal
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

CResultsDisplayerNormal::CResultsDisplayerNormal (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr) :
    CResultsDisplayerWithHeaderAndFooter (cmdLinePtr, consolePtr, configPtr)
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
        WORD             textAttr    = m_configPtr->GetTextAttrForFile (fileInfo);
        ECloudStatus     cloudStatus = GetCloudStatus (fileInfo, fInSyncRoot);
        const FILETIME & ftDisplay   = GetTimeFieldForDisplay (fileInfo);

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

    m_consolePtr->Printf (CConfig::EAttribute::Date,    L"%s", szDate);
    m_consolePtr->Printf (CConfig::EAttribute::Default, L"  ");

    m_consolePtr->Printf (CConfig::EAttribute::Time,    L"%s", szTime);
    m_consolePtr->Printf (CConfig::EAttribute::Default, L" ");


    
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
    struct SFileAttributeEntry
    {
        DWORD m_dwAttr;
        WCHAR m_chAttr;
    };
    
    static constexpr SFileAttributeEntry s_krgAttrMap[] =
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
        CConfig::EAttribute attr      = CConfig::EAttribute::FileAttributeNotPresent;
        WCHAR               chDisplay = L'-';
        

        if (dwFileAttributes & fileAttributeEntry.m_dwAttr)
        {
            attr = CConfig::EAttribute::FileAttributePresent;
            chDisplay = fileAttributeEntry.m_chAttr;
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
    static constexpr WCHAR  kszDirSize[] = L"<DIR>";
    static constexpr size_t kcchDirSize  = ARRAYSIZE (kszDirSize) - 1;

    size_t         cchMaxFileSize = max (cchStringLengthOfMaxFileSize, kcchDirSize);
    ULARGE_INTEGER uliFileSize;
    


    uliFileSize.LowPart  = fileInfo.nFileSizeLow;
    uliFileSize.HighPart = fileInfo.nFileSizeHigh;

    if ((fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
    {
        m_consolePtr->Printf (CConfig::EAttribute::Size, 
                              L" %*s ", 
                              cchMaxFileSize, 
                              FormatNumberWithSeparators (uliFileSize.QuadPart));
    }
    else
    {
        size_t cchLeftSidePadding = (cchMaxFileSize - kcchDirSize) / 2;            
        m_consolePtr->Printf (CConfig::EAttribute::Directory, 
                              L" %*s%-*s ", 
                              cchLeftSidePadding, 
                              L"", 
                              cchMaxFileSize - cchLeftSidePadding, 
                              kszDirSize);
    }        
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerNormal::IsUnderSyncRoot
//
//  Checks if a path is under a cloud storage sync root (OneDrive, etc.)
//  using the Cloud Files API. This is called once per directory.
// 
////////////////////////////////////////////////////////////////////////////////  

bool CResultsDisplayerNormal::IsUnderSyncRoot (LPCWSTR pszPath)
{
    //
    // CfGetSyncRootInfoByPath fails if the path is not under a sync root,
    // so we just need to check if it succeeds. We don't need the actual info.
    //

    HRESULT                 hr   = S_OK;
    CF_SYNC_ROOT_BASIC_INFO info = {};
    


    hr = CfGetSyncRootInfoByPath (pszPath, CF_SYNC_ROOT_INFO_BASIC, &info, sizeof (info), nullptr);

    return SUCCEEDED (hr);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerNormal::GetCloudStatus
//
//  Determines cloud sync status using a hybrid approach:
//  1. Check explicit file attributes (PINNED, UNPINNED, RECALL_*, OFFLINE)
//  2. If no explicit attributes but we're in a sync root, the file is
//     locally synced (fully hydrated files lose their placeholder metadata)
//
//  The fInSyncRoot parameter is determined once per directory using
//  CfGetSyncRootInfoByPath to avoid per-file overhead.
// 
////////////////////////////////////////////////////////////////////////////////  

ECloudStatus CResultsDisplayerNormal::GetCloudStatus (const WIN32_FIND_DATA & wfd, bool fInSyncRoot)
{
    ECloudStatus status = ECloudStatus::CS_NONE;



    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_PINNED)
    {
        // Pinned takes priority - always available locally
        status = ECloudStatus::CS_PINNED;
    }
    else if (wfd.dwFileAttributes & (FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS | 
                                     FILE_ATTRIBUTE_RECALL_ON_OPEN | 
                                     FILE_ATTRIBUTE_OFFLINE))
    {
        // Cloud-only: placeholder that requires download
        status = ECloudStatus::CS_CLOUD_ONLY;
    }
    else if (wfd.dwFileAttributes & FILE_ATTRIBUTE_UNPINNED)
    {
        // Unpinned means locally available but can be dehydrated
        status = ECloudStatus::CS_LOCAL;
    }
    else if (fInSyncRoot)
    {
        // No cloud attributes set - if we're in a sync root, the file is
        // fully hydrated (locally synced). OneDrive removes placeholder
        // metadata from fully hydrated files.
        status = ECloudStatus::CS_LOCAL;
    }

    return status;
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
    CConfig::EAttribute    attr         = CConfig::EAttribute::Default;
    WCHAR                  symbol       = L' ';



    switch (status)
    {
        case ECloudStatus::CS_CLOUD_ONLY:
            attr   = CConfig::EAttribute::CloudStatusCloudOnly;
            symbol = UnicodeSymbols::CircleHollow;
            break;

        case ECloudStatus::CS_LOCAL:
            attr   = CConfig::EAttribute::CloudStatusLocallyAvailable;
            symbol = UnicodeSymbols::CircleHalfFilled;
            break;

        case ECloudStatus::CS_PINNED:
            attr   = CConfig::EAttribute::CloudStatusAlwaysLocallyAvailable;
            symbol = UnicodeSymbols::CircleFilled;
            break;

        case ECloudStatus::CS_NONE:
        default:
            break;
    }

    m_consolePtr->Printf (attr, L"%c ", symbol);
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
    m_consolePtr->Printf (CConfig::EAttribute::Owner, L"%-*s ", static_cast<int>(cchColumnWidth), owner.c_str ());
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
    //   Size: " %*s " (1 leading space + width + 1 trailing space)
    //   Cloud status: 2 chars (symbol + space)
    //   Owner: cchOwnerWidth + 1 for trailing space (if showing owner)
    // Use same width calculation as DisplayResultsNormalFileSize (max of file size or 5 for "<DIR>")
    //

    size_t cchMaxFileSize = max (cchStringLengthOfMaxFileSize, size_t (5));

    for (const SStreamInfo & si : fileEntry.m_vStreams)
    {
        LPCWSTR pszStreamSize = FormatNumberWithSeparators (si.m_liSize.QuadPart);

        m_consolePtr->Printf (CConfig::EAttribute::Default, L"%*c", 30, L' ');                      // 30 spaces (21 + 9)
        m_consolePtr->Printf (CConfig::EAttribute::Size,    L" %*s ", static_cast<int>(cchMaxFileSize), pszStreamSize);
        m_consolePtr->Printf (CConfig::EAttribute::Default, L"  ");                                 // Cloud status placeholder

        if (cchOwnerWidth > 0)
        {
            m_consolePtr->Printf (CConfig::EAttribute::Default, L"%*c", static_cast<int>(cchOwnerWidth + 1), L' ');
        }

        m_consolePtr->Printf (CConfig::EAttribute::Stream,  L"%s%s\n", fileEntry.cFileName, si.m_strName.c_str ());
    }
}
