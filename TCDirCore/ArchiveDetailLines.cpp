#include "pch.h"

#include "ArchiveDetailLines.h"

#include "ResultsDisplayerWithHeaderAndFooter.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::FormatName
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveDetailLines::FormatName (const SArchiveDetails & details)
{
    wstring strName = L"Unknown";



    switch (details.m_format)
    {
        case EArchiveFormat::Zip:
            strName = ZipName (details.m_zip);
            break;

        case EArchiveFormat::Iso:
            strName = L"Disc image";
            break;

        case EArchiveFormat::LibArchive:
            strName = details.m_libArchive.m_strFormatName.empty() ? wstring (L"Archive") : details.m_libArchive.m_strFormatName;
            break;

        default:
            break;
    }

    return strName;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::AddEncryption
//
//  ZIP archives list each encryption type with its entry count.  Returns
//  false for other formats, which use the common encryption row.
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveDetailLines::AddEncryption (const SArchiveReport & report, vector<SInfoLine> & lines)
{
    vector<wstring> rows;



    if (report.m_details.m_format != EArchiveFormat::Zip)
    {
        return false;
    }

    for (const auto & [encryption, cEntries] : report.m_details.m_zip.m_mapEncryption)
    {
        if (encryption != EZipEncryption::None)
        {
            rows.push_back (format (L"{} ({})", EncryptionName (encryption), CArchiveInfoDisplayer::FormatCount (cEntries, L"entry", L"entries")));
        }
    }

    if (rows.empty())
    {
        rows.push_back (L"None");
    }

    for (size_t i = 0; i < rows.size(); ++i)
    {
        lines.push_back ({ i == 0 ? wstring (L"Encryption") : wstring(), rows[i] });
    }

    return true;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::AddDetails
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveDetailLines::AddDetails (const SArchiveReport & report, vector<SInfoLine> & lines)
{
    if (report.m_details.m_format == EArchiveFormat::Zip)
    {
        AddZipDetails (report.m_details.m_zip, lines);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::ZipName
//
//  "ZIP, made by NTFS 2.0, UTF-8 paths".  The creator is the most common
//  "version made by" value among the entries.
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveDetailLines::ZipName (const SZipDetails & zip)
{
    wstring     strName  = L"ZIP";
    WORD        wCreator = 0;
    ULONGLONG   cMost    = 0;
    BYTE        bVersion = 0;



    for (const auto & [wMadeBy, cEntries] : zip.m_mapCreators)
    {
        if (cEntries > cMost)
        {
            cMost    = cEntries;
            wCreator = wMadeBy;
        }
    }

    if (cMost > 0)
    {
        bVersion  = static_cast<BYTE> (wCreator & s_kbVersionMask);
        strName  += format (L", made by {} {}.{}",
                            HostName (static_cast<BYTE> (wCreator >> s_kcHostShift)),
                            bVersion / s_kcVersionDivisor,
                            bVersion % s_kcVersionDivisor);
    }

    if (zip.m_fUtf8Paths)
    {
        strName += L", UTF-8 paths";
    }

    return strName;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::AddZipDetails
//
//  Methods, then the rows that appear only when they apply: ZIP64, several
//  disks, a self-extractor prefix, and the comment.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveDetailLines::AddZipDetails (const SZipDetails & zip, vector<SInfoLine> & lines)
{
    AddZipMethods (zip, lines);

    if (zip.m_fZip64)
    {
        lines.push_back ({ L"ZIP64", L"Yes" });
    }

    if (zip.m_fMultiDisk)
    {
        lines.push_back ({ L"Disks", L"The archive spans several disks" });
    }

    if (zip.m_ullPrefixSize > 0)
    {
        lines.push_back ({ L"Prefix", CArchiveInfoDisplayer::FormatBytes (zip.m_ullPrefixSize) + L" before the archive data (self-extracting)" });
    }

    AddCommentLines (zip.m_strComment, lines);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::AddZipMethods
//
//  One row per method, largest uncompressed total first, with aligned
//  columns: method, file count, compressed and uncompressed size.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveDetailLines::AddZipMethods (const SZipDetails & zip, vector<SInfoLine> & lines)
{
    vector<pair<WORD, SMethodGroup>>   groups (zip.m_mapMethods.begin(), zip.m_mapMethods.end());
    vector<wstring>                    vNames;
    vector<wstring>                    vCounts;
    vector<wstring>                    vSizes;
    size_t                             cchName  = 0;
    size_t                             cchCount = 0;



    stable_sort (groups.begin(), groups.end(), [] (const auto & a, const auto & b)
    {
        return a.second.m_ullUncompressed > b.second.m_ullUncompressed;
    });

    for (const auto & [wMethod, group] : groups)
    {
        vNames.push_back  (MethodName (wMethod));
        vCounts.push_back (CArchiveInfoDisplayer::FormatCount (group.m_cFiles, L"file", L"files"));
        vSizes.push_back  (CResultsDisplayerWithHeaderAndFooter::FormatNumberWithSeparators (group.m_ullCompressed) + L" of " +
                           CArchiveInfoDisplayer::FormatBytes (group.m_ullUncompressed));

        cchName  = max (cchName,  vNames.back().size());
        cchCount = max (cchCount, vCounts.back().size());
    }

    for (size_t i = 0; i < groups.size(); ++i)
    {
        lines.push_back ({ i == 0 ? wstring (L"Methods") : wstring(),
                           format (L"{:<{}}   {:<{}}   {}", vNames[i], cchName, vCounts[i], cchCount, vSizes[i]) });
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::AddCommentLines
//
//  Each line of the comment is a row, so later lines line up under the
//  first.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveDetailLines::AddCommentLines (const wstring & strComment, vector<SInfoLine> & lines)
{
    wstring   strLine;
    bool      fFirst  = true;



    if (strComment.empty())
    {
        return;
    }

    for (size_t i = 0; i <= strComment.size(); ++i)
    {
        if (i < strComment.size() && strComment[i] != L'\n')
        {
            if (strComment[i] != L'\r')
            {
                strLine += strComment[i];
            }

            continue;
        }

        lines.push_back ({ fFirst ? wstring (L"Comment") : wstring(), strLine });
        strLine.clear();
        fFirst = false;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::MethodName
//
//  Method numbers from APPNOTE.TXT 4.4.5.
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveDetailLines::MethodName (WORD wMethod)
{
    struct SMethodName
    {
        WORD      m_wMethod;
        LPCWSTR   m_pszName;
    };

    static constexpr SMethodName s_krgMethods[] =
    {
        {  0, L"Store"     },
        {  1, L"Shrink"    },
        {  6, L"Implode"   },
        {  8, L"Deflate"   },
        {  9, L"Deflate64" },
        { 12, L"BZip2"     },
        { 14, L"LZMA"      },
        { 93, L"Zstd"      },
        { 95, L"XZ"        },
        { 96, L"JPEG"      },
        { 97, L"WavPack"   },
        { 98, L"PPMd"      },
    };

    wstring strName = format (L"Method {}", wMethod);



    for (const SMethodName & method : s_krgMethods)
    {
        if (method.m_wMethod == wMethod)
        {
            strName = method.m_pszName;
            break;
        }
    }

    return strName;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::HostName
//
//  "Version made by" host systems from APPNOTE.TXT 4.4.2.
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveDetailLines::HostName (BYTE bHost)
{
    static constexpr LPCWSTR s_krgHosts[] =
    {
        L"MS-DOS", L"Amiga", L"OpenVMS", L"Unix", L"VM/CMS", L"Atari ST", L"OS/2 HPFS", L"Macintosh",
        L"Z-System", L"CP/M", L"NTFS", L"MVS", L"VSE", L"Acorn RISC", L"VFAT", L"MVS",
        L"BeOS", L"Tandem", L"OS/400", L"macOS",
    };



    return (bHost < ARRAYSIZE (s_krgHosts)) ? wstring (s_krgHosts[bHost]) : format (L"host {}", bHost);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::EncryptionName
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveDetailLines::EncryptionName (EZipEncryption encryption)
{
    wstring strName = L"None";



    switch (encryption)
    {
        case EZipEncryption::ZipCrypto:    strName = L"ZipCrypto";      break;
        case EZipEncryption::Aes128:       strName = L"AES-128";        break;
        case EZipEncryption::Aes192:       strName = L"AES-192";        break;
        case EZipEncryption::Aes256:       strName = L"AES-256";        break;
        case EZipEncryption::PkwareStrong: strName = L"PKWARE strong";  break;
        default:                                                        break;
    }

    return strName;
}





