#pragma once

#include <windows.h>




class CConsole;




class CUsage
{
public:

    static void    DisplayUsage                    (CConsole & console);
    static void    DisplayEnvVarConfigurationReport (CConsole & console);



private:

    static bool    IsEnvVarSet                       (LPCWSTR kpszEnvVarName);
    static bool    IsPowerShell                      (void);
    static bool    IsTcdirEnvVarSet                  (void);

    static HRESULT DisplayEnvVarIssues               (CConsole & console);
    static void    DisplayConfigurationTable         (CConsole & console);

    static void    DisplayAttributeConfiguration     (CConsole & console, const wstring & tableSeparator, int columnWidthAttr, int columnWidthSource);
    static void    DisplayFileAttributeConfiguration (CConsole & console, const wstring & tableSeparator, int columnWidthAttr, int columnWidthSource);
    static void    DisplayExtensionConfiguration     (CConsole & console, const wstring & tableSeparator, int columnWidthAttr, int columnWidthSource);

    static void    DisplayExtensionConfigurationSingleColumn (CConsole & console, const wstring & tableSeparator, int columnWidthAttr, int columnWidthSource, const vector<pair<wstring, WORD>> & extensions);
    static void    DisplayExtensionConfigurationMultiColumn  (CConsole & console, const vector<pair<wstring, WORD>> & extensions, size_t maxExtLen, size_t cxSourceWidth, size_t cxAvailable, size_t cColumns);

    static void    DisplayColorConfiguration         (CConsole & console);
    static WORD    GetColorAttribute                 (CConsole & console, wstring_view colorName);
};
