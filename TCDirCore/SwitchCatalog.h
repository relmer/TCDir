#pragma once





////////////////////////////////////////////////////////////////////////////////
//
//  ESwitchForm
//
//  Short switches take one prefix character (-W, /W); long switches take
//  "--" or "/" (--Tree, /Tree).
//
////////////////////////////////////////////////////////////////////////////////

enum class ESwitchForm
{
    Short,
    Long
};





////////////////////////////////////////////////////////////////////////////////
//
//  ESwitchSection
//
//  The group of the -? help a switch appears in.
//
////////////////////////////////////////////////////////////////////////////////

enum class ESwitchSection
{
    Primary,
    Alias
};





////////////////////////////////////////////////////////////////////////////////
//
//  SSwitchValue
//
//  One value of a switch, such as an attribute letter.  A prefix value (the
//  "-" that negates an attribute or reverses a sort) is combined with the
//  other values rather than used alone.
//
////////////////////////////////////////////////////////////////////////////////

struct SSwitchValue
{
    LPCWSTR   m_pszValue;
    LPCWSTR   m_pszDescription;
    bool      m_fPrefix = false;
};





////////////////////////////////////////////////////////////////////////////////
//
//  SSwitchDefinition
//
//  One switch as the -? help documents it.  Descriptions are std::format
//  templates with color markers in doubled braces; {0} is the short-switch
//  prefix, {1} the long-switch prefix and {2} the Nerd Font face.
//
////////////////////////////////////////////////////////////////////////////////

struct SSwitchDefinition
{
    ESwitchForm                 m_eForm;
    LPCWSTR                     m_pszName;              // Casing as in the help
    LPCWSTR                     m_pszValueSyntax;       // Help label suffix with color markers, such as "={InformationHighlight}N{Information}"
    wchar_t                     m_chValueSeparator;     // ':' or '='; 0 when the switch doesn't take a value
    span<const SSwitchValue>    m_values;
    bool                        m_fNegatable;           // A trailing '-' turns the switch off
    bool                        m_fRepeatable;
    ESwitchSection              m_eSection;
    LPCWSTR                     m_pszDescription;
    LPCWSTR                     m_pszContinuation;      // Second help line; empty for none
    bool                        m_fDebugOnly;
    bool                        m_fCombinableValues = false;    // Values combine, as attribute letters do
};





////////////////////////////////////////////////////////////////////////////////
//
//  CSwitchCatalog
//
//  The switches the -? help documents.  The help text and the PowerShell
//  completion candidates both come from this table.
//
////////////////////////////////////////////////////////////////////////////////

class CSwitchCatalog
{
public:
    static span<const SSwitchDefinition> All();
    static bool                          IsAvailable    (const SSwitchDefinition & definition);
    static const SSwitchDefinition *     Find           (ESwitchForm eForm, wstring_view name);
    static wstring                       Label          (const SSwitchDefinition & definition, wstring_view shortPrefix, wstring_view longPrefix);
    static wstring                       FormatText     (LPCWSTR pszTemplate, wstring_view shortPrefix, wstring_view longPrefix);
    static wstring                       PlainText      (wstring_view markup);
    static wstring                       PrefixFor      (const SSwitchDefinition & definition, wstring_view shortPrefix, wstring_view longPrefix);
    static span<const SSwitchValue>      ValuesOf       (wstring_view shortName);

private:
    static const SSwitchValue            s_krgAttributeValues[];
    static const SSwitchValue            s_krgSortValues[];
    static const SSwitchValue            s_krgTimeValues[];
    static const SSwitchValue            s_krgSizeValues[];
    static const SSwitchValue            s_krgTreeIndentValues[];
    static const SSwitchDefinition       s_krgSwitches[];
};
