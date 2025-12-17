#include "pch.h"
#include "Config.h"

#include "Color.h"





constexpr CConfig::STextAttr CConfig::s_rgTextAttrs[] =
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
//  CConfig::Initialize
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::Initialize (WORD wDefaultAttr)
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
    m_rgAttributes[EAttribute::Error]                   = FC_LightRed;

    

    InitializeExtensionToTextAttrMap();
    ApplyUserColorOverrides();
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
        m_mapExtensionSources[strExtension] = EAttributeSource::Default;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ApplyUserColorOverrides
//
//  Parse the TCDIR environment variable for user color overrides.
//  Format: <attr|.ext>=<fore>[on <back>][;...]
//  Example: .cpp=LightGreen;.h=Yellow on Blue
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ApplyUserColorOverrides (void)
{
    HRESULT hr               = S_OK;
    DWORD   cchBufNeeded     = 0;
    DWORD   cchExcludingNull = 0;
    wstring envValue;

    // Clear previous validation results
    m_lastParseResult.warnings.clear();
    m_lastParseResult.errors.clear();

    //
    // Read environment variable directly into wstring
    //
    
    cchBufNeeded = GetEnvironmentVariableW (TCDIR_ENV_VAR_NAME, nullptr, 0);
    BAIL_OUT_IF (cchBufNeeded == 0, S_OK);  // Variable not set
    
    cchExcludingNull = cchBufNeeded - 1;
    envValue.resize (cchExcludingNull, L'\0');  // -1 because cchBufNeeded includes null terminator
    
    cchBufNeeded = GetEnvironmentVariableW (TCDIR_ENV_VAR_NAME, envValue.data(), cchBufNeeded);
    CWRA (cchBufNeeded == cchExcludingNull);
    
    //
    // Use ranges/views to avoid copies
    //
    
    for (auto token : envValue | std::views::split (L';'))
    {
        wstring_view entry (token.begin (), token.end ());
        ProcessColorOverrideEntry (entry);
    }



Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ProcessColorOverrideEntry
//
//  Process a single color override entry in the format: <key>=<value>
//  Example: .cpp=Yellow 
//           .h=LightCyan on Blue
//           D=Red
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ProcessColorOverrideEntry (wstring_view entry)
{
    HRESULT      hr        = S_OK;
    wstring_view keyView;
    wstring_view valueView;
    WORD         colorAttr = 0;


    // Skip empty entries (from trailing semicolons or multiple semicolons)
    entry = TrimWhitespace(entry);
    BAIL_OUT_IF (entry.empty(), S_OK);
        
    hr = ParseKeyAndValue (entry, keyView, valueView);
    if (FAILED(hr))
    {
        wstring msg = L"Invalid entry format: '";
        msg += entry;
        msg += L"' (expected format: key=value)";
        m_lastParseResult.errors.push_back(msg);
        BAIL_OUT_IF(TRUE, S_OK);
    }

    BAIL_OUT_IF (keyView.empty(), S_OK);

    {
        WORD foreColor = 0;
        WORD backColor = 0;



        auto result = std::ranges::search (
            valueView,
            L" on "sv,
            [] (wchar_t a, wchar_t b) { return towlower (a) == towlower (b); }
        );

        if (result.empty ())
        {
            wstring_view foreView = TrimWhitespace (valueView);
            foreColor = ParseColorName (foreView, false);

            if (foreColor == 0)
            {
                wstring msg = L"Invalid foreground color: '";
                msg += foreView;
                msg += L"' in entry '";
                msg += entry;
                msg += L"'";
                m_lastParseResult.errors.push_back (msg);
                BAIL_OUT_IF (TRUE, S_OK);
            }
        }
        else
        {
            size_t onPos = result.begin () - valueView.begin ();

            wstring_view foreView = TrimWhitespace (valueView.substr (0, onPos));
            wstring_view backView = TrimWhitespace (valueView.substr (onPos + 4));

            foreColor = ParseColorName (foreView, false);
            if (foreColor == 0)
            {
                wstring msg = L"Invalid foreground color: '";
                msg += foreView;
                msg += L"' in entry '";
                msg += entry;
                msg += L"'";
                m_lastParseResult.errors.push_back (msg);
                BAIL_OUT_IF (TRUE, S_OK);
            }

            backColor = ParseColorName (backView, true);
            if (backColor == 0)
            {
                wstring msg = L"Invalid background color: '";
                msg += backView;
                msg += L"' in entry '";
                msg += entry;
                msg += L"'";
                m_lastParseResult.warnings.push_back (msg);
            }
        }

        colorAttr = foreColor | backColor;
    }
    
    //
    // Apply the override based on key type
    //
    
    if (keyView[0] == L'.')
    {
        ProcessFileExtensionOverride (keyView, colorAttr);
    }
    else if (keyView.length() == 1)
    {
        ProcessDisplayAttributeOverride (keyView[0], colorAttr);
    }
    else
    {
        wstring msg = L"Invalid key: '";
        msg += keyView;
        msg += L"' (expected single character or file extension starting with '.')";
        m_lastParseResult.errors.push_back(msg);
    }


Error:
    return;
}




////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ProcessFileExtensionOverride
//
//  Apply color override for a file extension (e.g., ".cpp")
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ProcessFileExtensionOverride (wstring_view extension, WORD colorAttr)
{
    wstring key (extension);
    
    std::ranges::transform (key, key.begin(), towlower);
    m_mapExtensionToTextAttr[key] = colorAttr;
    m_mapExtensionSources[key] = EAttributeSource::Environment;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ProcessDisplayAttributeOverride
//
//  Apply color override for a display attribute (e.g., "D" for Date)
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ProcessDisplayAttributeOverride (wchar_t attrChar, WORD colorAttr)
{
    struct AttrMapping
    {
        wchar_t    ch;
        EAttribute attr;
    };
    
    static constexpr AttrMapping s_attrMappings[] =
    {
        { L'D', EAttribute::Date                    },
        { L'T', EAttribute::Time                    },
        { L'A', EAttribute::FileAttributePresent    },
        { L'-', EAttribute::FileAttributeNotPresent },
        { L'S', EAttribute::Size                    },
        { L'R', EAttribute::Directory               },
        { L'I', EAttribute::Information             },
        { L'H', EAttribute::InformationHighlight    },
        // { L'L', EAttribute::SeparatorLine        }, // Currently unused
        { L'E', EAttribute::Error                   },
        { L'F', EAttribute::Default                 },
    };
    
    attrChar = towupper (attrChar);
    auto iter = std::ranges::find (s_attrMappings, attrChar, &AttrMapping::ch);
    
    if (iter != std::ranges::end (s_attrMappings))
    {
        m_rgAttributes[iter->attr] = colorAttr;
        m_rgAttributeSources[iter->attr] = EAttributeSource::Environment;
    }
    else
    {
        wstring msg = L"Invalid display attribute character: '";
        msg += attrChar;
        msg += L"' (valid: D,T,A,-,S,R,I,H,E,F)";
        m_lastParseResult.errors.push_back(msg);
    }
}



////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ParseKeyAndValue
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConfig::ParseKeyAndValue (wstring_view entry, wstring_view & keyView, wstring_view & valueView)
{
    HRESULT      hr = S_OK;
    size_t       equalPos = 0;



    equalPos = entry.find (L'=');
    CBREx (equalPos != wstring_view::npos, E_INVALIDARG);

    keyView   = entry.substr (0, equalPos);
    valueView = entry.substr (equalPos + 1);

    keyView   = TrimWhitespace (keyView);
    valueView = TrimWhitespace (valueView);

    CBREx (!keyView.empty () && !valueView.empty (), E_INVALIDARG);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ParseColorSpec
//
//  Parse a color specification in the format: <fore> [on <back>]
//  Examples: "Yellow", "LightCyan on Blue"
//
////////////////////////////////////////////////////////////////////////////////

WORD CConfig::ParseColorSpec (wstring_view colorSpec)
{
    WORD foreColor = 0;
    WORD backColor = 0;

    

    auto result = std::ranges::search (
        colorSpec,
        L" on "sv,
        [] (wchar_t a, wchar_t b) { return towlower (a) == towlower (b); }
    );

    if (result.empty ())
    {
        // Only foreground specified
        wstring_view foreView = TrimWhitespace (colorSpec);
        foreColor = ParseColorName (foreView, false);
    }
    else
    {
        size_t onPos = result.begin () - colorSpec.begin ();

        // Both foreground and background specified
        wstring_view foreView = TrimWhitespace (colorSpec.substr (0, onPos));
        wstring_view backView = TrimWhitespace (colorSpec.substr (onPos + 4));

        foreColor = ParseColorName (foreView, false);
        backColor = ParseColorName (backView, true);
    }

    return foreColor | backColor;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::TrimWhitespace
//
//  Remove leading and trailing whitespace from a string_view.
//  Returns a new string with whitespace removed.
//
////////////////////////////////////////////////////////////////////////////////

wstring_view CConfig::TrimWhitespace (wstring_view str)
{
    size_t start = str.find_first_not_of (L" ");
    if (start == wstring_view::npos)
    {
        return L"";  // String is all whitespace
    }
    
    size_t end = str.find_last_not_of (L" ");
    return str.substr (start, end - start + 1);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ParseColorName
//
//  Convert a color name string to a WORD color value.
//
////////////////////////////////////////////////////////////////////////////////

WORD CConfig::ParseColorName (wstring_view colorName, bool isBackground)
{
    struct ColorMapping
    {
        wstring_view name;
        WORD         foreValue;
        WORD         backValue;
    };
    
    static const ColorMapping s_colors[] =
    {
        { L"Black"sv,        FC_Black,        BC_Black        },
        { L"Blue"sv,         FC_Blue,         BC_Blue         },
        { L"Green"sv,        FC_Green,        BC_Green        },
        { L"Cyan"sv,         FC_Cyan,         BC_Cyan         },
        { L"Red"sv,          FC_Red,          BC_Red          },
        { L"Magenta"sv,      FC_Magenta,      BC_Magenta      },
        { L"Brown"sv,        FC_Brown,        BC_Brown        },
        { L"LightGrey"sv,    FC_LightGrey,    BC_LightGrey    },
        { L"DarkGrey"sv,     FC_DarkGrey,     BC_DarkGrey     },
        { L"LightBlue"sv,    FC_LightBlue,    BC_LightBlue    },
        { L"LightGreen"sv,   FC_LightGreen,   BC_LightGreen   },
        { L"LightCyan"sv,    FC_LightCyan,    BC_LightCyan    },
        { L"LightRed"sv,     FC_LightRed,     BC_LightRed     },
        { L"LightMagenta"sv, FC_LightMagenta, BC_LightMagenta },
        { L"Yellow"sv,       FC_Yellow,       BC_Yellow       },
        { L"White"sv,        FC_White,        BC_White        },
    };



    //
    // Find matching color name (case-insensitive)
    //

    for (const ColorMapping & mapping : s_colors)
    {
        // Use ranges::equal with case-insensitive comparison
        if (std::ranges::equal (
            colorName,
            mapping.name,
            [] (wchar_t a, wchar_t b) { return towlower (a) == towlower (b); }
        ))
        {
            return isBackground ? mapping.backValue : mapping.foreValue;
        }
    }

    return 0;  // Default/not found
}



////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::GetTextAttrForFile
//
//  
//
////////////////////////////////////////////////////////////////////////////////

WORD CConfig::GetTextAttrForFile (const WIN32_FIND_DATA & wfd)
{
    HRESULT              hr           = S_OK;
    WORD                 textAttr     = m_rgAttributes[EAttribute::Default];
    filesystem::path     filename       (wfd.cFileName);
    wstring              strExtension;
    TextAttrMapConstIter iter;




    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
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





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ValidateEnvironmentVariable
//
//  Returns the validation results collected during ApplyUserColorOverrides
//
////////////////////////////////////////////////////////////////////////////////

CConfig::ValidationResult CConfig::ValidateEnvironmentVariable (void)
{
    return m_lastParseResult;
}
