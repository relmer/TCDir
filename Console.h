#pragma once





class CConsole
{
public:

    CConsole(void);
    virtual ~CConsole(void);
    
    //
    // Public Methods
    //

    virtual HRESULT Puts                 (WORD attr, LPCWSTR psz)                               PURE;
    virtual int     Printf               (WORD attr, LPCWSTR pszFormat, ...)                    PURE;
    virtual HRESULT WriteString          (WORD attr, __in_ecount(cch) WCHAR * p, size_t cch)    PURE;
    virtual HRESULT WriteSeparatorLine   (void)                                                 PURE;

    virtual HRESULT ScrollBuffer         (UINT cLines)                                          PURE;

    //
    // Public members
    //

    HANDLE                       m_hStdOut; 
    CONSOLE_SCREEN_BUFFER_INFOEX m_consoleScreenBufferInfoEx;
    COORD                        m_coord;

protected:

    //
    // Protected members
    //

    DWORD m_consoleMode;
};               
