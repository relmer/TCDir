#include "stdafx.h"
#include "Config.h"

#include "Color.h"
#include "Console.h"




const CConfig::STextAttr CConfig::s_rgTextAttrs[] =
{
    //
    // Code
    //

    { TEXT(".c"),              FC_LightGreen   },
    { TEXT(".cpp"),            FC_LightGreen   },
    { TEXT(".cxx"),            FC_LightGreen   },
    { TEXT(".h"),              FC_LightGreen   },
    { TEXT(".hpp"),            FC_LightGreen   },
    { TEXT(".hxx"),            FC_LightGreen   },
    { TEXT(".asm"),            FC_LightGreen   },
    { TEXT(".cs"),             FC_LightGreen   },
    { TEXT(".resx"),           FC_LightGreen   },
    { TEXT(".rc"),             FC_LightGreen   },
    { TEXT(".rcml"),           FC_LightGreen   },

    //
    // Intermediate files
    //

    { TEXT(".obj"),            FC_Green        },
    { TEXT(".lib"),            FC_Green        },
    { TEXT(".res"),            FC_Green        },
    { TEXT(".pch"),            FC_Green        },

    //
    // Miscellaneous code-related files
    //

    { TEXT(".i"),              FC_Green        },
    { TEXT(".cod"),            FC_Green        },

    //
    // Build
    //

    { TEXT(".wrn"),            FC_LightRed     },
    { TEXT(".err"),            FC_LightRed     },
    { TEXT(".log"),            FC_White        },

    //
    // Executable files
    //

    { TEXT(".cmd"),            FC_LightRed     },
    { TEXT(".bat"),            FC_LightRed     },
    { TEXT(".btm"),            FC_LightRed     },
    { TEXT(".wsh"),            FC_LightCyan    },
    { TEXT(".exe"),            FC_LightCyan    },
    { TEXT(".com"),            FC_LightCyan    },

    //
    // Visual Studio files
    //

    { TEXT(".sln"),            FC_Magenta      },
    { TEXT(".vcproj"),         FC_Magenta      },
    { TEXT(".csdproj"),        FC_DarkGrey     },
    { TEXT(".user"),           FC_DarkGrey     },
    { TEXT(".ncb"),            FC_DarkGrey     },

    //
    // Source Insight files
    //

    { TEXT(".cf3"),            FC_DarkGrey     },
    { TEXT(".iab"),            FC_DarkGrey     },
    { TEXT(".iad"),            FC_DarkGrey     },
    { TEXT(".imb"),            FC_DarkGrey     },
    { TEXT(".imd"),            FC_DarkGrey     },
    { TEXT(".pfi"),            FC_DarkGrey     },
    { TEXT(".po"),             FC_DarkGrey     },
    { TEXT(".pr"),             FC_Magenta      },
    { TEXT(".pri"),            FC_DarkGrey     },
    { TEXT(".ps"),             FC_DarkGrey     },
    { TEXT(".wk3"),            FC_DarkGrey     },
    { TEXT(".SearchResults"),  FC_DarkGrey     },

    //
    // Document types
    //

    { TEXT(".txt"),            FC_White        },
    { TEXT(".me"),             FC_White        },
    { TEXT(".text"),           FC_White        },
    { TEXT(".1st"),            FC_White        },
    { TEXT(".now"),            FC_White        },
    { TEXT(".!!!"),            FC_White        },
    { TEXT(".doc"),            FC_White        },
    { TEXT(".xls"),            FC_White        },
    { TEXT(".ppt"),            FC_White        },
    { TEXT(".eml"),            FC_White        },
    { TEXT(".html"),           FC_White        },

    //
    // Compressed/archive types
    //

    { TEXT(".zip"),            FC_Magenta      },
    { TEXT(".rar"),            FC_Magenta      },
    { TEXT(".arj"),            FC_Magenta      },
    { TEXT(".gz"),             FC_Magenta      },
    { TEXT(".tar"),            FC_Magenta      },

    //
    // End of table
    //

    { NULL,                     0              }
};





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::CConfig
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CConfig::CConfig (WORD wDefaultAttr)
{
    InitializeTextAttrs (wDefaultAttr);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::~CConfig
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CConfig::~CConfig()
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::InitializeTextAttrs
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::InitializeTextAttrs (WORD wDefaultAttr)
{
    m_rgAttributes[EAttribute::Default]                 = wDefaultAttr;
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
//  CConfig::InitializeExtensionToTextAttrMap
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::InitializeExtensionToTextAttrMap (void)
{
    HRESULT            hr = S_OK;
    const STextAttr * pTextAttr;
    WCHAR              szExtension[MAX_PATH];
    errno_t            err = 0;



    pTextAttr = s_rgTextAttrs;
    while (pTextAttr->m_pszExtension != NULL)
    {
        hr = StringCchCopy(szExtension, ARRAYSIZE(szExtension), pTextAttr->m_pszExtension);
        CHRA(hr);

        err = _wcslwr_s(szExtension, ARRAYSIZE(szExtension));
        CBRA(err == 0);

        m_mapExtensionToTextAttr[szExtension] = pTextAttr->m_wAttr;
        ++pTextAttr;
    }

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::GetTextAttrForFile
//
//  
//
////////////////////////////////////////////////////////////////////////////////

WORD CConfig::GetTextAttrForFile(const WIN32_FIND_DATA * pwfd)
{
    HRESULT hr = S_OK;
    WORD    wAttr = m_rgAttributes[EAttribute::Default];
    errno_t err = 0;



    if (pwfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        wAttr = m_rgAttributes[EAttribute::Directory];
    }
    else
    {
        LPCWSTR              pszExtension;
        WCHAR                szExtension[MAX_PATH];
        TextAttrMapConstIter iter;



        pszExtension = PathFindExtension(pwfd->cFileName);
        StringCchCopy(szExtension, ARRAYSIZE(szExtension), pszExtension);

        err = _wcslwr_s(szExtension, ARRAYSIZE(szExtension));
        CBRA(err == 0);

        //
        // Look up the color for this extension.  If no match, bail out and use the default.
        //

        iter = m_mapExtensionToTextAttr.find(szExtension);
        CBR(iter != m_mapExtensionToTextAttr.end());

        //
        // Need to investigate... looks like if the extension specified only a 
        // background color, make sure we OR in the default foreground text color ?
        //

        wAttr = iter->second;
        if ((wAttr & BC_Mask) == 0)
        {
            wAttr |= m_rgAttributes[EAttribute::Default] & BC_Mask;
        }
    }

Error:
    return wAttr;
}
