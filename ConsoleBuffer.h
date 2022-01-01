#pragma once
#include <Windows.h>

#include "Console.h"



class CConsoleBuffer : CConsole
{
public:
    CConsoleBuffer          (void);
    virtual ~CConsoleBuffer (void);

    //
    // Public Methods
    //

    virtual HRESULT Puts                 (WORD attr, LPCWSTR psz);
    virtual int     Printf               (WORD attr, LPCWSTR pszFormat, ...);
    virtual HRESULT WriteString          (WORD attr, __in_ecount(cch) WCHAR * p, size_t cch);
    virtual HRESULT WriteSeparatorLine   (WORD attr);

    virtual HRESULT ReserveLines         (UINT cLines);
    virtual HRESULT ScrollBuffer         (UINT cLines);

protected:
    HRESULT InitializeBuffer     (void);
    HRESULT FlushBuffer          (void);


    DWORD         m_cScreenBuffer;
    CHAR_INFO   * m_prgScreenBuffer;
};               
