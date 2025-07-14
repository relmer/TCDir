#pragma once





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
    WORD GetTextAttrForFile (const WIN32_FIND_DATA * pwfd);

    WORD         m_rgAttributes[EAttribute::__count];
    TextAttrMap  m_mapExtensionToTextAttr;


protected:
    void InitializeExtensionToTextAttrMap (void);

    static const STextAttr s_rgTextAttrs[];

};

