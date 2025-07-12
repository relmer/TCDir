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

CResultsDisplayerNormal::CResultsDisplayerNormal (__in CCommandLine * pCmdLine, __in CConsole * pConsole, __in CConfig * pConfig) :
    CResultsDisplayerBase (pCmdLine, pConsole, pConfig)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerNormal::DisplayFileResults
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerNormal::DisplayFileResults (__in CDirectoryInfo * pdi)
{
    HRESULT hr                           = S_OK;
    size_t  cchStringLengthOfMaxFileSize = GetStringLengthOfMaxFileSize (&pdi->m_uliLargestFileSize);

    

    for (const WIN32_FIND_DATA & fileInfo : pdi->m_vMatches)
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

    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Date],    L"%s", szDate);
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"  ");
    
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Time],    L"%s", szTime);
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L" ");

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
//  CResultsDisplayerNormal::DisplayResultsNormalFileSize
//
//  Displays the file size
//  
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerNormal::DisplayResultsNormalFileSize (const WIN32_FIND_DATA & fileInfo, size_t cchStringLengthOfMaxFileSize)
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
