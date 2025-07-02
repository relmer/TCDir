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

    CConsole          (void);
    virtual ~CConsole (void);
    
    HRESULT         Initialize           (void);
    HRESULT         SetWrapMode          (EWrapMode mode);

    virtual HRESULT Puts                 (WORD attr, LPCWSTR psz)                               PURE;
    virtual int     Printf               (WORD attr, LPCWSTR pszFormat, ...)                    PURE;
    virtual HRESULT WriteString          (WORD attr, __in_ecount(cch) WCHAR * p, size_t cch)    PURE;
    virtual HRESULT WriteSeparatorLine   (WORD attr)                                            PURE;

#ifdef _DEBUG
    void Test                                (void);
#endif 

    HANDLE                       m_hStdOut; 
    CONSOLE_SCREEN_BUFFER_INFOEX m_consoleScreenBufferInfoEx;
    COORD                        m_coord;



protected:

    virtual HRESULT Flush (void) { return S_OK; }

    EWrapMode m_wrapMode;
    WORD      m_attrDefault;
    wstring   m_strBuffer;
};               
