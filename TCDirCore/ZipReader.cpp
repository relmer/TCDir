#include "pch.h"

#include "ZipReader.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::Read
//
//  Locates the central directory through the end record (and the ZIP64 end
//  record when present), reads it in one call, and parses every entry.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CZipReader::Read (IByteSource & source, SArchiveReadResult & result)
{
    HRESULT              hr           = S_OK;
    ULONGLONG            ullEndOffset = 0;
    SDirectoryLocation   location;
    vector<BYTE>         directory;



    result.Reset();

    hr = FindEndRecord (source, ullEndOffset, location);
    CHR (hr);

    CBREx (location.m_ullSize <= s_kcbMaxDirectory, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));
    CBREx (location.m_ullOffset <= ullEndOffset && location.m_ullSize <= ullEndOffset - location.m_ullOffset,
           HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    directory.resize (static_cast<size_t> (location.m_ullSize));

    if (!directory.empty())
    {
        hr = source.ReadAt (location.m_ullOffset, directory.data(), static_cast<DWORD> (directory.size()));
        CHREx (hr, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));
    }

    hr = ParseDirectory (directory, location.m_cEntries, result);
    CHR (hr);

    if (result.m_detailsPtr != nullptr)
    {
        AddLocationDetails (location, result.m_detailsPtr->m_zip);
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::FindEndRecord
//
//  Scans backward from the end of the file for the end-of-central-directory
//  record.  The archive comment can be up to 64 KB, so the record can be
//  anywhere in the last 64 KB + 22 bytes.  A candidate whose comment ends
//  exactly at the end of the file is preferred.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CZipReader::FindEndRecord (IByteSource & source, ULONGLONG & ullEndOffset, SDirectoryLocation & location)
{
    HRESULT        hr        = S_OK;
    ULONGLONG      ullSize   = source.Size();
    size_t         cbTail    = static_cast<size_t> (min<ULONGLONG> (ullSize, s_kcbEndRecord + s_kcbMaxComment));
    ULONGLONG      ullTail   = ullSize - cbTail;
    vector<BYTE>   tail      (cbTail);
    size_t         ibFound   = SIZE_MAX;
    const BYTE   * pRecord   = nullptr;



    CBREx (cbTail >= s_kcbEndRecord, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    hr = source.ReadAt (ullTail, tail.data(), static_cast<DWORD> (cbTail));
    CHR (hr);

    for (size_t ib = cbTail - s_kcbEndRecord + 1; ib-- > 0; )
    {
        if (Read32 (&tail[ib]) != s_kdwEndSignature)
        {
            continue;
        }

        if (ibFound == SIZE_MAX)
        {
            ibFound = ib;
        }

        if (ib + s_kcbEndRecord + Read16 (&tail[ib + s_kibEndCommentLength]) == cbTail)
        {
            ibFound = ib;
            break;
        }
    }

    CBREx (ibFound != SIZE_MAX, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    pRecord                   = &tail[ibFound];
    ullEndOffset              = ullTail + ibFound;
    location.m_cEntries       = Read16 (pRecord + s_kibEndEntryCount);
    location.m_ullSize        = Read32 (pRecord + s_kibEndDirectorySize);
    location.m_ullOffset      = Read32 (pRecord + s_kibEndDirectoryOffset);
    location.m_wDisk          = Read16 (pRecord + s_kibEndDiskNumber);
    location.m_wDirectoryDisk = Read16 (pRecord + s_kibEndDirectoryDisk);

    location.m_vComment.assign (pRecord + s_kcbEndRecord,
                                pRecord + s_kcbEndRecord + min<size_t> (Read16 (pRecord + s_kibEndCommentLength), cbTail - ibFound - s_kcbEndRecord));

    if (location.m_cEntries == s_kwMax16 || location.m_ullSize == s_kdwMax32 || location.m_ullOffset == s_kdwMax32)
    {
        hr = ReadZip64EndRecord (source, ullEndOffset, location);
        CHR (hr);
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::ReadZip64EndRecord
//
//  The ZIP64 locator sits immediately before the end record and points to
//  the ZIP64 end record, which contains 64-bit counts and offsets.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CZipReader::ReadZip64EndRecord (IByteSource & source, ULONGLONG ullEndOffset, SDirectoryLocation & location)
{
    HRESULT     hr                               = S_OK;
    BYTE        locator[s_kcbZip64Locator]       = {};
    BYTE        record[s_kcbZip64EndRecord]      = {};
    ULONGLONG   ullRecordOffset                  = 0;
    DWORD       dwSignature                      = 0;



    CBREx (ullEndOffset >= s_kcbZip64Locator, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    hr = source.ReadAt (ullEndOffset - s_kcbZip64Locator, locator, sizeof (locator));
    CHREx (hr, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    dwSignature = Read32 (locator);
    CBREx (dwSignature == s_kdwZip64LocatorSignature, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    ullRecordOffset = Read64 (locator + s_kibLocatorEndOffset);

    hr = source.ReadAt (ullRecordOffset, record, sizeof (record));
    CHREx (hr, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    dwSignature = Read32 (record);
    CBREx (dwSignature == s_kdwZip64EndSignature, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    location.m_cEntries  = Read64 (record + s_kibZip64EntryCount);
    location.m_ullSize   = Read64 (record + s_kibZip64DirectorySize);
    location.m_ullOffset = Read64 (record + s_kibZip64DirectoryOffset);
    location.m_fZip64    = true;



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::ParseDirectory
//
//  A directory with fewer entries than the end record's entry count is
//  truncated or corrupt.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CZipReader::ParseDirectory (const vector<BYTE> & directory, ULONGLONG cEntries, SArchiveReadResult & result)
{
    HRESULT         hr       = S_OK;
    size_t          ib       = 0;
    size_t          cbEntry  = 0;
    SArchiveEntry   entry;
    SEntryInfo      info;



    result.m_vEntries.reserve (static_cast<size_t> (min<ULONGLONG> (cEntries, directory.size() / s_kcbCentralHeader)));

    if (result.m_detailsPtr != nullptr)
    {
        result.m_detailsPtr->m_zip.m_ullPrefixSize = (cEntries > 0) ? ULLONG_MAX : 0;
    }

    for (ULONGLONG i = 0; i < cEntries; ++i)
    {
        hr = ParseEntry (directory.data() + ib, directory.size() - ib, cbEntry, entry, info);
        CHR (hr);

        if (result.m_detailsPtr != nullptr)
        {
            AddEntryDetails (entry, info, result.m_detailsPtr->m_zip);
        }

        result.m_vEntries.push_back (move (entry));
        ib += cbEntry;
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::ParseEntry
//
//  Parses one central directory header and its extra fields.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CZipReader::ParseEntry (
    const BYTE      * pb,
    size_t            cbAvailable,
    size_t          & cbEntry,
    SArchiveEntry   & entry,
    SEntryInfo      & info)
{
    HRESULT        hr        = S_OK;
    WORD           wFlags    = 0;
    size_t         cchName   = 0;
    size_t         cbExtra   = 0;
    size_t         cbComment = 0;
    DWORD          dwSize    = 0;
    DWORD          dwSig     = 0;
    SEntrySizes    sizes;
    SZip64Fields   zip64;



    entry = {};
    info  = {};

    CBREx (cbAvailable >= s_kcbCentralHeader, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    dwSig = Read32 (pb);
    CBREx (dwSig == s_kdwCentralSignature, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    wFlags    = Read16 (pb + s_kibFlags);
    cchName   = Read16 (pb + s_kibNameLength);
    cbExtra   = Read16 (pb + s_kibExtraLength);
    cbComment = Read16 (pb + s_kibCommentLength);
    cbEntry   = s_kcbCentralHeader + cchName + cbExtra + cbComment;

    CBREx (cbEntry <= cbAvailable, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    dwSize                  = Read32 (pb + s_kibUncompressedSize);
    sizes.m_ullUncompressed = dwSize;
    sizes.m_ullCompressed   = Read32 (pb + s_kibCompressedSize);
    sizes.m_ullLocalOffset  = Read32 (pb + s_kibLocalHeaderOffset);
    zip64.m_fUncompressed   = dwSize == s_kdwMax32;
    zip64.m_fCompressed     = sizes.m_ullCompressed == s_kdwMax32;
    zip64.m_fLocalOffset    = sizes.m_ullLocalOffset == s_kdwMax32;

    ParseExtraFields (pb + s_kcbCentralHeader + cchName, cbExtra, zip64, sizes);

    entry.m_strPath      = sizes.m_strUnicodePath.empty()
                               ? DecodeName (pb + s_kcbCentralHeader, cchName, (wFlags & s_kwFlagUtf8) ? CP_UTF8 : CP_OEMCP)
                               : sizes.m_strUnicodePath;
    entry.m_ullSize      = sizes.m_ullUncompressed;
    entry.m_ftLastWrite  = sizes.m_fHasNtfsTime ? sizes.m_ftNtfsLastWrite
                                                : DosToUtc (Read16 (pb + s_kibDate), Read16 (pb + s_kibTime));
    entry.m_dwAttributes = MapAttributes (Read16 (pb + s_kibVersionMadeBy), Read32 (pb + s_kibExternalAttributes));
    entry.m_fDirectory   = IsFolderName (entry.m_strPath) || (entry.m_dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    entry.m_fEncrypted   = (wFlags & s_kwFlagEncrypted) != 0;

    info.m_wMethod        = Read16 (pb + s_kibMethod);
    info.m_wFlags         = wFlags;
    info.m_wVersionMadeBy = Read16 (pb + s_kibVersionMadeBy);
    info.m_ullCompressed  = sizes.m_ullCompressed;
    info.m_ullLocalOffset = sizes.m_ullLocalOffset;
    info.m_fZip64         = sizes.m_fZip64;
    info.m_fAes           = sizes.m_fAes;
    info.m_bAesStrength   = sizes.m_bAesStrength;
    info.m_wAesMethod     = sizes.m_wAesMethod;



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::ParseExtraFields
//
//  Applies the ZIP64, NTFS, Info-ZIP Unicode Path and WinZip AES extra
//  fields.  Malformed extra data is ignored.
//
////////////////////////////////////////////////////////////////////////////////

void CZipReader::ParseExtraFields (const BYTE * pb, size_t cb, const SZip64Fields & zip64, SEntrySizes & sizes)
{
    size_t        ib      = 0;
    WORD          wId     = 0;
    size_t        cbField = 0;
    const BYTE  * pField  = nullptr;



    while (ib + s_kcbExtraHeader <= cb)
    {
        wId     = Read16 (pb + ib);
        cbField = Read16 (pb + ib + sizeof (WORD));
        pField  = pb + ib + s_kcbExtraHeader;

        if (ib + s_kcbExtraHeader + cbField > cb)
        {
            break;
        }

        if (wId == s_kwExtraZip64)
        {
            ParseZip64Extra (pField, cbField, zip64, sizes);
        }
        else if (wId == s_kwExtraAes && cbField >= s_kcbAesExtra)
        {
            sizes.m_fAes         = true;
            sizes.m_bAesStrength = pField[s_kibAesStrength];
            sizes.m_wAesMethod   = Read16 (pField + s_kibAesMethod);
        }
        else if (wId == s_kwExtraNtfs)
        {
            ParseNtfsExtra (pField, cbField, sizes);
        }
        else if (wId == s_kwExtraUnicodePath && cbField > s_kcbUnicodePathHeader && pField[0] == s_kbUnicodePathVersion)
        {
            sizes.m_strUnicodePath = DecodeName (pField + s_kcbUnicodePathHeader, cbField - s_kcbUnicodePathHeader, CP_UTF8);
        }

        ib += s_kcbExtraHeader + cbField;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::ParseZip64Extra
//
//  The ZIP64 extra field contains 64-bit values only for the header fields
//  that are 0xFFFFFFFF, in this order: uncompressed size, compressed size,
//  local header offset.
//
////////////////////////////////////////////////////////////////////////////////

void CZipReader::ParseZip64Extra (const BYTE * pb, size_t cb, const SZip64Fields & zip64, SEntrySizes & sizes)
{
    size_t ib = 0;



    if (zip64.m_fUncompressed && ib + sizeof (ULONGLONG) <= cb)
    {
        sizes.m_ullUncompressed  = Read64 (pb + ib);
        sizes.m_fZip64           = true;
        ib                      += sizeof (ULONGLONG);
    }

    if (zip64.m_fCompressed && ib + sizeof (ULONGLONG) <= cb)
    {
        sizes.m_ullCompressed  = Read64 (pb + ib);
        sizes.m_fZip64         = true;
        ib                    += sizeof (ULONGLONG);
    }

    if (zip64.m_fLocalOffset && ib + sizeof (ULONGLONG) <= cb)
    {
        sizes.m_ullLocalOffset = Read64 (pb + ib);
        sizes.m_fZip64         = true;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::AddLocationDetails
//
//  Values from the end record: ZIP64, disk numbers and the comment.
//
////////////////////////////////////////////////////////////////////////////////

void CZipReader::AddLocationDetails (const SDirectoryLocation & location, SZipDetails & zip)
{
    zip.m_fZip64     = zip.m_fZip64 || location.m_fZip64;
    zip.m_fMultiDisk = location.m_wDisk != 0 || location.m_wDirectoryDisk != 0;
    zip.m_strComment = DecodeComment (location.m_vComment);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::ParseNtfsExtra
//
//  The NTFS extra field has 4 reserved bytes, then tagged attributes.  Tag 1
//  contains the modification, access and creation times as UTC FILETIMEs.
//
////////////////////////////////////////////////////////////////////////////////

void CZipReader::ParseNtfsExtra (const BYTE * pb, size_t cb, SEntrySizes & sizes)
{
    size_t      ib       = s_kcbNtfsReserved;
    WORD        wTag     = 0;
    size_t      cbTag    = 0;
    ULONGLONG   ullTime  = 0;



    while (ib + s_kcbExtraHeader <= cb)
    {
        wTag  = Read16 (pb + ib);
        cbTag = Read16 (pb + ib + sizeof (WORD));

        if (ib + s_kcbExtraHeader + cbTag > cb)
        {
            break;
        }

        if (wTag == s_kwNtfsTimesTag && cbTag >= s_kcbFileTime)
        {
            ullTime = Read64 (pb + ib + s_kcbExtraHeader);

            sizes.m_ftNtfsLastWrite.dwLowDateTime  = static_cast<DWORD> (ullTime);
            sizes.m_ftNtfsLastWrite.dwHighDateTime = static_cast<DWORD> (ullTime >> (sizeof (DWORD) * CHAR_BIT));
            sizes.m_fHasNtfsTime                   = true;
        }

        ib += s_kcbExtraHeader + cbTag;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::AddEntryDetails
//
//  Adds one entry to the method, encryption and creator totals.  Folder
//  entries count toward the creator and UTF-8 totals only.
//
////////////////////////////////////////////////////////////////////////////////

void CZipReader::AddEntryDetails (const SArchiveEntry & entry, const SEntryInfo & info, SZipDetails & zip)
{
    WORD wMethod = info.m_fAes ? info.m_wAesMethod : info.m_wMethod;



    zip.m_mapCreators[info.m_wVersionMadeBy] += 1;
    zip.m_fUtf8Paths    = zip.m_fUtf8Paths || (info.m_wFlags & s_kwFlagUtf8) != 0;
    zip.m_fZip64        = zip.m_fZip64 || info.m_fZip64;
    zip.m_ullPrefixSize = min (zip.m_ullPrefixSize, info.m_ullLocalOffset);

    if (entry.m_fDirectory)
    {
        return;
    }

    SMethodGroup & group = zip.m_mapMethods[wMethod];

    group.m_cFiles          += 1;
    group.m_ullCompressed   += info.m_ullCompressed;
    group.m_ullUncompressed += entry.m_ullSize;

    zip.m_mapEncryption[EncryptionOf (info)] += 1;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::EncryptionOf
//
//  Flag bit 0 marks an encrypted entry.  Bit 6 marks PKWARE strong
//  encryption, and the 0x9901 extra field marks WinZip AES; otherwise the
//  entry uses the original ZipCrypto scheme.
//
////////////////////////////////////////////////////////////////////////////////

EZipEncryption CZipReader::EncryptionOf (const SEntryInfo & info)
{
    EZipEncryption encryption = EZipEncryption::None;



    if ((info.m_wFlags & s_kwFlagEncrypted) == 0)
    {
        encryption = EZipEncryption::None;
    }
    else if ((info.m_wFlags & s_kwFlagStrongEncryption) != 0)
    {
        encryption = EZipEncryption::PkwareStrong;
    }
    else if (info.m_fAes && info.m_bAesStrength == s_kbAes128)
    {
        encryption = EZipEncryption::Aes128;
    }
    else if (info.m_fAes && info.m_bAesStrength == s_kbAes192)
    {
        encryption = EZipEncryption::Aes192;
    }
    else if (info.m_fAes)
    {
        encryption = EZipEncryption::Aes256;
    }
    else
    {
        encryption = EZipEncryption::ZipCrypto;
    }

    return encryption;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::DecodeComment
//
//  The archive comment has no encoding flag.  UTF-8 is used when the bytes
//  are valid UTF-8, and code page 437 otherwise.
//
////////////////////////////////////////////////////////////////////////////////

wstring CZipReader::DecodeComment (const vector<BYTE> & comment)
{
    int       cch        = 0;
    wstring   strComment;



    if (!comment.empty())
    {
        cch = MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS, reinterpret_cast<LPCCH> (comment.data()), static_cast<int> (comment.size()), nullptr, 0);

        strComment = (cch > 0) ? DecodeName (comment.data(), comment.size(), CP_UTF8)
                               : DecodeName (comment.data(), comment.size(), s_kuCodePageDos);
    }

    return strComment;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::DosToSystemTime
//
//  Splits MS-DOS date and time bit fields.  The result is a local time.
//
////////////////////////////////////////////////////////////////////////////////

SYSTEMTIME CZipReader::DosToSystemTime (WORD wDate, WORD wTime)
{
    SYSTEMTIME st = {};



    st.wYear   = static_cast<WORD> (s_kwDosYearBase + (wDate >> s_kcDosYearShift));
    st.wMonth  = static_cast<WORD> ((wDate >> s_kcDosMonthShift) & s_kwDosMonthMask);
    st.wDay    = static_cast<WORD> (wDate & s_kwDosDayMask);
    st.wHour   = static_cast<WORD> (wTime >> s_kcDosHourShift);
    st.wMinute = static_cast<WORD> ((wTime >> s_kcDosMinuteShift) & s_kwDosMinuteMask);
    st.wSecond = static_cast<WORD> ((wTime & s_kwDosSecondMask) * s_kwDosSecondScale);

    return st;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::DosToUtc
//
//  MS-DOS times are local times with no zone.  The conversion uses the
//  daylight saving rules in effect on that date, not today's offset.
//  Invalid dates return a zero FILETIME.
//
////////////////////////////////////////////////////////////////////////////////

FILETIME CZipReader::DosToUtc (WORD wDate, WORD wTime)
{
    HRESULT      hr       = S_OK;
    SYSTEMTIME   stLocal  = DosToSystemTime (wDate, wTime);
    SYSTEMTIME   stUtc    = {};
    FILETIME     ft       = {};
    BOOL         fSuccess = FALSE;



    fSuccess = TzSpecificLocalTimeToSystemTime (nullptr, &stLocal, &stUtc);
    CWR (fSuccess);

    fSuccess = SystemTimeToFileTime (&stUtc, &ft);
    CWR (fSuccess);



Error:
    if (FAILED (hr))
    {
        ft = {};
    }

    return ft;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::MapAttributes
//
//  MS-DOS, NTFS and VFAT hosts store DOS attributes in the low byte.  Unix
//  hosts store the mode in the high 16 bits; no owner write permission maps
//  to read-only.  Other hosts have no attributes that map.
//
////////////////////////////////////////////////////////////////////////////////

DWORD CZipReader::MapAttributes (WORD wVersionMadeBy, DWORD dwExternalAttributes)
{
    BYTE    bHost   = static_cast<BYTE> (wVersionMadeBy >> s_kcHostShift);
    DWORD   dwMode  = dwExternalAttributes >> s_kcUnixModeShift;
    DWORD   dwAttrs = 0;



    if (bHost == s_kbHostMsDos || bHost == s_kbHostNtfs || bHost == s_kbHostVfat)
    {
        dwAttrs = dwExternalAttributes & s_kdwDosAttributeMask;
    }
    else if (bHost == s_kbHostUnix && dwMode != 0)
    {
        if ((dwMode & s_kdwUnixTypeMask) == s_kdwUnixDirectory)
        {
            dwAttrs |= FILE_ATTRIBUTE_DIRECTORY;
        }

        if ((dwMode & s_kdwUnixOwnerWrite) == 0)
        {
            dwAttrs |= FILE_ATTRIBUTE_READONLY;
        }
    }

    return dwAttrs;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::DecodeName
//
////////////////////////////////////////////////////////////////////////////////

wstring CZipReader::DecodeName (const BYTE * pb, size_t cb, UINT codePage)
{
    wstring   name;
    int       cch  = 0;



    if (cb > 0)
    {
        cch = MultiByteToWideChar (codePage, 0, reinterpret_cast<LPCCH> (pb), static_cast<int> (cb), nullptr, 0);

        if (cch > 0)
        {
            name.resize (static_cast<size_t> (cch));
            MultiByteToWideChar (codePage, 0, reinterpret_cast<LPCCH> (pb), static_cast<int> (cb), name.data(), cch);
        }
    }

    return name;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::IsFolderName
//
////////////////////////////////////////////////////////////////////////////////

bool CZipReader::IsFolderName (const wstring & name)
{
    return !name.empty() && (name.back() == L'/' || name.back() == L'\\');
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::Read16
//
//  Zip fields are little-endian and unaligned.
//
////////////////////////////////////////////////////////////////////////////////

WORD CZipReader::Read16 (const BYTE * pb)
{
    WORD w = 0;



    memcpy (&w, pb, sizeof (w));

    return w;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::Read32
//
////////////////////////////////////////////////////////////////////////////////

DWORD CZipReader::Read32 (const BYTE * pb)
{
    DWORD dw = 0;



    memcpy (&dw, pb, sizeof (dw));

    return dw;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::Read64
//
////////////////////////////////////////////////////////////////////////////////

ULONGLONG CZipReader::Read64 (const BYTE * pb)
{
    ULONGLONG ull = 0;



    memcpy (&ull, pb, sizeof (ull));

    return ull;
}





