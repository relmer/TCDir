#pragma once
#include <Windows.h>

#include "IConsole.h"



class CConsoleBuffer : IConsole
{
public:
    //
    // Public Methods
    //

    HRESULT Puts                 (WORD attr, LPCWSTR psz);
    int     Printf               (WORD attr, LPCWSTR pszFormat, ...);
    HRESULT WriteString          (WORD attr, __in_ecount(cch) WCHAR * p, size_t cch);
    HRESULT WriteSeparatorLine   (void);
    HRESULT ScrollBuffer         (UINT cLines);
    HRESULT InitializeBuffer     (void);
    HRESULT FlushBuffer          (void);

    //
    // Public members
    //

    HANDLE                       m_hStdOut; 
    CONSOLE_SCREEN_BUFFER_INFOEX m_consoleScreenBufferInfoEx;
    COORD                        m_coord;



protected:
    DWORD         m_cScreenBuffer;
    CHAR_INFO   * m_prgScreenBuffer;
    DWORD         m_consoleMode;
};               
