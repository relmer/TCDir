#include "pch.h"
#include "ResultsDisplayerNormal.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "Flag.h"





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
    HRESULT hr                           = S_OK;
    size_t  cchStringLengthOfMaxFileSize = GetStringLengthOfMaxFileSize (di.m_uliLargestFileSize);
    


    for (const WIN32_FIND_DATA & fileInfo : di.m_vMatches)
    {
        WORD         textAttr    = m_configPtr->GetTextAttrForFile (fileInfo);
        ECloudStatus cloudStatus = GetCloudStatus (fileInfo.dwFileAttributes);

        hr = DisplayResultsNormalDateAndTime (fileInfo.ftLastWriteTime);
        CHR (hr);

        DisplayResultsNormalAttributes  (fileInfo.dwFileAttributes);
        DisplayResultsNormalFileSize    (fileInfo, cchStringLengthOfMaxFileSize);
        DisplayCloudStatusSymbol        (cloudStatus);

        if (m_cmdLinePtr->m_fDebug)
        {
            DisplayRawAttributes (fileInfo.dwFileAttributes);
        }

        m_consolePtr->Printf (textAttr, L"%s\n", fileInfo.cFileName);
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
//  CResultsDisplayerNormal::GetCloudStatus
//
//  Determines cloud sync status from file attributes
// 
////////////////////////////////////////////////////////////////////////////////  

ECloudStatus CResultsDisplayerNormal::GetCloudStatus (DWORD dwFileAttributes)
{
    ECloudStatus status = ECloudStatus::CS_NONE;



    //
    // Pinned takes priority - always available locally
    //
    
    if (dwFileAttributes & FILE_ATTRIBUTE_PINNED)
    {
        status = ECloudStatus::CS_PINNED;
    }

    //
    // Cloud-only: placeholder that requires download
    //
    
    else if (dwFileAttributes & (FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS | 
                                 FILE_ATTRIBUTE_RECALL_ON_OPEN | 
                                 FILE_ATTRIBUTE_OFFLINE))
    {
        status = ECloudStatus::CS_CLOUD_ONLY;
    }

    //
    // Unpinned means locally available but can be dehydrated
    //
    
    else if (dwFileAttributes & FILE_ATTRIBUTE_UNPINNED)
    {
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
    static constexpr WCHAR kchCloudOnly = L'\x25CB';  // ○ (hollow circle - not locally available)
    static constexpr WCHAR kchLocal     = L'\x25D0';  // ◐ (half-filled - local but can dehydrate)
    static constexpr WCHAR kchPinned    = L'\x25CF';  // ● (filled - always local)

    CConfig::EAttribute    attr         = CConfig::EAttribute::Default;
    WCHAR                  symbol       = L' ';



    switch (status)
    {
        case ECloudStatus::CS_CLOUD_ONLY:
            attr   = CConfig::EAttribute::CloudStatusCloudOnly;
            symbol = kchCloudOnly;
            break;

        case ECloudStatus::CS_LOCAL:
            attr   = CConfig::EAttribute::CloudStatusLocal;
            symbol = kchLocal;
            break;

        case ECloudStatus::CS_PINNED:
            attr   = CConfig::EAttribute::CloudStatusPinned;
            symbol = kchPinned;
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
//  Displays raw file attributes in hex format for debugging
// 
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerNormal::DisplayRawAttributes (DWORD dwFileAttributes)
{
    m_consolePtr->Printf (CConfig::EAttribute::Information, L"[%08X] ", dwFileAttributes);
}
