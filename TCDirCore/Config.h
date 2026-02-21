#pragma once

#include "EnvironmentProviderBase.h"
#include "EnvironmentProvider.h"
#include "IconMapping.h"
#include "SizeFormat.h"

#define TCDIR_ENV_VAR_NAME L"TCDIR"





class CConfig
{
protected:
    struct STextAttr
    {
        LPCWSTR m_pszExtension;
        WORD    m_wAttr;
    };

    struct SSwitchMapping
    {
        wstring_view              m_name;
        bool                      m_state;
        optional<bool> CConfig::* m_pMember;
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

    struct SFileDisplayStyle
    {
        WORD      m_wTextAttr;       // Resolved color attribute (Windows console WORD)
        char32_t  m_iconCodePoint;   // Resolved icon code point (0 = no icon configured)
        bool      m_fIconSuppressed; // true if icon explicitly set to empty (user typed ",")
    };

    enum class EIconActivation
    {
        Auto,       // Determined by auto-detection
        ForceOn,    // /Icons CLI flag or TCDIR=Icons
        ForceOff    // /Icons- CLI flag or TCDIR=Icons-
    };

    // X-Macro list of all EAttribute values.
    // Used to generate both the enum and the string lookup table in Console.cpp.
    #define EATTRIBUTE_LIST(MACRO)              \
        MACRO(Default)                          \
        MACRO(Date)                             \
        MACRO(Time)                             \
        MACRO(FileAttributePresent)             \
        MACRO(FileAttributeNotPresent)          \
        MACRO(Size)                             \
        MACRO(Directory)                        \
        MACRO(Information)                      \
        MACRO(InformationHighlight)             \
        MACRO(SeparatorLine)                    \
        MACRO(Error)                            \
        MACRO(Owner)                            \
        MACRO(Stream)                           \
        MACRO(CloudStatusCloudOnly)             \
        MACRO(CloudStatusLocallyAvailable)      \
        MACRO(CloudStatusAlwaysLocallyAvailable) \
        MACRO(TreeConnector)

    enum EAttribute
    {
        #define DECLARE_EATTRIBUTE(name) name,
        EATTRIBUTE_LIST(DECLARE_EATTRIBUTE)
        #undef DECLARE_EATTRIBUTE

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

    struct SOverrideValue
    {
        WORD     m_colorAttr   = 0;
        char32_t m_iconCP      = 0;
        bool     m_fSuppressed = false;
        bool     m_fHasColor   = false;
        bool     m_fHasIcon    = false;
    };



    CConfig (void);
   
    void              Initialize                  (WORD wDefaultAttr);
    SFileDisplayStyle GetDisplayStyleForFile      (const WIN32_FIND_DATA & wfd);
    char32_t          GetCloudStatusIcon          (DWORD dwCloudStatus);
    ValidationResult  ValidateEnvironmentVariable (void);
    void              SetEnvironmentProvider      (const IEnvironmentProvider * pProvider);
    HRESULT           ParseColorName              (wstring_view colorName, bool isBackground, WORD & colorValue);
    HRESULT           ParseColorSpec              (wstring_view colorSpec, WORD & colorAttr);
    static WORD       EnsureVisibleColorAttr      (WORD colorAttr, WORD defaultAttr);

    WORD                                       m_rgAttributes[EAttribute::__count]       = { 0 };
    EAttributeSource                           m_rgAttributeSources[EAttribute::__count] = { EAttributeSource::Default };
    TextAttrMap                                m_mapExtensionToTextAttr;
    unordered_map<wstring, EAttributeSource>   m_mapExtensionSources;
    FileAttrMap                                m_mapFileAttributesTextAttr;
    CEnvironmentProvider                       m_environmentProviderDefault;
    const IEnvironmentProvider               * m_pEnvironmentProvider             = nullptr;

    // Switch defaults from environment variable
    optional<bool>                             m_fWideListing;
    optional<bool>                             m_fBareListing;
    optional<bool>                             m_fRecurse;
    optional<bool>                             m_fPerfTimer;
    optional<bool>                             m_fMultiThreaded;
    optional<bool>                             m_fShowOwner;
    optional<bool>                             m_fShowStreams;
    optional<bool>                             m_fIcons;
    optional<bool>                             m_fTree;
    optional<int>                              m_cMaxDepth;
    optional<int>                              m_cTreeIndent;
    optional<ESizeFormat>                      m_eSizeFormat;

    // Icon mapping tables (parallel to color tables)
    unordered_map<wstring, char32_t>           m_mapExtensionToIcon;
    unordered_map<wstring, char32_t>           m_mapWellKnownDirToIcon;
    unordered_map<DWORD, char32_t>             m_mapFileAttributeToIcon;

    // Icon source tracking (parallel to color source tracking)
    unordered_map<wstring, EAttributeSource>   m_mapExtensionIconSources;
    unordered_map<wstring, EAttributeSource>   m_mapWellKnownDirIconSources;

    // Type fallback icons
    char32_t                                   m_iconDirectoryDefault  = NfIcon::CustomFolder;
    char32_t                                   m_iconFileDefault       = NfIcon::FaFile;
    char32_t                                   m_iconSymlink           = NfIcon::CodFileSymlinkDir;
    char32_t                                   m_iconJunction          = NfIcon::FaExternalLink;

    // Cloud status NF glyphs (used when icons active)
    char32_t                                   m_iconCloudOnly        = NfIcon::MdCloudOutline;
    char32_t                                   m_iconLocallyAvailable = NfIcon::MdCloudCheck;
    char32_t                                   m_iconAlwaysLocal      = NfIcon::MdPin;


    
protected:
    void         InitializeExtensionToTextAttrMap     (void);
    void         InitializeFileAttributeToTextAttrMap (void);
    void         PopulateIconMap                      (const SIconMappingEntry * rgEntries, size_t cEntries, unordered_map<wstring, char32_t> & mapIcons, unordered_map<wstring, EAttributeSource> & mapSources);
    void         ApplyUserColorOverrides              (void);
    void         ProcessColorOverrideEntry            (wstring_view entry);
    HRESULT      ParseOverrideValue                   (wstring_view entry, wstring_view valueView, SOverrideValue & ov);
    void         ApplyOverrideByKeyType               (wstring_view entry, wstring_view keyView, const SOverrideValue & ov);
    void         ProcessSwitchOverride                (wstring_view entry);
    bool         IsSwitchName                         (wstring_view entry);
    bool         TryProcessIntSwitch                  (wstring_view entry);
    void         ProcessFileExtensionOverride         (wstring_view extension, WORD colorAttr);
    void         ProcessDisplayAttributeOverride      (wchar_t attrChar, WORD colorAttr, wstring_view entry);
    void         ProcessFileAttributeOverride         (wstring_view keyView, wstring_view entry, const SOverrideValue & ov);
    HRESULT      ParseKeyAndValue                     (wstring_view entry, wstring_view & keyView, wstring_view & valueView);
    HRESULT      ParseColorValue                      (wstring_view entry, wstring_view valueView, WORD & colorAttr);
    HRESULT      ParseIconValue                       (wstring_view iconSpec, char32_t & codePoint, bool & fSuppressed);
    void         ApplyIconOverride                    (wstring_view name, char32_t iconCodePoint, bool fSuppressed, unordered_map<wstring, char32_t> & mapIcons, unordered_map<wstring, EAttributeSource> & mapSources);
    void         ProcessFileAttributeIconOverride     (DWORD dwAttribute, char32_t iconCodePoint);
    void         ResolveFileAttributeStyle             (const WIN32_FIND_DATA & wfd, SFileDisplayStyle & style);
    void         ResolveDirectoryStyle                 (const WIN32_FIND_DATA & wfd, SFileDisplayStyle & style);
    void         ResolveExtensionStyle                 (const WIN32_FIND_DATA & wfd, SFileDisplayStyle & style);
    void         ResolveFileFallbackIcon               (const WIN32_FIND_DATA & wfd, SFileDisplayStyle & style);
    wstring_view TrimWhitespace                       (wstring_view str);

    ValidationResult m_lastParseResult;

    static const STextAttr      s_rgTextAttrs[];
    static const SSwitchMapping s_switchMappings[];
};
