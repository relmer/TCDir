#include "pch.h"

#include "JsonParser.h"





////////////////////////////////////////////////////////////////////////////////
//
//  JsonParser::JsonParser
//
////////////////////////////////////////////////////////////////////////////////

JsonParser::JsonParser (const string & input, JsonSyntax syntax)
    : m_input  (input),
      m_syntax (syntax)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  JsonParser::Parse
//
//  Strict JSON: no comments, no trailing commas.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT JsonParser::Parse (const string & input, JsonValue & outValue, JsonParseError & outError)
{
    HRESULT    hr     = S_OK;
    JsonParser parser   (input, JsonSyntax::Strict);



    hr = parser.ParseValue (outValue);
    CHRF (hr, outError = parser.m_error);

    parser.SkipWhitespace();

    CBRF (parser.AtEnd(), parser.SetError ("Unexpected content after JSON value"));

Error:
    if (FAILED (hr))
    {
        outError = parser.m_error;
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  JsonParser::ParseJsonc
//
//  JSONC: tolerates // and block comments and trailing commas, as used by
//  Windows Terminal's settings.json.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT JsonParser::ParseJsonc (const string & input, JsonValue & outValue, JsonParseError & outError)
{
    HRESULT    hr     = S_OK;
    JsonParser parser   (input, JsonSyntax::Jsonc);



    hr = parser.ParseValue (outValue);
    CHRF (hr, outError = parser.m_error);

    parser.SkipWhitespace();

    CBRF (parser.AtEnd(), parser.SetError ("Unexpected content after JSON value"));

Error:
    if (FAILED (hr))
    {
        outError = parser.m_error;
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  JsonParser::ParseValue
//
//  Records the value's source span [begin, end) covering its full text.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT JsonParser::ParseValue (JsonValue & outValue)
{
    HRESULT    hr       = S_OK;
    char       ch       = 0;
    JsonString str;
    size_t     posStart = 0;



    m_depth++;
    CBRF (m_depth <= s_kMaxDepth, SetError ("Maximum nesting depth exceeded"));

    SkipWhitespace();

    CBR (!AtEnd());

    posStart = m_pos;
    ch       = Peek();

    switch (ch)
    {
        case '"':
            hr = ParseString (str);
            CHR (hr);
            outValue = JsonValue (move (str));
            break;

        case '{':
            hr = ParseObject (outValue);
            CHR (hr);
            break;

        case '[':
            hr = ParseArray (outValue);
            CHR (hr);
            break;

        case 't':
            hr = ParseKeyword ("true", outValue);
            CHR (hr);
            outValue = JsonValue (true);
            break;

        case 'f':
            hr = ParseKeyword ("false", outValue);
            CHR (hr);
            outValue = JsonValue (false);
            break;

        case 'n':
            hr = ParseKeyword ("null", outValue);
            CHR (hr);
            outValue = JsonValue (nullptr);
            break;

        default:
            CBRF (ch == '-' || (ch >= '0' && ch <= '9'), SetError (format ("Unexpected character '{}'", ch)));
            hr = ParseNumber (outValue);
            CHR (hr);
            break;
    }

    outValue.m_spanBegin = posStart;
    outValue.m_spanEnd   = m_pos;

Error:
    m_depth--;
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  JsonParser::ParseString
//
//  Zero-copy by default: the result is a string_view into the input buffer.  An
//  owned, decoded copy is produced ONLY when the text contains escape sequences.
//  A single pass collects the raw run and switches to decoding the moment the
//  first backslash is seen.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT JsonParser::ParseString (JsonString & outStr)
{
    HRESULT       hr           = S_OK;
    char          ch           = 0;
    char          esc          = 0;
    string        hex;
    unsigned long code         = 0;
    bool          fDone        = false;
    bool          fDecoding    = false;
    int           i            = 0;
    size_t        contentBegin = 0;
    string        decoded;



    CBRA (Peek() == '"');
    Advance();

    contentBegin = m_pos;

    while (!AtEnd() && !fDone)
    {
        ch = Advance();

        if (ch == '"')
        {
            fDone = true;
            continue;
        }

        if (ch != '\\')
        {
            if (fDecoding)
            {
                decoded += ch;
            }
            
            continue;
        }

        // First escape: switch from zero-copy to building a decoded buffer,
        // seeding it with the raw run gathered so far.
        if (!fDecoding)
        {
            fDecoding = true;
            decoded.assign (m_input, contentBegin, (m_pos - 1) - contentBegin);
        }

        CBR (!AtEnd());

        esc = Advance();

        switch (esc)
        {
            case '"':  decoded += '"';  break;
            case '\\': decoded += '\\'; break;
            case '/':  decoded += '/';  break;
            case 'b':  decoded += '\b'; break;
            case 'f':  decoded += '\f'; break;
            case 'n':  decoded += '\n'; break;
            case 'r':  decoded += '\r'; break;
            case 't':  decoded += '\t'; break;

            case 'u':
                hex.clear();

                for (i = 0; i < 4; i++)
                {
                    CBR (!AtEnd());
                    hex += Advance();
                }

                code = strtoul (hex.c_str(), nullptr, 16);
                if (code < 0x80)
                {
                    decoded += static_cast<char> (code);
                }
                break;

            default:
                CBRF (false, SetError (format ("Invalid escape sequence '\\{}'", esc)));
                break;
        }
    }

    CBRF (fDone, SetError ("Unterminated string"));

    if (fDecoding)
    {
        outStr = JsonString (move (decoded));
    }
    else
    {
        // Content is the half-open range [contentBegin, closing-quote); the
        // closing quote sits at m_pos - 1 because we just advanced past it.
        outStr = JsonString (string_view (m_input).substr (contentBegin, (m_pos - 1) - contentBegin));
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  JsonParser::ParseNumber
//
////////////////////////////////////////////////////////////////////////////////

HRESULT JsonParser::ParseNumber (JsonValue & outValue)
{
    HRESULT hr    = S_OK;
    size_t  start = m_pos;
    double  value = 0.0;



    if (m_pos + 2 < m_input.size() && m_input[m_pos] == '0' &&
        (m_input[m_pos + 1] == 'x' || m_input[m_pos + 1] == 'X'))
    {
        Advance();
        Advance();

        while (!AtEnd() && isxdigit (static_cast<unsigned char> (Peek())))
        {
            Advance();
        }

        value = static_cast<double> (strtoul (m_input.c_str() + start + 2, nullptr, 16));
    }
    else
    {
        if (Peek() == '-')
        {
            Advance();
        }

        while (!AtEnd() && isdigit (static_cast<unsigned char> (Peek())))
        {
            Advance();
        }

        if (!AtEnd() && Peek() == '.')
        {
            Advance();

            while (!AtEnd() && isdigit (static_cast<unsigned char> (Peek())))
            {
                Advance();
            }
        }

        if (!AtEnd() && (Peek() == 'e' || Peek() == 'E'))
        {
            Advance();

            if (!AtEnd() && (Peek() == '+' || Peek() == '-'))
            {
                Advance();
            }

            while (!AtEnd() && isdigit (static_cast<unsigned char> (Peek())))
            {
                Advance();
            }
        }

        value = strtod (m_input.c_str() + start, nullptr);
    }

    outValue = JsonValue (value);

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  JsonParser::ParseObject
//
////////////////////////////////////////////////////////////////////////////////

HRESULT JsonParser::ParseObject (JsonValue & outValue)
{
    HRESULT                             hr    = S_OK;
    vector<pair<JsonString, JsonValue>> entries;
    JsonString                          key;
    JsonValue                           val;
    bool                                fDone = false;



    CBRA (Peek() == '{');
    Advance();

    entries.reserve (8);

    SkipWhitespace();

    if (!AtEnd() && Peek() == '}')
    {
        Advance();
        fDone = true;
    }

    while (!fDone)
    {
        SkipWhitespace();
        CBR (!AtEnd());

        CBRF (Peek() == '"', SetError ("Expected string key in object"));

        hr = ParseString (key);
        CHR (hr);

        SkipWhitespace();
        CBR (!AtEnd() && Peek() == ':');
        Advance();

        val = JsonValue();
        hr = ParseValue (val);
        CHR (hr);

        entries.emplace_back (move (key), move (val));

        SkipWhitespace();
        CBR (!AtEnd());

        if (Peek() == '}')
        {
            Advance();
            fDone = true;
            continue;
        }

        CBRF (Peek() == ',', SetError ("Expected ',' or '}' in object"));
        Advance();

        if (m_syntax == JsonSyntax::Jsonc)
        {
            SkipWhitespace();

            if (!AtEnd() && Peek() == '}')
            {
                Advance();
                fDone = true;
            }
        }
    }

    outValue = JsonValue (move (entries));

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  JsonParser::ParseArray
//
////////////////////////////////////////////////////////////////////////////////

HRESULT JsonParser::ParseArray (JsonValue & outValue)
{
    HRESULT           hr    = S_OK;
    vector<JsonValue> elements;
    JsonValue         val;
    bool              fDone = false;



    CBRA (Peek() == '[');
    Advance();

    elements.reserve (8);

    SkipWhitespace();

    if (!AtEnd() && Peek() == ']')
    {
        Advance();
        fDone = true;
    }

    while (!fDone)
    {
        val = JsonValue();
        hr = ParseValue (val);
        CHR (hr);

        elements.push_back (move (val));

        SkipWhitespace();
        CBR (!AtEnd());

        if (Peek() == ']')
        {
            Advance();
            fDone = true;
            continue;
        }

        CBRF (Peek() == ',', SetError ("Expected ',' or ']' in array"));
        Advance();

        if (m_syntax == JsonSyntax::Jsonc)
        {
            SkipWhitespace();

            if (!AtEnd() && Peek() == ']')
            {
                Advance();
                fDone = true;
            }
        }
    }

    outValue = JsonValue (move (elements));

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  JsonParser::ParseKeyword
//
////////////////////////////////////////////////////////////////////////////////

HRESULT JsonParser::ParseKeyword (const char * keyword, JsonValue & outValue)
{
    HRESULT hr  = S_OK;
    size_t  len = strlen (keyword);
    size_t  i   = 0;



    UNREFERENCED_PARAMETER (outValue);

    for (i = 0; i < len; i++)
    {
        CBRF (!AtEnd() && Peek() == keyword[i], SetError (format ("Expected '{}'", keyword)));
        Advance();
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  JsonParser::SkipWhitespace
//
//  In JSONC mode also skips // line comments and block comments.
//
////////////////////////////////////////////////////////////////////////////////

void JsonParser::SkipWhitespace()
{
    bool fSkipping = true;



    while (fSkipping && !AtEnd())
    {
        char ch = Peek();

        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n')
        {
            Advance();
        }
        else if (m_syntax == JsonSyntax::Jsonc && ch == '/' && m_pos + 1 < m_input.size() && m_input[m_pos + 1] == '/')
        {
            while (!AtEnd() && Peek() != '\n')
            {
                Advance();
            }
        }
        else if (m_syntax == JsonSyntax::Jsonc && ch == '/' && m_pos + 1 < m_input.size() && m_input[m_pos + 1] == '*')
        {
            Advance();
            Advance();

            while (!AtEnd() && !(Peek() == '*' && m_pos + 1 < m_input.size() && m_input[m_pos + 1] == '/'))
            {
                Advance();
            }

            if (!AtEnd())
            {
                Advance();
                Advance();
            }
        }
        else
        {
            fSkipping = false;
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  JsonParser::Peek
//
////////////////////////////////////////////////////////////////////////////////

char JsonParser::Peek() const
{
    return m_input[m_pos];
}





////////////////////////////////////////////////////////////////////////////////
//
//  JsonParser::Advance
//
////////////////////////////////////////////////////////////////////////////////

char JsonParser::Advance()
{
    char ch = m_input[m_pos++];



    if (ch == '\n')
    {
        m_line++;
        m_column = 1;
    }
    else
    {
        m_column++;
    }

    return ch;
}





////////////////////////////////////////////////////////////////////////////////
//
//  JsonParser::AtEnd
//
////////////////////////////////////////////////////////////////////////////////

bool JsonParser::AtEnd() const
{
    return m_pos >= m_input.size();
}





////////////////////////////////////////////////////////////////////////////////
//
//  JsonParser::SetError
//
////////////////////////////////////////////////////////////////////////////////

void JsonParser::SetError (const string & msg)
{
    m_error.line    = m_line;
    m_error.column  = m_column;
    m_error.message = msg;
}
