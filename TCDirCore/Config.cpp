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
    { L".c++",            FC_LightGreen   },
    { L".cpp",            FC_LightGreen   },
    { L".cxx",            FC_LightGreen   },
    { L".h",              FC_LightGreen   },
    { L".hpp",            FC_LightGreen   },
    { L".hxx",            FC_LightGreen   },
    { L".rc",             FC_LightGreen   },
    
    { L".cs",             FC_LightGreen   },
    { L".csx",            FC_LightGreen   },
    { L".resx",           FC_LightGreen   },
    { L".xaml",           FC_LightGreen   },
    
    { L".js",             FC_LightGreen   },
    { L".mjs",            FC_LightGreen   },
    { L".cjs",            FC_LightGreen   },
    { L".jsx",            FC_LightGreen   },
    { L".ts",             FC_LightGreen   },
    { L".tsx",            FC_LightGreen   },

    { L".html",           FC_LightGreen   },
    { L".htm",            FC_LightGreen   },
    { L".xhtml",          FC_LightGreen   },
    { L".css",            FC_LightGreen   },
    { L".scss",           FC_LightGreen   },
    { L".sass",           FC_LightGreen   },
    { L".less",           FC_LightGreen   },
    { L".vue",            FC_LightGreen   },
    { L".svelte",         FC_LightGreen   },

    { L".py",             FC_LightGreen   },
    { L".pyw",            FC_LightGreen   },
    { L".ipynb",          FC_LightGreen   },

    { L".rs",             FC_LightGreen   },

    { L".jar",            FC_LightGreen   },
    { L".java",           FC_LightGreen   },
    { L".class",          FC_LightGreen   },
    { L".gradle",         FC_LightGreen   },
    
    { L".go",             FC_LightGreen   },
    { L".rb",             FC_LightGreen   },
    { L".erb",            FC_LightGreen   },
    { L".fs",             FC_LightGreen   },
    { L".fsx",            FC_LightGreen   },
    { L".fsi",            FC_LightGreen   },
    { L".lua",            FC_LightGreen   },
    { L".pl",             FC_LightGreen   },
    { L".pm",             FC_LightGreen   },
    { L".php",            FC_LightGreen   },
    { L".hs",             FC_LightGreen   },
    { L".dart",           FC_LightGreen   },
    { L".kt",             FC_LightGreen   },
    { L".kts",            FC_LightGreen   },
    { L".swift",          FC_LightGreen   },
    { L".scala",          FC_LightGreen   },
    { L".sc",             FC_LightGreen   },
    { L".sbt",            FC_LightGreen   },
    { L".clj",            FC_LightGreen   },
    { L".cljs",           FC_LightGreen   },
    { L".cljc",           FC_LightGreen   },
    { L".ex",             FC_LightGreen   },
    { L".exs",            FC_LightGreen   },
    { L".erl",            FC_LightGreen   },
    { L".groovy",         FC_LightGreen   },
    { L".jl",             FC_LightGreen   },
    { L".r",              FC_LightGreen   },
    { L".rmd",            FC_LightGreen   },
    { L".elm",            FC_LightGreen   },

    { L".xml",            FC_Brown        },
    { L".xsd",            FC_Brown        },
    { L".xsl",            FC_Brown        },
    { L".xslt",           FC_Brown        },
    { L".dtd",            FC_Brown        },
    { L".plist",          FC_Brown        },
    { L".manifest",       FC_Brown        },
    { L".json",           FC_Brown        },
    { L".toml",           FC_Brown        },
    { L".yml",            FC_Brown        },
    { L".yaml",           FC_Brown        },
    { L".ini",            FC_Brown        },
    { L".cfg",            FC_Brown        },
    { L".conf",           FC_Brown        },
    { L".config",         FC_Brown        },
    { L".properties",     FC_Brown        },
    { L".settings",       FC_Brown        },
    { L".reg",            FC_Brown        },

    { L".sql",            FC_Brown        },
    { L".sqlite",         FC_Brown        },
    { L".mdb",            FC_Brown        },
    { L".accdb",          FC_Brown        },
    { L".pgsql",          FC_Brown        },
    { L".db",             FC_Brown        },
    { L".csv",            FC_Brown        },
    { L".tsv",            FC_Brown        },

    //
    // Intermediate files
    //

    { L".obj",            FC_Green        },
    { L".lib",            FC_Green        },
    { L".res",            FC_Green        },
    { L".pch",            FC_Green        },
    { L".pdb",            FC_Green        },

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
    { L".ps1xml",         FC_LightRed     },
    { L".sh",             FC_LightRed     },
    { L".zsh",            FC_LightRed     },
    { L".fish",           FC_LightRed     },
    { L".sys",            FC_LightCyan    },
    { L".msi",            FC_LightCyan    },
    { L".msix",           FC_LightCyan    },
    { L".deb",            FC_LightCyan    },
    { L".rpm",            FC_LightCyan    },
    

    //
    // Visual Studio files
    //

    { L".sln",            FC_Magenta      },
    { L".vcproj",         FC_Magenta      },
    { L".csproj",         FC_DarkGrey     },
    { L".vcxproj",        FC_Magenta      },
    { L".csxproj",        FC_DarkGrey     },
    { L".fsproj",         FC_DarkGrey     },
    { L".user",           FC_DarkGrey     },
    { L".ncb",            FC_DarkGrey     },
    { L".suo",            FC_DarkGrey     },
    { L".code-workspace", FC_DarkGrey     },

    //
    // Document types
    //

    { L".!!!",            FC_White        },
    { L".1st",            FC_White        },
    { L".doc",            FC_White        },
    { L".docx",           FC_White        },
    { L".rtf",            FC_White        },
    { L".eml",            FC_White        },
    { L".md",             FC_White        },
    { L".markdown",       FC_White        },
    { L".rst",            FC_White        },
    { L".me",             FC_White        },
    { L".now",            FC_White        },
    { L".ppt",            FC_White        },
    { L".pptx",           FC_White        },
    { L".pdf",            FC_White        },
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
    { L".xz",             FC_Magenta      },
    { L".bz2",            FC_Magenta      },
    { L".tgz",            FC_Magenta      },
    { L".cab",            FC_Magenta      },
    { L".zst",            FC_Magenta      },

    //
    // Media types
    //

    { L".png",            FC_Cyan         },
    { L".jpg",            FC_Cyan         },
    { L".jpeg",           FC_Cyan         },
    { L".gif",            FC_Cyan         },
    { L".bmp",            FC_Cyan         },
    { L".ico",            FC_Cyan         },
    { L".svg",            FC_Cyan         },
    { L".webp",           FC_Cyan         },
    { L".mp3",            FC_Cyan         },
    { L".wav",            FC_Cyan         },
    { L".flac",           FC_Cyan         },
    { L".mp4",            FC_Cyan         },
    { L".avi",            FC_Cyan         },
    { L".mkv",            FC_Cyan         },
    { L".mov",            FC_Cyan         },

    //
    // Fonts
    //

    { L".ttf",            FC_DarkGrey     },
    { L".otf",            FC_DarkGrey     },
    { L".woff",           FC_DarkGrey     },
    { L".woff2",          FC_DarkGrey     },

    //
    // Security / Certificates
    //

    { L".cer",            FC_Yellow       },
    { L".crt",            FC_Yellow       },
    { L".pem",            FC_Yellow       },
    { L".key",            FC_Yellow       },
    { L".pfx",            FC_Yellow       },

    //
    // Docker / Terraform / Lock
    //

    { L".dockerfile",     FC_LightGreen   },
    { L".dockerignore",   FC_DarkGrey     },
    { L".tf",             FC_LightGreen   },
    { L".tfvars",         FC_LightGreen   },
    { L".bicep",          FC_LightGreen   },
    { L".lock",           FC_DarkGrey     },
};





constexpr CConfig::SSwitchMapping CConfig::s_switchMappings[] =
{
    { L"s"sv,       true,  &CConfig::m_fRecurse       },
    { L"s-"sv,      false, &CConfig::m_fRecurse       },
    { L"w"sv,       true,  &CConfig::m_fWideListing   },
    { L"w-"sv,      false, &CConfig::m_fWideListing   },
    { L"b"sv,       true,  &CConfig::m_fBareListing   },
    { L"b-"sv,      false, &CConfig::m_fBareListing   },
    { L"p"sv,       true,  &CConfig::m_fPerfTimer     },
    { L"p-"sv,      false, &CConfig::m_fPerfTimer     },
    { L"m"sv,       true,  &CConfig::m_fMultiThreaded },
    { L"m-"sv,      false, &CConfig::m_fMultiThreaded },
    { L"owner"sv,   true,  &CConfig::m_fShowOwner     },
    { L"streams"sv, true,  &CConfig::m_fShowStreams   },
    { L"icons"sv,   true,  &CConfig::m_fIcons         },
    { L"icons-"sv,  false, &CConfig::m_fIcons         },
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
    InitializeFileAttributeToTextAttrMap();
  
    PopulateIconMap (g_rgDefaultExtensionIcons,    g_cDefaultExtensionIcons,    m_mapExtensionToIcon,    m_mapExtensionIconSources);
    PopulateIconMap (g_rgDefaultWellKnownDirIcons, g_cDefaultWellKnownDirIcons, m_mapWellKnownDirToIcon, m_mapWellKnownDirIconSources);
  
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
        m_mapExtensionSources[strExtension]    = EAttributeSource::Default;
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
//  CConfig::PopulateIconMap
//
//  Shared helper that seeds an icon map and its companion source-tracking
//  map from a default SIconMappingEntry table.
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::PopulateIconMap (
    const SIconMappingEntry                  * prgEntries,
    size_t                                     cEntries,
    unordered_map<wstring, char32_t>         & mapIcons,
    unordered_map<wstring, EAttributeSource> & mapSources)
{
    for (size_t i = 0; i < cEntries; i++)
    {
        const SIconMappingEntry & entry = prgEntries[i];
        wstring                   key     (entry.m_pszKey);



        std::ranges::transform (key, key.begin(), towlower);

        ASSERT (!mapIcons.contains (key));

        mapIcons[key]    = entry.m_codePoint;
        mapSources[key]  = EAttributeSource::Default;
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
    HRESULT        hr = S_OK;
    wstring_view   keyView;
    wstring_view   valueView;
    SOverrideValue ov;



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

    hr = ParseOverrideValue (entry, valueView, ov);
    CHR (hr);

    ApplyOverrideByKeyType (entry, keyView, ov);


Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ParseOverrideValue
//
//  Parse the value portion of a color/icon override entry.  The value may
//  contain a color spec, a comma-separated icon spec, or both:
//    "Yellow"              → color only
//    "Yellow,U+E0A0"      → color + icon
//    ",U+E0A0"            → icon only
//    "Yellow,"            → color + suppressed icon
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConfig::ParseOverrideValue (
    wstring_view     entry,
    wstring_view     valueView,
    SOverrideValue & ov)
{
    HRESULT      hr        = S_OK;
    wstring_view colorView;
    wstring_view iconView;
    bool         fHasComma = false;



    //
    // Split on comma: [color][,icon]
    // No comma means the entire value is a color (backward compatible).
    //

    size_t commaPos = valueView.find (L',');

    if (commaPos != wstring_view::npos)
    {
        colorView = TrimWhitespace (valueView.substr (0, commaPos));
        iconView  = valueView.substr (commaPos + 1);
        fHasComma = true;
    }
    else
    {
        colorView = valueView;
    }

    //
    // Parse color part (if non-empty)
    //

    if (!colorView.empty())
    {
        hr = ParseColorValue (entry, colorView, ov.m_colorAttr);
        CHR (hr);
        ov.m_fHasColor = true;
    }

    //
    // Parse icon part (only when comma was present)
    //

    if (fHasComma)
    {
        hr = ParseIconValue (iconView, ov.m_iconCP, ov.m_fSuppressed);
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

        ov.m_fHasIcon = true;
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ApplyOverrideByKeyType
//
//  Route the parsed color/icon override to the appropriate handler based
//  on the key type:
//    .ext     → file extension override
//    dir:name → well-known directory override
//    attr:x   → file attribute override
//    D, T, …  → display attribute override (single character)
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ApplyOverrideByKeyType (
    wstring_view           entry,
    wstring_view           keyView,
    const SOverrideValue & ov)
{
    bool         fIsDir = (keyView.length() > 4 && _wcsnicmp (keyView.data(), L"dir:", 4) == 0);
    wstring_view name   = fIsDir ? keyView.substr (4) : keyView;



    if (fIsDir || keyView[0] == L'.')
    {
        //
        // Extension or well-known directory override
        //

        if (ov.m_fHasColor)
        {
            ProcessFileExtensionOverride (name, ov.m_colorAttr);
        }

        if (ov.m_fHasIcon)
        {
            ApplyIconOverride (name, ov.m_iconCP, ov.m_fSuppressed,
                               fIsDir ? m_mapWellKnownDirToIcon      : m_mapExtensionToIcon,
                               fIsDir ? m_mapWellKnownDirIconSources : m_mapExtensionIconSources);
        }
    }
    else if (keyView.length() == 6 && _wcsnicmp (keyView.data(), L"attr:", 5) == 0)
    {
        ProcessFileAttributeOverride (keyView, entry, ov);
    }
    else if (keyView.length() == 1)
    {
        if (ov.m_fHasColor)
        {
            ProcessDisplayAttributeOverride (keyView[0], ov.m_colorAttr, entry);
        }
    }
    else
    {
        m_lastParseResult.errors.push_back ( { L"Invalid key (expected single character, .extension, dir:name, or attr:x)",
                                               wstring (entry), wstring (keyView), entry.find (keyView) } );
    }
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
//    - Single BMP character (literal glyph, U+0000-U+FFFF — fits in one
//      wchar_t, no surrogate pair needed)
//    - Surrogate pair (two wchar_t values encoding U+10000-U+10FFFF)
//    - Empty string (suppressed — user typed trailing comma)
//  Returns S_OK on success, E_FAIL on invalid input.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConfig::ParseIconValue (wstring_view iconSpec, char32_t & codePoint, bool & fSuppressed)
{
    HRESULT  hr            = S_OK;
    bool     fIsSuppressed = false;
    char32_t cpResult      = 0;



    iconSpec = TrimWhitespace (iconSpec);

    switch (iconSpec.length())
    {
        case 0:
        {
            //
            // Empty icon spec means suppressed (user typed e.g., ".obj=Green," or ".obj=,")
            //

            fIsSuppressed = true;
            break;
        }

        case 1:
        {
            //
            // Single BMP character — must not be a lone surrogate.
            // U+D800-U+DFFF is the surrogate range, reserved by Unicode
            // for UTF-16 encoding pairs.  A lone surrogate is malformed.
            //

            wchar_t ch = iconSpec[0];
            CBR (ch < 0xD800 || ch > 0xDFFF);

            cpResult = static_cast<char32_t>(ch);
            break;
        }

        case 2:
        {
            //
            // Surrogate pair — two wchar_t values that together encode a
            // supplementary character (U+10000-U+10FFFF).  The first must
            // be a high surrogate (U+D800-U+DBFF) and the second a low
            // surrogate (U+DC00-U+DFFF).
            //

            wchar_t high = iconSpec[0];
            wchar_t low  = iconSpec[1];

            CBR (high >= 0xD800 && high <= 0xDBFF);
            CBR (low  >= 0xDC00 && low  <= 0xDFFF);

            cpResult = static_cast<char32_t>(((high - 0xD800) << 10) + (low - 0xDC00) + 0x10000);
            break;
        }

        default:
        {
            //
            // U+XXXX hex notation (length >= 6)
            //

            CBR (iconSpec.length() >= 6);
            CBR ((iconSpec[0] == L'U' || iconSpec[0] == L'u') && iconSpec[1] == L'+');

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
            break;
        }
    }

    codePoint   = cpResult;
    fSuppressed = fIsSuppressed;

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ProcessSwitchOverride
//
//  Look up entry in the s_switchMappings table (case-insensitive).
//  On match, set the corresponding bool member.
//  On miss, record a parse error.
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ProcessSwitchOverride (wstring_view entry)
{
    for (const SSwitchMapping & mapping : s_switchMappings)
    {
        if (entry.length() == mapping.m_name.length() &&
            _wcsnicmp (entry.data(), mapping.m_name.data(), mapping.m_name.length()) == 0)
        {
            this->*mapping.m_pMember = mapping.m_state;
            return;
        }
    }

    m_lastParseResult.errors.push_back ({
        L"Invalid switch",
        wstring (entry),
        wstring (entry),
        0
    });
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::IsSwitchName
//
//  Check if entry matches any name in the s_switchMappings table
//  (case-insensitive).
//
////////////////////////////////////////////////////////////////////////////////

bool CConfig::IsSwitchName (wstring_view entry)
{
    for (const SSwitchMapping & mapping : s_switchMappings)
    {
        if (entry.length() == mapping.m_name.length() &&
            _wcsnicmp (entry.data(), mapping.m_name.data(), mapping.m_name.length()) == 0)
        {
            return true;
        }
    }

    return false;
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
//  CConfig::ApplyIconOverride
//
//  Apply an icon override (extension or well-known directory) to the
//  specified icon/source maps.  First-write-wins: subsequent duplicates
//  for the same key are flagged in m_lastParseResult.
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ApplyIconOverride (
    wstring_view                               name,
    char32_t                                   iconCodePoint,
    bool                                       fSuppressed,
    unordered_map<wstring, char32_t>         & mapIcons,
    unordered_map<wstring, EAttributeSource> & mapSources)
{
    wstring key (name);

    std::ranges::transform (key, key.begin(), towlower);

    if (mapSources.count (key) && mapSources[key] == EAttributeSource::Environment)
    {
        m_lastParseResult.errors.push_back ({
            L"Duplicate icon override (first-write-wins)",
            wstring (name),
            wstring (name),
            0
        });
        return;
    }

    mapIcons[key]   = fSuppressed ? U'\0' : iconCodePoint;
    mapSources[key] = EAttributeSource::Environment;
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

void CConfig::ProcessFileAttributeOverride (wstring_view keyView, wstring_view entry, const SOverrideValue & ov)
{
    DWORD   dwFileAttribute = 0;
    bool    found           = false;
    wchar_t chAttr          = 0;



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

    if (ov.m_fHasColor)
    {
        m_mapFileAttributesTextAttr[dwFileAttribute] = { ov.m_colorAttr, EAttributeSource::Environment };
    }

    if (ov.m_fHasIcon)
    {
        ProcessFileAttributeIconOverride (dwFileAttribute, ov.m_iconCP);
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
        // Both foreground and background specified
        size_t       onPos    = result.begin() - colorSpec.begin();
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
//  CConfig::ResolveFileAttributeStyle
//
//  Highest priority.  Walk the attribute precedence order in reverse
//  (lowest priority first) so the highest-priority attribute overwrites.
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ResolveFileAttributeStyle (const WIN32_FIND_DATA & wfd, SFileDisplayStyle & style)
{
    for (size_t i = g_cAttributePrecedenceOrder; i-- > 0; )
    {
        const SFileAttributeMap & mapping = g_rgAttributePrecedenceOrder[i];



        if ((wfd.dwFileAttributes & mapping.m_dwAttribute) == 0)
        {
            continue;
        }

        auto colorIter = m_mapFileAttributesTextAttr.find (mapping.m_dwAttribute);
        if (colorIter != m_mapFileAttributesTextAttr.end())
        {
            style.m_wTextAttr = colorIter->second.m_wAttr;
        }

        auto iconIter = m_mapFileAttributeToIcon.find (mapping.m_dwAttribute);
        if (iconIter != m_mapFileAttributeToIcon.end())
        {
            style.m_iconCodePoint   = iconIter->second;
            style.m_fIconSuppressed = (style.m_iconCodePoint == 0);
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ResolveDirectoryStyle
//
//  Directory color and icon from well-known names or reparse-point type.
//  Called before ResolveFileAttributeStyle so attributes can overwrite.
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ResolveDirectoryStyle (const WIN32_FIND_DATA & wfd, SFileDisplayStyle & style)
{
    filesystem::path filename      (wfd.cFileName);
    wstring          strFilename = filename.filename().wstring();



    style.m_wTextAttr = m_rgAttributes[EAttribute::Directory];

    //
    // Check well-known directory names
    //

    std::ranges::transform (strFilename, strFilename.begin(), towlower);

    auto dirIter = m_mapWellKnownDirToIcon.find (strFilename);
    if (dirIter != m_mapWellKnownDirToIcon.end())
    {
        style.m_iconCodePoint   = dirIter->second;
        style.m_fIconSuppressed = (style.m_iconCodePoint == 0);
        return;
    }

    //
    // Reparse points get special icons
    //

    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
    {
        switch (wfd.dwReserved0)
        {
            case IO_REPARSE_TAG_SYMLINK:     style.m_iconCodePoint = m_iconSymlink;          break;
            case IO_REPARSE_TAG_MOUNT_POINT: style.m_iconCodePoint = m_iconJunction;         break;
            default:                         style.m_iconCodePoint = m_iconDirectoryDefault; break;
        }
    }
    else
    {
        style.m_iconCodePoint = m_iconDirectoryDefault;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ResolveExtensionStyle
//
//  Extension-based color and icon lookup (files only).
//  Called before ResolveFileAttributeStyle so attributes can overwrite.
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ResolveExtensionStyle (const WIN32_FIND_DATA & wfd, SFileDisplayStyle & style)
{
    filesystem::path filename (wfd.cFileName);
    wstring          strExtension = filename.extension().wstring();



    std::ranges::transform (strExtension, strExtension.begin(), towlower);

    auto colorIter = m_mapExtensionToTextAttr.find (strExtension);
    if (colorIter != m_mapExtensionToTextAttr.end())
    {
        style.m_wTextAttr = colorIter->second;
    }

    auto iconIter = m_mapExtensionToIcon.find (strExtension);
    if (iconIter != m_mapExtensionToIcon.end())
    {
        style.m_iconCodePoint   = iconIter->second;
        style.m_fIconSuppressed = (style.m_iconCodePoint == 0);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::ResolveFileFallbackIcon
//
//  Lowest priority.  Assigns the default file icon.
//  File symlinks get the symlink icon instead.
//
////////////////////////////////////////////////////////////////////////////////

void CConfig::ResolveFileFallbackIcon (const WIN32_FIND_DATA & wfd, SFileDisplayStyle & style)
{
    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT &&
        wfd.dwReserved0 == IO_REPARSE_TAG_SYMLINK)
    {
        style.m_iconCodePoint = m_iconSymlink;
    }
    else
    {
        style.m_iconCodePoint = m_iconFileDefault;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfig::GetDisplayStyleForFile
//
//  Unified precedence resolver.  Levels are called lowest-priority first
//  so that higher-priority levels overwrite.
//
//                (lower priority)   <      ...       < (higher priority)    
//  
//  Directories:  fallback dir icon  < well-known dir < attributes
//  Files:        fallback file icon < extension      < attributes
//
////////////////////////////////////////////////////////////////////////////////

CConfig::SFileDisplayStyle CConfig::GetDisplayStyleForFile (const WIN32_FIND_DATA & wfd)
{
    SFileDisplayStyle style = { m_rgAttributes[EAttribute::Default], 0, false };



    if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        ResolveDirectoryStyle     (wfd, style);
    }
    else
    {
        ResolveFileFallbackIcon   (wfd, style);
        ResolveExtensionStyle     (wfd, style);
    }

    ResolveFileAttributeStyle (wfd, style);

    if ((style.m_wTextAttr & BC_Mask) == 0)
    {
        style.m_wTextAttr |= m_rgAttributes[EAttribute::Default] & BC_Mask;
    }

    style.m_wTextAttr = EnsureVisibleColorAttr (style.m_wTextAttr, m_rgAttributes[EAttribute::Default]);

    return style;
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
