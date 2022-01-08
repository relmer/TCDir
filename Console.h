#pragma once





class CConsole
{
public:
    enum class EWrapMode
    {
        Clip = 0,
        Wrap = 1,

        __count
    };

    CConsole(void);
    virtual ~CConsole(void);
    
    //
    // Public Methods
    //

    HRESULT         SetWrapMode          (EWrapMode mode);

    virtual HRESULT Puts                 (WORD attr, LPCWSTR psz)                               PURE;
    virtual int     Printf               (WORD attr, LPCWSTR pszFormat, ...)                    PURE;
    virtual HRESULT WriteString          (WORD attr, __in_ecount(cch) WCHAR * p, size_t cch)    PURE;
    virtual HRESULT WriteSeparatorLine   (WORD attr)                                            PURE;

    virtual HRESULT ReserveLines         (int cLines)                                           PURE;
    virtual HRESULT ScrollBuffer         (int cLines)                                           PURE;

#ifdef _DEBUG
    void Test                                (void);
    void TestCanWriteMoreLinesThanWindowSize (void);
    void TestCanScrollWindow                 (void);
#endif 

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

    EWrapMode m_wrapMode;
    WORD      m_attrDefault;
    DWORD     m_consoleMode;
};               
