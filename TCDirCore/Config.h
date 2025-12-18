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

    struct ValidationResult
    {
        vector<wstring> errors;
        bool            hasIssues() const { return !errors.empty(); }
    };



    CConfig (void);
   
    void              Initialize                  (WORD wDefaultAttr);
    WORD              GetTextAttrForFile          (const WIN32_FIND_DATA & wfd);
    ValidationResult  ValidateEnvironmentVariable (void);
    void              SetEnvironmentProvider      (const IEnvironmentProvider * pProvider);
    WORD              ParseColorName              (wstring_view colorName, bool isBackground);

    WORD                                       m_rgAttributes[EAttribute::__count]       = { 0 };
    EAttributeSource                           m_rgAttributeSources[EAttribute::__count] = { EAttributeSource::Default };
    TextAttrMap                                m_mapExtensionToTextAttr;
    unordered_map<wstring, EAttributeSource>   m_mapExtensionSources;
    FileAttrMap                                m_mapFileAttributesTextAttr;
    CEnvironmentProvider                       m_environmentProviderDefault;
    const IEnvironmentProvider               * m_pEnvironmentProvider             = nullptr;


    
protected:
    void         InitializeExtensionToTextAttrMap     (void);
    void         InitializeFileAttributeToTextAttrMap (void);
    void         ApplyUserColorOverrides              (void);
    void         ProcessColorOverrideEntry            (wstring_view entry);
    void         ProcessFileExtensionOverride         (wstring_view extension, WORD colorAttr);
    void         ProcessDisplayAttributeOverride      (wchar_t attrChar, WORD colorAttr);
    void         ProcessFileAttributeOverride         (wstring_view keyView, WORD colorAttr);
    HRESULT      ParseKeyAndValue                     (wstring_view entry, wstring_view & keyView, wstring_view & valueView);
    WORD         ParseColorSpec                       (wstring_view colorSpec);
    wstring_view TrimWhitespace                       (wstring_view str);

    ValidationResult m_lastParseResult;

    static const STextAttr s_rgTextAttrs[];
};







