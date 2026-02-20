#pragma once

#include "ResultsDisplayerNormal.h"
#include "TreeConnectorState.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerTree
//
//  Tree-mode directory displayer.  Derives from CResultsDisplayerNormal to
//  reuse all column rendering helpers (date/time, attributes, size, cloud
//  status, owner, icons).
//
//  In tree mode the MT lister drives the interleaved display (each entry is
//  printed immediately followed by its children when it is a directory).
//  The displayer provides per-entry rendering via DisplaySingleEntry() and
//  helpers for headers/footers.
//
////////////////////////////////////////////////////////////////////////////////

class CResultsDisplayerTree : public CResultsDisplayerNormal
{
public:
    CResultsDisplayerTree (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr, bool fIconsActive);



    //
    // IResultsDisplayer overrides
    //
    // DisplayResults and DisplayFileResults are not used in tree mode.
    // The MT lister calls the tree-specific methods below instead.
    // These overrides delegate to the base class for safety.
    //

    void DisplayResults     (const CDriveInfo & driveInfo, const CDirectoryInfo & di, EDirectoryLevel level) override;
    void DisplayFileResults (const CDirectoryInfo & di) override;



    //
    // Tree-specific display methods called by CMultiThreadedLister
    //

    void DisplayTreeRootHeader          (const CDriveInfo & driveInfo, const CDirectoryInfo & di);
    void BeginDirectory                 (const CDirectoryInfo & di);
    void DisplaySingleEntry             (const FileInfo & entry, STreeConnectorState & treeState, bool fIsLastEntry, size_t idxFile);
    void DisplayTreeRootSummary         (const CDirectoryInfo & di);
    void DisplayTreeEmptyRootMessage    (const CDirectoryInfo & di);



private:
    //
    // Per-directory state set by BeginDirectory()
    //

    size_t          m_cchStringLengthOfMaxFileSize = 0;
    bool            m_fInSyncRoot                  = false;
    vector<wstring> m_owners;
    size_t          m_cchMaxOwnerLength            = 0;
};
