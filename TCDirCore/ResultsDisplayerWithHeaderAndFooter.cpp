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

void CResultsDisplayerWithHeaderAndFooter::DisplayRecursiveSummary (const CDirectoryInfo & diInitial, const SListingTotals & totals)
{
    DisplayListingSummary (diInitial, totals);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWithHeaderAndFooter::DisplayDriveHeader
//
//  Displays information about the drive, e.g.: 
// 
//      Volume in drive C is a hard drive (NTFS)
//      Volume Name is "Maxtor 250GB"
//
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerWithHeaderAndFooter::DisplayDriveHeader (const CDriveInfo & driveInfo)
{
    //
    // Build the first line: "Volume [path] is [description] [mapped to X] (filesystem)"
    //

    if (driveInfo.IsUncPath())
    {
        m_consolePtr->ColorPrintf (L"{Information} Volume {InformationHighlight}%s{Information} is {InformationHighlight}%s{Information}",
                                   driveInfo.GetUncPath().c_str(),
                                   driveInfo.GetVolumeDescription().c_str());
    }
    else
    {
        m_consolePtr->ColorPrintf (L"{Information} Volume in drive {InformationHighlight}%c{Information} is {InformationHighlight}%s{Information}",
                                   driveInfo.GetRootPath().c_str()[0],
                                   driveInfo.GetVolumeDescription().c_str());
    }

    //
    // If this is a mapped drive, get the remote name that it's mapped to
    //

    const wstring & remoteName = driveInfo.GetRemoteName();
    if (remoteName.length() > 0)
    {
        m_consolePtr->ColorPrintf (L"{Information} mapped to {InformationHighlight}%s{Information}", remoteName.c_str());
    }

    m_consolePtr->ColorPrintf (L"{Information} ({InformationHighlight}%s{Information})\n", driveInfo.GetFileSystemName());

    //
    // Display the volume name (second line)
    //

    LPCWSTR pszVolumeName = driveInfo.GetVolumeName();
    if (pszVolumeName[0] != L'\0')
    {
        m_consolePtr->ColorPrintf (L"{Information} Volume name is \"{InformationHighlight}%s{Information}\"\n", pszVolumeName);
    }
    else
    {
        m_consolePtr->ColorPuts (L"{Information} Volume has no name");
    }

    m_consolePtr->ColorPuts (L"");
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
    m_consolePtr->ColorPrintf (L"{Information} Directory of {InformationHighlight}%s{Information}\n\n", dirPath.c_str());
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWithHeaderAndFooter::DisplayListingSummary
//
//  Display full recursive summary information:
// 
//   Total files listed:
//         143 files using 123,456 bytes
//           3 streams using 1,234 bytes (if streams found)
//           7 subdirectories
// 
//   123,123,123,123 bytes free on volume
//   123,117,699,072 bytes available to user %s
//
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerWithHeaderAndFooter::DisplayListingSummary (const CDirectoryInfo & di, const SListingTotals & totals)
{
    int cMaxDigits = 1;

    if (totals.m_cFiles > 0 || totals.m_cDirectories > 0)
    { 
        cMaxDigits = (int) log10 (max (totals.m_cFiles, totals.m_cDirectories)) + 1;
        cMaxDigits += cMaxDigits / 3;  // add space for each comma
    }

    m_consolePtr->ColorPrintf (L"{Information} Total files listed:\n\n{InformationHighlight}    %*s{Information}%s{InformationHighlight}%s{Information}%s\n{InformationHighlight}    %*s{Information}%s\n",
                               cMaxDigits, FormatNumberWithSeparators (totals.m_cFiles).c_str(),
                               totals.m_cFiles == 1 ? L" file using " : L" files using ",
                               FormatNumberWithSeparators (totals.m_uliFileBytes.QuadPart).c_str(),
                               totals.m_uliFileBytes.QuadPart == 1 ? L" byte" : L" bytes",
                               cMaxDigits, FormatNumberWithSeparators (totals.m_cDirectories).c_str(),
                               totals.m_cDirectories == 1 ? L" subdirectory" : L" subdirectories");

    if (totals.m_cStreams > 0)
    {
        m_consolePtr->ColorPrintf (L"{InformationHighlight}    %*s{Information}%s{InformationHighlight}%s{Information}%s\n",
                                   cMaxDigits, FormatNumberWithSeparators (totals.m_cStreams).c_str(),
                                   totals.m_cStreams == 1 ? L" stream using " : L" streams using ",
                                   FormatNumberWithSeparators (totals.m_uliStreamBytes.QuadPart).c_str(),
                                   totals.m_uliStreamBytes.QuadPart == 1 ? L" byte" : L" bytes");
    }

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
//   (optionally: , 3 streams using 1,234 bytes)
//
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerWithHeaderAndFooter::DisplayDirectorySummary (const CDirectoryInfo & di)
{
    m_consolePtr->ColorPrintf (L"\n{Information} {InformationHighlight}%d{Information}%s{InformationHighlight}%d{Information}%s{InformationHighlight}%s{Information}%s",
                               di.m_cSubDirectories,
                               di.m_cSubDirectories == 1 ? L" dir, " : L" dirs, ",
                               di.m_cFiles,
                               di.m_cFiles == 1 ? L" file using " : L" files using ",
                               FormatNumberWithSeparators (di.m_uliBytesUsed.QuadPart).c_str(),
                               di.m_uliBytesUsed.QuadPart == 1 ? L" byte" : L" bytes");

    if (di.m_cStreams > 0)
    {
        m_consolePtr->ColorPrintf (L"{Information}, {InformationHighlight}%d{Information}%s{InformationHighlight}%s{Information}%s",
                                   di.m_cStreams,
                                   di.m_cStreams == 1 ? L" stream using " : L" streams using ",
                                   FormatNumberWithSeparators (di.m_uliStreamBytesUsed.QuadPart).c_str(),
                                   di.m_uliStreamBytesUsed.QuadPart == 1 ? L" byte" : L" bytes");
    }

    m_consolePtr->ColorPuts (L"");
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
    HRESULT        hr                    = S_OK;
    BOOL           fSuccess              = {};              
    ULARGE_INTEGER uliFreeBytesAvailable = {}; 
    ULARGE_INTEGER uliTotalBytes         = {};         
    ULARGE_INTEGER uliTotalFreeBytes     = {};     

    

    fSuccess = GetDiskFreeSpaceEx (di.m_dirPath.c_str(), &uliFreeBytesAvailable, &uliTotalBytes, &uliTotalFreeBytes);
    CBRA (fSuccess);

    m_consolePtr->ColorPrintf (L"{InformationHighlight} %s{Information}%s\n",
                               FormatNumberWithSeparators (uliTotalFreeBytes.QuadPart).c_str(),
                               uliTotalFreeBytes.QuadPart == 1 ? L" byte free on volume" : L" bytes free on volume");

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

    m_consolePtr->ColorPrintf (L"{Information} {InformationHighlight}%s{Information}%s{InformationHighlight}%s{Information} due to quota\n",
                                FormatNumberWithSeparators (uliFreeBytesAvailable.QuadPart).c_str(),
                                uliFreeBytesAvailable.QuadPart == 1 ? L" byte available to " : L" bytes available to ",
                                strUsername.c_str());
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
//  Formats a number with thousands separators using the user's locale.
//
////////////////////////////////////////////////////////////////////////////////  

wstring CResultsDisplayerWithHeaderAndFooter::FormatNumberWithSeparators (ULONGLONG n)
{
    static const locale s_loc ("");  // Cache the locale - construction is expensive
    return format (s_loc, L"{:L}", n);
}
