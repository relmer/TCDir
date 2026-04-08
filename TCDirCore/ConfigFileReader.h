#pragma once





class CConfigFileReader
{
public:
    HRESULT ReadLines (const string & bytes, vector<wstring> & lines, wstring & errorMessage);

private:
    HRESULT CheckAndStripBom     (string & bytes, wstring & errorMessage);
    HRESULT ConvertUtf8ToWide    (const string & bytes, wstring & wideContent, wstring & errorMessage);
    void    SplitLines           (const wstring & content, vector<wstring> & lines);
};
