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

    bool               m_fRecurse;             
    DWORD              m_dwAttributesRequired; 
    DWORD              m_dwAttributesExcluded; 
    ESortOrder         m_rgSortPreference[(size_t) ESortOrder::__SO_COUNT];
    ESortOrder         m_sortorder;
    ESortDirection     m_sortdirection;        
    list<std::wstring> m_listMask;
    bool               m_fWideListing;
    bool               m_fPerfTimer;          // Enable performance timer
    bool               m_fMultiThreaded;      // Enable multi-threaded enumeration


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
