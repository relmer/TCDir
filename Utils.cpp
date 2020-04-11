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
    ResetTextAttr();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::ResetTextAttribute
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CUtils::ResetTextAttr (void)
{
    SetTextAttr (m_consoleScreenBufferInfoEx.wAttributes);
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
    m_wDateAttr                 = FC_Red;
    m_wTimeAttr                 = FC_Brown;
    m_wAttributePresentAttr     = FC_Cyan;
    m_wAttributeNotPresentAttr  = FC_DarkGrey;
    m_wInformationStandardAttr  = FC_Cyan;
    m_wInformationHighlightAttr = FC_White;
    m_wSizeAttr                 = FC_Yellow;
    m_wDirAttr                  = FC_LightBlue;
    m_wSeparatorLineAttr        = FC_LightBlue;
    
    
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
//  CUtils::ConsolePrintf
//
//  
//
////////////////////////////////////////////////////////////////////////////////

int CUtils::ConsolePrintf (LPCWSTR pszFormat, ...)
{
    HRESULT       hr            = S_OK;
    va_list       vaArgs        = 0;  
    int           cchRequired   = 0;
    int           cchFormatted  = 0; 
    static int    s_cchBuf      = 256;
    static LPWSTR s_pszBuf      = (LPWSTR) HeapAlloc (GetProcessHeap(), 0, s_cchBuf);     
    DWORD         cchWritten    = 0;   


    
    va_start (vaArgs, pszFormat);
    
    cchRequired = _vscwprintf (pszFormat, vaArgs) + 1;   // _vscprintf doesn't count the terminating '\0'
    if (cchRequired > s_cchBuf)
    {
        HeapReAlloc (GetProcessHeap(), 0, s_pszBuf, cchRequired);
		s_cchBuf = cchRequired;
    }

    cchFormatted = vswprintf_s (s_pszBuf, s_cchBuf, pszFormat, vaArgs);
    CBRA (cchFormatted >= 0);
    CBRA (cchFormatted <= s_cchBuf);

    WriteConsole (m_hStdOut, s_pszBuf, cchFormatted, &cchWritten, NULL);

Error:
    return cchFormatted;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::DisplayLine
//
//  Draws a line across the console at the current cursor position
// 
////////////////////////////////////////////////////////////////////////////////

void CUtils::ConsoleDrawSeparator (void)
{
    DWORD written = 0;
    CONSOLE_SCREEN_BUFFER_INFOEX csbi = { 0 };



    csbi.cbSize = sizeof (csbi);
    GetConsoleScreenBufferInfoEx (m_hStdOut, &csbi);


    FillConsoleOutputAttribute (m_hStdOut,
                                m_wSeparatorLineAttr,
                                csbi.srWindow.Right - csbi.srWindow.Left + 1,
                                csbi.dwCursorPosition,
                                &written);

    FillConsoleOutputCharacter (m_hStdOut,
                                L'═',
                                csbi.srWindow.Right - csbi.srWindow.Left + 1,
                                csbi.dwCursorPosition,
                                &written);

    ++csbi.dwCursorPosition.Y;
    SetConsoleCursorPosition (m_hStdOut, csbi.dwCursorPosition);
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
        wAttr = GetDirAttr();
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
//  CUtils::GetDateAttr
//
//  
//
////////////////////////////////////////////////////////////////////////////////

WORD CUtils::GetDateAttr (void)
{
    return m_wDateAttr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::GetTimeAttr
//
//  
//
////////////////////////////////////////////////////////////////////////////////

WORD CUtils::GetTimeAttr (void)
{
    return m_wTimeAttr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::GetAttributePresentAttr
//
//  
//
////////////////////////////////////////////////////////////////////////////////

WORD CUtils::GetAttributePresentAttr (void)
{
    return m_wAttributePresentAttr;    
}




////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::GetAttributeNotPresentAttr
//
//  
//
////////////////////////////////////////////////////////////////////////////////

WORD CUtils::GetAttributeNotPresentAttr (void)
{
    return m_wAttributeNotPresentAttr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::GetAttributeNotPresentAttr
//
//  
//
////////////////////////////////////////////////////////////////////////////////

WORD CUtils::GetInformationStandardAttr (void)
{
    return m_wInformationStandardAttr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::GetAttributeNotPresentAttr
//
//  
//
////////////////////////////////////////////////////////////////////////////////

WORD CUtils::GetInformationHighlightAttr (void)
{
    return m_wInformationHighlightAttr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::GetSizeAttr
//
//  
//
////////////////////////////////////////////////////////////////////////////////

WORD CUtils::GetSizeAttr (void)
{
    return m_wSizeAttr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::GetDirAttr
//
//  
//
////////////////////////////////////////////////////////////////////////////////

WORD CUtils::GetDirAttr (void)
{
    return m_wDirAttr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::GetDirAttr
//
//  
//
////////////////////////////////////////////////////////////////////////////////

WORD CUtils::GetSeparatorLineAttr (void)
{
    return m_wSeparatorLineAttr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUtils::SetTextAttribute
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CUtils::SetTextAttr (WORD wAttr)
{
    SetConsoleTextAttribute (m_hStdOut, wAttr);
}






