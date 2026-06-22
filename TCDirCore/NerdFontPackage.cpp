#include "pch.h"

#include "NerdFontPackage.h"
#include "AutoHandle.h"
#include "JsonParser.h"





//
// HTTP status codes that indicate GitHub API rate limiting on this endpoint.
//

static constexpr DWORD s_kHttpOk          = 200;
static constexpr DWORD s_kHttpForbidden   = 403;
static constexpr DWORD s_kHttpTooManyReqs = 429;





////////////////////////////////////////////////////////////////////////////////
//
//  EscapePsSingleQuoted
//
//  Doubles single quotes so a path can sit safely inside a PowerShell
//  single-quoted string literal.
//
////////////////////////////////////////////////////////////////////////////////

static wstring EscapePsSingleQuoted (LPCWSTR psz)
{
    wstring strResult;



    for (LPCWSTR p = psz; *p != L'\0'; ++p)
    {
        strResult += *p;

        if (*p == L'\'')
        {
            strResult += L'\'';
        }
    }

    return strResult;
}





////////////////////////////////////////////////////////////////////////////////
//
//  BuildExtractCommand
//
//  Builds the powershell command line that extracts a zip via the .NET
//  ZipFile API (avoids module auto-load dependencies).
//
////////////////////////////////////////////////////////////////////////////////

static wstring BuildExtractCommand (LPCWSTR pszZip, LPCWSTR pszDest)
{
    wstring strZip  = EscapePsSingleQuoted (pszZip);
    wstring strDest = EscapePsSingleQuoted (pszDest);



    return format (L"powershell -NoProfile -Command \"$ErrorActionPreference='Stop'; "
                   L"Add-Type -AssemblyName System.IO.Compression.FileSystem; "
                   L"[System.IO.Compression.ZipFile]::ExtractToDirectory('{}','{}')\"",
                   strZip, strDest);
}





////////////////////////////////////////////////////////////////////////////////
//
//  ParseTagName
//
//  Extracts the "tag_name" string from a GitHub releases/latest JSON response.
//
////////////////////////////////////////////////////////////////////////////////

static HRESULT ParseTagName (string strResponse, wstring & strTag)
{
    HRESULT        hr     = S_OK;
    JsonDocument   doc;
    JsonParseError jsonErr;
    string         strTagUtf8;
    int            cchTag = 0;



    strTag.clear();

    hr = doc.Parse (move (strResponse), jsonErr);
    CHR (hr);

    hr = doc.Root().GetString ("tag_name", strTagUtf8);
    CHR (hr);

    CBR (!strTagUtf8.empty());

    cchTag = MultiByteToWideChar (CP_UTF8, 0, strTagUtf8.c_str(), static_cast<int> (strTagUtf8.length()), NULL, 0);
    CBR (cchTag > 0);

    strTag.resize (cchTag);
    MultiByteToWideChar (CP_UTF8, 0, strTagUtf8.c_str(), static_cast<int> (strTagUtf8.length()), &strTag[0], cchTag);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontPackage::ResolveLatestTag
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontPackage::ResolveLatestTag (wstring & strTag, bool & fRateLimited)
{
    HRESULT            hr               = S_OK;
    AutoInternetHandle hInternet;
    AutoInternetHandle hConnect;
    AutoInternetHandle hRequest;
    DWORD              dwStatus         = 0;
    DWORD              dwStatusSize     = sizeof (dwStatus);
    DWORD              dwDownloaded     = 0;
    BOOL               fSuccess         = FALSE;
    CHAR               szResponse[2048] = { };
    string             strResponse;



    strTag.clear();
    fRateLimited = false;

    hInternet = InternetOpenW (NerdFontConst::kpszUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    CBR (hInternet != NULL);

    hConnect = InternetConnectW (hInternet, NerdFontConst::kpszGithubApiHost, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, INTERNET_FLAG_SECURE, 0);
    CBR (hConnect != NULL);

    hRequest = HttpOpenRequestW (hConnect, L"GET", NerdFontConst::kpszLatestReleasePath, NULL, NULL, NULL, INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_UI, 0);
    CBR (hRequest != NULL);

    fSuccess = HttpSendRequestW (hRequest, NULL, 0, NULL, 0);
    CWR (fSuccess);

    fSuccess = HttpQueryInfoW (hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &dwStatus, &dwStatusSize, NULL);
    CWR (fSuccess);

    fRateLimited = (dwStatus == s_kHttpForbidden || dwStatus == s_kHttpTooManyReqs);
    CBR (!fRateLimited);
    CBR (dwStatus == s_kHttpOk);

    while (InternetReadFile (hRequest, szResponse, sizeof (szResponse) - 1, &dwDownloaded) && dwDownloaded != 0)
    {
        szResponse[dwDownloaded] = '\0';
        strResponse.append (szResponse, dwDownloaded);
    }

    hr = ParseTagName (move (strResponse), strTag);
    CHR (hr);

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontPackage::Download
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontPackage::Download (LPCWSTR pszDestDir, LPCWSTR pszReleaseTag, wstring & strZipPath)
{
    HRESULT hr                   = S_OK;
    wchar_t szZipPath[MAX_PATH]  = { };
    LPCWSTR pszZipName           = NerdFontConst::kpszZipName;
    wstring strUrl;



    strZipPath.clear();

    wcsncpy_s (szZipPath, ARRAYSIZE (szZipPath), pszDestDir, _TRUNCATE);
    hr = PathCchAppend (szZipPath, ARRAYSIZE (szZipPath), pszZipName);
    CHRA (hr);

    strUrl = vformat (wstring_view (NerdFontConst::kpszReleaseUrlFormat), make_wformat_args (pszReleaseTag, pszZipName));

    hr = URLDownloadToFileW (NULL, strUrl.c_str(), szZipPath, 0, NULL);
    CHR (hr);

    strZipPath = szZipPath;

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CNerdFontPackage::Extract
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CNerdFontPackage::Extract (LPCWSTR pszZipPath, LPCWSTR pszDestDir)
{
    HRESULT             hr                   = S_OK;
    wchar_t             szFullZip[MAX_PATH]  = { };
    wchar_t             szFullDest[MAX_PATH] = { };
    wstring             strCmd;
    STARTUPINFOW        si                   = { };
    PROCESS_INFORMATION pi                   = { };
    AutoHandle          hProcess;
    AutoHandle          hThread;
    BOOL                fSuccess             = FALSE;
    DWORD               dwWait               = 0;
    DWORD               dwExitCode           = 0;
    DWORD               dwLen                = 0;



    dwLen = GetFullPathNameW (pszZipPath, ARRAYSIZE (szFullZip), szFullZip, NULL);
    CWR (dwLen);

    dwLen = GetFullPathNameW (pszDestDir, ARRAYSIZE (szFullDest), szFullDest, NULL);
    CWR (dwLen);

    strCmd = BuildExtractCommand (szFullZip, szFullDest);

    si.cb          = sizeof (si);
    si.dwFlags     = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    fSuccess = CreateProcessW (NULL, strCmd.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    CWR (fSuccess);

    hProcess = pi.hProcess;
    hThread  = pi.hThread;

    dwWait = WaitForSingleObject (hProcess, INFINITE);
    CBR (dwWait == WAIT_OBJECT_0);

    fSuccess = GetExitCodeProcess (hProcess, &dwExitCode);
    CWR (fSuccess);
    CBR (dwExitCode == 0);

Error:
    return hr;
}





