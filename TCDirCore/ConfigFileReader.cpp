#include "pch.h"
#include "ConfigFileReader.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CConfigFileReader::CConfigFileReader (istream *)
//
//  Constructor that accepts an optional stream override for unit testing.
//
////////////////////////////////////////////////////////////////////////////////

CConfigFileReader::CConfigFileReader (istream * pStream) :
    m_pStream (pStream)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfigFileReader::ReadLines
//
//  Read a UTF-8 text file, handle BOM, convert to wide string, split into
//  lines.  Returns S_OK on success, S_FALSE if file not found, E_FAIL with
//  errorMessage on I/O or encoding error.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConfigFileReader::ReadLines (
    const wstring   & path,
    vector<wstring> & lines,
    wstring         & errorMessage)
{
    HRESULT hr           = S_OK;
    string  bytes;
    wstring wideContent;



    lines.clear();
    errorMessage.clear();

    hr = ReadAllBytes (path, bytes, errorMessage);
    BAIL_OUT_IF (hr == S_FALSE, S_FALSE);
    CHR (hr);

    hr = CheckAndStripBom (bytes, errorMessage);
    CHR (hr);

    if (bytes.empty())
    {
        BAIL_OUT_IF (TRUE, S_OK);
    }

    hr = ConvertUtf8ToWide (bytes, wideContent, errorMessage);
    CHR (hr);

    SplitLines (wideContent, lines);


Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfigFileReader::ReadAllBytes
//
//  Read raw bytes from a file (or injected stream).  Returns S_FALSE if the
//  file does not exist, E_FAIL on other I/O errors.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConfigFileReader::ReadAllBytes (
    const wstring & path,
    string        & bytes,
    wstring       & errorMessage)
{
    HRESULT hr = S_OK;



    if (m_pStream != nullptr)
    {
        //
        // Test path: read from injected stream
        //

        ostringstream oss;
        oss << m_pStream->rdbuf();
        bytes = oss.str();

        BAIL_OUT_IF (TRUE, S_OK);
    }

    //
    // Production path: open file via ifstream
    //

    {
        ifstream file (path, ios::binary);

        if (!file.is_open())
        {
            //
            // Distinguish not-found from other errors.
            // Use std::filesystem::exists to check — _doserrno is unreliable.
            //

            if (!filesystem::exists (path))
            {
                BAIL_OUT_IF (TRUE, S_FALSE);
            }

            errorMessage = L"Cannot open file";
            CBR (FALSE);
        }

        ostringstream oss;
        oss << file.rdbuf();
        bytes = oss.str();

        CBR (file.good() || file.eof());
    }


Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfigFileReader::CheckAndStripBom
//
//  Detect and handle BOM:
//  - UTF-8 BOM (EF BB BF): strip it
//  - UTF-16 LE BOM (FF FE): reject with error
//  - UTF-16 BE BOM (FE FF): reject with error
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConfigFileReader::CheckAndStripBom (
    string  & bytes,
    wstring & errorMessage)
{
    HRESULT hr = S_OK;



    if (bytes.size() >= 3 &&
        static_cast<unsigned char>(bytes[0]) == 0xEF &&
        static_cast<unsigned char>(bytes[1]) == 0xBB &&
        static_cast<unsigned char>(bytes[2]) == 0xBF)
    {
        // UTF-8 BOM — strip it
        bytes.erase (0, 3);
        BAIL_OUT_IF (TRUE, S_OK);
    }

    if (bytes.size() >= 2)
    {
        unsigned char b0 = static_cast<unsigned char>(bytes[0]);
        unsigned char b1 = static_cast<unsigned char>(bytes[1]);

        if (b0 == 0xFF && b1 == 0xFE)
        {
            errorMessage = L"Unsupported encoding: UTF-16 LE (config file must be UTF-8)";
            CBR (FALSE);
        }

        if (b0 == 0xFE && b1 == 0xFF)
        {
            errorMessage = L"Unsupported encoding: UTF-16 BE (config file must be UTF-8)";
            CBR (FALSE);
        }
    }


Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfigFileReader::ConvertUtf8ToWide
//
//  Convert raw UTF-8 bytes to a wide string using MultiByteToWideChar.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConfigFileReader::ConvertUtf8ToWide (
    const string & bytes,
    wstring      & wideContent,
    wstring      & errorMessage)
{
    HRESULT hr     = S_OK;
    int     cchLen = 0;



    cchLen = MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS,
                                  bytes.data(), static_cast<int>(bytes.size()),
                                  nullptr, 0);
    if (cchLen == 0)
    {
        errorMessage = L"Failed to convert file from UTF-8";
        CBR (FALSE);
    }

    wideContent.resize (cchLen);

    cchLen = MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS,
                                  bytes.data(), static_cast<int>(bytes.size()),
                                  wideContent.data(), cchLen);
    CBR (cchLen > 0);


Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConfigFileReader::SplitLines
//
//  Split a wide string on \r\n, \n, or \r line endings.
//
////////////////////////////////////////////////////////////////////////////////

void CConfigFileReader::SplitLines (
    const wstring   & content,
    vector<wstring> & lines)
{
    size_t pos   = 0;
    size_t start = 0;
    size_t len   = content.size();



    while (pos < len)
    {
        if (content[pos] == L'\r')
        {
            lines.push_back (content.substr (start, pos - start));

            if (pos + 1 < len && content[pos + 1] == L'\n')
            {
                pos++;  // skip \n of \r\n
            }

            start = pos + 1;
        }
        else if (content[pos] == L'\n')
        {
            lines.push_back (content.substr (start, pos - start));
            start = pos + 1;
        }

        pos++;
    }

    // Last line (no trailing newline)
    if (start <= len)
    {
        lines.push_back (content.substr (start));
    }
}
