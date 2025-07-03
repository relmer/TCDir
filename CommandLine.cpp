#include "pch.h"

#include "CommandLine.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CCommandLine::CCommandLine
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CCommandLine::CCommandLine (void) :
    m_fRecurse              (FALSE),        
    m_dwAttributesRequired  (0),            
    m_dwAttributesExcluded  (0),            
    m_sortorder             (ESortOrder::SO_DEFAULT),   
    m_sortdirection         (ESortDirection::SD_ASCENDING),
    m_fWideListing          (FALSE)
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
                m_listMask.push_back (pszArg);
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
    struct SwitchMap
    {
        WCHAR              m_chSwitch; 
        BOOL             * m_pfToggle; 
        SwitchParserFunc   m_pHandler; 
    };                             

    SwitchMap map[] =
    {
        {  L's',   &m_fRecurse,     NULL                             },
        {  L'o',   NULL,            &CCommandLine::OrderByHandler    },
        {  L'a',   NULL,            &CCommandLine::AttributeHandler  },
        {  L'w',   &m_fWideListing, NULL                             },

        {  L'\0',  NULL,            NULL                             },  // End of map
    };      
    
    HRESULT     hr           = S_OK;
    BOOL        fToggleState = TRUE;  
    SwitchMap * pMap         = map;
	WCHAR       ch           = L'\0';



    CBRAEx ((pszArg != NULL) && (*pszArg != L'\0'), E_INVALIDARG);
    
    if (*pszArg == '-')
    {
        fToggleState = FALSE;
        ++pszArg;

        CBRAEx (*pszArg != L'\0', E_INVALIDARG);    
    }

	ch = (WCHAR) tolower (*pszArg);

    hr = E_FAIL;
    while (pMap->m_chSwitch != L'\0')
    {
        if (pMap->m_chSwitch == ch)
        {
            if (pMap->m_pfToggle != NULL)
            {
                *pMap->m_pfToggle = fToggleState;
				hr = S_OK;
            }
            else
            {
                hr = (this->*(pMap->m_pHandler))(pszArg + 1);
            }

            break;
        }

        ++pMap;
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
    HRESULT              hr                = S_OK;;               
    static const WCHAR   s_kszOrderByMap[] = L"nesd";
    const WCHAR        * pchOrderBy;       
    int                  idxOrderBy;       



    assert (pszArg != NULL);


    //
    // Make sure the arg isn't empty
    //

    CBRAEx (pszArg != NULL, E_INVALIDARG);
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

    pchOrderBy = wcschr (s_kszOrderByMap, towlower (*pszArg));
    if (pchOrderBy != NULL)
    {        
        //
        // Convert to an index into the map array
        //

        idxOrderBy = (int) (pchOrderBy - s_kszOrderByMap);

        //
        // Convert to the sort order enumeration
        //

        m_sortorder = (ESortOrder) idxOrderBy;

        //
        // Use this as the primary sort attribute
        //

        m_rgSortPreference[0] = m_sortorder;
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
    HRESULT              hr                = S_OK;      
    DWORD              * pdwMask; 
    const WCHAR        * pchAttribute;       
    int                  idxAttribute;       
    static const WCHAR   s_kszAttributes[] = L"dhsratecp0";
    static const DWORD   s_kdwAttributes[] = 
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






