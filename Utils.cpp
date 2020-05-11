#include "StdAfx.h"
#include "utils.h"





CUtils & g_util = CUtils::GetSingleInstance();




const CUtils::STextAttr CUtils::s_rgTextAttrs[]     =
{
    //
    // Code
    //
    
    { TEXT (".c"),              CUtils::FC_LightGreen   },
    { TEXT (".cpp"),            CUtils::FC_LightGreen   },
    { TEXT (".cxx"),            CUtils::FC_LightGreen   },
    { TEXT (".h"),              CUtils::FC_LightGreen   },
    { TEXT (".hpp"),            CUtils::FC_LightGreen   },
    { TEXT (".hxx"),            CUtils::FC_LightGreen   },
    { TEXT (".asm"),            CUtils::FC_LightGreen   },    
    { TEXT (".cs"),             CUtils::FC_LightGreen   },
    { TEXT (".resx"),           CUtils::FC_LightGreen   },
    { TEXT (".rc"),             CUtils::FC_LightGreen   },
    { TEXT (".rcml"),           CUtils::FC_LightGreen   },    
        
    //
    // Intermediate files
    //
    
    { TEXT (".obj"),            CUtils::FC_Green        },
    { TEXT (".lib"),            CUtils::FC_Green        },        
    { TEXT (".res"),            CUtils::FC_Green        },
    { TEXT (".pch"),            CUtils::FC_Green        },

    //
    // Miscellaneous code-related files
    //
    
    { TEXT (".i"),              CUtils::FC_Green        },
    { TEXT (".cod"),            CUtils::FC_Green        },

    //
    // Build
    //
    
    { TEXT (".wrn"),            CUtils::FC_LightRed     },
    { TEXT (".err"),            CUtils::FC_LightRed     },
    { TEXT (".log"),            CUtils::FC_White        },
        
        
    //
    // Executable files
    //
    
    { TEXT (".cmd"),            CUtils::FC_LightRed     },
    { TEXT (".bat"),            CUtils::FC_LightRed     },
    { TEXT (".btm"),            CUtils::FC_LightRed     },    
    { TEXT (".wsh"),            CUtils::FC_LightCyan    },    
    { TEXT (".exe"),            CUtils::FC_LightCyan    },
    { TEXT (".com"),            CUtils::FC_LightCyan    },    

    //
    // Visual Studio files
    //
    
    { TEXT (".sln"),            CUtils::FC_Magenta      },
    { TEXT (".vcproj"),         CUtils::FC_Magenta      },
    { TEXT (".csdproj"),        CUtils::FC_DarkGrey     },
    { TEXT (".user"),           CUtils::FC_DarkGrey     },
    { TEXT (".ncb"),            CUtils::FC_DarkGrey     },

    //
    // Source Insight files
    //
    
    { TEXT (".cf3"),            CUtils::FC_DarkGrey     },
    { TEXT (".iab"),            CUtils::FC_DarkGrey     },
    { TEXT (".iad"),            CUtils::FC_DarkGrey     },
    { TEXT (".imb"),            CUtils::FC_DarkGrey     },
    { TEXT (".imd"),            CUtils::FC_DarkGrey     },
    { TEXT (".pfi"),            CUtils::FC_DarkGrey     },
    { TEXT (".po"),             CUtils::FC_DarkGrey     },
    { TEXT (".pr"),             CUtils::FC_Magenta      },
    { TEXT (".pri"),            CUtils::FC_DarkGrey     },
    { TEXT (".ps"),             CUtils::FC_DarkGrey     },
    { TEXT (".wk3"),            CUtils::FC_DarkGrey     },
    { TEXT (".SearchResults"),  CUtils::FC_DarkGrey     },        



    //
    // Document types
    //
    
    { TEXT (".txt"),            CUtils::FC_White        },
    { TEXT (".me"),             CUtils::FC_White        },
    { TEXT (".text"),           CUtils::FC_White        },
    { TEXT (".1st"),            CUtils::FC_White        },
    { TEXT (".now"),            CUtils::FC_White        },
    { TEXT (".!!!"),            CUtils::FC_White        },
    { TEXT (".doc"),            CUtils::FC_White        },
    { TEXT (".xls"),            CUtils::FC_White        },
    { TEXT (".ppt"),            CUtils::FC_White        },
    { TEXT (".eml"),            CUtils::FC_White        },
    { TEXT (".html"),           CUtils::FC_White        },


    //
    // Compressed/archive types
    //
    
    { TEXT (".zip"),            CUtils::FC_Magenta      },
    { TEXT (".rar"),            CUtils::FC_Magenta      },
    { TEXT (".arj"),            CUtils::FC_Magenta      },
    { TEXT (".gz"),             CUtils::FC_Magenta      },
    { TEXT (".tar"),            CUtils::FC_Magenta      },


    //
    // End of table
    //
    
    { NULL,             0                       }
};





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::CUtils
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CUtils::CUtils(void)
{
    m_hStdOut = GetStdHandle (STD_OUTPUT_HANDLE);   

    ZeroMemory (&m_consoleScreenBufferInfoEx, sizeof (m_consoleScreenBufferInfoEx));
    m_consoleScreenBufferInfoEx.cbSize = sizeof (m_consoleScreenBufferInfoEx);

    GetConsoleScreenBufferInfoEx (m_hStdOut, &m_consoleScreenBufferInfoEx);

    m_coord = m_consoleScreenBufferInfoEx.dwCursorPosition;

    InitializeTextAttrs();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::~CUtils
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CUtils::~CUtils(void)
{
    //
    // Update the console cursor position with our final internal cursor position
    //

    SetConsoleCursorPosition (m_hStdOut, m_coord);

#ifdef USE_SCREEN_BUFFER
    HeapFree (GetProcessHeap (), 0, m_prgScreenBuffer);
#endif
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::InitializeTextAttrs
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CUtils::InitializeTextAttrs (void)
{
    m_rgAttributes[EAttribute::Default]                 = m_consoleScreenBufferInfoEx.wAttributes;
    m_rgAttributes[EAttribute::Date]                    = FC_Red;
    m_rgAttributes[EAttribute::Time]                    = FC_Brown;
    m_rgAttributes[EAttribute::FileAttributePresent]    = FC_Cyan;
    m_rgAttributes[EAttribute::FileAttributeNotPresent] = FC_DarkGrey;
    m_rgAttributes[EAttribute::Information]             = FC_Cyan;
    m_rgAttributes[EAttribute::InformationHighlight]    = FC_White;
    m_rgAttributes[EAttribute::Size]                    = FC_Yellow;
    m_rgAttributes[EAttribute::Directory]               = FC_LightBlue;
    m_rgAttributes[EAttribute::SeparatorLine]           = FC_LightBlue;
    
    InitializeExtensionToTextAttrMap();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::InitializeExtensionToTextAttrMap
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CUtils::InitializeExtensionToTextAttrMap (void)
{
    HRESULT           hr = S_OK;
    const STextAttr * pTextAttr;
    WCHAR             szExtension[MAX_PATH]; 
    errno_t           err = 0;



    pTextAttr = s_rgTextAttrs;
    while (pTextAttr->m_pszExtension != NULL)
    {
        hr = StringCchCopy (szExtension, ARRAYSIZE (szExtension), pTextAttr->m_pszExtension);
        CHRA (hr);
        
        err = _wcslwr_s (szExtension, ARRAYSIZE (szExtension));
        CBRA (err == 0);

        m_mapExtensionToTextAttr[szExtension] = pTextAttr->m_wAttr;
		++pTextAttr;
    }

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::GetSingleInstance
//
//  
//
////////////////////////////////////////////////////////////////////////////////

/* static */ CUtils & CUtils::GetSingleInstance (void)
{
    static CUtils util;

    return util;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::InitializeBuffer
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUtils::InitializeBuffer (void)
{
    HRESULT hr = S_OK;

#ifdef USE_SCREEN_BUFFER

    COORD      coordDestination = { 0, 0 };
    SMALL_RECT srRead = { 0 };

    //
    // Allocate an off-screen buffer
    //

    m_cScreenBuffer = m_consoleScreenBufferInfoEx.dwSize.X * m_consoleScreenBufferInfoEx.dwSize.Y;
    m_prgScreenBuffer = (CHAR_INFO *) HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, m_cScreenBuffer * sizeof (*m_prgScreenBuffer));

    //
    // Initialize it with the current screen buffer's contents
    //

    srRead.Top    = 0;
    srRead.Left   = 0;
    srRead.Bottom = m_consoleScreenBufferInfoEx.dwSize.Y;
    srRead.Right  = m_consoleScreenBufferInfoEx.dwSize.X;

    ReadConsoleOutput (m_hStdOut, m_prgScreenBuffer, m_consoleScreenBufferInfoEx.dwSize, coordDestination, &srRead);

#endif

//Error:

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::FlushBuffer
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUtils::FlushBuffer (void)
{
    HRESULT hr = S_OK;

#ifdef USE_SCREEN_BUFFER

    COORD      coordDestination = { 0, 0 };
    SMALL_RECT srWrite = { 0 };

    //
    // Write the screen buffer to the console
    //

    srWrite.Top    = 0;
    srWrite.Left   = 0;
    srWrite.Bottom = m_consoleScreenBufferInfoEx.dwSize.Y;
    srWrite.Right  = m_consoleScreenBufferInfoEx.dwSize.X;

    WriteConsoleOutput (m_hStdOut, m_prgScreenBuffer, m_consoleScreenBufferInfoEx.dwSize, coordDestination, &srWrite);

    HeapFree (GetProcessHeap (), 0, m_prgScreenBuffer);
    m_prgScreenBuffer = NULL;
    m_cScreenBuffer = 0;

#endif

//Error:

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::ConsolePrintf
//
//  
//
////////////////////////////////////////////////////////////////////////////////

int CUtils::ConsolePrintf (WORD attr, LPCWSTR pszFormat, ...)
{
    static  int      s_cchBuf = 256;
    static  LPWSTR   s_pszBuf = (LPWSTR) HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, s_cchBuf);

    HRESULT   hr              = S_OK;
    va_list   vaArgs          = 0;  
    int       cchRequired     = 0;
    int       cchFormatted    = 0; 
    WCHAR   * p               = NULL;
    WCHAR   * pStart          = NULL;
    WCHAR   * pEnd            = NULL;

    
    va_start (vaArgs, pszFormat);
    
    cchRequired = _vscwprintf (pszFormat, vaArgs) + 1;   // _vscprintf doesn't count the terminating '\0'
    if (cchRequired > s_cchBuf)
    {
        HeapReAlloc (GetProcessHeap(), HEAP_ZERO_MEMORY, s_pszBuf, cchRequired);
		s_cchBuf = cchRequired;
    }

    cchFormatted = vswprintf_s (s_pszBuf, s_cchBuf, pszFormat, vaArgs);
    CBRA (cchFormatted >= 0);
    CBRA (cchFormatted <= s_cchBuf);

    //
    // WriteConsoleOutputCharacter does not support newline characters, so
    // we'll have to handle those manually
    //

    pStart = s_pszBuf;
    pEnd = s_pszBuf + cchFormatted;

    p = pStart;

    while (p < pEnd)
    {
        if (*p == L'\n')
        {
            hr = WriteConsoleString (attr, pStart, p - pStart);
            CHR (hr);

            pStart = p + 1;

            // Handle the newline
            m_coord.X = 0;
            ++m_coord.Y;
        }

        ++p;
    }

    hr = WriteConsoleString (attr, pStart, p - pStart);
    CHR (hr);


Error:
    return cchFormatted;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::WriteConsoleString
//
//  Writes a string to the console using the given attributes at the location 
//  specified by the internal m_coord member and updates the interal cursor 
//  position.  This low-level function writes directly to the console buffer 
//  and does not interpret newline characters or scroll the window.
// 
////////////////////////////////////////////////////////////////////////////////

HRESULT CUtils::WriteConsoleString (WORD attr, __in_ecount(cch) WCHAR * p, size_t cch)
{
    HRESULT   hr   = S_OK;


#ifdef USE_SCREEN_BUFFER

    WCHAR     * pEnd         = p + cch;
    CHAR_INFO * pciBuffer    = m_prgScreenBuffer + ((size_t) m_coord.Y * m_consoleScreenBufferInfoEx.dwSize.X) + m_coord.X;
    CHAR_INFO * pciBufferEnd = m_prgScreenBuffer + m_cScreenBuffer;
    size_t      idx          = 0;



    while (p < pEnd)
    {
        pciBuffer->Attributes       = attr;
        pciBuffer->Char.UnicodeChar = *p;

        ++p;
        ++pciBuffer;

        CBRAEx (pciBuffer < pciBufferEnd, E_NOT_SUFFICIENT_BUFFER)
    }

    idx = pciBuffer - m_prgScreenBuffer;

    m_coord.Y = (short) (idx / m_consoleScreenBufferInfoEx.dwSize.X);
    m_coord.X = (short) (idx % m_consoleScreenBufferInfoEx.dwSize.X);

#else

    BOOL    fSuccess = FALSE;
    DWORD   cWritten = 0;



    fSuccess = FillConsoleOutputAttribute (m_hStdOut, attr, (DWORD) cch, m_coord, &cWritten);
    CWRA (fSuccess);

    fSuccess = WriteConsoleOutputCharacter (m_hStdOut, p, (DWORD) cch, m_coord, &cWritten);
    CWRA (fSuccess);

    m_coord.Y += (m_coord.X + (SHORT) cWritten) / m_consoleScreenBufferInfoEx.dwSize.X;
    m_coord.X += cWritten % m_consoleScreenBufferInfoEx.dwSize.X;

#endif

Error:

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::WriteConsoleLine
//
//  Draws a line across the console at the current internal cursor position
//  and moves to the next line
// 
////////////////////////////////////////////////////////////////////////////////

HRESULT CUtils::WriteConsoleSeparatorLine (void)
{
    HRESULT hr     = S_OK;

#ifdef USE_SCREEN_BUFFER

    size_t  idx    = m_coord.Y * m_consoleScreenBufferInfoEx.dwSize.X;
    size_t  idxEnd = idx + m_consoleScreenBufferInfoEx.dwSize.X;



    while (idx < idxEnd)
    {
        m_prgScreenBuffer[idx].Attributes       = m_rgAttributes[EAttribute::SeparatorLine];
        m_prgScreenBuffer[idx].Char.UnicodeChar = L'═';

        ++idx;
    }

#else 

    BOOL    fSuccess    = FALSE;
    DWORD   bufferWidth = m_consoleScreenBufferInfoEx.dwSize.X;
    DWORD   cWritten    = 0;


    assert (m_coord.X == 0);

    fSuccess = FillConsoleOutputAttribute (m_hStdOut, m_rgAttributes[EAttribute::SeparatorLine], bufferWidth, m_coord, &cWritten);
    CWRA (fSuccess);

    fSuccess = FillConsoleOutputCharacter (m_hStdOut, L'═', bufferWidth, m_coord, &cWritten);
    CWRA (fSuccess);

#endif

    ++m_coord.Y;

#ifndef USE_SCREEN_BUFFER
Error:
#endif
    return hr;
}




////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::IsDots
//
//  
//
////////////////////////////////////////////////////////////////////////////////

BOOL CUtils::IsDots (LPCWSTR pszFileName)
{
    static const WCHAR kchDot  = L'.';
    static const WCHAR kchNull = L'\0';
    BOOL               fDots   = FALSE;


    
    if (pszFileName[0] == kchDot)
    {
        if (pszFileName[1] == kchNull)
        {
            fDots = TRUE;
        }
        else if (pszFileName[1] == kchDot)
        {
            if (pszFileName[2] == kchNull)
            {
                fDots = TRUE;
            }
        }
    }


    return fDots;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::GetTextAttrForFile
//
//  
//
////////////////////////////////////////////////////////////////////////////////

WORD CUtils::GetTextAttrForFile (const WIN32_FIND_DATA * pwfd)
{
    HRESULT hr    = S_OK;
    WORD    wAttr = m_consoleScreenBufferInfoEx.wAttributes;
    errno_t err   = 0;



    if (pwfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        wAttr = m_rgAttributes[EAttribute::Directory];
    }
    else
    {       
        LPCWSTR              pszExtension;          
        WCHAR                szExtension[MAX_PATH]; 
        TextAttrMapConstIter iter;                  


        
        pszExtension = PathFindExtension (pwfd->cFileName);
        StringCchCopy (szExtension, ARRAYSIZE (szExtension), pszExtension);
        
        err = _wcslwr_s (szExtension, ARRAYSIZE (szExtension));
        CBRA (err == 0);

        //
        // Look up the color for this extension.  If no match, bail out and use the default.
        //

        iter = m_mapExtensionToTextAttr.find (szExtension);
        CBR (iter != m_mapExtensionToTextAttr.end ());  

        //
        // Need to investigate... looks like if the extension specified a 
        // background color, make sure we OR in the default foreground text color ?
        //

        wAttr = iter->second;
        if ((wAttr & BC_Mask) == 0)
        {
            wAttr |= m_consoleScreenBufferInfoEx.wAttributes & BC_Mask;
        }
    }    

Error:
    return wAttr;    
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::ScrollBuffer
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUtils::ScrollBuffer (UINT cLines)
{
    HRESULT    hr = S_OK;
    CHAR_INFO  ciFill = { 0 };

    ciFill.Attributes = m_rgAttributes[CUtils::EAttribute::Default];
    ciFill.Char.UnicodeChar = L' ';

#ifdef USE_SCREEN_BUFFER

    size_t      idx           = (short) cLines * m_consoleScreenBufferInfoEx.dwSize.X;
    size_t      cb            = (m_consoleScreenBufferInfoEx.dwSize.Y - (short) cLines) * m_consoleScreenBufferInfoEx.dwSize.X;
    CHAR_INFO * pNewEmptyArea = m_prgScreenBuffer + ((m_consoleScreenBufferInfoEx.dwSize.Y - (short) cLines) * m_consoleScreenBufferInfoEx.dwSize.X);
    CHAR_INFO * pEnd          = m_prgScreenBuffer + (m_consoleScreenBufferInfoEx.dwSize.Y * m_consoleScreenBufferInfoEx.dwSize.X);
   
    MoveMemory (m_prgScreenBuffer, m_prgScreenBuffer + idx, cb);
    
    while (pNewEmptyArea < pEnd)
    {
        *pNewEmptyArea = ciFill;
        ++pNewEmptyArea;
    }

    m_coord.Y = (short) (idx / m_consoleScreenBufferInfoEx.dwSize.X);
    m_coord.X = (short) (idx % m_consoleScreenBufferInfoEx.dwSize.X);

#else

    BOOL       fSuccess       = FALSE;
    SMALL_RECT srcScrollRect  = { 0 };
    COORD      coordDest      = { 0 };
    COORD      coordCursorPos = { 0 };

    srcScrollRect.Top    = (short) cLines;
    srcScrollRect.Bottom = m_consoleScreenBufferInfoEx.dwSize.Y - 1;
    srcScrollRect.Left   = 0;
    srcScrollRect.Right  = m_consoleScreenBufferInfoEx.dwSize.X - 1;

    fSuccess = ScrollConsoleScreenBuffer (m_hStdOut, &srcScrollRect, NULL, coordDest, &ciFill);
    CWRA (fSuccess);

    // this looks wrong...
    //m_coord.Y = srcScrollRect.Top;

#endif

    m_coord.Y -= (short) cLines;

#ifndef USE_SCREEN_BUFFER
Error:
#endif
    return hr;
}
