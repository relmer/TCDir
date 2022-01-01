#pragma once

#include "Console.h"
#include "Config.h"





class CConsoleApi : public CConsole
{
public:
    CConsoleApi  (void); 
    ~CConsoleApi (void);

    HRESULT Puts                 (WORD attr, LPCWSTR psz);
    int     Printf               (WORD attr, LPCWSTR pszFormat, ...);
    HRESULT WriteString          (WORD attr, __in_ecount(cch) WCHAR * p, size_t cch);
    HRESULT WriteSeparatorLine   (void);
    HRESULT ScrollBuffer         (UINT cLines);


protected:
    HANDLE                       m_hStdOut; 
    CONSOLE_SCREEN_BUFFER_INFOEX m_consoleScreenBufferInfoEx;
    COORD                        m_coord;
    WORD                         m_rgAttributes[CConfig::EAttribute::__count];
    
};               
