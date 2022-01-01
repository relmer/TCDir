#pragma once

#include "Console.h"
#include "Config.h"





class CConsoleApi : public CConsole
{
public:
    CConsoleApi          (void); 
    virtual ~CConsoleApi (void);

    virtual HRESULT Puts                 (WORD attr, LPCWSTR psz);
    virtual int     Printf               (WORD attr, LPCWSTR pszFormat, ...);
    virtual HRESULT WriteString          (WORD attr, __in_ecount(cch) WCHAR * p, size_t cch);
    virtual HRESULT WriteSeparatorLine   (WORD attr);

    virtual HRESULT ReserveLines         (UINT cLines);
    virtual HRESULT ScrollBuffer         (UINT cLines);

};               
