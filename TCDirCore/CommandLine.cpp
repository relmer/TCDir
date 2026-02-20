#include "pch.h"
#include "CommandLine.h"

#include "Config.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CCommandLine::CCommandLine
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CCommandLine::CCommandLine (void)
{          
    //
    // Define the ranking of sort attributes.  If the requested
    // sort attribute compares identical, we'll move to the next 
    // one in this table and try again.
    //

    // Placeholder -- we'll overwrite the first element with the specified sort attribute
    m_rgSortPreference[0] = CCommandLine::ESortOrder::SO_DEFAULT;

    m_rgSortPreference[1] = CCommandLine::ESortOrder::SO_NAME;
    m_rgSortPreference[2] = CCommandLine::ESortOrder::SO_DATE;
    m_rgSortPreference[3] = CCommandLine::ESortOrder::SO_EXTENSION;
    m_rgSortPreference[4] = CCommandLine::ESortOrder::SO_SIZE;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCommandLine::~CCommandLine
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CCommandLine::~CCommandLine (void)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCommandLine::ApplyConfigDefaults
//
//  Apply switch defaults from the CConfig (parsed from TCDIR environment var).
//  These can be overridden by explicit command-line arguments.
//
////////////////////////////////////////////////////////////////////////////////

void CCommandLine::ApplyConfigDefaults (const CConfig & config)
{
    if (config.m_fWideListing.has_value())                     m_fWideListing   = config.m_fWideListing.value();
    if (config.m_fBareListing.has_value())                     m_fBareListing   = config.m_fBareListing.value();
    if (config.m_fRecurse.has_value())                         m_fRecurse       = config.m_fRecurse.value();
    if (config.m_fPerfTimer.has_value())                       m_fPerfTimer     = config.m_fPerfTimer.value();
    if (config.m_fMultiThreaded.has_value())                   m_fMultiThreaded = config.m_fMultiThreaded.value();
    if (config.m_fShowOwner.has_value())                       m_fShowOwner     = config.m_fShowOwner.value();
    if (config.m_fShowStreams.has_value())                     m_fShowStreams   = config.m_fShowStreams.value();
    if (config.m_fIcons.has_value() && !m_fIcons.has_value())  m_fIcons         = config.m_fIcons.value();
    if (config.m_fTree.has_value())                            m_fTree          = config.m_fTree.value();

    //
    // Only apply tree-specific config when tree mode is active.
    // "TCDIR=Depth=2" without "Tree" is silently ignored.
    //

    if (m_fTree)
    {
        if (config.m_cMaxDepth.has_value())                    m_cMaxDepth      = config.m_cMaxDepth.value();
        if (config.m_cTreeIndent.has_value())                  m_cTreeIndent    = config.m_cTreeIndent.value();
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCommandLine::Parse
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CCommandLine::Parse (int cArg, WCHAR ** ppszArg)
{
    HRESULT hr = S_OK; 



    assert (ppszArg != NULL);

    

    //
    // See if there are any args to parse
    //

    CBREx (cArg, S_OK);

    //
    // Iterate over the arguments
    //
    
    while (cArg > 0)
    {
        LPCWSTR pszArg = *ppszArg;

        switch (*pszArg)
        {
            //
            // Switches start with a '-' or a '/'
            // Long switches use '--' prefix (e.g., --env, --config)
            //
            
            case L'-':
            case L'/':
            {
                m_chSwitchPrefix = *pszArg;

                LPCWSTR pszSwitchArg  = pszArg + 1;
                bool    fLongOption   = false;

                // Check for '--' prefix (long option style)
                if (*pszArg == L'-' && *(pszArg + 1) == L'-')
                {
                    pszSwitchArg = pszArg + 2;
                    fLongOption  = true;
                }

                //
                // Detect long switch: 3+ chars without ':' or '-' at position 1
                // Reject single-dash long switches (e.g., -env) - must use --env
                //

                size_t cchArg = wcslen (pszSwitchArg);
                bool   fLooksLikeLongSwitch = (cchArg >= 3 && pszSwitchArg[1] != L':' && pszSwitchArg[1] != L'-');

                if (fLooksLikeLongSwitch && !fLongOption && m_chSwitchPrefix == L'-')
                {
                    hr = E_INVALIDARG;
                }
                else
                {
                    hr = HandleSwitch (pszSwitchArg, cArg, ppszArg);
                }

                CHR (hr);
                
                break;
            }


            //
            // If it's not a switch, it must be a file mask
            //

            default:                
                m_listMask.emplace_back (pszArg);
                break;
        }

        --cArg;
        ++ppszArg;       
    }

    //
    // Post-parse validation: check switch conflicts
    //

    if (m_fTree)
    {
        CBREx (!(m_fWideListing), E_INVALIDARG);
        CBREx (!(m_fBareListing), E_INVALIDARG);
        CBREx (!(m_fRecurse),     E_INVALIDARG);
    }

    CBREx (m_cMaxDepth == 0   || m_fTree, E_INVALIDARG);
    CBREx (m_cTreeIndent == 4 || m_fTree, E_INVALIDARG);
    CBREx (m_cTreeIndent >= 1 && m_cTreeIndent <= 8, E_INVALIDARG);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCommandLine::HandleSwitch
//
//  Handles both single-char switches (s, w, b, etc.) and long switches (env, config).
//  Long switch validation (requiring -- prefix) is done in Parse before calling this.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CCommandLine::HandleSwitch (LPCWSTR pszArg, int & cArg, WCHAR ** & ppszArg)
{
    struct SwitchEntry
    {
        WCHAR                 m_chSwitch; 
        bool CCommandLine:: * m_pfValueOfSwitch; 
        SwitchParserFunc      m_pHandler; 
    };                             

    static const SwitchEntry s_krgSingleCharSwitches[] =
    {
        {  L's',   &CCommandLine::m_fRecurse,        nullptr                          },
        {  L'o',   nullptr,                          &CCommandLine::OrderByHandler    },
        {  L'a',   nullptr,                          &CCommandLine::AttributeHandler  },
        {  L't',   nullptr,                          &CCommandLine::TimeFieldHandler  },
        {  L'w',   &CCommandLine::m_fWideListing,    nullptr                          },
        {  L'b',   &CCommandLine::m_fBareListing,    nullptr                          },
        {  L'p',   &CCommandLine::m_fPerfTimer,      nullptr                          },
        {  L'm',   &CCommandLine::m_fMultiThreaded,  nullptr                          },
        {  L'?',   &CCommandLine::m_fHelp,           nullptr                          },
    };
    
    HRESULT hr       = S_OK;
    WCHAR   ch       = L'\0';
    bool    fDisable = false;



    //
    // Long switches are 3+ chars without ':' or '-' after first char (e.g., "env", "config")
    // Short switches are single char, optionally followed by ':' or '-' (e.g., "s", "m-", "a:hs", "o-s")
    //

    if (wcslen (pszArg) >= 3 && pszArg[1] != L':' && pszArg[1] != L'-')
    {
        return HandleLongSwitch (pszArg, cArg, ppszArg);
    }

    //
    // Single-character switches
    //

    ch = (WCHAR) towlower (*pszArg);

    //
    // Check if the next character is '-' to disable the flag
    //

    if (*(pszArg + 1) == L'-')
    {
        fDisable = true;
    }

    // Default to E_INVALIDARG for unrecognized switches
    hr = E_INVALIDARG;

    for (const SwitchEntry & entry : s_krgSingleCharSwitches)
    {
        if (entry.m_chSwitch == ch)
        {
            if (entry.m_pfValueOfSwitch != nullptr)
            {
                this->*(entry.m_pfValueOfSwitch) = !fDisable;
                hr = S_OK;
            }
            else
            {
                hr = (this->*(entry.m_pHandler))(pszArg + 1);
            }

            break;
        }
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCommandLine::HandleLongSwitch
//
//  Handle multi-character switches (env, config, debug).
//  Validation of -- prefix requirement is done in Parse before calling this.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CCommandLine::HandleLongSwitch (LPCWSTR pszArg, int & cArg, WCHAR ** & ppszArg)
{
    struct LongSwitchEntry
    {
        LPCWSTR               m_pszSwitch;
        bool CCommandLine:: * m_pfValue;
    };

    static const LongSwitchEntry s_krgLongSwitches[] =
    {
        {  L"env",     &CCommandLine::m_fEnv         },
        {  L"config",  &CCommandLine::m_fConfig      },
        {  L"owner",   &CCommandLine::m_fShowOwner   },
        {  L"streams", &CCommandLine::m_fShowStreams },
#ifdef _DEBUG
        {  L"debug",   &CCommandLine::m_fDebug       },
#endif
    };

    HRESULT hr = E_INVALIDARG;

    //
    //  Check for trailing '-' suffix which indicates negation (e.g., /Icons-)
    //

    size_t  cch       = wcslen (pszArg);
    bool    fNegated  = (cch > 1) && (pszArg[cch - 1] == L'-');
    wstring strSwitch = fNegated ? wstring (pszArg, cch - 1) : wstring (pszArg);

    //
    //  Check the bool table first (negation not supported for these)
    //

    if (!fNegated)
    {
        for (const LongSwitchEntry & entry : s_krgLongSwitches)
        {
            if (_wcsicmp (strSwitch.c_str (), entry.m_pszSwitch) == 0)
            {
                this->*(entry.m_pfValue) = true;
                hr = S_OK;
                break;
            }
        }
    }

    //
    //  Icons switch — supports negation via /Icons- (sets optional<bool>)
    //

    if (hr != S_OK)
    {
        if (_wcsicmp (strSwitch.c_str (), L"icons") == 0)
        {
            m_fIcons = !fNegated;
            hr = S_OK;
        }
    }

    //
    //  Tree switch — supports negation via --Tree-
    //  Negation resets Depth and TreeIndent to defaults (protects env var override)
    //

    if (hr != S_OK)
    {
        if (_wcsicmp (strSwitch.c_str (), L"tree") == 0)
        {
            m_fTree = !fNegated;

            if (fNegated)
            {
                m_cMaxDepth   = 0;
                m_cTreeIndent = 4;
            }

            hr = S_OK;
        }
    }

    //
    //  Parameterized switches: --Depth=N, --TreeIndent=N
    //  Support both '=' separator and space separator
    //

    if (hr != S_OK)
    {
        size_t  eqPos       = strSwitch.find (L'=');
        wstring switchName  = (eqPos != wstring::npos) ? strSwitch.substr (0, eqPos) : strSwitch;
        wstring switchValue;
        bool    fHasValue   = false;

        if (eqPos != wstring::npos)
        {
            switchValue = strSwitch.substr (eqPos + 1);
            fHasValue   = true;
        }
        else if (cArg > 1)
        {
            //
            // Space separator: consume next arg if it looks numeric
            //

            LPCWSTR pszNext = *(ppszArg + 1);

            if (iswdigit (pszNext[0]) || (pszNext[0] == L'-' && iswdigit (pszNext[1])))
            {
                switchValue = pszNext;
                fHasValue   = true;
                --cArg;
                ++ppszArg;
            }
        }

        if (_wcsicmp (switchName.c_str (), L"depth") == 0)
        {
            CBREx (fHasValue, E_INVALIDARG);

            int n = _wtoi (switchValue.c_str ());

            CBREx (n > 0, E_INVALIDARG);

            m_cMaxDepth = n;
            hr = S_OK;
        }
        else if (_wcsicmp (switchName.c_str (), L"treeindent") == 0)
        {
            CBREx (fHasValue, E_INVALIDARG);

            int n = _wtoi (switchValue.c_str ());

            CBREx (n >= 1 && n <= 8, E_INVALIDARG);

            m_cTreeIndent = n;
            hr = S_OK;
        }
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCommandLine::OrderByHandler
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CCommandLine::OrderByHandler (LPCWSTR pszArg)
{
    struct SSortOrderMap
    {
        WCHAR      ch;
        ESortOrder sortorder;
    };

    static constexpr SSortOrderMap s_krgSortOrderMap[] =
    {
        { L'n', ESortOrder::SO_NAME      },
        { L'e', ESortOrder::SO_EXTENSION },
        { L's', ESortOrder::SO_SIZE      },
        { L'd', ESortOrder::SO_DATE      }
    };



    HRESULT hr = S_OK;               
    WCHAR   ch = 0; 



    //
    // Make sure the arg isn't empty
    //

    CBRAEx (pszArg != NULL, E_INVALIDARG);

    //
    // Support optional ':' prefix (DIR-style switch syntax: /o:d)
    //

    if (*pszArg == L':')
    {
        ++pszArg;
    }

    CBRAEx (*pszArg != L'\0', E_INVALIDARG);

    //
    // Check to see if we're reversing the sort order
    //

    if (*pszArg == L'-')
    {
        m_sortdirection = ESortDirection::SD_DESCENDING;

        //
        // Move to the next character and make sure it's not empty
        //

        ++pszArg;

        CBRAEx (*pszArg != L'\0', E_INVALIDARG);
    }

    //
    // Find the sort order character in the map array
    //

    hr = E_INVALIDARG; // If there's no match, we'll return this

    ch = towlower (*pszArg);
    for (const SSortOrderMap & entry : s_krgSortOrderMap)
    {
        if (ch == entry.ch)
        {
            m_sortorder           = entry.sortorder;
            m_rgSortPreference[0] = entry.sortorder;

            hr = S_OK;
            break;
        }
    }
    


Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCommandLine::AttributeHandler
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CCommandLine::AttributeHandler (LPCWSTR pszArg)
{
    HRESULT                  hr                = S_OK;      
    DWORD                  * pdwMask; 
    const WCHAR            * pchAttribute;       
    int                      idxAttribute;       
    static constexpr WCHAR   s_kszAttributes[] = L"dhsratecp0xoiblv";
    static constexpr DWORD   s_kdwAttributes[] =
    { 
        FILE_ATTRIBUTE_DIRECTORY, 
        FILE_ATTRIBUTE_HIDDEN, 
        FILE_ATTRIBUTE_SYSTEM,
        FILE_ATTRIBUTE_READONLY,
        FILE_ATTRIBUTE_ARCHIVE,
        FILE_ATTRIBUTE_TEMPORARY,
        FILE_ATTRIBUTE_ENCRYPTED,
        FILE_ATTRIBUTE_COMPRESSED,
        FILE_ATTRIBUTE_REPARSE_POINT,
        FILE_ATTRIBUTE_SPARSE_FILE,
        FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,
        FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_RECALL_ON_OPEN | FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS,  // 'o' - cloud-only (composite)
        FILE_ATTRIBUTE_INTEGRITY_STREAM,
        FILE_ATTRIBUTE_NO_SCRUB_DATA,
        FILE_ATTRIBUTE_UNPINNED,      // 'l' - locally available
        FILE_ATTRIBUTE_PINNED,        // 'v' - always locally available
    };



    //
    // Make sure the arg isn't empty
    //

    CBRAEx (pszArg != NULL, E_INVALIDARG);
    CBRAEx (*pszArg != L'\0', E_INVALIDARG);

    //
    // By default, any attributes specified will be required to be present
    //

    pdwMask = &m_dwAttributesRequired;

    //
    // Iterate over the arg characters
    //

    while (*pszArg != L'\0')
    {
        WCHAR ch = (WCHAR) tolower (*pszArg);
        
        switch (ch)
        {
            case L'-':
            {
                //
                // If an attribute is forcibly excluded, point to
                // the appropriate mask.
                //

                CBRAEx (pdwMask == &m_dwAttributesRequired, E_INVALIDARG);
                pdwMask = &m_dwAttributesExcluded;
                break;
            }
            
            default:
            {
                pchAttribute = wcschr (s_kszAttributes, ch);
                if (pchAttribute != NULL)
                {                    
                    //
                    // Convert to an index into the map array
                    //
                
                    idxAttribute = (int) (pchAttribute - s_kszAttributes);

                
                    //
                    // Add the mapped attribute to the appropriate mask
                    //
                
                    *pdwMask |= s_kdwAttributes[idxAttribute];
                }

                //
                // Default back to the required attributes mask
                //

                pdwMask = &m_dwAttributesRequired;
            }
        }

        ++pszArg;
    }

    

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCommandLine::TimeFieldHandler
//
//  Handles /T: switch to select which time field to display and sort by
//  /T:C = Creation time
//  /T:A = Access time
//  /T:W = Last write time (default)
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CCommandLine::TimeFieldHandler (LPCWSTR pszArg)
{
    struct STimeFieldMap
    {
        WCHAR      ch;
        ETimeField timeField;
    };

    static constexpr STimeFieldMap s_krgTimeFieldMap[] =
    {
        { L'c', ETimeField::TF_CREATION },
        { L'a', ETimeField::TF_ACCESS   },
        { L'w', ETimeField::TF_WRITTEN  }
    };

    HRESULT hr = S_OK;
    WCHAR   ch = 0;



    CBRAEx (pszArg != NULL, E_INVALIDARG);

    //
    // Support optional ':' prefix (DIR-style switch syntax: /T:C)
    //

    if (*pszArg == L':')
    {
        ++pszArg;
    }

    CBRAEx (*pszArg != L'\0', E_INVALIDARG);

    //
    // Find the time field character in the map array
    //

    hr = E_INVALIDARG; // If there's no match, we'll return this

    ch = towlower (*pszArg);
    for (STimeFieldMap entry : s_krgTimeFieldMap)
    {
        if (ch == entry.ch)
        {
            m_timeField = entry.timeField;
            hr = S_OK;
            break;
        }
    }



Error:
    return hr;
}
