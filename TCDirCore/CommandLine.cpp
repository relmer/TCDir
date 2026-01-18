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
    if (config.m_fWideListing.has_value())   m_fWideListing   = config.m_fWideListing.value();
    if (config.m_fBareListing.has_value())   m_fBareListing   = config.m_fBareListing.value();
    if (config.m_fRecurse.has_value())       m_fRecurse       = config.m_fRecurse.value();
    if (config.m_fPerfTimer.has_value())     m_fPerfTimer     = config.m_fPerfTimer.value();
    if (config.m_fMultiThreaded.has_value()) m_fMultiThreaded = config.m_fMultiThreaded.value();
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
            //
            
            case L'-':
            case L'/':
                hr = HandleSwitch (pszArg + 1);
                CHR (hr);
                
                break;


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

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CCommandLine::HandleSwitch
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CCommandLine::HandleSwitch (LPCWSTR pszArg)
{
    struct SwitchEntry
    {
        WCHAR              m_chSwitch; 
        bool             * m_pfValueOfSwitch; 
        SwitchParserFunc   m_pHandler; 
    };                             

    SwitchEntry allSwitches[] =
    {
        {  L's',   &m_fRecurse,        NULL                             },
        {  L'o',   NULL,               &CCommandLine::OrderByHandler    },
        {  L'a',   NULL,               &CCommandLine::AttributeHandler  },
        {  L'w',   &m_fWideListing,    NULL                             },
        {  L'b',   &m_fBareListing,    NULL                             },
        {  L'p',   &m_fPerfTimer,      NULL                             },
        {  L'm',   &m_fMultiThreaded,  NULL                             },
        {  L'?',   &m_fHelp,           NULL                             },
    };
    
    HRESULT hr       = S_OK;
    WCHAR   ch       = L'\0';
    bool    fDisable = false;



    if (_wcsicmp (pszArg, L"env") == 0)
    {
        m_fEnv = true;
        return S_OK;
    }

    if (_wcsicmp (pszArg, L"config") == 0)
    {
        m_fConfig = true;
        return S_OK;
    }



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

    for (SwitchEntry entry : allSwitches)
    {
        if (entry.m_chSwitch == ch)
        {
            if (entry.m_pfValueOfSwitch != NULL)
            {
                *(entry.m_pfValueOfSwitch) = !fDisable;
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
    for (SSortOrderMap entry : s_krgSortOrderMap)
    {
        if (ch == entry.ch)
        {
            m_sortorder           = entry.sortorder;
            m_rgSortPreference[0] = entry.sortorder;

            hr = S_OK;
            break;
        }
    }
    
    return hr;

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
    static constexpr WCHAR   s_kszAttributes[] = L"dhsratecp0";
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
