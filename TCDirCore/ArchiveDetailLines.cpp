#include "pch.h"

#include "ArchiveDetailLines.h"

#include "IsoDetails.h"
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
            strName = LibArchiveName (details.m_libArchive);
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
    else if (report.m_details.m_format == EArchiveFormat::Iso)
    {
        AddDiscDetails (report.m_details.m_disc, lines);
    }
    else if (report.m_details.m_format == EArchiveFormat::LibArchive)
    {
        AddLibArchiveDetails (report.m_details.m_libArchive, lines);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::ZipName
//
//  "ZIP 2.0, Windows file attributes, UTF-8 paths".  The creator is the
//  most common "version made by" value among the entries.  Its low byte
//  specifies the ZIP specification version the creating tool supports, and
//  its high byte specifies the layout of the entry attributes.  The high byte
//  doesn't indicate the operating system: libarchive writes Unix on every
//  platform, for example.
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
        strName  += format (L" {}.{}, {} file attributes",
                            bVersion / s_kcVersionDivisor,
                            bVersion % s_kcVersionDivisor,
                            HostName (static_cast<BYTE> (wCreator >> s_kcHostShift)));
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
//  CArchiveDetailLines::AddDiscDetails
//
//  File systems, labels and IDs, volume dates, sectors and boot platforms.
//  Blank IDs and unset dates are omitted.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveDetailLines::AddDiscDetails (const SDiscDetails & disc, vector<SInfoLine> & lines)
{
    struct STextRow
    {
        LPCWSTR                        m_pszLabel;
        wstring SDiscDetails::*        m_pstr;
    };

    struct SDateRow
    {
        LPCWSTR                        m_pszLabel;
        optional<FILETIME> SDiscDetails::* m_pft;
    };

    static constexpr STextRow s_krgTextRows[] =
    {
        { L"Volume label", &SDiscDetails::m_strVolumeLabel },
        { L"System ID",    &SDiscDetails::m_strSystemId    },
        { L"Publisher",    &SDiscDetails::m_strPublisher   },
        { L"Preparer",     &SDiscDetails::m_strPreparer    },
        { L"Application",  &SDiscDetails::m_strApplication },
    };

    static constexpr SDateRow s_krgDateRows[] =
    {
        { L"Created",  &SDiscDetails::m_ftCreated  },
        { L"Modified", &SDiscDetails::m_ftModified },
        { L"Expires",  &SDiscDetails::m_ftExpires  },
    };



    lines.push_back ({ L"File systems", FileSystemsOf (disc) });

    for (const STextRow & row : s_krgTextRows)
    {
        if (!(disc.*(row.m_pstr)).empty())
        {
            lines.push_back ({ row.m_pszLabel, disc.*(row.m_pstr) });
        }
    }

    for (const SDateRow & row : s_krgDateRows)
    {
        if ((disc.*(row.m_pft)).has_value())
        {
            lines.push_back ({ row.m_pszLabel, CArchiveInfoDisplayer::FormatDateTime ((disc.*(row.m_pft)).value()) });
        }
    }

    lines.push_back ({ L"Sectors", CArchiveInfoDisplayer::FormatCount (disc.m_ullSectorCount, L"sector", L"sectors") +
                                   L" of " + CArchiveInfoDisplayer::FormatBytes (disc.m_ulSectorSize) });
    lines.push_back ({ L"Bootable", BootPlatformsOf (disc) });
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::FileSystemsOf
//
//  "UDF 2.50, ISO 9660, Joliet; listed from UDF".
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveDetailLines::FileSystemsOf (const SDiscDetails & disc)
{
    vector<wstring>   systems;
    wstring           strText;
    LPCWSTR           pszListed = L"ISO 9660";



    if (disc.m_wUdfRevision != 0)
    {
        systems.push_back (format (L"UDF {:X}.{:02X}", disc.m_wUdfRevision >> s_kcHostShift, disc.m_wUdfRevision & s_kbVersionMask));
    }

    if (disc.m_fIso9660)   { systems.push_back (L"ISO 9660");   }
    if (disc.m_fJoliet)    { systems.push_back (L"Joliet");     }
    if (disc.m_fRockRidge) { systems.push_back (L"Rock Ridge"); }

    for (const wstring & strName : systems)
    {
        strText += (strText.empty() ? L"" : L", ") + strName;
    }

    if (disc.m_eListedTree == EDiscTree::Udf)
    {
        pszListed = L"UDF";
    }
    else if (disc.m_eListedTree == EDiscTree::Joliet)
    {
        pszListed = L"Joliet";
    }

    return strText + L"; listed from " + pszListed;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::BootPlatformsOf
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveDetailLines::BootPlatformsOf (const SDiscDetails & disc)
{
    wstring strText;



    for (BYTE bPlatform : disc.m_vBootPlatforms)
    {
        strText += strText.empty() ? L"" : L", ";

        switch (bPlatform)
        {
            case CIsoDetails::s_kPlatformBios:    strText += L"BIOS";    break;
            case CIsoDetails::s_kPlatformUefi:    strText += L"UEFI";    break;
            case CIsoDetails::s_kPlatformPowerPc: strText += L"PowerPC"; break;
            case CIsoDetails::s_kPlatformMac:     strText += L"Mac";     break;
            default:                              strText += format (L"platform 0x{:02X}", bPlatform); break;
        }
    }

    return strText.empty() ? wstring (L"No") : L"Yes: " + strText;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::AddLibArchiveDetails
//
//  Stream totals, then the rows that apply: encrypted headers, links,
//  owners, groups, permissions, and entries with extra metadata.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveDetailLines::AddLibArchiveDetails (const SLibArchiveDetails & lib, vector<SInfoLine> & lines)
{
    vector<wstring> parts;



    if (lib.m_ullStreamCompressed.has_value() && lib.m_ullStreamUncompressed.value_or (0) > 0)
    {
        lines.push_back ({ L"Stream", format (L"{} of {} ({})",
                                              CResultsDisplayerWithHeaderAndFooter::FormatNumberWithSeparators (lib.m_ullStreamCompressed.value()),
                                              CArchiveInfoDisplayer::FormatBytes (lib.m_ullStreamUncompressed.value()),
                                              CArchiveInfoDisplayer::FormatRatio (lib.m_ullStreamCompressed.value(), lib.m_ullStreamUncompressed.value())) });
    }

    if (lib.m_fHeadersEncrypted)
    {
        lines.push_back ({ L"Headers", L"Encrypted" });
    }

    AddCountPart (lib.m_cSymlinks,  L"symbolic link", L"symbolic links", parts);
    AddCountPart (lib.m_cHardLinks, L"hard link",     L"hard links",     parts);
    AddJoinedLine (L"Links", parts, lines);

    AddNameCounts (L"Owners", lib.m_mapOwners, lines);
    AddNameCounts (L"Groups", lib.m_mapGroups, lines);
    AddPermissionCounts (lib.m_mapPermissions, lines);

    AddCountPart (lib.m_cSparse,      L"sparse file",                   L"sparse files",                   parts);
    AddCountPart (lib.m_cXattrs,      L"entry with extended attributes", L"entries with extended attributes", parts);
    AddCountPart (lib.m_cAcls,        L"entry with ACLs",               L"entries with ACLs",              parts);
    AddCountPart (lib.m_cBirthTimes,  L"entry with a creation time",    L"entries with a creation time",   parts);
    AddCountPart (lib.m_cAccessTimes, L"entry with an access time",     L"entries with an access time",    parts);
    AddJoinedLine (L"Metadata", parts, lines);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::LibArchiveName
//
//  "POSIX ustar format, compressed with gzip".
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveDetailLines::LibArchiveName (const SLibArchiveDetails & lib)
{
    wstring strName = lib.m_strFormatName.empty() ? wstring (L"Archive") : lib.m_strFormatName;



    for (size_t i = 0; i < lib.m_vFilters.size(); ++i)
    {
        strName += (i == 0 ? L", compressed with " : L" and ") + lib.m_vFilters[i];
    }

    return strName;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::AddCountPart
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveDetailLines::AddCountPart (ULONGLONG cItems, LPCWSTR pszSingular, LPCWSTR pszPlural, vector<wstring> & parts)
{
    if (cItems > 0)
    {
        parts.push_back (CArchiveInfoDisplayer::FormatCount (cItems, pszSingular, pszPlural));
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::AddJoinedLine
//
//  Adds the parts as one comma-separated row, then clears them.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveDetailLines::AddJoinedLine (LPCWSTR pszLabel, vector<wstring> & parts, vector<SInfoLine> & lines)
{
    wstring strValue;



    for (const wstring & strPart : parts)
    {
        strValue += (strValue.empty() ? L"" : L", ") + strPart;
    }

    if (!strValue.empty())
    {
        lines.push_back ({ pszLabel, strValue });
    }

    parts.clear();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::AddNameCounts
//
//  One row per owner or group, most entries first.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveDetailLines::AddNameCounts (LPCWSTR pszLabel, const map<wstring, ULONGLONG> & counts, vector<SInfoLine> & lines)
{
    vector<pair<wstring, ULONGLONG>> sorted (counts.begin(), counts.end());



    stable_sort (sorted.begin(), sorted.end(), [] (const auto & a, const auto & b) { return a.second > b.second; });

    for (size_t i = 0; i < sorted.size(); ++i)
    {
        lines.push_back ({ i == 0 ? wstring (pszLabel) : wstring(),
                           format (L"{} ({})", sorted[i].first, CArchiveInfoDisplayer::FormatCount (sorted[i].second, L"entry", L"entries")) });
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines::AddPermissionCounts
//
//  One row per permission set in octal, most entries first.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveDetailLines::AddPermissionCounts (const map<unsigned, ULONGLONG> & counts, vector<SInfoLine> & lines)
{
    map<wstring, ULONGLONG> octal;



    for (const auto & [uPerm, cEntries] : counts)
    {
        octal[format (L"{:04o}", uPerm)] = cEntries;
    }

    AddNameCounts (L"Permissions", octal, lines);
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
//  "Version made by" host systems from APPNOTE.TXT 4.4.2.  Hosts 0 (FAT),
//  10 (NTFS) and 14 (VFAT) all specify DOS attribute flags, so they display
//  as Windows.
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveDetailLines::HostName (BYTE bHost)
{
    static constexpr LPCWSTR s_krgHosts[] =
    {
        L"Windows", L"Amiga", L"OpenVMS", L"Unix", L"VM/CMS", L"Atari ST", L"OS/2 HPFS", L"Macintosh",
        L"Z-System", L"CP/M", L"Windows", L"MVS", L"VSE", L"Acorn RISC", L"Windows", L"MVS",
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





