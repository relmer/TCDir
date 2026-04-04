#pragma once





struct IConfigFileReader
{
    virtual ~IConfigFileReader () = default;

    //
    //  ReadLines
    //
    //  Read a UTF-8 text file and split into lines.
    //  Returns S_OK on success, S_FALSE if file not found, E_FAIL on I/O error.
    //  On E_FAIL, errorMessage describes the issue.
    //

    virtual HRESULT ReadLines (const wstring & path, vector<wstring> & lines, wstring & errorMessage) = 0;
};





class CConfigFileReader : public IConfigFileReader
{
public:
    CConfigFileReader (void) = default;
    explicit CConfigFileReader (istream * pStream);

    HRESULT ReadLines (const wstring & path, vector<wstring> & lines, wstring & errorMessage) override;

private:
    HRESULT ReadAllBytes         (const wstring & path, string & bytes, wstring & errorMessage);
    HRESULT CheckAndStripBom     (string & bytes, wstring & errorMessage);
    HRESULT ConvertUtf8ToWide    (const string & bytes, wstring & wideContent, wstring & errorMessage);
    void    SplitLines           (const wstring & content, vector<wstring> & lines);

    istream * m_pStream = nullptr;
};
