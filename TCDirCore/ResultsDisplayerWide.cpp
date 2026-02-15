#include "pch.h"
#include "ResultsDisplayerWide.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "IconMapping.h"





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
//  CResultsDisplayerWide::DisplayFileResults
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerWide::DisplayFileResults (const CDirectoryInfo & di)
{                                 
    HRESULT hr             = S_OK;
    size_t  cColumns;
    size_t  cxColumnWidth; 
    size_t  cRows;
    size_t  cItemsInLastRow;
    


    CBRA (di.m_cchLargestFileName > 0);

    GetColumnInfo (di, cColumns, cxColumnWidth);

    cRows           = (di.m_vMatches.size() + cColumns - 1) / cColumns;
    cItemsInLastRow = di.m_vMatches.size() % cColumns;
   
    //
    // Display the matches in columns
    //

    for (size_t nRow = 0; nRow < cRows; ++nRow)
    {
        for (size_t nCol = 0; nCol < cColumns; ++nCol)
        {   
            size_t idx      = 0;
            size_t fullRows = cItemsInLastRow ? cRows - 1 : cRows;
            
            

            if ((nRow * cColumns + nCol) >= di.m_vMatches.size())
            {
                break;
            }

            // We print in column-major order, so skip over 
            // items in previous columns.  The last row
            // may not be full, so assume one fewer row for now.
            idx = nRow + (nCol * fullRows);

            // Skip past any items in the last row prior to this column
            if (nCol < cItemsInLastRow)
            {
                idx += nCol;
            }
            else
            {
                idx += cItemsInLastRow;
            }

            hr = DisplayFile (di.m_vMatches[idx], cxColumnWidth);
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

HRESULT CResultsDisplayerWide::DisplayFile (const WIN32_FIND_DATA & wfd, size_t cxColumnWidth)
{
    WCHAR                        szDirName[MAX_PATH + 3]; // '[' + MAX_PATH + ']' + '\0'
    CConfig::SFileDisplayStyle   style    = m_configPtr->GetDisplayStyleForFile (wfd);
    WORD                         textAttr = style.m_wTextAttr;
    wstring_view                 name     = GetWideFormattedName (wfd, szDirName, ARRAYSIZE (szDirName));
    size_t                       cchName  = name.length();



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

    m_consolePtr->Printf (textAttr, L"%s", name.data ());

    if (cxColumnWidth > cchName)
    {
        m_consolePtr->ColorPrintf (L"{Default}%*s", cxColumnWidth - cchName, L"");
    }

    return S_OK;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWide::GetColumnInfo
//
//  Figure out how many columns fit on the screen
//
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerWide::GetColumnInfo (const CDirectoryInfo & di, size_t & cColumns, size_t & cxColumnWidth)
{
    UINT   cxConsoleWidth     = m_consolePtr->GetWidth();
    size_t cchLargestFileName = di.m_cchLargestFileName;

    

    //
    // When icons are active, account for icon + space (+2) in column width
    //

    if (m_fIconsActive)
    {
        cchLargestFileName += 2;
    }

    if (cchLargestFileName + 1 > cxConsoleWidth)
    {
        cColumns = 1;
    }
    else
    {
        // 1 space between columns
        cColumns = cxConsoleWidth / (cchLargestFileName + 1);    
    }

    cxColumnWidth = cxConsoleWidth / cColumns;
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
