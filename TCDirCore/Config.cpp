#include "pch.h"
#include "Config.h"

#include "Color.h"
#include "EnvironmentProvider.h"





struct SFileAttributeKeyMap
{
    DWORD   m_dwAttribute;
    wchar_t m_chKey;
};

static constexpr SFileAttributeKeyMap s_krgFileAttributeKeyMap[] =
{
    {  FILE_ATTRIBUTE_READONLY,      L'R' },
    {  FILE_ATTRIBUTE_HIDDEN,        L'H' },
    {  FILE_ATTRIBUTE_SYSTEM,        L'S' },
    {  FILE_ATTRIBUTE_ARCHIVE,       L'A' },
    {  FILE_ATTRIBUTE_TEMPORARY,     L'T' },
    {  FILE_ATTRIBUTE_ENCRYPTED,     L'E' },
    {  FILE_ATTRIBUTE_COMPRESSED,    L'C' },
    {  FILE_ATTRIBUTE_REPARSE_POINT, L'P' },
    {  FILE_ATTRIBUTE_SPARSE_FILE,   L'0' },
};





constexpr CConfig::STextAttr CConfig::s_rgTextAttrs[] =
{
    //
    // Code
    //

    { L".asm",            FC_LightGreen   },
    { L".cod",            FC_Green        },
    { L".i",              FC_Green        },

    { L".c",              FC_LightGreen   },
    { L".cpp",            FC_LightGreen   },
    { L".cxx",            FC_LightGreen   },
    { L".h",              FC_LightGreen   },
    { L".hpp",            FC_LightGreen   },
    { L".hxx",            FC_LightGreen   },
    { L".rc",             FC_LightGreen   },
    
    { L".cs",             FC_LightGreen   },
    { L".resx",           FC_LightGreen   },
    { L".rcml",           FC_LightGreen   },
    
    { L".js",             FC_LightGreen   },
    { L".jsx",            FC_LightGreen   },
    { L".ts",             FC_LightGreen   },
    { L".tsx",            FC_LightGreen   },

    { L".html",           FC_LightGreen   },
    { L".htm",            FC_LightGreen   },
    { L".css",            FC_LightGreen   },
    { L".scss",           FC_LightGreen   },
    { L".less",           FC_LightGreen   },

    { L".py",             FC_LightGreen   },
    { L".pyw",            FC_LightGreen   },

    { L".jar",            FC_LightGreen   },
    { L".java",           FC_LightGreen   },
    { L".class",          FC_LightGreen   },

    { L".xml",            FC_Brown       },
    { L".json",           FC_Brown       },
    { L".yml",            FC_Brown       },
    { L".yaml",           FC_Brown       },

    //
    // Intermediate files
    //

    { L".obj",            FC_Green        },
    { L".lib",            FC_Green        },
    { L".res",            FC_Green        },
    { L".pch",            FC_Green        },

    //
    // Build
    //

    { L".wrn",            FC_LightRed     },
    { L".err",            FC_LightRed     },
    { L".log",            FC_White        },

    //
    // Executable files
    //

    { L".bash",           FC_LightRed     },
    { L".bat",            FC_LightRed     },
    { L".cmd",            FC_LightRed     },
    { L".dll",            FC_LightCyan    },
    { L".exe",            FC_LightCyan    },
    { L".ps1",            FC_LightRed     },
    { L".psd1",           FC_LightRed     },
    { L".psm1",           FC_LightRed     },
    { L".sh",             FC_LightRed     },
    { L".sys",            FC_LightCyan    },
    

    //
    // Visual Studio files
    //

    { L".sln",            FC_Magenta      },
    { L".vcproj",         FC_Magenta      },
    { L".csproj",         FC_DarkGrey     },
    { L".vcxproj",        FC_Magenta      },
    { L".csxproj",        FC_DarkGrey     },
    { L".user",           FC_DarkGrey     },
    { L".ncb",            FC_DarkGrey     },

    //
    // Document types
    //

    { L".!!!",            FC_White        },
    { L".1st",            FC_White        },
    { L".doc",            FC_White        },
    { L".docx",           FC_White        },
    { L".eml",            FC_White        },
    { L".md",             FC_White        },
    { L".me",             FC_White        },
    { L".now",            FC_White        },
    { L".ppt",            FC_White        },
    { L".pptx",           FC_White        },
    { L".text",           FC_White        },
    { L".txt",            FC_White        },
    { L".xls",            FC_White        },
    { L".xlsx",           FC_White        },

    //
    // Compressed/archive types
    //

    { L".7z",             FC_Magenta      },
    { L".arj",            FC_Magenta      },
    { L".gz",             FC_Magenta      },
    { L".rar",            FC_Magenta      },
    { L".tar",            FC_Magenta      },
    { L".zip",            FC_Magenta      },
};





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::CConfig
//
////////////////////////////////////////////////////////////////////////////////

CConfig::CConfig (void)
{
    m_pEnvironmentProvider = &m_environmentProviderDefault;
}





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
    m_rgAttributes[EAttribute::CloudStatusCloudOnly]    = FC_LightBlue;
    m_rgAttributes[EAttribute::CloudStatusLocal]        = FC_LightGreen;
    m_rgAttributes[EAttribute::CloudStatusPinned]       = FC_Green;
  
    InitializeExtensionToTextAttrMap();
    InitializeFileAttributeToTextAttrMap();
    ApplyUserColorOverrides();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::SetEnvironmentProvider
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::SetEnvironmentProvider (const IEnvironmentProvider * pProvider)
{
    if (pProvider == nullptr)
    {
        m_pEnvironmentProvider = &m_environmentProviderDefault;
    }
    else
    {
        m_pEnvironmentProvider = pProvider;
    }
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
//  CConfig::InitializeFileAttributeToTextAttrMap
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::InitializeFileAttributeToTextAttrMap (void)
{
    // Only default colors requested for now.
    m_mapFileAttributesTextAttr.clear();

    m_mapFileAttributesTextAttr[FILE_ATTRIBUTE_HIDDEN]    = { FC_DarkGrey,   EAttributeSource::Default };
    m_mapFileAttributesTextAttr[FILE_ATTRIBUTE_ENCRYPTED] = { FC_LightGreen, EAttributeSource::Default };
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ApplyUserColorOverrides
//
//  Parse the TCDIR environment variable for user color overrides.
//  Format: <attr|.ext|attr:fileattr>=<fore>[on <back>][;...]
//  Example: .cpp=LightGreen;.h=Yellow on Blue
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ApplyUserColorOverrides (void)
{
    HRESULT hr       = S_OK;
    wstring envValue;



    // Clear previous validation results
    m_lastParseResult.errors.clear();

    bool isSet = m_pEnvironmentProvider->TryGetEnvironmentVariable (TCDIR_ENV_VAR_NAME, envValue);
    BAIL_OUT_IF (!isSet, S_OK);
    
    //
    // Use ranges/views to avoid copies
    //
    
    for (auto token : envValue | std::views::split (L';'))
    {
        wstring_view entry (token.begin(), token.end());
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
//           attr:h=DarkGrey
//           /w (switch)
//           -s (switch)
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ProcessColorOverrideEntry (wstring_view entry)
{
    HRESULT      hr        = S_OK;
    wstring_view keyView;
    wstring_view valueView;
    WORD         colorAttr = 0;



    // Skip empty entries (from trailing semicolons or multiple semicolons)
    entry = TrimWhitespace (entry);
    CBR (!entry.empty());

    if (entry[0] == L'/' || entry[0] == L'-')
    {
        ProcessSwitchOverride (entry);
        BAIL_OUT_IF (TRUE, S_OK);
    }
        
    hr = ParseKeyAndValue (entry, keyView, valueView);
    if (FAILED (hr))
    {
        m_lastParseResult.errors.push_back ( { L"Invalid entry format (expected key = value)", 
                                               wstring (entry), wstring (entry), 0 } );
        CHR (hr);
    }

    CBR (!keyView.empty());

    hr = ParseColorValue (entry, valueView, colorAttr);
    CHR (hr);
    
    //
    // Apply the override based on key type
    //
    
    if (keyView[0] == L'.')
    {
        ProcessFileExtensionOverride (keyView, colorAttr);
    }
    else if (keyView.length() == 6 &&
             towlower (keyView[0]) == L'a' &&
             towlower (keyView[1]) == L't' &&
             towlower (keyView[2]) == L't' &&
             towlower (keyView[3]) == L'r' &&
             keyView[4] == L':')
    {
        ProcessFileAttributeOverride (keyView, colorAttr, entry);
    }
    else if (keyView.length() == 1)
    {
        ProcessDisplayAttributeOverride (keyView[0], colorAttr, entry);
    }
    else
    {
        m_lastParseResult.errors.push_back ( { L"Invalid key (expected single character, .extension, or attr:x)",
                                               wstring (entry), wstring (keyView), entry.find (keyView) } );
    }


Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ParseColorValue
//
//  Parse a color value specification in the format: <fore>[on <back>]
//  Records errors to m_lastParseResult if parsing fails.
//  Returns S_OK on success, E_FAIL if foreground color is invalid.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConfig::ParseColorValue (wstring_view entry, wstring_view valueView, WORD & colorAttr)
{
    HRESULT      hr          = S_OK;
    WORD         foreColor   = 0;
    WORD         backColor   = 0;
    wstring_view foreView;
    wstring_view backView;
    size_t       foreOffset  = 0;
    size_t       backOffset  = 0;



    //
    // Find the '=' position for error reporting
    //

    size_t equalPos = entry.find (L'=');

    //
    // Check for " on " separator (case-insensitive)
    //

    auto result = std::ranges::search (valueView, L" on "sv,
                                       [] (wchar_t a, wchar_t b) { return towlower (a) == towlower (b); } );
    if (result.empty())
    {
        // Foreground only
        foreView = TrimWhitespace (valueView);
    }
    else
    {
        // Foreground and background
        size_t onPosValue = result.begin() - valueView.begin();
        foreView = TrimWhitespace (valueView.substr (0, onPosValue));
        backView = TrimWhitespace (valueView.substr (onPosValue + 4));

        // Calculate background offset in entry: skip past " on " and whitespace
        if (equalPos != wstring_view::npos)
        {
            backOffset = equalPos + 1 + onPosValue + 4;  // Skip past "=" and " on "
            while (backOffset < entry.length() && iswspace(entry[backOffset]))
            {
                backOffset++;
            }
        }
    }

    //
    // Calculate foreground offset in entry: skip past "=" and whitespace
    //

    if (equalPos != wstring_view::npos)
    {
        foreOffset = equalPos + 1;
        while (foreOffset < entry.length() && iswspace(entry[foreOffset]))
        {
            foreOffset++;
        }
    }

    //
    // Parse foreground color
    //

    hr = ParseColorName (foreView, false, foreColor);
    if (FAILED (hr))
    {
        m_lastParseResult.errors.push_back({
            L"Invalid foreground color",
            wstring(entry),
            wstring(foreView),
            foreOffset
        });
        CBR (FALSE);
    }

    //
    // Parse background color if present
    //

    if (!backView.empty())
    {
        HRESULT hrBack = ParseColorName (backView, true, backColor);
        if (FAILED (hrBack))
        {
            m_lastParseResult.errors.push_back({
                L"Invalid background color",
                wstring(entry),
                wstring(backView),
                backOffset
            });
            // Continue with foreground only - don't fail
            backColor = 0;
        }
    }

    colorAttr = foreColor | backColor;

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ProcessSwitchOverride
//
//  Process a switch override entry (e.g., /s, /w, /m-, -s, -w)
//  Supports: /S (recurse), /W (wide), /P (perf timer), /M (multithreaded)
//  Use trailing '-' to disable (e.g., /M-)
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ProcessSwitchOverride (wstring_view entry)
{
    static constexpr LPCWSTR s_krgExample[] =
    {
        L"Invalid switch (expected -S, -W, -P, or -M)",
        L"Invalid switch (expected /S, /W, /P, or /M)",
    };

    HRESULT      hr                = S_OK;
    size_t       idxExample        = 0;
    wstring_view invalidText       = entry;
    size_t       invalidTextOffset = 0;
    wchar_t      ch                = 0;
    bool         fValue            = true;



    // Entry should be at least 2 characters (e.g., "/s" or "-w")
    switch (entry.length())
    {
        case 2:
            break;

        case 3:
            CBR (entry[2] == L'-');
            fValue = false;
            break;  

        default:
            invalidText       = entry;
            invalidTextOffset = 0;
            CBR (FALSE);
            break;
    }

    idxExample = (entry[0] == L'/');

    ch = towlower (entry[1]);
    switch (ch)
    {
        case L's':  m_fRecurse       = fValue;  break;
        case L'w':  m_fWideListing   = fValue;  break;
        case L'b':  m_fBareListing   = fValue;  break;
        case L'p':  m_fPerfTimer     = fValue;  break;
        case L'm':  m_fMultiThreaded = fValue;  break;

        default:
            invalidText       = entry.substr (1, 1);
            invalidTextOffset = 1;
            CBR (FALSE);
            break;
    }

    
Error:
    if (FAILED (hr))
    {
        m_lastParseResult.errors.push_back ( {
            s_krgExample[idxExample],
            wstring (entry), 
            wstring (invalidText),
            invalidTextOffset 
        } );
    }

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
    m_mapExtensionSources[key]    = EAttributeSource::Environment;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ProcessDisplayAttributeOverride
//
//  Apply color override for a display attribute (e.g., "D" for Date)
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ProcessDisplayAttributeOverride (wchar_t attrChar, WORD colorAttr, wstring_view entry)
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
        // For single-char key, invalidText is the character, offset is 0 in entry
        wstring invalidChar(1, attrChar);
        m_lastParseResult.errors.push_back({
            L"Invalid display attribute character (valid: D,T,A,-,S,R,I,H,E,F)",
            wstring(entry),
            invalidChar,
            0
        });
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ProcessFileAttributeOverride
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ProcessFileAttributeOverride (wstring_view keyView, WORD colorAttr, wstring_view entry)
{
    DWORD   dwFileAttribute = 0;
    bool    found           = false;
    wchar_t chAttr          = 0;
    

    // Expected key format: attr:<x> (e.g., attr:h, attr:r, attr:0). Case-insensitive.
    if (keyView.length() != 6 ||
        towlower (keyView[0]) != L'a' ||
        towlower (keyView[1]) != L't' ||
        towlower (keyView[2]) != L't' ||
        towlower (keyView[3]) != L'r' ||
        keyView[4] != L':')
    {
        m_lastParseResult.errors.push_back({
            L"Invalid file attribute key (expected attr:<x>)",
            wstring(entry),
            wstring(keyView),
            entry.find(keyView)
        });
        return;
    }

    chAttr = towupper (keyView[5]);

    for (const SFileAttributeKeyMap & mapping : s_krgFileAttributeKeyMap)
    {
        if (mapping.m_chKey == chAttr)
        {
            dwFileAttribute = mapping.m_dwAttribute;
            found = true;
            break;
        }
    }

    if (!found)
    {
        // The invalid character is at position 5 of the key (attr:X)
        wstring invalidChar(1, keyView[5]);
        size_t  keyPos = entry.find(keyView);
        size_t  charOffset = (keyPos != wstring_view::npos) ? keyPos + 5 : 5;
        m_lastParseResult.errors.push_back({
            L"Invalid file attribute character (expected R, H, S, A, T, E, C, P or 0)",
            wstring(entry),
            invalidChar,
            charOffset
        });
        return;
    }

    m_mapFileAttributesTextAttr[dwFileAttribute] = { colorAttr, EAttributeSource::Environment };
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

    CBREx (!keyView.empty() && !valueView.empty(), E_INVALIDARG);

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

HRESULT CConfig::ParseColorSpec (wstring_view colorSpec, WORD & colorAttr)
{
    HRESULT hr        = S_OK;
    WORD    foreColor = 0;
    WORD    backColor = 0;



    auto result = std::ranges::search (
        colorSpec,
        L" on "sv,
        [] (wchar_t a, wchar_t b) { return towlower (a) == towlower (b); }
    );

    if (result.empty())
    {
        // Only foreground specified
        wstring_view foreView = TrimWhitespace (colorSpec);
        hr = ParseColorName (foreView, false, foreColor);
        CHR (hr);
    }
    else
    {
        size_t onPos = result.begin() - colorSpec.begin();

        // Both foreground and background specified
        wstring_view foreView = TrimWhitespace (colorSpec.substr (0, onPos));
        wstring_view backView = TrimWhitespace (colorSpec.substr (onPos + 4));

        hr = ParseColorName (foreView, false, foreColor);
        CHR (hr);

        if (!backView.empty())
        {
            HRESULT hrBack = ParseColorName (backView, true, backColor);
            if (FAILED (hrBack))
            {
                // Background invalid; treat as foreground-only.
                backColor = 0;
            }
        }
    }

    colorAttr = foreColor | backColor;



Error:
    return hr;
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

HRESULT CConfig::ParseColorName (wstring_view colorName, bool isBackground, WORD & colorValue)
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

    HRESULT hr = S_OK;

    

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
            colorValue = isBackground ? mapping.backValue : mapping.foreValue;
            BAIL_OUT_IF (TRUE, S_OK);
        }
    }

    colorValue = 0;
    hr = E_INVALIDARG;



Error:
    return hr;
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



    //
    // File attribute colors override everything else (including directory color)
    // and follow a fixed precedence order.
    //

    for (const SFileAttributeKeyMap & mapping : s_krgFileAttributeKeyMap)
    {
        if ((wfd.dwFileAttributes & mapping.m_dwAttribute) == 0)
        {
            continue;
        }

        auto attrIter = m_mapFileAttributesTextAttr.find (mapping.m_dwAttribute);
        if (attrIter == m_mapFileAttributesTextAttr.end())
        {
            continue;
        }

        textAttr = attrIter->second.m_wAttr;
        BAIL_OUT_IF (TRUE, S_OK);
    }



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
  

Error:
    if ((textAttr & BC_Mask) == 0)
    {
        textAttr |= m_rgAttributes[EAttribute::Default] & BC_Mask;
    }

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
