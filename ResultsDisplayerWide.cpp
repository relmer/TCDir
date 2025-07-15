#include "pch.h"
#include "ResultsDisplayerWide.h"

#include "CommandLine.h"
#include "Config.h"
#include "Console.h"
#include "Flag.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWide::CResultsDisplayerWide
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

CResultsDisplayerWide::CResultsDisplayerWide (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr) :
    CResultsDisplayerBase (cmdLinePtr, consolePtr, configPtr)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWide::DisplayFileResults
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

void CResultsDisplayerWide::DisplayFileResults (__in CDirectoryInfo * pdi)
{                                 
    HRESULT hr             = S_OK;
    size_t  cColumns;
    size_t  cxColumnWidth; 
    size_t  nRow;
    size_t  nCol;
    size_t  cRows;
    size_t  cItemsInLastRow;
    


    CBRA (pdi->m_cchLargestFileName > 0);

    hr = GetColumnInfo (pdi, &cColumns, &cxColumnWidth);
    CHR (hr);    

    cRows = (pdi->m_vMatches.size() + cColumns - 1) / cColumns;
    cItemsInLastRow = pdi->m_vMatches.size () % cColumns;
   
    //
    // Display the matches in columns
    //

    for (nRow = 0; nRow < cRows; ++nRow)
    {
        for (nCol = 0; nCol < cColumns; ++nCol)
        {   
            size_t            idx           = 0;
            size_t            fullRows      = cItemsInLastRow ? cRows - 1 : cRows;
            LPCWSTR           pszName       = NULL;    
            size_t            cchName       = 0;
            WIN32_FIND_DATA * pwfd          = NULL;       
            WORD              wAttr         = 0;      
            size_t            cSpacesNeeded = 0;
            
            

            if ((nRow * cColumns + nCol) >= pdi->m_vMatches.size ())
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

            pwfd = &pdi->m_vMatches[idx];
            hr = GetWideFormattedName (pwfd, &pszName);
            CHR (hr);

            wAttr = m_configPtr->GetTextAttrForFile (pwfd);
            m_consolePtr->Printf (wAttr, L"%.*s", cxColumnWidth, pszName);

            cchName = wcslen (pszName);
            if (cxColumnWidth > cchName)
            {
                for (cSpacesNeeded = cxColumnWidth - wcslen (pszName); cSpacesNeeded > 0; cSpacesNeeded--)
                {
                    m_consolePtr->Printf (CConfig::EAttribute::Default, L" ");
                }
            }
        }

        m_consolePtr->Puts (CConfig::EAttribute::Default, L"");
    }

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWide::GetColumnInfo
//
//  Figure out how many columns fit on the screen
//
////////////////////////////////////////////////////////////////////////////////  

HRESULT CResultsDisplayerWide::GetColumnInfo (__in const CDirectoryInfo * pdi, __out size_t * pcColumns, __out size_t * pcxColumnWidth)
{
    HRESULT                    hr              = S_OK;
    BOOL                       fSuccess;       
    CONSOLE_SCREEN_BUFFER_INFO csbi;           
    UINT                       cxConsoleWidth; 

    

    fSuccess = GetConsoleScreenBufferInfo (m_consolePtr->m_hStdOut, &csbi);
    CBRA (fSuccess);

    cxConsoleWidth  = csbi.srWindow.Right - csbi.srWindow.Left + 1;

    if (pdi->m_cchLargestFileName + 1 > cxConsoleWidth)
    {
        *pcColumns = 1;
    }
    else
    {
        // 1 space between columns
        *pcColumns = cxConsoleWidth / (pdi->m_cchLargestFileName + 1);    
    }

    *pcxColumnWidth = cxConsoleWidth / *pcColumns;

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CResultsDisplayerWide::GetWideFormattedName
//
//  
//
////////////////////////////////////////////////////////////////////////////////  

HRESULT CResultsDisplayerWide::GetWideFormattedName (__in const WIN32_FIND_DATA * pwfd, __deref_out_z LPCWSTR * ppszName)
{
    HRESULT      hr                      = S_OK;
    static WCHAR szDirName[MAX_PATH + 2] = L"[";
    

    if (pwfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        LPWSTR pszBufEnd    = szDirName + 1;
        size_t cchRemaining = 0;
        

        hr = StringCchCopyEx (szDirName + 1, ARRAYSIZE (szDirName) - 2, pwfd->cFileName, &pszBufEnd, &cchRemaining, 0);
        CHRA (hr);

        hr = StringCchCat (pszBufEnd,  cchRemaining, L"]");
        CHRA (hr);

        *ppszName = szDirName;
    }
    else
    {
        *ppszName = pwfd->cFileName;
    }

Error:
    return hr;
}
