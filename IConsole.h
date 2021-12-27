#pragma once
#include <Windows.h>



__interface IConsole
{
public:


    
    //
    // Public Methods
    //

    HRESULT Puts                 (WORD attr, LPCWSTR psz);
    int     Printf               (WORD attr, LPCWSTR pszFormat, ...);
    HRESULT WriteString          (WORD attr, __in_ecount(cch) WCHAR * p, size_t cch);
    HRESULT WriteSeparatorLine   (void);

    BOOL    IsDots                      (LPCWSTR pszFileName);
    WORD    GetTextAttrForFile          (const WIN32_FIND_DATA * pwfd);
    HRESULT ScrollBuffer                (UINT cLines);
    HRESULT InitializeBuffer            (void);
    HRESULT FlushBuffer                 (void);

    //
    // Public members
    //

    HANDLE                       m_hStdOut; 
    CONSOLE_SCREEN_BUFFER_INFOEX m_consoleScreenBufferInfoEx;
    COORD                        m_coord;
    WORD                         m_rgAttributes[EAttribute::__count];



protected:

    typedef unordered_map<wstring, WORD> TextAttrMap;
    typedef TextAttrMap::const_iterator  TextAttrMapConstIter;
    

    struct STextAttr
    {
        LPCWSTR m_pszExtension; 
        WORD    m_wAttr;        
    };                        
    
    //
    // This is a single-instance class, so there's no public constructor
    //

    CUtils  (void); 
    ~CUtils (void); 


    //
    // Protected methods
    //
     
    void InitializeTextAttrs              (void);
    void InitializeExtensionToTextAttrMap (void);
    
    //
    // Protected members
    //

    static const STextAttr s_rgTextAttrs[];

#ifdef USE_SCREEN_BUFFER
    DWORD         m_cScreenBuffer;
    CHAR_INFO   * m_prgScreenBuffer;
#endif
    TextAttrMap   m_mapExtensionToTextAttr;   
    DWORD         m_consoleMode;
};               





extern CUtils & g_util;






