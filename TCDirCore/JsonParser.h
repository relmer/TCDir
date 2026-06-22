#pragma once

#include "JsonValue.h"





////////////////////////////////////////////////////////////////////////////////
//
//  JsonSyntax
//
//  Selects how strict the parser is.  Strict is standard JSON; Jsonc also
//  tolerates // and block comments and trailing commas (as Windows Terminal's
//  settings.json uses).
//
////////////////////////////////////////////////////////////////////////////////

enum class JsonSyntax
{
    Strict,
    Jsonc
};





////////////////////////////////////////////////////////////////////////////////
//
//  JsonParseError
//
////////////////////////////////////////////////////////////////////////////////

struct JsonParseError
{
    int    line   = 0;
    int    column = 0;
    string message;
};





////////////////////////////////////////////////////////////////////////////////
//
//  JsonParser
//
////////////////////////////////////////////////////////////////////////////////

class JsonParser
{
public:
    //
    // Parse the given input into outValue.  The result BORROWS from input: string
    // values are zero-copy views into the input buffer, so input must outlive
    // outValue.  For an owning, lifetime-safe result, use JsonDocument instead.
    //
    static HRESULT Parse      (const string & input, JsonValue & outValue, JsonParseError & outError);
    static HRESULT ParseJsonc (const string & input, JsonValue & outValue, JsonParseError & outError);

    // Forbid binding to a temporary: its bytes would die before the borrowing
    // result is used.  Name the buffer (or use JsonDocument) instead.
    static HRESULT Parse      (string && input, JsonValue & outValue, JsonParseError & outError) = delete;
    static HRESULT ParseJsonc (string && input, JsonValue & outValue, JsonParseError & outError) = delete;

private:
    JsonParser (const string & input, JsonSyntax syntax);

    HRESULT ParseValue   (JsonValue & outValue);
    HRESULT ParseString  (JsonString & outStr);
    HRESULT ParseNumber  (JsonValue & outValue);
    HRESULT ParseObject  (JsonValue & outValue);
    HRESULT ParseArray   (JsonValue & outValue);
    HRESULT ParseKeyword (const char * keyword, JsonValue & outValue);

    void SkipWhitespace();
    char Peek() const;
    char Advance();
    bool AtEnd() const;
    void SetError       (const string & msg);

    // Maximum container nesting depth.  Bounds recursion so pathological deeply
    // nested input fails gracefully instead of overflowing the stack.  Far deeper
    // than any real GitHub API or Windows Terminal settings payload.
    static constexpr int s_kMaxDepth = 200;

    const string   & m_input;
    size_t           m_pos     = 0;
    int              m_line    = 1;
    int              m_column  = 1;
    int              m_depth   = 0;
    JsonSyntax       m_syntax  = JsonSyntax::Strict;
    JsonParseError   m_error;
};





////////////////////////////////////////////////////////////////////////////////
//
//  JsonDocument
//
//  Owns the source text and the parsed tree together, so the zero-copy string
//  views in the tree can never dangle.  Move the source in (by value) to parse
//  it; the document keeps the bytes alive for as long as the tree is used.
//
//  The buffer is held behind a unique_ptr so moving the document leaves the
//  views — which point into that heap buffer — valid (a moved std::string would
//  relocate small/SSO contents and invalidate them).
//
////////////////////////////////////////////////////////////////////////////////

class JsonDocument
{
public:
    HRESULT Parse      (string source, JsonParseError & outError) { return ParseImpl (move (source), outError, false); }
    HRESULT ParseJsonc (string source, JsonParseError & outError) { return ParseImpl (move (source), outError, true);  }

    const JsonValue & Root() const { return m_root; }
    const string    & Source() const { return *m_source; }

private:
    HRESULT ParseImpl (string source, JsonParseError & outError, bool fJsonc)
    {
        m_source = make_unique<string> (move (source));
        m_root   = JsonValue();

        return fJsonc
            ? JsonParser::ParseJsonc (*m_source, m_root, outError)
            : JsonParser::Parse      (*m_source, m_root, outError);
    }

    unique_ptr<string> m_source = make_unique<string> ();
    JsonValue          m_root;
};
