#pragma once





class CConsole;





class CUsage
{
public:

    enum class EItemDisplayMode
    {
        SingleColumn,   // Indent + newline (for single-column lists)
        MultiColumn,    // No indent, column padding (for multi-column grids)
    };

    static void    DisplayUsage                     (CConsole & console, wchar_t chPrefix = L'-');
    static void    DisplayEnvVarHelp                (CConsole & console, wchar_t chPrefix = L'-');
    static void    DisplayCurrentConfiguration      (CConsole & console, wchar_t chPrefix = L'-', optional<bool> fIconsCli = nullopt);
    static void    DisplayEnvVarIssues              (CConsole & console, wchar_t chPrefix = L'-', bool fShowHint = true);



private:

    static bool    IsEnvVarSet                               (LPCWSTR kpszEnvVarName);
    static bool    IsPowerShell                              (void);
    static bool    IsTcdirEnvVarSet                          (void);

    static void    DisplayConfigurationTable                 (CConsole & console, bool fShowIcons);
    static void    DisplaySynopsis                              (CConsole & console, wchar_t chPrefix);

    static void    DisplayAttributeConfiguration             (CConsole & console, int columnWidthAttr, int columnWidthSource, bool fShowIcons);
    static void    DisplayFileAttributeConfiguration         (CConsole & console, int columnWidthAttr, int columnWidthSource);
    static void    DisplayExtensionConfiguration             (CConsole & console, int columnWidthAttr, int columnWidthSource, bool fShowIcons);
    static void    DisplayWellKnownDirConfiguration          (CConsole & console, bool fShowIcons);
    static void    DisplayItemAndSource                      (CConsole & console, wstring_view item, WORD attr, bool isEnv, size_t columnWidthItem, size_t columnWidthSource, size_t cxColumnWidth, EItemDisplayMode mode, wstring_view iconPrefix = L"", bool fShowIcons = false);

    static void    DisplayExtensionConfigurationSingleColumn (CConsole & console, int columnWidthAttr, int columnWidthSource, const vector<pair<wstring, WORD>> & extensions, bool fShowIcons);
    static void    DisplayExtensionConfigurationMultiColumn  (CConsole & console, const vector<pair<wstring, WORD>> & extensions, size_t maxExtLen, size_t cxSourceWidth, size_t cxAvailable, size_t cColumns, bool fShowIcons);

    static void    DisplayColorConfiguration                 (CConsole & console);
    static WORD    GetColorAttribute                         (CConsole & console, wstring_view colorName);

    static HRESULT DisplayEnvVarSegment                      (CConsole & console, wstring_view segment);
    static void    DisplayEnvVarCurrentValue                 (CConsole & console, LPCWSTR pszEnvVarName);
    static void    DisplayEnvVarDecodedSettings              (CConsole & console);
};
