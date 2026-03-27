#include "pch.h"
#include "ProfileFileManager.h"





//
// BOM byte sequences
//

static constexpr unsigned char k_rgUtf8Bom[]  = { 0xEF, 0xBB, 0xBF };
static constexpr unsigned char k_rgUtf16LE[]  = { 0xFF, 0xFE };
static constexpr unsigned char k_rgUtf16BE[]  = { 0xFE, 0xFF };

//
// Marker strings for finding alias blocks
//

static constexpr LPCWSTR k_pszHeaderMarker = L"#  TCDir Aliases";
static constexpr LPCWSTR k_pszFooterMarker = L"#  End TCDir Aliases";





////////////////////////////////////////////////////////////////////////////////
//
//  CProfileFileManager::ReadProfileFile
//
//  Reads a UTF-8 file (with or without BOM) into a vector of lines.
//  Detects and rejects UTF-16 files. Sets fHasBom if UTF-8 BOM present.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CProfileFileManager::ReadProfileFile (const wstring & strPath, vector<wstring> & rgLines, bool & fHasBom)
{
    HRESULT hr     = S_OK;
    FILE *  pf     = nullptr;
    long    cbFile = 0;
    string  strRaw;



    rgLines.clear();
    fHasBom = false;

    //
    // Read the raw bytes
    //

    _wfopen_s (&pf, strPath.c_str(), L"rb");
    CBRAEx (pf != nullptr, HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND));

    fseek (pf, 0, SEEK_END);
    cbFile = ftell (pf);
    fseek (pf, 0, SEEK_SET);

    if (cbFile > 0)
    {
        strRaw.resize (static_cast<size_t>(cbFile));
        size_t cbRead = fread (strRaw.data(), 1, strRaw.size(), pf);
        strRaw.resize (cbRead);
    }

    fclose (pf);
    pf = nullptr;

    //
    // Check BOM
    //

    if (strRaw.size() >= 2)
    {
        auto pb = reinterpret_cast<const unsigned char *>(strRaw.data());

        if ((pb[0] == k_rgUtf16LE[0] && pb[1] == k_rgUtf16LE[1]) ||
            (pb[0] == k_rgUtf16BE[0] && pb[1] == k_rgUtf16BE[1]))
        {
            //
            // UTF-16 — bail with clear error
            //

            CBRAEx (false, HRESULT_FROM_WIN32 (ERROR_UNSUPPORTED_TYPE));
        }
    }

    if (strRaw.size() >= 3)
    {
        auto pb = reinterpret_cast<const unsigned char *>(strRaw.data());

        if (pb[0] == k_rgUtf8Bom[0] && pb[1] == k_rgUtf8Bom[1] && pb[2] == k_rgUtf8Bom[2])
        {
            fHasBom = true;
            strRaw.erase (0, 3);
        }
    }

    //
    // Convert UTF-8 to wstring lines
    //

    if (!strRaw.empty())
    {
        int cchNeeded = MultiByteToWideChar (CP_UTF8, 0, strRaw.data(), static_cast<int>(strRaw.size()), nullptr, 0);

        CBRAEx (cchNeeded > 0, HRESULT_FROM_WIN32 (GetLastError()));

        wstring strWide (static_cast<size_t>(cchNeeded), L'\0');

        MultiByteToWideChar (CP_UTF8, 0, strRaw.data(), static_cast<int>(strRaw.size()), strWide.data(), cchNeeded);

        //
        // Split into lines (handle \r\n, \n, \r)
        //

        size_t pos = 0;

        while (pos < strWide.size())
        {
            size_t end = strWide.find_first_of (L"\r\n", pos);

            if (end == wstring::npos)
            {
                rgLines.push_back (strWide.substr (pos));
                break;
            }

            rgLines.push_back (strWide.substr (pos, end - pos));

            if (end + 1 < strWide.size() && strWide[end] == L'\r' && strWide[end + 1] == L'\n')
            {
                pos = end + 2;
            }
            else
            {
                pos = end + 1;
            }
        }
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CProfileFileManager::FindAliasBlock
//
//  Scans lines for the opening/closing marker comments.
//  Sets block.fFound = true if a complete block is detected.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CProfileFileManager::FindAliasBlock (const vector<wstring> & rgLines, SAliasBlock & block)
{
    HRESULT hr = S_OK;



    block = {};
    block.fFound = false;

    //
    // Find opening marker: line containing "#  TCDir Aliases"
    //

    for (size_t i = 0; i < rgLines.size(); ++i)
    {
        if (rgLines[i].find (k_pszHeaderMarker) != wstring::npos)
        {
            block.iStartLine = i;

            //
            // Scan backward to find the start of the banner.
            // Banner lines are either all-# lines or "# " comment lines.
            // Stop at blank lines or non-banner content.
            //

            while (block.iStartLine > 0)
            {
                const wstring & prev = rgLines[block.iStartLine - 1];

                if (prev.empty())
                    break;

                if (prev.find (L"####") != wstring::npos ||
                    (prev.size() <= 2 && prev[0] == L'#'))
                {
                    --block.iStartLine;
                }
                else
                {
                    break;
                }
            }

            //
            // Parse version from the marker line (e.g., "tcdir v5.2.1150")
            //

            size_t posV = rgLines[i].find (L"tcdir v");

            if (posV != wstring::npos)
            {
                block.strVersion = rgLines[i].substr (posV + 7);

                // Trim trailing whitespace
                size_t posEnd = block.strVersion.find_last_not_of (L" \t\r\n");

                if (posEnd != wstring::npos)
                {
                    block.strVersion.resize (posEnd + 1);
                }
            }

            //
            // Find closing marker: line containing "#  End TCDir Aliases"
            //

            for (size_t j = i + 1; j < rgLines.size(); ++j)
            {
                if (rgLines[j].find (k_pszFooterMarker) != wstring::npos)
                {
                    block.iEndLine = j;

                    //
                    // Scan forward past any trailing ### lines
                    //

                    while (block.iEndLine + 1 < rgLines.size() && rgLines[block.iEndLine + 1].find (L"####") != wstring::npos)
                    {
                        ++block.iEndLine;
                    }

                    block.fFound = true;
                    break;
                }
            }

            break;
        }
    }

    //
    // If found, parse function names from the block
    //

    if (block.fFound)
    {
        for (size_t k = block.iStartLine; k <= block.iEndLine; ++k)
        {
            const wstring & line = rgLines[k];

            if (line.starts_with (L"function "))
            {
                size_t posSpace = line.find (L' ', 9);

                if (posSpace != wstring::npos)
                {
                    wstring strName = line.substr (9, posSpace - 9);

                    block.rgAliasNames.push_back (strName);

                    if (block.strRootAlias.empty())
                    {
                        block.strRootAlias = strName;
                    }
                }
            }
        }
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CProfileFileManager::WriteProfileFile
//
//  Writes lines back to a UTF-8 file, optionally preserving BOM.
//  Creates parent directories if needed.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CProfileFileManager::WriteProfileFile (const wstring & strPath, const vector<wstring> & rgLines, bool fPreserveBom)
{
    HRESULT hr         = S_OK;
    wstring strContent;
    int     cbNeeded   = 0;
    string  strUtf8;
    FILE *  pf         = nullptr;



    //
    // Ensure parent directories exist
    //

    {
        filesystem::path parentDir = filesystem::path (strPath).parent_path();

        if (!parentDir.empty() && !filesystem::exists (parentDir))
        {
            filesystem::create_directories (parentDir);
        }
    }

    //
    // Join lines with \r\n
    //

    for (size_t i = 0; i < rgLines.size(); ++i)
    {
        if (i > 0)
        {
            strContent += L"\r\n";
        }

        strContent += rgLines[i];
    }

    // Ensure trailing newline
    if (!strContent.empty())
    {
        strContent += L"\r\n";
    }

    //
    // Convert to UTF-8
    //

    cbNeeded = WideCharToMultiByte (CP_UTF8, 0, strContent.data(), static_cast<int>(strContent.size()), nullptr, 0, nullptr, nullptr);

    CBRAEx (cbNeeded > 0 || strContent.empty(), HRESULT_FROM_WIN32 (GetLastError()));

    strUtf8.resize (static_cast<size_t>(cbNeeded), '\0');

    WideCharToMultiByte (CP_UTF8, 0, strContent.data(), static_cast<int>(strContent.size()), strUtf8.data(), cbNeeded, nullptr, nullptr);

    //
    // Write to file
    //

    _wfopen_s (&pf, strPath.c_str(), L"wb");
    CBRAEx (pf != nullptr, HRESULT_FROM_WIN32 (ERROR_ACCESS_DENIED));

    if (fPreserveBom)
    {
        fwrite (k_rgUtf8Bom, 1, sizeof (k_rgUtf8Bom), pf);
    }

    if (!strUtf8.empty())
    {
        fwrite (strUtf8.data(), 1, strUtf8.size(), pf);
    }

    fclose (pf);
    pf = nullptr;

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CProfileFileManager::CreateBackup
//
//  Creates a .bak copy of the profile file before modification.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CProfileFileManager::CreateBackup (const wstring & strPath)
{
    HRESULT hr = S_OK;



    if (filesystem::exists (strPath))
    {
        //
        // Build timestamped backup name: filename.2026-03-26-05-37-10.bak
        //

        auto              now       = chrono::system_clock::now();
        auto              timeT     = chrono::system_clock::to_time_t (now);
        tm                localTime = {};
        WCHAR             szTimestamp[32] = {};

        localtime_s (&localTime, &timeT);
        swprintf_s (szTimestamp, L".%04d-%02d-%02d-%02d-%02d-%02d.bak",
                    localTime.tm_year + 1900, localTime.tm_mon + 1, localTime.tm_mday,
                    localTime.tm_hour, localTime.tm_min, localTime.tm_sec);

        wstring strBakPath = strPath + szTimestamp;

        filesystem::copy_file (strPath, strBakPath, filesystem::copy_options::overwrite_existing);
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CProfileFileManager::ReplaceAliasBlock
//
//  Removes the existing block lines and inserts new block at the same position.
//
////////////////////////////////////////////////////////////////////////////////

void CProfileFileManager::ReplaceAliasBlock (vector<wstring> & rgLines, const SAliasBlock & block, const vector<wstring> & rgNewBlock)
{
    rgLines.erase (rgLines.begin() + block.iStartLine, rgLines.begin() + block.iEndLine + 1);
    rgLines.insert (rgLines.begin() + block.iStartLine, rgNewBlock.begin(), rgNewBlock.end());
}





////////////////////////////////////////////////////////////////////////////////
//
//  CProfileFileManager::AppendAliasBlock
//
//  Appends the alias block at the end of the file.
//
////////////////////////////////////////////////////////////////////////////////

void CProfileFileManager::AppendAliasBlock (vector<wstring> & rgLines, const vector<wstring> & rgNewBlock)
{
    //
    // Add a blank line separator if the file isn't empty
    //

    if (!rgLines.empty() && !rgLines.back().empty())
    {
        rgLines.push_back (L"");
    }

    rgLines.insert (rgLines.end(), rgNewBlock.begin(), rgNewBlock.end());
}





////////////////////////////////////////////////////////////////////////////////
//
//  CProfileFileManager::RemoveAliasBlock
//
//  Deletes the block lines[start..end] inclusive from the vector.
//
////////////////////////////////////////////////////////////////////////////////

void CProfileFileManager::RemoveAliasBlock (vector<wstring> & rgLines, const SAliasBlock & block)
{
    rgLines.erase (rgLines.begin() + block.iStartLine, rgLines.begin() + block.iEndLine + 1);

    //
    // Clean up trailing blank lines at the end of the file
    //

    while (!rgLines.empty() && rgLines.back().empty())
    {
        rgLines.pop_back();
    }
}
