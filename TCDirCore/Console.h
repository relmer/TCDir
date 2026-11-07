#pragma once

#include "Config.h"





class CConsole
{
public:

             CConsole   (void);
    virtual ~CConsole   (void);
    
    HRESULT         Initialize                (shared_ptr<CConfig> configPtr);
    void            Putchar                   (WORD attr, WCHAR ch);
    void            Puts                      (int attributeIndex, LPCWSTR psz);
    int             Printf                    (CConfig::EAttribute attributeIndex, LPCWSTR pszFormat, ...);
    int             Printf                    (WORD attr, LPCWSTR pszFormat, ...);
    void            ColorPuts                 (LPCWSTR psz);
    void            ColorPrintf               (LPCWSTR pszFormat, ...);
    void            PrintColorfulString       (LPCWSTR psz);
    void            WriteSeparatorLine        (WORD attr);
    virtual HRESULT Flush                     (void);
    void            SetAutoFlush              (bool fAutoFlush)  { m_fAutoFlush = fAutoFlush; }

    UINT    GetWidth                  (void)     { return m_cxConsoleWidth; }

    static UINT WidthFromColumns      (const wstring & strColumns, UINT cxDefault);

    shared_ptr<CConfig> m_configPtr;

    

protected:

    HRESULT InitializeConsoleMode               (void);
    HRESULT InitializeConsoleWidth              (void);
    static wstring ReadColumnsVariable          (void);
    void    SetColor                            (WORD attr);
    void    ColorPrint                          (LPCWSTR psz);
    void    ProcessMultiLineStringWithAttribute (wstring_view text, WORD attr);
    bool    ParseColorMarker                    (wstring_view text, size_t pos, CConfig::EAttribute & outAttr, size_t & outMarkerLen);
    void    FlushIfAuto                         (void);

    static constexpr size_t s_kcchInitialBufferSize = 10 * 1024 * 1024;
    static constexpr UINT   s_kcxMinColumns         = 20;
    static constexpr UINT   s_kcxMaxColumns         = 1000;
    static constexpr size_t s_kcchMaxColumnsValue   = 4;
    static constexpr UINT   s_kuDecimalBase         = 10;

    HANDLE              m_hStdOut        = nullptr;
    bool                m_fIsRedirected  = true;    // True if redirected (e.g., in a unit test)
    bool                m_fAutoFlush     = false;   // When set, each output call flushes immediately
    WORD                m_attrDefault    = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    wstring             m_strBuffer;
    UINT                m_cxConsoleWidth = 80;
};
