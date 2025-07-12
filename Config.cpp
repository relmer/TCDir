#include "pch.h"
#include "Config.h"

#include "Color.h"
#include "Console.h"




const CConfig::STextAttr CConfig::s_rgTextAttrs[] =
{
    //
    // Code
    //

    { L".c",              FC_LightGreen   },
    { L".cpp",            FC_LightGreen   },
    { L".cxx",            FC_LightGreen   },
    { L".h",              FC_LightGreen   },
    { L".hpp",            FC_LightGreen   },
    { L".hxx",            FC_LightGreen   },
    { L".js",             FC_LightGreen   },
    { L".ts",             FC_LightGreen   },
    { L".asm",            FC_LightGreen   },
    { L".cs",             FC_LightGreen   },
    { L".resx",           FC_LightGreen   },
    { L".rc",             FC_LightGreen   },
    { L".rcml",           FC_LightGreen   },

    //
    // Intermediate files
    //

    { L".obj",            FC_Green        },
    { L".lib",            FC_Green        },
    { L".res",            FC_Green        },
    { L".pch",            FC_Green        },

    //
    // Miscellaneous code-related files
    //

    { L".i",              FC_Green        },
    { L".cod",            FC_Green        },

    //
    // Build
    //

    { L".wrn",            FC_LightRed     },
    { L".err",            FC_LightRed     },
    { L".log",            FC_White        },

    //
    // Executable files
    //

    { L".cmd",            FC_LightRed     },
    { L".bat",            FC_LightRed     },
    { L".btm",            FC_LightRed     },
    { L".ps1",            FC_LightRed     },
    { L".wsh",            FC_LightCyan    },
    { L".exe",            FC_LightCyan    },
    { L".com",            FC_LightCyan    },

    //
    // Visual Studio files
    //

    { L".sln",            FC_Magenta      },
    { L".vcproj",         FC_Magenta      },
    { L".csproj",         FC_DarkGrey     },
    { L".user",           FC_DarkGrey     },
    { L".ncb",            FC_DarkGrey     },

    //
    // Source Insight files
    //

    { L".cf3",            FC_DarkGrey     },
    { L".iab",            FC_DarkGrey     },
    { L".iad",            FC_DarkGrey     },
    { L".imb",            FC_DarkGrey     },
    { L".imd",            FC_DarkGrey     },
    { L".pfi",            FC_DarkGrey     },
    { L".po",             FC_DarkGrey     },
    { L".pr",             FC_Magenta      },
    { L".pri",            FC_DarkGrey     },
    { L".ps",             FC_DarkGrey     },
    { L".wk3",            FC_DarkGrey     },
    { L".SearchResults",  FC_DarkGrey     },

    //
    // Document types
    //

    { L".txt",            FC_White        },
    { L".md",             FC_White        },
    { L".me",             FC_White        },
    { L".text",           FC_White        },
    { L".1st",            FC_White        },
    { L".now",            FC_White        },
    { L".!!!",            FC_White        },
    { L".doc",            FC_White        },
    { L".docx",           FC_White        },
    { L".xls",            FC_White        },
    { L".xlsx",           FC_White        },
    { L".ppt",            FC_White        },
    { L".pptx",           FC_White        },
    { L".eml",            FC_White        },
    { L".html",           FC_White        },

    //
    // Compressed/archive types
    //

    { L".zip",            FC_Magenta      },
    { L".rar",            FC_Magenta      },
    { L".arj",            FC_Magenta      },
    { L".7z",             FC_Magenta      },
    { L".gz",             FC_Magenta      },
    { L".tar",            FC_Magenta      },
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

CConfig::~CConfig ()
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

    {
        TextAttrMap  footest;
    }

    InitializeExtensionToTextAttrMap ();

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
    for (const STextAttr & textAttr : s_rgTextAttrs)
    {
        wstring strExtension (textAttr.m_pszExtension);

        std::transform (strExtension.begin(), 
                        strExtension.end(), 
                        strExtension.begin(),
                        [] (wchar_t c) { return towlower (c); });

        ASSERT (!m_mapExtensionToTextAttr.contains (strExtension));

        m_mapExtensionToTextAttr[strExtension] = textAttr.m_wAttr;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::GetTextAttrForFile
//
//  
//
////////////////////////////////////////////////////////////////////////////////

WORD CConfig::GetTextAttrForFile (const WIN32_FIND_DATA * pwfd)
{
    HRESULT              hr           = S_OK;
    WORD                 textAttr     = m_rgAttributes[EAttribute::Default];
    filesystem::path     filename       (pwfd->cFileName);
    wstring              strExtension;
    TextAttrMapConstIter iter;




    if (pwfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        textAttr = m_rgAttributes[EAttribute::Directory];
        BAIL_OUT_IF (TRUE, S_OK);
    }

    //
    // Convert the extension to lowecase
    //

    strExtension = filename.extension().wstring();

    transform (strExtension.begin(),
               strExtension.end(),
               strExtension.begin(),
               [] (wchar_t c) { return towlower (c); });

    //
    // Look up the color for this extension.  If no match, bail out and use the default.
    //

    iter = m_mapExtensionToTextAttr.find (strExtension);
    BAIL_OUT_IF (iter == m_mapExtensionToTextAttr.end(), S_OK);

    textAttr = iter->second;
    if ((textAttr & BC_Mask) == 0)
    {
        textAttr |= m_rgAttributes[EAttribute::Default] & BC_Mask;
    }
  

Error:
    return textAttr;
}
