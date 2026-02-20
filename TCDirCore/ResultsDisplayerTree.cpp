#include "pch.h"
#include "ResultsDisplayerTree.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "IconMapping.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerTree::CResultsDisplayerTree
//
//
//
////////////////////////////////////////////////////////////////////////////////

CResultsDisplayerTree::CResultsDisplayerTree (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr, bool fIconsActive) :
    CResultsDisplayerNormal (cmdLinePtr, consolePtr, configPtr, fIconsActive)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerTree::DisplayResults
//
//  Not used in tree mode — the MT lister calls tree-specific methods instead.
//  Delegates to base class in case of unexpected calls.
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerTree::DisplayResults (const CDriveInfo & driveInfo, const CDirectoryInfo & di, EDirectoryLevel level)
{
    CResultsDisplayerNormal::DisplayResults (driveInfo, di, level);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerTree::DisplayFileResults
//
//  Not used in tree mode — individual entries are displayed via
//  DisplaySingleEntry().  Delegates to base class for safety.
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerTree::DisplayFileResults (const CDirectoryInfo & di)
{
    CResultsDisplayerNormal::DisplayFileResults (di);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerTree::DisplayTreeRootHeader
//
//  Shows the separator, drive header, and path header for the root directory.
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerTree::DisplayTreeRootHeader (const CDriveInfo & driveInfo, const CDirectoryInfo & di)
{
    m_consolePtr->WriteSeparatorLine (m_configPtr->m_rgAttributes[CConfig::EAttribute::SeparatorLine]);

    DisplayDriveHeader (driveInfo);
    DisplayPathHeader  (di.m_dirPath);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerTree::BeginDirectory
//
//  Computes per-directory display state (max file size width, sync root
//  status, file owners) so that DisplaySingleEntry can be called
//  repeatedly without recomputing these for every entry.
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerTree::BeginDirectory (const CDirectoryInfo & di)
{
    m_cchStringLengthOfMaxFileSize = GetStringLengthOfMaxFileSize (di.m_uliLargestFileSize);
    m_fInSyncRoot                  = IsUnderSyncRoot (di.m_dirPath.c_str());
    m_owners.clear();
    m_cchMaxOwnerLength            = 0;



    if (m_cmdLinePtr->m_fShowOwner)
    {
        GetFileOwners (di, m_owners, m_cchMaxOwnerLength);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerTree::DisplaySingleEntry
//
//  Displays one file/directory entry with tree connector prefix.
//  Calls inherited column helpers for date/time, attributes, size,
//  cloud status, debug attributes, and owner.  Then prepends the tree
//  connector prefix before the icon and filename.
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerTree::DisplaySingleEntry (const FileInfo & entry, STreeConnectorState & treeState, bool fIsLastEntry, size_t idxFile)
{
    HRESULT hr = S_OK;

    CConfig::SFileDisplayStyle style       = m_configPtr->GetDisplayStyleForFile (entry);
    WORD                       textAttr    = style.m_wTextAttr;
    ECloudStatus               cloudStatus = GetCloudStatus (entry, m_fInSyncRoot);
    const FILETIME           & ftDisplay   = GetTimeFieldForDisplay (entry);
    wstring                    prefix;



    //
    // Metadata columns (same order as Normal mode)
    //

    hr = DisplayResultsNormalDateAndTime (ftDisplay);
    CHR (hr);

    DisplayResultsNormalAttributes (entry.dwFileAttributes);
    DisplayResultsNormalFileSize   (entry, m_cchStringLengthOfMaxFileSize);
    DisplayCloudStatusSymbol       (cloudStatus);

    if (m_cmdLinePtr->m_fDebug)
    {
        DisplayRawAttributes (entry);
    }

    if (m_cmdLinePtr->m_fShowOwner)
    {
        DisplayFileOwner (m_owners[idxFile], m_cchMaxOwnerLength);
    }

    //
    // Tree connector prefix (before icon/filename)
    //

    prefix = treeState.GetPrefix (fIsLastEntry);
    if (!prefix.empty())
    {
        m_consolePtr->Printf (CConfig::EAttribute::TreeConnector, L"%s", prefix.c_str());
    }

    //
    // Icon glyph (when icons are active)
    //

    if (m_fIconsActive && style.m_iconCodePoint != 0 && !style.m_fIconSuppressed)
    {
        WideCharPair pair      = CodePointToWideChars (style.m_iconCodePoint);
        wchar_t      szIcon[3] = { pair.chars[0], pair.chars[1], L'\0' };

        m_consolePtr->Printf (textAttr, L"%s ", szIcon);
    }

    //
    // Filename
    //

    m_consolePtr->Printf (textAttr, L"%s\n", entry.cFileName);

    //
    // Alternate data streams (when --Streams is active)
    //

    if (m_cmdLinePtr->m_fShowStreams && !entry.m_vStreams.empty())
    {
        DisplayFileStreamsWithTreePrefix (entry, treeState);
    }



Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerTree::DisplayFileStreamsWithTreePrefix
//
//  Displays alternate data streams for a file with tree continuation
//  prefix prepended to each stream line.  Uses the tree state's stream
//  continuation method (│ + padding) instead of simple space indentation.
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerTree::DisplayFileStreamsWithTreePrefix (const FileInfo & entry, const STreeConnectorState & treeState)
{
    wstring continuationPrefix = treeState.GetStreamContinuation();
    size_t  cchMaxFileSize     = max (m_cchStringLengthOfMaxFileSize, size_t (5));
    int     cchOwnerPadding    = (m_cchMaxOwnerLength > 0) ? static_cast<int>(m_cchMaxOwnerLength + 1) : 0;



    for (const SStreamInfo & si : entry.m_vStreams)
    {
        wstring pszStreamSize = FormatNumberWithSeparators (si.m_liSize.QuadPart);

        m_consolePtr->ColorPrintf (L"{Default}%*c{Size} %*s {Default}  %*s",
                                   30, L' ',
                                   static_cast<int>(cchMaxFileSize), pszStreamSize.c_str(),
                                   cchOwnerPadding, L"");

        if (!continuationPrefix.empty())
        {
            m_consolePtr->Printf (CConfig::EAttribute::TreeConnector, L"%s", continuationPrefix.c_str());
        }

        m_consolePtr->Printf (CConfig::EAttribute::Stream, L"%s%s\n",
                              entry.cFileName,
                              si.m_strName.c_str());
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerTree::DisplayTreeRootSummary
//
//  Displays the per-directory summary for the root, the separator,
//  and flushes.
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerTree::DisplayTreeRootSummary (const CDirectoryInfo & di)
{
    DisplayDirectorySummary (di);

    m_consolePtr->Puts               (CConfig::EAttribute::Default, L"");
    m_consolePtr->WriteSeparatorLine (m_configPtr->m_rgAttributes[CConfig::EAttribute::SeparatorLine]);
    m_consolePtr->Puts               (CConfig::EAttribute::Default, L"");

    m_consolePtr->Flush();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerTree::DisplayTreeEmptyRootMessage
//
//  Displays the empty-directory message for the root in tree mode.
//  Also closes the separator and flushes.
//
////////////////////////////////////////////////////////////////////////////////

void CResultsDisplayerTree::DisplayTreeEmptyRootMessage (const CDirectoryInfo & di)
{
    DisplayEmptyDirectoryMessage (di);

    m_consolePtr->Puts               (CConfig::EAttribute::Default, L"");
    m_consolePtr->WriteSeparatorLine (m_configPtr->m_rgAttributes[CConfig::EAttribute::SeparatorLine]);
    m_consolePtr->Puts               (CConfig::EAttribute::Default, L"");

    m_consolePtr->Flush();
}
