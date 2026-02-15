#include "pch.h"
#include "Config.h"

#include "Color.h"
#include "FileAttributeMap.h"





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
    m_rgAttributes[EAttribute::Default]                           = wDefaultAttr;
    m_rgAttributes[EAttribute::Date]                              = FC_Red;
    m_rgAttributes[EAttribute::Time]                              = FC_Brown;
    m_rgAttributes[EAttribute::FileAttributePresent]              = FC_Cyan;
    m_rgAttributes[EAttribute::FileAttributeNotPresent]           = FC_DarkGrey;
    m_rgAttributes[EAttribute::Information]                       = FC_Cyan;
    m_rgAttributes[EAttribute::InformationHighlight]              = FC_White;
    m_rgAttributes[EAttribute::Size]                              = FC_Yellow;
    m_rgAttributes[EAttribute::Directory]                         = FC_LightBlue;
    m_rgAttributes[EAttribute::SeparatorLine]                     = FC_LightBlue;
    m_rgAttributes[EAttribute::Error]                             = FC_LightRed;
    m_rgAttributes[EAttribute::Owner]                             = FC_Green;
    m_rgAttributes[EAttribute::Stream]                            = FC_DarkGrey;
    m_rgAttributes[EAttribute::CloudStatusCloudOnly]              = FC_LightBlue;
    m_rgAttributes[EAttribute::CloudStatusLocallyAvailable]       = FC_LightGreen;
    m_rgAttributes[EAttribute::CloudStatusAlwaysLocallyAvailable] = FC_LightGreen;
  
    InitializeExtensionToTextAttrMap();
    InitializeExtensionToIconMap();
    InitializeFileAttributeToTextAttrMap();
    InitializeWellKnownDirToIconMap();
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
//  CConfig::InitializeExtensionToIconMap
//
//  Seed the extension → icon map from the default icon table.
//  Parallels InitializeExtensionToTextAttrMap for colors.
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::InitializeExtensionToIconMap (void)
{
    for (size_t i = 0; i < g_cDefaultExtensionIcons; i++)
    {
        const SIconMappingEntry & entry = g_rgDefaultExtensionIcons[i];

        wstring key (entry.m_pszKey);

        std::ranges::transform (key, key.begin(), towlower);

        ASSERT (!m_mapExtensionToIcon.contains (key));

        m_mapExtensionToIcon[key]        = entry.m_codePoint;
        m_mapExtensionIconSources[key]   = EAttributeSource::Default;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::InitializeWellKnownDirToIconMap
//
//  Seed the well-known directory name → icon map from the default table.
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::InitializeWellKnownDirToIconMap (void)
{
    for (size_t i = 0; i < g_cDefaultWellKnownDirIcons; i++)
    {
        const SIconMappingEntry & entry = g_rgDefaultWellKnownDirIcons[i];

        wstring key (entry.m_pszKey);

        std::ranges::transform (key, key.begin(), towlower);

        ASSERT (!m_mapWellKnownDirToIcon.contains (key));

        m_mapWellKnownDirToIcon[key]        = entry.m_codePoint;
        m_mapWellKnownDirIconSources[key]   = EAttributeSource::Default;
    }
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
    HRESULT      hr          = S_OK;
    wstring_view keyView;
    wstring_view valueView;
    WORD         colorAttr   = 0;
    char32_t     iconCP      = 0;
    bool         fSuppressed = false;
    bool         fHasColor   = false;
    bool         fHasIcon    = false;



    // Skip empty entries (from trailing semicolons or multiple semicolons)
    entry = TrimWhitespace (entry);
    CBR (!entry.empty());

    //
    // Check if this is a switch (W, S, P, M, M-, Owner, Streams)
    // Prefixes (/, -, --) are NOT allowed in the env var - give helpful error
    //

    if (entry[0] == L'/' || entry[0] == L'-')
    {
        m_lastParseResult.errors.push_back ( { L"Switch prefixes (/, -, --) are not allowed in env var",
                                               wstring (entry), wstring (entry.substr (0, entry.starts_with (L"--") ? 2 : 1)), 0 } );
        BAIL_OUT_IF (TRUE, S_OK);
    }

    if (IsSwitchName (entry))
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

    //
    // Check for comma in value — split into [color][,icon]
    //

    {
        size_t commaPos = valueView.find (L',');

        if (commaPos != wstring_view::npos)
        {
            wstring_view colorView = TrimWhitespace (valueView.substr (0, commaPos));
            wstring_view iconView  = valueView.substr (commaPos + 1);

            //
            // Parse color part (if non-empty)
            //

            if (!colorView.empty())
            {
                hr = ParseColorValue (entry, colorView, colorAttr);
                CHR (hr);
                fHasColor = true;
            }

            //
            // Parse icon part
            //

            hr = ParseIconValue (iconView, iconCP, fSuppressed);
            if (FAILED (hr))
            {
                wstring_view trimmedIcon = TrimWhitespace (iconView);
                size_t       entryComma  = entry.find (L',');
                size_t       iconOffset  = (entryComma != wstring_view::npos) ? entryComma + 1 : 0;
                m_lastParseResult.errors.push_back ({
                    L"Invalid icon specification (expected U+XXXX, literal glyph, or empty)",
                    wstring (entry),
                    wstring (trimmedIcon),
                    iconOffset
                });
                CHR (hr);
            }
            fHasIcon = true;
        }
        else
        {
            //
            // No comma — entire value is color (backward compatible)
            //

            hr = ParseColorValue (entry, valueView, colorAttr);
            CHR (hr);
            fHasColor = true;
        }
    }
    
    //
    // Apply the override based on key type
    //

    if (keyView.length() > 4 &&
        towlower (keyView[0]) == L'd' &&
        towlower (keyView[1]) == L'i' &&
        towlower (keyView[2]) == L'r' &&
        keyView[3] == L':')
    {
        //
        // Well-known directory override (dir:name)
        //

        wstring_view dirName = keyView.substr (4);

        if (fHasColor)
        {
            ProcessFileExtensionOverride (dirName, colorAttr);
        }

        if (fHasIcon)
        {
            ProcessWellKnownDirIconOverride (dirName, iconCP, fSuppressed);
        }
    }
    else if (keyView[0] == L'.')
    {
        if (fHasColor)
        {
            ProcessFileExtensionOverride (keyView, colorAttr);
        }

        if (fHasIcon)
        {
            ProcessFileExtensionIconOverride (keyView, iconCP, fSuppressed);
        }
    }
    else if (keyView.length() == 6 &&
             towlower (keyView[0]) == L'a' &&
             towlower (keyView[1]) == L't' &&
             towlower (keyView[2]) == L't' &&
             towlower (keyView[3]) == L'r' &&
             keyView[4] == L':')
    {
        ProcessFileAttributeOverride (keyView, colorAttr, entry, fHasColor, fHasIcon, iconCP);
    }
    else if (keyView.length() == 1)
    {
        if (fHasColor)
        {
            ProcessDisplayAttributeOverride (keyView[0], colorAttr, entry);
        }
    }
    else
    {
        m_lastParseResult.errors.push_back ( { L"Invalid key (expected single character, .extension, dir:name, or attr:x)",
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
            // Reject the entire entry so we don't apply a partial/unreadable color
            CBR (FALSE);
        }

        //
        // Reject if foreground and background are the same color (unreadable)
        //

        if (foreColor == (backColor >> 4))
        {
            m_lastParseResult.errors.push_back({
                L"Foreground and background colors are the same",
                wstring(entry),
                wstring(TrimWhitespace (valueView)),
                (equalPos != wstring_view::npos) ? equalPos + 1 : 0
            });
            CBR (FALSE);
        }
    }

    colorAttr = foreColor | backColor;

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ParseIconValue
//
//  Parse an icon specification from the value part after a comma.
//  Supported formats:
//    - U+XXXX  (4-6 hex digits, range 0x0001-0x10FFFF, not surrogates)
//    - Single BMP character (literal glyph)
//    - Empty string (suppressed — user typed trailing comma)
//  Returns S_OK on success, E_FAIL on invalid input.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConfig::ParseIconValue (wstring_view iconSpec, char32_t & codePoint, bool & fSuppressed)
{
    HRESULT hr         = S_OK;
    bool    fIsSuppressed = false;
    char32_t cpResult  = 0;



    iconSpec = TrimWhitespace (iconSpec);

    //
    // Empty icon spec means suppressed (user typed e.g., ".obj=Green," or ".obj=,")
    //

    if (iconSpec.empty())
    {
        fIsSuppressed = true;
        goto Done;
    }

    //
    // U+XXXX hex notation
    //

    if (iconSpec.length() >= 6 &&
        (iconSpec[0] == L'U' || iconSpec[0] == L'u') &&
        iconSpec[1] == L'+')
    {
        wstring_view hexPart = iconSpec.substr (2);

        // Must be 4-6 hex digits
        CBR (hexPart.length() >= 4 && hexPart.length() <= 6);

        // Validate all characters are hex digits
        for (wchar_t ch : hexPart)
        {
            CBR (iswxdigit (ch));
        }

        // Parse the hex value
        unsigned long ulValue = wcstoul (wstring (hexPart).c_str(), nullptr, 16);

        // Range check: 0x0001 - 0x10FFFF, excluding surrogates D800-DFFF
        CBR (ulValue >= 0x0001 && ulValue <= 0x10FFFF);
        CBR (ulValue < 0xD800 || ulValue > 0xDFFF);

        cpResult = static_cast<char32_t>(ulValue);
        goto Done;
    }

    //
    // Literal glyph: single BMP character or a surrogate pair
    //

    if (iconSpec.length() == 1)
    {
        // Single BMP character — must not be a lone surrogate
        wchar_t ch = iconSpec[0];
        CBR (ch < 0xD800 || ch > 0xDFFF);

        cpResult = static_cast<char32_t>(ch);
        goto Done;
    }

    if (iconSpec.length() == 2)
    {
        // Possible surrogate pair
        wchar_t high = iconSpec[0];
        wchar_t low  = iconSpec[1];

        CBR (high >= 0xD800 && high <= 0xDBFF);
        CBR (low  >= 0xDC00 && low  <= 0xDFFF);

        cpResult = static_cast<char32_t>(
            ((high - 0xD800) << 10) + (low - 0xDC00) + 0x10000
        );
        goto Done;
    }

    //
    // Anything else is invalid
    //

    CBR (FALSE);

Done:
    codePoint   = cpResult;
    fSuppressed = fIsSuppressed;

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ProcessSwitchOverride
//
//  Process a switch override entry (e.g., /s, /w, /m-, -s, -w, --owner)
//  Short switches: /S (recurse), /W (wide), /P (perf timer), /M (multithreaded)
//  Long switches: --owner
//  Use trailing '-' to disable (e.g., /M-)
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ProcessSwitchOverride (wstring_view entry)
{
    static constexpr LPCWSTR s_kpszInvalidSwitch = L"Invalid switch (expected W, S, P, M, B, Owner, or Streams)";

    HRESULT      hr                = S_OK;
    wstring_view invalidText       = entry;
    size_t       invalidTextOffset = 0;
    wchar_t      ch                = 0;
    bool         fValue            = true;



    //
    // Check for long switch names first (owner, streams)
    //

    if (entry.length() >= 5)
    {
        hr = ProcessLongSwitchOverride (entry);
        BAIL_OUT_IF (SUCCEEDED (hr), S_OK);  // Success - skip short switch processing

        // Not a recognized long switch, fall through to try short switch
        hr = S_OK;
    }

    //
    // Short switch: single letter, optionally followed by '-' to disable
    //

    switch (entry.length())
    {
        case 1:
            break;

        case 2:
            CBR (entry[1] == L'-');
            fValue = false;
            break;  

        default:
            invalidText       = entry;
            invalidTextOffset = 0;
            CBR (FALSE);
            break;
    }

    ch = towlower (entry[0]);
    switch (ch)
    {
        case L's':  m_fRecurse       = fValue;  break;
        case L'w':  m_fWideListing   = fValue;  break;
        case L'b':  m_fBareListing   = fValue;  break;
        case L'p':  m_fPerfTimer     = fValue;  break;
        case L'm':  m_fMultiThreaded = fValue;  break;

        default:
            invalidText       = entry.substr (0, 1);
            invalidTextOffset = 0;
            CBR (FALSE);
            break;
    }

    
Error:
    if (FAILED (hr))
    {
        m_lastParseResult.errors.push_back ( {
            s_kpszInvalidSwitch,
            wstring (entry), 
            wstring (invalidText),
            invalidTextOffset 
        } );
    }

    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::IsSwitchName
//
//  Check if entry is a valid switch name without prefix.
//  Returns true for: W, S, P, M, B, M-, Owner, Streams (case-insensitive)
//
////////////////////////////////////////////////////////////////////////////////

bool CConfig::IsSwitchName (wstring_view entry)
{
    // Single-letter switches (optionally with '-' suffix)
    if (entry.length() == 1 || (entry.length() == 2 && entry[1] == L'-'))
    {
        wchar_t ch = towlower (entry[0]);
        return ch == L'w' || ch == L's' || ch == L'p' || ch == L'm' || ch == L'b';
    }

    // Long switch names
    if (entry.length() == 5 && _wcsnicmp (entry.data(), L"owner", 5) == 0)
    {
        return true;
    }

    if (entry.length() == 7 && _wcsnicmp (entry.data(), L"streams", 7) == 0)
    {
        return true;
    }

    if (entry.length() == 5 && _wcsnicmp (entry.data(), L"icons", 5) == 0)
    {
        return true;
    }

    if (entry.length() == 6 && _wcsnicmp (entry.data(), L"icons-", 6) == 0)
    {
        return true;
    }

    return false;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ProcessLongSwitchOverride
//
//  Process a long switch entry (e.g., "--owner" or "/owner").
//  Sets idxExample for error reporting (0 = dash prefix, 1 = slash prefix).
//  Returns S_OK if recognized, E_INVALIDARG if not.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConfig::ProcessLongSwitchOverride (wstring_view switchName)
{
    HRESULT hr = E_INVALIDARG;



    //
    // Match against known long switch names (case-insensitive)
    //

    if (switchName.length() == 5 && _wcsnicmp (switchName.data(), L"owner", 5) == 0)
    {
        m_fShowOwner = true;
        hr           = S_OK;
    }
    else if (switchName.length() == 7 && _wcsnicmp (switchName.data(), L"streams", 7) == 0)
    {
        m_fShowStreams = true;
        hr             = S_OK;
    }
    else if (switchName.length() == 5 && _wcsnicmp (switchName.data(), L"icons", 5) == 0)
    {
        m_fIcons = true;
        hr       = S_OK;
    }
    else if (switchName.length() == 6 && _wcsnicmp (switchName.data(), L"icons-", 6) == 0)
    {
        m_fIcons = false;
        hr       = S_OK;
    }

    return hr;
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
//  CConfig::ProcessFileExtensionIconOverride
//
//  Apply icon override for a file extension (e.g., ".cpp").
//  First-write-wins: subsequent duplicates are flagged in m_lastParseResult.
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ProcessFileExtensionIconOverride (wstring_view extension, char32_t iconCodePoint, bool fSuppressed)
{
    wstring key (extension);

    std::ranges::transform (key, key.begin(), towlower);

    if (m_mapExtensionIconSources.count (key) && m_mapExtensionIconSources[key] == EAttributeSource::Environment)
    {
        m_lastParseResult.errors.push_back ({
            L"Duplicate icon override (first-write-wins)",
            wstring (extension),
            wstring (extension),
            0
        });
        return;
    }

    m_mapExtensionToIcon[key]      = fSuppressed ? U'\0' : iconCodePoint;
    m_mapExtensionIconSources[key] = EAttributeSource::Environment;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ProcessWellKnownDirIconOverride
//
//  Apply icon override for a well-known directory name (e.g., ".git", "src").
//  First-write-wins: subsequent duplicates are flagged in m_lastParseResult.
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ProcessWellKnownDirIconOverride (wstring_view dirName, char32_t iconCodePoint, bool fSuppressed)
{
    wstring key (dirName);

    std::ranges::transform (key, key.begin(), towlower);

    if (m_mapWellKnownDirIconSources.count (key) && m_mapWellKnownDirIconSources[key] == EAttributeSource::Environment)
    {
        m_lastParseResult.errors.push_back ({
            L"Duplicate icon override (first-write-wins)",
            wstring (dirName),
            wstring (dirName),
            0
        });
        return;
    }

    m_mapWellKnownDirToIcon[key]        = fSuppressed ? U'\0' : iconCodePoint;
    m_mapWellKnownDirIconSources[key]   = EAttributeSource::Environment;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ProcessFileAttributeIconOverride
//
//  Apply icon override for a file attribute (e.g., hidden, system).
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ProcessFileAttributeIconOverride (DWORD dwAttribute, char32_t iconCodePoint)
{
    m_mapFileAttributeToIcon[dwAttribute] = iconCodePoint;
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
        { L'O', EAttribute::Owner                   },
        { L'M', EAttribute::Stream                  },
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
            L"Invalid display attribute character (valid: D,T,A,-,S,R,I,H,E,F,O)",
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

void CConfig::ProcessFileAttributeOverride (wstring_view keyView, WORD colorAttr, wstring_view entry, bool fHasColor, bool fHasIcon, char32_t iconCP)
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

    for (const SFileAttributeMap & mapping : k_rgFileAttributeMap)
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

    if (fHasColor)
    {
        m_mapFileAttributesTextAttr[dwFileAttribute] = { colorAttr, EAttributeSource::Environment };
    }

    if (fHasIcon)
    {
        ProcessFileAttributeIconOverride (dwFileAttribute, iconCP);
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
//  CConfig::GetCloudStatusIcon
//
//  Returns the Nerd Font glyph code point for a given cloud status.
//  The DWORD parameter maps to ECloudStatus enum values.
//  Returns 0 for CS_NONE (status 0).
//
////////////////////////////////////////////////////////////////////////////////

char32_t CConfig::GetCloudStatusIcon (DWORD dwCloudStatus)
{
    switch (dwCloudStatus)
    {
        case 1:  return m_iconCloudOnly;         // CS_CLOUD_ONLY
        case 2:  return m_iconLocallyAvailable;  // CS_LOCAL
        case 3:  return m_iconAlwaysLocal;       // CS_PINNED
        default: return 0;                       // CS_NONE or invalid
    }
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
    return GetDisplayStyleForFile (wfd).m_wTextAttr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::GetDisplayStyleForFile
//
//  Unified precedence resolver.  Walks the attribute → well-known dir →
//  extension → type-fallback levels with independent color and icon locking.
//  Color and icon are resolved in a single pass.
//
////////////////////////////////////////////////////////////////////////////////

CConfig::SFileDisplayStyle CConfig::GetDisplayStyleForFile (const WIN32_FIND_DATA & wfd)
{
    WORD             textAttr       = m_rgAttributes[EAttribute::Default];
    char32_t         iconCodePoint  = 0;
    bool             fIconSuppressed = false;
    bool             fColorLocked   = false;
    bool             fIconLocked    = false;
    filesystem::path filename         (wfd.cFileName);
    wstring          strExtension;
    wstring          strFilename;



    //
    // Level 1 — File attribute precedence (highest priority)
    // Walk the fixed precedence order (PSHERC0TA).  Each attribute can
    // independently lock color and/or icon.
    //

    for (size_t i = 0; i < g_cAttributePrecedenceOrder; i++)
    {
        const SFileAttributeMap & mapping = g_rgAttributePrecedenceOrder[i];

        if ((wfd.dwFileAttributes & mapping.m_dwAttribute) == 0)
        {
            continue;
        }

        // Lock color if this attribute has a color override
        if (!fColorLocked)
        {
            auto colorIter = m_mapFileAttributesTextAttr.find (mapping.m_dwAttribute);
            if (colorIter != m_mapFileAttributesTextAttr.end())
            {
                textAttr     = colorIter->second.m_wAttr;
                fColorLocked = true;
            }
        }

        // Lock icon if this attribute has an icon override
        if (!fIconLocked)
        {
            auto iconIter = m_mapFileAttributeToIcon.find (mapping.m_dwAttribute);
            if (iconIter != m_mapFileAttributeToIcon.end())
            {
                iconCodePoint   = iconIter->second;
                fIconSuppressed = (iconCodePoint == 0);
                fIconLocked     = true;
            }
        }

        if (fColorLocked && fIconLocked)
        {
            break;
        }
    }

    //
    // Level 2 — Directory handling (well-known dir names and type fallback)
    //

    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        if (!fColorLocked)
        {
            textAttr     = m_rgAttributes[EAttribute::Directory];
            fColorLocked = true;
        }

        if (!fIconLocked)
        {
            // Check well-known directory names
            strFilename = filename.filename().wstring();
            std::ranges::transform (strFilename, strFilename.begin(), towlower);

            auto dirIter = m_mapWellKnownDirToIcon.find (strFilename);
            if (dirIter != m_mapWellKnownDirToIcon.end())
            {
                iconCodePoint   = dirIter->second;
                fIconSuppressed = (iconCodePoint == 0);
                fIconLocked     = true;
            }
            else
            {
                // Reparse points get special icons
                if (wfd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
                {
                    if (wfd.dwReserved0 == IO_REPARSE_TAG_SYMLINK)
                    {
                        iconCodePoint = m_iconSymlink;
                    }
                    else if (wfd.dwReserved0 == IO_REPARSE_TAG_MOUNT_POINT)
                    {
                        iconCodePoint = m_iconJunction;
                    }
                    else
                    {
                        iconCodePoint = m_iconDirectoryDefault;
                    }
                }
                else
                {
                    iconCodePoint = m_iconDirectoryDefault;
                }
                fIconLocked = true;
            }
        }

        goto Done;
    }

    //
    // Level 3 — Extension-based lookup (files only)
    //

    strExtension = filename.extension().wstring();

    std::ranges::transform (strExtension, strExtension.begin(), towlower);

    if (!fColorLocked)
    {
        auto colorIter = m_mapExtensionToTextAttr.find (strExtension);
        if (colorIter != m_mapExtensionToTextAttr.end())
        {
            textAttr     = colorIter->second;
            fColorLocked = true;
        }
    }

    if (!fIconLocked)
    {
        auto iconIter = m_mapExtensionToIcon.find (strExtension);
        if (iconIter != m_mapExtensionToIcon.end())
        {
            iconCodePoint   = iconIter->second;
            fIconSuppressed = (iconCodePoint == 0);
            fIconLocked     = true;
        }
    }

    //
    // Level 4 — Type fallback (file default icon if nothing else matched)
    //

    if (!fIconLocked)
    {
        // Reparse points (file symlinks) get the symlink icon
        if (wfd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            if (wfd.dwReserved0 == IO_REPARSE_TAG_SYMLINK)
            {
                iconCodePoint = m_iconSymlink;
            }
            else
            {
                iconCodePoint = m_iconFileDefault;
            }
        }
        else
        {
            iconCodePoint = m_iconFileDefault;
        }
    }

Done:
    if ((textAttr & BC_Mask) == 0)
    {
        textAttr |= m_rgAttributes[EAttribute::Default] & BC_Mask;
    }

    textAttr = EnsureVisibleColorAttr (textAttr, m_rgAttributes[EAttribute::Default]);

    return { textAttr, iconCodePoint, fIconSuppressed };
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::EnsureVisibleColorAttr
//
//  Given a color attribute and a default attribute, returns a modified color
//  attribute that ensures visibility. If the foreground matches the background,
//  a contrasting background is applied.
//
////////////////////////////////////////////////////////////////////////////////

WORD CConfig::EnsureVisibleColorAttr (WORD colorAttr, WORD defaultAttr)
{
    WORD foreAttr        = colorAttr   & (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    WORD backAttr        = colorAttr   & (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY);
    WORD defaultBackAttr = defaultAttr & (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_INTENSITY);



    //
    // If no explicit background in colorAttr, use the default background
    //

    if (backAttr == 0)
    {
        backAttr = defaultBackAttr;
    }

    //
    // If foreground matches background, use a contrasting background so text is visible.
    // Compare by shifting foreground bits to background position.
    //

    if ((foreAttr << 4) == backAttr)
    {
        // Use opposite brightness: if dark background, use light; if light, use dark
        WORD contrastBack = (backAttr & BACKGROUND_INTENSITY) ? static_cast<WORD>(BC_Black) : static_cast<WORD>(BC_LightGrey);
        return foreAttr | contrastBack;
    }

    return foreAttr | backAttr;
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
