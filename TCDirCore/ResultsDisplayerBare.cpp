#include "pch.h"
#include "ResultsDisplayerBare.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerBare::CResultsDisplayerBare
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

CResultsDisplayerBare::CResultsDisplayerBare (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr, bool fIconsActive) :
    m_cmdLinePtr   (cmdLinePtr),
    m_consolePtr   (consolePtr),
    m_configPtr    (configPtr),
    m_fIconsActive (fIconsActive)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerBare::DisplayResults
//
//  Bare output: just filenames (or full paths if recursing), no headers/footers.
//
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerBare::DisplayResults (const CDriveInfo & driveInfo, const CDirectoryInfo & di, EDirectoryLevel level)
{
    UNREFERENCED_PARAMETER (driveInfo);
    UNREFERENCED_PARAMETER (level);



    for (const FileInfo & fileInfo : di.m_vMatches)
    {
        WORD textAttr = m_configPtr->GetTextAttrForFile (fileInfo);

        if (m_cmdLinePtr->m_fRecurse)
        {
            // When recursing, show full path
            filesystem::path fullPath = di.m_dirPath / fileInfo.cFileName;
            m_consolePtr->Printf (textAttr, L"%s\n", fullPath.c_str());
        }
        else
        {
            // Just filename
            m_consolePtr->Printf (textAttr, L"%s\n", fileInfo.cFileName);
        }
    }

    m_consolePtr->Flush();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerBare::DisplayRecursiveSummary
//
//  Bare output: no summary displayed.
//
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerBare::DisplayRecursiveSummary (const CDirectoryInfo & diInitial, const SListingTotals & totals)
{
    // Bare mode doesn't display summary
    UNREFERENCED_PARAMETER (diInitial);
    UNREFERENCED_PARAMETER (totals);
}

