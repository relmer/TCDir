#include "pch.h"
#include "ResultsDisplayerWide.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "IconMapping.h"
#include "UnicodeSymbols.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWide::CResultsDisplayerWide
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

CResultsDisplayerWide::CResultsDisplayerWide (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr, bool fIconsActive) :
    CResultsDisplayerWithHeaderAndFooter (cmdLinePtr, consolePtr, configPtr, fIconsActive)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWide::ComputeDisplayWidth
//
//  Compute the display width of a single entry for column fitting.
//  Pure function — no member state, testable with synthetic data.
//
////////////////////////////////////////////////////////////////////////////////

size_t CResultsDisplayerWide::ComputeDisplayWidth (const WIN32_FIND_DATA & wfd, bool fIconsActive, bool fIconSuppressed, bool fInSyncRoot)
{
    size_t cch = wcslen (wfd.cFileName);



    //
    // Directories get [brackets] when icons are off
    //

    if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !fIconsActive)
    {
        cch += 2;
    }

    //
    // Icon glyph + space
    //

    if (fIconsActive && !fIconSuppressed)
    {
        cch += 2;
    }

    //
    // Cloud status symbol + space
    //

    if (fInSyncRoot)
    {
        cch += 2;
    }

    return cch;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWide::ComputeMedianDisplayWidth
//
//  Return the median of a vector of display widths.
//  Takes by value because nth_element mutates the vector.
//
////////////////////////////////////////////////////////////////////////////////

size_t CResultsDisplayerWide::ComputeMedianDisplayWidth (vector<size_t> vDisplayWidths)
{
    if (vDisplayWidths.empty())
    {
        return 0;
    }



    size_t mid = vDisplayWidths.size() / 2;

    nth_element (vDisplayWidths.begin(), vDisplayWidths.begin() + mid, vDisplayWidths.end());

    return vDisplayWidths[mid];
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWide::ComputeColumnLayout
//
//  GNU ls-style variable-width column fitting.
//  Pure function — takes display widths and console width, returns layout.
//
////////////////////////////////////////////////////////////////////////////////

SColumnLayout CResultsDisplayerWide::ComputeColumnLayout (const vector<size_t> & vDisplayWidths, size_t cxConsoleWidth, bool fEllipsize)
{
    size_t cEntries = vDisplayWidths.size();



    //
    // Trivial cases: 0 or 1 entries
    //

    if (cEntries <= 1)
    {
        return { .cColumns = 1, .cRows = cEntries, .vColumnWidths = { cxConsoleWidth }, .cchTruncCap = 0 };
    }

    //
    // Build effective widths, applying outlier truncation if enabled.
    // Only use truncation if it actually produces more columns than
    // the un-truncated layout — otherwise it hurts readability for no gain.
    //

    vector<size_t> vEffective = vDisplayWidths;

    if (fEllipsize)
    {
        size_t median = ComputeMedianDisplayWidth (vDisplayWidths);
        size_t cap    = max (2 * median, static_cast<size_t>(20));

        bool fHasOutliers = false;

        for (auto w : vEffective)
        {
            if (w > cap)
            {
                fHasOutliers = true;
                break;
            }
        }

        if (fHasOutliers)
        {
            //
            // Compute layout without truncation first
            //

            SColumnLayout cleanLayout = FitColumns (vDisplayWidths, cxConsoleWidth);

            //
            // Compute layout with truncation
            //

            for (auto & w : vEffective)
            {
                if (w > cap)
                {
                    w = cap;
                }
            }

            SColumnLayout truncLayout = FitColumns (vEffective, cxConsoleWidth);

            //
            // Only use truncation if it produces more columns
            //

            if (truncLayout.cColumns > cleanLayout.cColumns)
            {
                truncLayout.cchTruncCap = cap;
                return truncLayout;
            }
            else
            {
                return cleanLayout;
            }
        }
    }

    //
    // No truncation needed — fit with original widths
    //

    return FitColumns (vDisplayWidths, cxConsoleWidth);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWide::FitColumns
//
//  Try column counts from max feasible down to 2.  Returns the first
//  (highest column count) layout that fits, or single-column fallback.
//
////////////////////////////////////////////////////////////////////////////////

SColumnLayout CResultsDisplayerWide::FitColumns (const vector<size_t> & vWidths, size_t cxConsoleWidth)
{
    size_t cEntries = vWidths.size();
    size_t maxCols  = min (cEntries, cxConsoleWidth / 2);



    for (size_t nCols = maxCols; nCols >= 2; --nCols)
    {
        SColumnLayout layout = TryColumnCount (vWidths, cxConsoleWidth, nCols);

        if (layout.cColumns > 0)
        {
            return layout;
        }
    }

    return { .cColumns = 1, .cRows = cEntries, .vColumnWidths = { cxConsoleWidth }, .cchTruncCap = 0 };
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWide::TryColumnCount
//
//  Try fitting entries into nCols columns.  Returns a valid layout if the
//  total width fits within cxConsoleWidth, or an empty layout (cColumns == 0)
//  if it does not fit.
//
////////////////////////////////////////////////////////////////////////////////

SColumnLayout CResultsDisplayerWide::TryColumnCount (const vector<size_t> & vEffective, size_t cxConsoleWidth, size_t nCols)
{
    size_t cEntries        = vEffective.size();
    size_t nRows           = (cEntries + nCols - 1) / nCols;
    size_t cItemsInLastRow = cEntries % nCols;
    size_t cFullCols       = cItemsInLastRow ? cItemsInLastRow : nCols;
    size_t cEntriesInFull  = cFullCols * nRows;



    //
    // Compute per-column widths using the same column-major mapping
    // as DisplayFileResults.  The first cFullCols columns have nRows
    // entries each; the remaining columns have (nRows - 1) entries.
    //

    vector<size_t> colWidths (nCols, 0);

    for (size_t i = 0; i < cEntries; ++i)
    {
        size_t col = (i < cEntriesInFull)
                   ? i / nRows
                   : cFullCols + (i - cEntriesInFull) / (nRows - 1);

        size_t w = vEffective[i] + (col < nCols - 1 ? 1 : 0);   // +1 base gap except last col

        if (w > colWidths[col])
        {
            colWidths[col] = w;
        }
    }

    //
    // Check if total fits.  Reserve 1 char so the last column's widest
    // entry doesn't push the cursor to the exact console edge and trigger
    // a terminal line-wrap before the explicit newline.
    //

    size_t totalWidth = 0;

    for (auto w : colWidths)
    {
        totalWidth += w;
    }

    if (totalWidth >= cxConsoleWidth)
    {
        return { .cColumns = 0, .cRows = 0, .vColumnWidths = {}, .cchTruncCap = 0 };
    }

    //
    // Distribute leftover space evenly across inter-column gaps.
    // Keep 1 char undistributed to maintain the strict-less-than guarantee.
    //

    size_t leftover = cxConsoleWidth - totalWidth - 1;

    if (nCols > 1 && leftover > 0)
    {
        size_t extraPerGap = leftover / (nCols - 1);
        size_t remainder   = leftover % (nCols - 1);

        for (size_t c = 0; c < nCols - 1; ++c)
        {
            colWidths[c] += extraPerGap + (c < remainder ? 1 : 0);
        }
    }

    return { .cColumns = nCols, .cRows = nRows, .vColumnWidths = move (colWidths), .cchTruncCap = 0 };
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWide::DisplayFileResults
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerWide::DisplayFileResults (const CDirectoryInfo & di)
{                                 
    HRESULT        hr          = S_OK;
    bool           fInSyncRoot = IsUnderSyncRoot (di.m_dirPath.c_str());
    bool           fEllipsize  = !m_cmdLinePtr->m_fEllipsize.has_value() || m_cmdLinePtr->m_fEllipsize.value();
    vector<size_t> vDisplayWidths;
    SColumnLayout  layout;



    CBRA (di.m_cchLargestFileName > 0);

    //
    // Build per-entry display widths
    //

    vDisplayWidths.reserve (di.m_vMatches.size());

    for (const auto & fi : di.m_vMatches)
    {
        CConfig::SFileDisplayStyle style = m_configPtr->GetDisplayStyleForFile (fi);
        bool fIconSuppressed = style.m_fIconSuppressed || style.m_iconCodePoint == 0;

        vDisplayWidths.push_back (ComputeDisplayWidth (fi, m_fIconsActive, fIconSuppressed, fInSyncRoot));
    }

    //
    // Compute variable-width column layout
    //

    layout = ComputeColumnLayout (vDisplayWidths, m_consolePtr->GetWidth(), fEllipsize);

    //
    // Display the matches in columns
    //

    for (size_t nRow = 0; nRow < layout.cRows; ++nRow)
    {
        for (size_t nCol = 0; nCol < layout.cColumns; ++nCol)
        {
            size_t cItemsInLastRow = di.m_vMatches.size() % layout.cColumns;
            size_t fullRows        = cItemsInLastRow ? layout.cRows - 1 : layout.cRows;



            if ((nRow * layout.cColumns + nCol) >= di.m_vMatches.size())
            {
                break;
            }

            //
            // Column-major index: skip over items in previous columns
            //

            size_t idx = nRow + (nCol * fullRows);

            if (nCol < cItemsInLastRow)
            {
                idx += nCol;
            }
            else
            {
                idx += cItemsInLastRow;
            }

            //
            // Use per-column width; last column gets no trailing whitespace
            //

            size_t cxColWidth = (nCol < layout.cColumns - 1) ? layout.vColumnWidths[nCol] : 0;

            hr = DisplayFile (di.m_vMatches[idx], cxColWidth, layout.cchTruncCap, fInSyncRoot);
            CHR (hr);
        }

        m_consolePtr->Puts (CConfig::EAttribute::Default, L"");
    }



Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWide::DisplayFile
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

HRESULT CResultsDisplayerWide::DisplayFile (const WIN32_FIND_DATA & wfd, size_t cxColumnWidth, size_t cchTruncCap, bool fInSyncRoot)
{
    WCHAR                        szDirName[MAX_PATH + 3]; // '[' + MAX_PATH + ']' + '\0'
    CConfig::SFileDisplayStyle   style    = m_configPtr->GetDisplayStyleForFile (wfd);
    WORD                         textAttr = style.m_wTextAttr;
    wstring_view                 name     = GetWideFormattedName (wfd, szDirName, ARRAYSIZE (szDirName));
    size_t                       cchName  = name.length();
    wstring                      strTruncated;



    //
    // Display cloud status symbol (when in a sync root)
    //

    if (fInSyncRoot)
    {
        ECloudStatus cloudStatus = GetCloudStatus (wfd, fInSyncRoot);

        if (m_fIconsActive)
        {
            char32_t iconCP = m_configPtr->GetCloudStatusIcon (static_cast<DWORD>(cloudStatus));

            if (iconCP != 0)
            {
                static constexpr LPCWSTR s_krgCloudColorMarkers[] =
                {
                    L"{Default}",
                    L"{CloudStatusCloudOnly}",
                    L"{CloudStatusLocallyAvailable}",
                    L"{CloudStatusAlwaysLocallyAvailable}",
                };

                WideCharPair pair   = CodePointToWideChars (iconCP);
                wchar_t      szIcon[3] = { pair.chars[0], pair.chars[1], L'\0' };

                m_consolePtr->ColorPrintf (L"%s%s ", s_krgCloudColorMarkers[static_cast<size_t>(cloudStatus)], szIcon);
            }
            else
            {
                m_consolePtr->ColorPrintf (L"{Default}  ");
            }
        }
        else
        {
            static constexpr struct { LPCWSTR pszColorMarker; WCHAR chSymbol; } s_krgCloudStatusMap[] =
            {
                { L"{Default}",                            L' '                             },
                { L"{CloudStatusCloudOnly}",               UnicodeSymbols::CircleHollow     },
                { L"{CloudStatusLocallyAvailable}",        UnicodeSymbols::CircleHalfFilled },
                { L"{CloudStatusAlwaysLocallyAvailable}",  UnicodeSymbols::CircleFilled     },
            };

            const auto & entry = s_krgCloudStatusMap[static_cast<size_t>(cloudStatus)];
            m_consolePtr->ColorPrintf (L"%s%c ", entry.pszColorMarker, entry.chSymbol);
        }

        cchName += 2;  // cloud symbol + space
    }

    //
    // Display icon glyph before filename (when icons are active)
    //

    if (m_fIconsActive && style.m_iconCodePoint != 0 && !style.m_fIconSuppressed)
    {
        WideCharPair pair   = CodePointToWideChars (style.m_iconCodePoint);
        wchar_t      szIcon[3] = { pair.chars[0], pair.chars[1], L'\0' };

        m_consolePtr->Printf (textAttr, L"%s ", szIcon);
        cchName += 2;  // icon + space
    }

    //
    // Truncate outlier names with ellipsis when they exceed the truncation cap.
    // Remove cchOver + 1 chars from the name: cchOver to reach the cap, plus 1
    // to make room for the single-char ellipsis (…).
    //

    if (cchTruncCap > 0 && cchName > cchTruncCap)
    {
        size_t cchPrefix = name.length();
        size_t cchOver   = cchName - cchTruncCap;

        if (cchOver + 1 < cchPrefix)
        {
            strTruncated.assign (name.data(), cchPrefix - cchOver - 1);
            strTruncated += UnicodeSymbols::Ellipsis;
            name    = strTruncated;
            cchName = cchTruncCap;
        }
    }

    m_consolePtr->Printf (textAttr, L"%s", name.data ());

    if (cxColumnWidth > cchName)
    {
        m_consolePtr->ColorPrintf (L"{Default}%*s", cxColumnWidth - cchName, L"");
    }

    return S_OK;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWide::GetWideFormattedName
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

wstring_view CResultsDisplayerWide::GetWideFormattedName (const WIN32_FIND_DATA & wfd, LPWSTR pszBuffer, size_t cchBuffer)
{
    LPCWSTR pszName = wfd.cFileName;



    //
    // Directories: use [name] brackets in classic mode, plain name when icons active
    // (the folder icon provides the distinction)
    //

    if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !m_fIconsActive)
    {
        auto [out, _] = format_to_n (pszBuffer, cchBuffer - 1, L"[{}]", wfd.cFileName);
        *out    = L'\0';
        pszName = pszBuffer;
    }

    return pszName;
}
