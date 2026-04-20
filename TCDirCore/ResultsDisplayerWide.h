#pragma once

#include "ResultsDisplayerWithHeaderAndFooter.h"





////////////////////////////////////////////////////////////////////////////////
//
//  SColumnLayout
//
//  Result of the variable-width column fitting algorithm.
//  Produced once per directory listing by ComputeColumnLayout().
//
////////////////////////////////////////////////////////////////////////////////

struct SColumnLayout
{
    size_t         cColumns     = 0;
    size_t         cRows        = 0;
    vector<size_t> vColumnWidths;       // Per-column display width (includes gap for non-last columns)
    size_t         cchTruncCap  = 0;    // Outlier truncation cap (0 = no truncation)
};





class CResultsDisplayerWide : public CResultsDisplayerWithHeaderAndFooter
{
public:
    CResultsDisplayerWide        (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr, bool fIconsActive);

    void DisplayFileResults      (const CDirectoryInfo & di) override;

    //
    // Pure helper functions — public for unit testing
    //

    static size_t        ComputeDisplayWidth        (const WIN32_FIND_DATA & wfd, bool fIconsActive, bool fIconSuppressed, bool fInSyncRoot);
    static SColumnLayout ComputeColumnLayout         (const vector<size_t> & vDisplayWidths, size_t cxConsoleWidth, bool fEllipsize);
    static size_t        ComputeMedianDisplayWidth   (vector<size_t> vDisplayWidths);

protected:
    HRESULT      DisplayFile          (const WIN32_FIND_DATA & wfd, size_t cxColumnWidth, size_t cchTruncCap, bool fInSyncRoot);
    wstring_view GetWideFormattedName (const WIN32_FIND_DATA & wfd, LPWSTR pszBuffer, size_t cchBuffer);

private:
    static SColumnLayout TryColumnCount (const vector<size_t> & vEffective, size_t cxConsoleWidth, size_t nCols);
    static SColumnLayout FitColumns     (const vector<size_t> & vWidths, size_t cxConsoleWidth);
};
