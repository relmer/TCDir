#include "pch.h"
#include "ResultsDisplayerBase.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"





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

void CResultsDisplayerBase::DisplayResults (const CDriveInfo & driveInfo, __in CDirectoryInfo * pdi, EDirectoryLevel level)
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

        DisplayDriveHeader (driveInfo);
    }

    DisplayPathHeader (pdi->m_dirPath); 

    if (pdi->m_vMatches.size() == 0)
    {
        if (pdi->m_fileSpec == L"*")
        {
            m_pConsole->Puts (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"Directory is empty.");
        }
        else
        {
            m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"No files matching '%s' found.\n", pdi->m_fileSpec.c_str());
        }
    }
    else 
    {
        DisplayFileResults      (pdi);
        DisplayDirectorySummary (pdi);

        //
        // Only show the volume footer here if we're not doing a recursive list.
        // Otherwise, it'll be shown after the recursive summary.
        //

        if (!m_pCmdLine->m_fRecurse)
        {
            DisplayVolumeFooter (pdi);
        }
    }
        
    m_pConsole->Puts               (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"");
    m_pConsole->WriteSeparatorLine (m_pConfig->m_rgAttributes[CConfig::EAttribute::SeparatorLine]);
    m_pConsole->Puts               (m_pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"");

    m_pConsole->Flush();
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

void CResultsDisplayerBase::DisplayDriveHeader (const CDriveInfo & driveInfo)
{
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information], L" Volume ");

    if (driveInfo.IsUncPath())
    {
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"%s", driveInfo.GetUncPath().c_str());
    }
    else
    {
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L"in drive ");
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"%c", driveInfo.GetRootPath().c_str()[0]);
    }        

    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L" is ");
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"%s", driveInfo.GetVolumeDescription().c_str());

    //
    // If this is a mapped drive, get the remote name that it's mapped to
    //
    
    const wstring & remoteName = driveInfo.GetRemoteName();
    if (remoteName.length() > 0)
    {
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L" mapped to ");
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"%s", remoteName.c_str());
    }

    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L" (");
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"%s", driveInfo.GetFileSystemName());
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L")\n");

    LPCWSTR pszVolumeName = driveInfo.GetVolumeName();
    if (pszVolumeName[0] != L'\0')
    {
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L" Volume name is \"");
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"%s", pszVolumeName);
        m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L"\"\n\n");
    }
    else
    {
        m_pConsole->Puts (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information], L" Volume has no name\n");
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerBase::DisplayPathHeader
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerBase::DisplayPathHeader (const filesystem::path & dirPath)
{
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L" Directory of ");
    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::InformationHighlight], L"%s", dirPath.c_str());
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

    m_pConsole->Printf (m_pConfig->m_rgAttributes[CConfig::EAttribute::Information],          L" Total files listed:");
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


    
    fSuccess = GetDiskFreeSpaceEx (pdi->m_dirPath.c_str(), &uliFreeBytesAvailable, &uliTotalBytes, &uliTotalFreeBytes);
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
