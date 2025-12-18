#pragma once





#include "pch.h"





class CCommandLine
{
public:
    //
    // Public types
    //
    
    enum class ESortOrder 
    {                       // Preface with '-' to reverse the order
        SO_DEFAULT,         // Unsorted (use the order returned by FindFirstFile)
        SO_NAME,            // N  By name (alphabetic)
        SO_EXTENSION,       // E  By extension (alphabetic)
        SO_SIZE,            // S  By size (smallest first)
        SO_DATE,            // D  By date/time (oldest first)

        __SO_COUNT          // Count of sort orders in this enum
    };

    enum class ESortDirection
    {
        SD_ASCENDING,
        SD_DESCENDING,    
    };



    //
    // Constructor, destructor
    //
    
    CCommandLine  (void); 
    ~CCommandLine (void); 



    //
    // Public methods
    //
    
    HRESULT Parse (int cArg, WCHAR ** ppszArg);



    //
    // Public members
    //

    bool               m_fRecurse                                          = false;
    DWORD              m_dwAttributesRequired                              = 0;
    DWORD              m_dwAttributesExcluded                              = 0;
    ESortOrder         m_rgSortPreference[(size_t) ESortOrder::__SO_COUNT] = { };
    ESortOrder         m_sortorder                                         = ESortOrder::SO_DEFAULT;
    ESortDirection     m_sortdirection                                     = ESortDirection::SD_ASCENDING;
    list<std::wstring> m_listMask;
    bool               m_fWideListing                                      = false;
    bool               m_fPerfTimer                                        = false;    // Enable performance timer
    bool               m_fMultiThreaded                                    = true;     // Enable multi-threaded enumeration
    bool               m_fEnv                                              = false;    // Display environment variable help
    bool               m_fConfig                                           = false;    // Display current configuration
    bool               m_fHelp                                             = false;    // Display usage

    

protected: 
    //
    // Protected types
    //
    
    typedef HRESULT (CCommandLine::*SwitchParserFunc)(LPCWSTR pszArg);


    //
    // Protected methods
    //
    
    HRESULT HandleSwitch     (LPCWSTR pszArg);
    HRESULT OrderByHandler   (LPCWSTR pszArg);
    HRESULT AttributeHandler (LPCWSTR pszArg);

    
};
