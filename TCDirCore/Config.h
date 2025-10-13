#pragma once




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

    
    void Initialize         (WORD wDefaultAttr);
    WORD GetTextAttrForFile (const WIN32_FIND_DATA & wfd);

    WORD         m_rgAttributes[EAttribute::__count];
    TextAttrMap  m_mapExtensionToTextAttr;


protected:
    void         InitializeExtensionToTextAttrMap (void);
    void         ApplyUserColorOverrides          (void);
    void         ProcessColorOverrideEntry        (wstring_view entry);
    HRESULT      ParseKeyAndValue                 (wstring_view entry, wstring_view & keyView, wstring_view & valueView);
    WORD         ParseColorSpec                   (wstring_view colorSpec);
    WORD         ParseColorName                   (wstring_view colorName, bool isBackground);
    wstring_view TrimWhitespace                   (wstring_view str);

    static const STextAttr s_rgTextAttrs[];
};


