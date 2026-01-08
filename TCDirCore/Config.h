#pragma once

#include "EnvironmentProviderBase.h"
#include "EnvironmentProvider.h"

#define TCDIR_ENV_VAR_NAME L"TCDIR"





class CConfig
{
protected:
    struct STextAttr
    {
        LPCWSTR m_pszExtension;
        WORD    m_wAttr;
    };

    typedef unordered_map<wstring, WORD> TextAttrMap;
    typedef TextAttrMap::const_iterator  TextAttrMapConstIter;



public:
    enum class EAttributeSource
    {
        Default,
        Environment
    };

    struct SFileAttributeStyle
    {
        WORD             m_wAttr;
        EAttributeSource m_source;
    };

    typedef unordered_map<DWORD, SFileAttributeStyle> FileAttrMap;

    enum EAttribute
    {
        Default,
        Date,
        Time,
        FileAttributePresent,
        FileAttributeNotPresent,
        Size,
        Directory,
        Information,
        InformationHighlight,
        SeparatorLine,
        Error,
        
        __count
    };

    struct ErrorInfo
    {
        wstring message;            // Error description (e.g., "Invalid foreground color")
        wstring entry;              // Full "Key=Value" segment from TCDIR
        wstring invalidText;        // The specific invalid portion to underline
        size_t  invalidTextOffset;  // Position of invalidText within entry
    };

    struct ValidationResult
    {
        vector<ErrorInfo> errors;
        bool              hasIssues() const { return !errors.empty(); }
    };



    CConfig (void);
   
    void              Initialize                  (WORD wDefaultAttr);
    WORD              GetTextAttrForFile          (const WIN32_FIND_DATA & wfd);
    ValidationResult  ValidateEnvironmentVariable (void);
    void              SetEnvironmentProvider      (const IEnvironmentProvider * pProvider);
    WORD              ParseColorName              (wstring_view colorName, bool isBackground);
    WORD              ParseColorSpec              (wstring_view colorSpec);

    WORD                                       m_rgAttributes[EAttribute::__count]       = { 0 };
    EAttributeSource                           m_rgAttributeSources[EAttribute::__count] = { EAttributeSource::Default };
    TextAttrMap                                m_mapExtensionToTextAttr;
    unordered_map<wstring, EAttributeSource>   m_mapExtensionSources;
    FileAttrMap                                m_mapFileAttributesTextAttr;
    CEnvironmentProvider                       m_environmentProviderDefault;
    const IEnvironmentProvider               * m_pEnvironmentProvider             = nullptr;

    // Switch defaults from environment variable
    optional<bool>                             m_fWideListing;
    optional<bool>                             m_fRecurse;
    optional<bool>                             m_fPerfTimer;
    optional<bool>                             m_fMultiThreaded;


    
protected:
    void         InitializeExtensionToTextAttrMap     (void);
    void         InitializeFileAttributeToTextAttrMap (void);
    void         ApplyUserColorOverrides              (void);
    void         ProcessColorOverrideEntry            (wstring_view entry);
    void         ProcessSwitchOverride                (wstring_view entry);
    void         ProcessFileExtensionOverride         (wstring_view extension, WORD colorAttr);
    void         ProcessDisplayAttributeOverride      (wchar_t attrChar, WORD colorAttr, wstring_view entry);
    void         ProcessFileAttributeOverride         (wstring_view keyView, WORD colorAttr, wstring_view entry);
    HRESULT      ParseKeyAndValue                     (wstring_view entry, wstring_view & keyView, wstring_view & valueView);
    HRESULT      ParseColorValue                      (wstring_view entry, wstring_view valueView, WORD & colorAttr);
    wstring_view TrimWhitespace                       (wstring_view str);

    ValidationResult m_lastParseResult;

    static const STextAttr s_rgTextAttrs[];
};
