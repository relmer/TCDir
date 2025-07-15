#pragma once





class CConfig;





class CConsole
{
public:

    CConsole   (void);
     ~CConsole (void);
    
    HRESULT Initialize           (shared_ptr<CConfig> configPtr);
    void    Puts                 (int attributeIndex, LPCWSTR psz);
    int     Printf               (int attributeIndex, LPCWSTR pszFormat, ...);
    void    WriteSeparatorLine   (WORD attr);
    HRESULT Flush                (void);

#ifdef _DEBUG
    void Test                    (void);
#endif 

    HANDLE                       m_hStdOut; 
    CONSOLE_SCREEN_BUFFER_INFOEX m_consoleScreenBufferInfoEx;



protected:

    void SetColor (WORD attr);

    static constexpr size_t s_kcchInitialBufferSize = 10 * 1024 * 1024;

    shared_ptr<CConfig> m_configPtr;
    WORD                m_attrDefault;
    wstring             m_strBuffer;
};
