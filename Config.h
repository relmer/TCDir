#pragma once





class CConfig
{
protected:
    struct STextAttr
    {
        LPCWSTR m_pszExtension;
        WORD    m_wAttr;
    };

    typedef unordered_map<LPCWSTR, WORD> TextAttrMap;
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

        __count
    };

    
    CConfig  (WORD wDefaultAttr);
    ~CConfig ();
        
    WORD GetTextAttrForFile (const WIN32_FIND_DATA * pwfd);

    WORD         m_rgAttributes[EAttribute::__count];
    TextAttrMap  m_mapExtensionToTextAttr;


protected:
    void InitializeTextAttrs              (WORD wDefaultAttr);
    void InitializeExtensionToTextAttrMap (void);

    static const STextAttr s_rgTextAttrs[];

};

