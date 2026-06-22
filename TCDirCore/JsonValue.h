#pragma once

//
// Ported from Casso\CassoEmuCore\Core\JsonValue.h.  Type names are kept
// identical to Casso so the two copies can eventually be sourced from one
// shared location.  TCDir adds source-span tracking (begin/end byte offsets
// of each value in the parsed text) so callers can locate and splice a single
// value without reserializing the whole document.
//




////////////////////////////////////////////////////////////////////////////////
//
//  JSON error codes
//
////////////////////////////////////////////////////////////////////////////////

static const HRESULT JSON_E_KEY_MISSING   = HRESULT_FROM_WIN32 (ERROR_NOT_FOUND);
static const HRESULT JSON_E_TYPE_MISMATCH = HRESULT_FROM_WIN32 (ERROR_DATATYPE_MISMATCH);




////////////////////////////////////////////////////////////////////////////////
//
//  JsonType
//
////////////////////////////////////////////////////////////////////////////////

enum class JsonType
{
    Null,
    Bool,
    Number,
    String,
    Array,
    Object
};




////////////////////////////////////////////////////////////////////////////////
//
//  JsonString
//
//  A parsed JSON string.  Normally a zero-copy view into the document's source
//  buffer; it owns a decoded copy ONLY when the original text contained escape
//  sequences.  The view is recomputed on every access (never stored pointing at
//  the owned buffer), so a JsonString stays valid when its node is moved.
//
//  Borrowed views point into the JsonDocument's source, which owns the bytes for
//  its whole lifetime — so the views never dangle.
//
////////////////////////////////////////////////////////////////////////////////

class JsonString
{
public:
    JsonString() = default;
    explicit JsonString (string_view view)  : m_view  (view)                          {}
    explicit JsonString (string &&   owned) : m_owned (move (owned)), m_ownsIt (true) {}

    string_view View() const { return m_ownsIt ? string_view (m_owned) : m_view; }

    bool operator== (string_view rhs) const { return View() == rhs; }

private:
    string_view m_view;
    string      m_owned;
    bool        m_ownsIt = false;
};




////////////////////////////////////////////////////////////////////////////////
//
//  JsonValue
//
////////////////////////////////////////////////////////////////////////////////

class JsonValue
{
    friend class JsonParser;

public:
    JsonValue() = default;
    explicit JsonValue (nullptr_t)                                                                                    {}
    explicit JsonValue (bool   value)                            : m_type (JsonType::Bool),   m_bool   (value)        {}
    explicit JsonValue (double value)                            : m_type (JsonType::Number), m_number (value)        {}
    explicit JsonValue (JsonString &&     value)                 : m_type (JsonType::String), m_string (move (value)) {}
    explicit JsonValue (vector<JsonValue>                   && arr)  : m_type (JsonType::Array),  m_array  (move (arr))   {}
    explicit JsonValue (vector<pair<JsonString, JsonValue>> && obj)  : m_type (JsonType::Object), m_object (move (obj))   {}

    JsonType GetType() const { return m_type; }

    bool        GetBool() const { return m_bool;   }
    double      GetNumber() const { return m_number; }
    int         GetInt() const { return static_cast<int> (m_number); }
    string_view GetString() const { return m_string.View(); }

    //
    // Source span: half-open byte range [SpanBegin, SpanEnd) covering this
    // value's text in the parsed input.  For strings the range includes the
    // surrounding quotes; for objects/arrays it includes the braces/brackets.
    //

    size_t SpanBegin() const { return m_spanBegin; }
    size_t SpanEnd() const { return m_spanEnd;   }

    // Array access
    size_t            ArraySize() const             { return m_array.size(); }
    const JsonValue & ArrayAt   (size_t index) const { return m_array[index]; }

    // Typed object accessors — key lookup + type check + value extraction
    HRESULT GetString (const string & key, string &           outValue) const { return GetValue (key, JsonType::String, outValue); }
    HRESULT GetNumber (const string & key, double &           outValue) const { return GetValue (key, JsonType::Number, outValue); }
    HRESULT GetInt    (const string & key, int &              outValue) const { return GetValue (key, JsonType::Number, outValue); }
    HRESULT GetUint32 (const string & key, uint32_t &         outValue) const { return GetValue (key, JsonType::Number, outValue); }
    HRESULT GetBool   (const string & key, bool &             outValue) const { return GetValue (key, JsonType::Bool,   outValue); }
    HRESULT GetObject (const string & key, const JsonValue *& outValue) const { return GetValue (key, JsonType::Object, outValue); }
    HRESULT GetArray  (const string & key, const JsonValue *& outValue) const { return GetValue (key, JsonType::Array,  outValue); }

    // Like GetObject, but returns nullptr instead of an error when absent/mismatched.
    const JsonValue * FindObject (const string & key) const;

    // Returns the child value for a key regardless of type (nullptr if absent).
    // Useful for reading a value's source span to splice it in place.
    const JsonValue * FindMember (const string & key) const { return Find (key); }

    const vector<pair<JsonString, JsonValue>> & GetObjectEntries() const { return m_object; }

private:
    // Internal helpers used by typed accessors
    const JsonValue * Find (string_view key) const;

    template <typename T>
    HRESULT GetValue (const string & key, JsonType expected, T & outValue) const;

    JsonType                            m_type      = JsonType::Null;
    bool                                m_bool      = false;
    double                              m_number    = 0.0;
    JsonString                          m_string;
    vector<JsonValue>                   m_array;
    vector<pair<JsonString, JsonValue>> m_object;
    size_t                              m_spanBegin = 0;
    size_t                              m_spanEnd   = 0;
};




////////////////////////////////////////////////////////////////////////////////
//
//  JsonValue::GetValue
//
//  Shared implementation for every typed object accessor: look up the key,
//  verify the value's type, then extract it. The result type is deduced from
//  the caller's out-parameter and dispatched at compile time.
//
////////////////////////////////////////////////////////////////////////////////

template <typename T>
HRESULT JsonValue::GetValue (const string & key, JsonType expected, T & outValue) const
{
    HRESULT           hr    = S_OK;
    const JsonValue * value = Find (key);



    CBREx (value != nullptr,          JSON_E_KEY_MISSING);
    CBREx (value->m_type == expected, JSON_E_TYPE_MISMATCH);

    if constexpr (is_same_v<T, string>)
    {
        outValue = string (value->m_string.View());
    }
    else if constexpr (is_same_v<T, bool>)
    {
        outValue = value->m_bool;
    }
    else if constexpr (is_same_v<T, double>)
    {
        outValue = value->m_number;
    }
    else if constexpr (is_same_v<T, int>)
    {
        outValue = static_cast<int> (value->m_number);
    }
    else if constexpr (is_same_v<T, uint32_t>)
    {
        outValue = static_cast<uint32_t> (value->m_number);
    }
    else
    {
        static_assert (is_same_v<T, const JsonValue *>, "JsonValue::GetValue: unsupported out-parameter type");
        outValue = value;
    }

Error:
    return hr;
}
