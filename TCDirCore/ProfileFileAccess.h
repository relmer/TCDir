#pragma once





//
// Suffix for the scratch file used to make profile writes atomic
//

static constexpr LPCWSTR k_pszProfileTempSuffix = L".tcdir-tmp";





////////////////////////////////////////////////////////////////////////////////
//
//  IProfileFileAccess
//
//  Injectable abstraction over the raw byte-level file I/O used when reading
//  and writing PowerShell profile files.  Production code uses
//  CProfileFileAccessReal; tests substitute CProfileFileAccessMock so that I/O
//  failure modes can be exercised without touching the real file system.
//
////////////////////////////////////////////////////////////////////////////////

class IProfileFileAccess
{
public:
    virtual ~IProfileFileAccess() = default;

    virtual HRESULT ReadAllBytes  (const wstring & strPath, string & strBytes) = 0;
    virtual HRESULT WriteAllBytes (const wstring & strPath, const string & strBytes) = 0;
};





////////////////////////////////////////////////////////////////////////////////
//
//  CProfileFileAccessReal
//
//  Production implementation — delegates to the C runtime file APIs.
//
////////////////////////////////////////////////////////////////////////////////

class CProfileFileAccessReal : public IProfileFileAccess
{
public:
    HRESULT ReadAllBytes  (const wstring & strPath, string & strBytes) override;
    HRESULT WriteAllBytes (const wstring & strPath, const string & strBytes) override;
};





////////////////////////////////////////////////////////////////////////////////
//
//  CProfileFileAccessReal::ReadAllBytes
//
//  Reads a file in full.  Succeeds ONLY when every byte was read.
//
//  Every seek/tell/read is checked.  An unchecked one here is how a profile
//  can come back silently empty and then be written straight back over the
//  user's real file, destroying it.
//
////////////////////////////////////////////////////////////////////////////////

inline HRESULT CProfileFileAccessReal::ReadAllBytes (const wstring & strPath, string & strBytes)
{
    HRESULT hr     = S_OK;
    FILE *  pf     = nullptr;
    long    cbFile = 0;
    size_t  cbRead = 0;
    int     iSeek  = 0;
    int     iError = 0;



    strBytes.clear();

    _wfopen_s (&pf, strPath.c_str(), L"rb");
    CBREx (pf != nullptr, HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND));

    iSeek = fseek (pf, 0, SEEK_END);
    CBREx (iSeek == 0, HRESULT_FROM_WIN32 (ERROR_READ_FAULT));

    cbFile = ftell (pf);
    CBREx (cbFile >= 0, HRESULT_FROM_WIN32 (ERROR_READ_FAULT));

    iSeek = fseek (pf, 0, SEEK_SET);
    CBREx (iSeek == 0, HRESULT_FROM_WIN32 (ERROR_READ_FAULT));

    if (cbFile > 0)
    {
        strBytes.resize (static_cast<size_t>(cbFile));

        cbRead = fread (strBytes.data(), 1, strBytes.size(), pf);

        //
        // A short read is a failure, never a silently truncated buffer.
        //

        CBREx (cbRead == strBytes.size(), HRESULT_FROM_WIN32 (ERROR_READ_FAULT));

        iError = ferror (pf);
        CBREx (iError == 0, HRESULT_FROM_WIN32 (ERROR_READ_FAULT));
    }

Error:
    if (pf != nullptr)
    {
        fclose (pf);
    }

    if (FAILED (hr))
    {
        strBytes.clear();
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CProfileFileAccessReal::WriteAllBytes
//
//  Writes to a scratch file and renames it over the target, so an interrupted
//  or failed write cannot leave a partially written profile behind.
//
////////////////////////////////////////////////////////////////////////////////

inline HRESULT CProfileFileAccessReal::WriteAllBytes (const wstring & strPath, const string & strBytes)
{
    HRESULT    hr        = S_OK;
    FILE *     pf        = nullptr;
    wstring    strTemp;
    size_t     cbWritten = 0;
    int        iFlush    = 0;
    error_code ec;



    strTemp = strPath + k_pszProfileTempSuffix;

    _wfopen_s (&pf, strTemp.c_str(), L"wb");
    CBREx (pf != nullptr, HRESULT_FROM_WIN32 (ERROR_ACCESS_DENIED));

    if (!strBytes.empty())
    {
        cbWritten = fwrite (strBytes.data(), 1, strBytes.size(), pf);
        CBREx (cbWritten == strBytes.size(), HRESULT_FROM_WIN32 (ERROR_WRITE_FAULT));
    }

    iFlush = fflush (pf);
    CBREx (iFlush == 0, HRESULT_FROM_WIN32 (ERROR_WRITE_FAULT));

    fclose (pf);
    pf = nullptr;

    filesystem::rename (strTemp, strPath, ec);
    CBREx (!ec, HRESULT_FROM_WIN32 (ERROR_WRITE_FAULT));

Error:
    if (pf != nullptr)
    {
        fclose (pf);
    }

    if (FAILED (hr))
    {
        filesystem::remove (strTemp, ec);
    }

    return hr;
}
