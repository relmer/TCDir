#pragma once



#define USE_SCREEN_BUFFER



class CUtils
{
public:
    enum EForeColor
    {
        FC_Black        = 0,
        FC_Blue         = FOREGROUND_BLUE,
        FC_Green        = FOREGROUND_GREEN,
        FC_Cyan         = FOREGROUND_BLUE | FOREGROUND_GREEN,
        FC_Red          = FOREGROUND_RED,
        FC_Magenta      = FOREGROUND_RED | FOREGROUND_BLUE,
        FC_Brown        = FOREGROUND_RED | FOREGROUND_GREEN,
        FC_LightGrey    = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
        FC_DarkGrey     = FOREGROUND_INTENSITY | FC_Black,
        FC_LightBlue    = FOREGROUND_INTENSITY | FC_Blue,
        FC_LightGreen   = FOREGROUND_INTENSITY | FC_Green,
        FC_LightCyan    = FOREGROUND_INTENSITY | FC_Cyan,
        FC_LightRed     = FOREGROUND_INTENSITY | FC_Red,
        FC_LightMagenta = FOREGROUND_INTENSITY | FC_Magenta,
        FC_Yellow       = FOREGROUND_INTENSITY | FC_Brown,
        FC_White        = FOREGROUND_INTENSITY | FC_LightGrey,
        FC_Mask         = FC_White,
    };                

    enum EBackColor
    {
        BC_Black        = 0,
        BC_Blue         = BACKGROUND_BLUE,
        BC_Green        = BACKGROUND_GREEN,
        BC_Cyan         = BACKGROUND_BLUE | BACKGROUND_GREEN,
        BC_Red          = BACKGROUND_RED,
        BC_Magenta      = BACKGROUND_RED | BACKGROUND_BLUE,
        BC_Brown        = BACKGROUND_RED | BACKGROUND_GREEN,
        BC_LightGrey    = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,
        BC_DarkGrey     = BACKGROUND_INTENSITY | BC_Black,
        BC_LightBlue    = BACKGROUND_INTENSITY | BC_Blue,
        BC_LightGreen   = BACKGROUND_INTENSITY | BC_Green,
        BC_LightCyan    = BACKGROUND_INTENSITY | BC_Cyan,
        BC_LightRed     = BACKGROUND_INTENSITY | BC_Red,
        BC_LightMagenta = BACKGROUND_INTENSITY | BC_Magenta,
        BC_Yellow       = BACKGROUND_INTENSITY | BC_Brown,
        BC_White        = BACKGROUND_INTENSITY | BC_LightGrey,
        BC_Mask         = BC_White,
    };                

    enum EAttribute
    {
        Default,
        Date,
        Time,
        FileAttributePresent,
        FileAttributeNotPresent,
        Size,
        Directory,
        Information,
        InformationHighlight,
        SeparatorLine,

        __count
    };


    
    //
    // Instead of a public constructor
    //

    static CUtils & GetSingleInstance (void);

    
    //
    // Public Methods
    //

    int     ConsolePrintf               (WORD attr, LPCWSTR pszFormat, ...);
    HRESULT WriteConsoleString          (WORD attr, __in_ecount(cch) WCHAR * p, size_t cch);
    HRESULT WriteConsoleSeparatorLine   (void);
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






