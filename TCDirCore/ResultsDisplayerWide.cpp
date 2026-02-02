#include "pch.h"
#include "ResultsDisplayerWide.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWide::CResultsDisplayerWide
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

CResultsDisplayerWide::CResultsDisplayerWide (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr) :
    CResultsDisplayerWithHeaderAndFooter (cmdLinePtr, consolePtr, configPtr)
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
    WCHAR        szDirName[MAX_PATH + 3]; // '[' + MAX_PATH + ']' + '\0'
    wstring_view name     = GetWideFormattedName (wfd, szDirName, ARRAYSIZE (szDirName));
    WORD         textAttr = m_configPtr->GetTextAttrForFile (wfd);



    m_consolePtr->Printf (textAttr, L"%s", name.data ());

    if (cxColumnWidth > name.length ())
    {
        m_consolePtr->ColorPrintf (L"{Default}%*s", cxColumnWidth - name.length (), L"");
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
    UINT cxConsoleWidth = m_consolePtr->GetWidth();

    

    if (di.m_cchLargestFileName + 1 > cxConsoleWidth)
    {
        cColumns = 1;
    }
    else
    {
        // 1 space between columns
        cColumns = cxConsoleWidth / (di.m_cchLargestFileName + 1);    
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



    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        auto [out, _] = format_to_n (pszBuffer, cchBuffer - 1, L"[{}]", wfd.cFileName);
        *out    = L'\0';
        pszName = pszBuffer;
    }

    return pszName;
}
