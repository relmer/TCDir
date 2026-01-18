#include "pch.h"
#include "ResultsDisplayerWithHeaderAndFooter.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWithHeaderAndFooter::CResultsDisplayerWithHeaderAndFooter
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

CResultsDisplayerWithHeaderAndFooter::CResultsDisplayerWithHeaderAndFooter (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr) :
    m_cmdLinePtr (cmdLinePtr),
    m_consolePtr (consolePtr),
    m_configPtr  (configPtr)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWithHeaderAndFooter::~CResultsDisplayerWithHeaderAndFooter
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

CResultsDisplayerWithHeaderAndFooter::~CResultsDisplayerWithHeaderAndFooter (void)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWithHeaderAndFooter::DisplayResults
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerWithHeaderAndFooter::DisplayResults (const CDriveInfo & driveInfo, const CDirectoryInfo & di, EDirectoryLevel level)
{
    // For subdirectories with no matches, skip displaying entirely
    if (level == EDirectoryLevel::Subdirectory && di.m_vMatches.size() == 0)
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
        
        m_consolePtr->WriteSeparatorLine (m_configPtr->m_rgAttributes[CConfig::EAttribute::SeparatorLine]);

        //
        // We'll only show the drive header on the initial directory processed.
        // Any recursed subdirs won't show it.
        //

        DisplayDriveHeader (driveInfo);
    }

    DisplayPathHeader (di.m_dirPath); 

    if (di.m_vMatches.size() == 0)
    {
        if (di.m_fileSpec == L"*")
        {
            m_consolePtr->Puts (CConfig::EAttribute::Default, L"Directory is empty.");
        }
        else
        {
            m_consolePtr->Printf (CConfig::EAttribute::Default, L"No files matching '%s' found.\n", di.m_fileSpec.c_str());
        }
    }
    else 
    {
        DisplayFileResults      (di);
        DisplayDirectorySummary (di);

        //
        // Only show the volume footer here if we're not doing a recursive list.
        // Otherwise, it'll be shown after the recursive summary.
        //

        if (!m_cmdLinePtr->m_fRecurse)
        {
            DisplayVolumeFooter (di);
        }
    }
        
    m_consolePtr->Puts               (CConfig::EAttribute::Default, L"");
    m_consolePtr->WriteSeparatorLine (m_configPtr->m_rgAttributes[CConfig::EAttribute::SeparatorLine]);
    m_consolePtr->Puts               (CConfig::EAttribute::Default, L"");

    m_consolePtr->Flush();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWithHeaderAndFooter::DisplayRecursiveSummary
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerWithHeaderAndFooter::DisplayRecursiveSummary (const CDirectoryInfo & diInitial, UINT cFilesFound, UINT cDirectoriesFound, const ULARGE_INTEGER & uliSizeOfAllFilesFound)
{
    DisplayListingSummary (diInitial, cFilesFound, cDirectoriesFound, uliSizeOfAllFilesFound);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWithHeaderAndFooter::DisplayDriveHeader
//
//  Displays information about the drive, eg: 
// 
//      Volume in drive C is a hard drive (NTFS)
//      Volume Name is "Maxtor 250GB"
//
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerWithHeaderAndFooter::DisplayDriveHeader (const CDriveInfo & driveInfo)
{
    m_consolePtr->Printf (CConfig::EAttribute::Information, L" Volume ");

    if (driveInfo.IsUncPath())
    {
        m_consolePtr->Printf (CConfig::EAttribute::InformationHighlight, L"%s", driveInfo.GetUncPath().c_str());
    }
    else
    {
        m_consolePtr->Printf (CConfig::EAttribute::Information,          L"in drive ");
        m_consolePtr->Printf (CConfig::EAttribute::InformationHighlight, L"%c", driveInfo.GetRootPath().c_str()[0]);
    }        

    m_consolePtr->Printf (CConfig::EAttribute::Information,          L" is ");
    m_consolePtr->Printf (CConfig::EAttribute::InformationHighlight, L"%s", driveInfo.GetVolumeDescription().c_str());

    //
    // If this is a mapped drive, get the remote name that it's mapped to
    //
    
    const wstring & remoteName = driveInfo.GetRemoteName();
    if (remoteName.length() > 0)
    {
        m_consolePtr->Printf (CConfig::EAttribute::Information,          L" mapped to ");
        m_consolePtr->Printf (CConfig::EAttribute::InformationHighlight, L"%s", remoteName.c_str());
    }

    m_consolePtr->Printf (CConfig::EAttribute::Information,          L" (");
    m_consolePtr->Printf (CConfig::EAttribute::InformationHighlight, L"%s", driveInfo.GetFileSystemName());
    m_consolePtr->Printf (CConfig::EAttribute::Information,          L")\n");

    LPCWSTR pszVolumeName = driveInfo.GetVolumeName();
    if (pszVolumeName[0] != L'\0')
    {
        m_consolePtr->Printf (CConfig::EAttribute::Information,          L" Volume name is \"");
        m_consolePtr->Printf (CConfig::EAttribute::InformationHighlight, L"%s", pszVolumeName);
        m_consolePtr->Printf (CConfig::EAttribute::Information,          L"\"\n\n");
    }
    else
    {
        m_consolePtr->Puts (CConfig::EAttribute::Information, L" Volume has no name\n");
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWithHeaderAndFooter::DisplayPathHeader
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerWithHeaderAndFooter::DisplayPathHeader (const filesystem::path & dirPath)
{
    m_consolePtr->Printf (CConfig::EAttribute::Information,          L" Directory of ");
    m_consolePtr->Printf (CConfig::EAttribute::InformationHighlight, L"%s", dirPath.c_str());
    m_consolePtr->Printf (CConfig::EAttribute::Information,          L"\n\n");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWithHeaderAndFooter::DisplayListingSummary
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

void CResultsDisplayerWithHeaderAndFooter::DisplayListingSummary (const CDirectoryInfo & di, UINT cFilesFound, UINT cDirectoriesFound, const ULARGE_INTEGER & uliSizeOfAllFilesFound)
{
    int cMaxDigits = 1;

    if (cFilesFound > 0 || cDirectoriesFound > 0)
    { 
        cMaxDigits = (int) log10 (max (cFilesFound, cDirectoriesFound)) + 1;
        cMaxDigits += cMaxDigits / 3;  // add space for each comma
    }

    m_consolePtr->Printf (CConfig::EAttribute::Information,          L" Total files listed:");
    m_consolePtr->Puts   (CConfig::EAttribute::Default,              L"\n");

    m_consolePtr->Printf (CConfig::EAttribute::InformationHighlight, L"    %*s", cMaxDigits, FormatNumberWithSeparators (cFilesFound));
    m_consolePtr->Printf (CConfig::EAttribute::Information,          cFilesFound == 1 ? L" file using " : L" files using ");
    m_consolePtr->Printf (CConfig::EAttribute::InformationHighlight, FormatNumberWithSeparators (uliSizeOfAllFilesFound.QuadPart));
    m_consolePtr->Printf (CConfig::EAttribute::Information,          uliSizeOfAllFilesFound.QuadPart == 1 ? L" byte\n" : L" bytes\n");

    m_consolePtr->Printf (CConfig::EAttribute::InformationHighlight, L"    %*s", cMaxDigits, FormatNumberWithSeparators (cDirectoriesFound));
    m_consolePtr->Puts   (CConfig::EAttribute::Information,          cDirectoriesFound == 1 ? L" subdirectory" : L" subdirectories");

    m_consolePtr->Puts   (CConfig::EAttribute::Default,              L"");
    
    DisplayVolumeFooter (di);

    m_consolePtr->WriteSeparatorLine (m_configPtr->m_rgAttributes[CConfig::EAttribute::SeparatorLine]);
    m_consolePtr->Puts               (CConfig::EAttribute::Default, L"");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWithHeaderAndFooter::DisplayDirectorySummary
//
//  Display summary information for the directory:
//
//   4 directories, 27 files using 284,475 bytes
//
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerWithHeaderAndFooter::DisplayDirectorySummary (const CDirectoryInfo & di)
{
    m_consolePtr->Printf (CConfig::EAttribute::Information,          L"\n ");
    m_consolePtr->Printf (CConfig::EAttribute::InformationHighlight, L"%d", di.m_cSubDirectories);
    m_consolePtr->Printf (CConfig::EAttribute::Information,          di.m_cSubDirectories == 1 ? L" dir, " : L" dirs, ");

    m_consolePtr->Printf (CConfig::EAttribute::InformationHighlight, L"%d", di.m_cFiles);
    m_consolePtr->Printf (CConfig::EAttribute::Information,          di.m_cFiles == 1 ? L" file using " : L" files using ");
    m_consolePtr->Printf (CConfig::EAttribute::InformationHighlight, FormatNumberWithSeparators (di.m_uliBytesUsed.QuadPart));
    m_consolePtr->Printf (CConfig::EAttribute::Information,          di.m_uliBytesUsed.QuadPart == 1 ? L" byte\n" : L" bytes\n");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWithHeaderAndFooter::DisplayVolumeFooter
//
//  Display free space info for volume:
//
//   123,123,123,123 bytes free on volume
//   123,117,699,072 bytes available to user %s
//
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerWithHeaderAndFooter::DisplayVolumeFooter (const CDirectoryInfo & di)
{
    HRESULT        hr                     = S_OK;
    BOOL           fSuccess;              
    ULARGE_INTEGER uliFreeBytesAvailable; 
    ULARGE_INTEGER uliTotalBytes;         
    ULARGE_INTEGER uliTotalFreeBytes;     

    

    fSuccess = GetDiskFreeSpaceEx (di.m_dirPath.c_str(), &uliFreeBytesAvailable, &uliTotalBytes, &uliTotalFreeBytes);
    CBRA (fSuccess);

    m_consolePtr->Printf (CConfig::EAttribute::InformationHighlight, L" %s", FormatNumberWithSeparators (uliTotalFreeBytes.QuadPart));
    m_consolePtr->Printf (CConfig::EAttribute::Information,          uliTotalFreeBytes.QuadPart == 1 ? L" byte free on volume\n" : L" bytes free on volume\n");

    if (uliFreeBytesAvailable.QuadPart != uliTotalFreeBytes.QuadPart)
    {
        DisplayFooterQuotaInfo (uliFreeBytesAvailable);
    }
    


Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWithHeaderAndFooter::DisplayFooterQuotaInfo
//
//  Display free space info for volume:
//
//   123,117,699,072 bytes available to user %s
//
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerWithHeaderAndFooter::DisplayFooterQuotaInfo (const ULARGE_INTEGER & uliFreeBytesAvailable)
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

    m_consolePtr->Printf(CConfig::EAttribute::Information,          L" ");
    m_consolePtr->Printf(CConfig::EAttribute::InformationHighlight, FormatNumberWithSeparators (uliFreeBytesAvailable.QuadPart));
    m_consolePtr->Printf(CConfig::EAttribute::Information,          uliFreeBytesAvailable.QuadPart == 1 ? L" byte available to " : L" bytes available to ");
    m_consolePtr->Printf(CConfig::EAttribute::InformationHighlight, strUsername.c_str());
    m_consolePtr->Printf(CConfig::EAttribute::Information,          L" due to quota\n");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWithHeaderAndFooter::GetStringLengthOfMaxFileSize
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

UINT CResultsDisplayerWithHeaderAndFooter::GetStringLengthOfMaxFileSize (const ULARGE_INTEGER & uli)
{
    UINT cch = 1;

    

    if (uli.QuadPart != 0)
    {
        cch += (UINT) log10 ((double) uli.QuadPart);
    }

    //
    // add space for the comma digit group separators
    //

    cch += (cch - 1) / 3;

    return cch;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWithHeaderAndFooter::FormatNumberWithSeparators
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

LPCWSTR CResultsDisplayerWithHeaderAndFooter::FormatNumberWithSeparators (ULONGLONG n)
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
