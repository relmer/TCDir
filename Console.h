#pragma once





class CConsole
{
public:

    CConsole   (void);
     ~CConsole (void);
    
    HRESULT Initialize           (void);
    void    Puts                 (WORD attr, LPCWSTR psz);
    int     Printf               (WORD attr, LPCWSTR pszFormat, ...);
    void    WriteSeparatorLine   (WORD attr);
    HRESULT Flush                (void);

#ifdef _DEBUG
    void Test                    (void);
#endif 

    HANDLE                       m_hStdOut; 
    CONSOLE_SCREEN_BUFFER_INFOEX m_consoleScreenBufferInfoEx;



protected:

    void SetColor (WORD attr);

    static const size_t s_kcchInitialBufferSize = 10 * 1024 * 1024;

    WORD      m_attrDefault;
    wstring   m_strBuffer;
};               
