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

    pRecord                = &tail[ibFound];
    ullEndOffset           = ullTail + ibFound;
    location.m_cEntries    = Read16 (pRecord + s_kibEndEntryCount);
    location.m_ullSize     = Read32 (pRecord + s_kibEndDirectorySize);
    location.m_ullOffset   = Read32 (pRecord + s_kibEndDirectoryOffset);

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



    result.m_vEntries.reserve (static_cast<size_t> (min<ULONGLONG> (cEntries, directory.size() / s_kcbCentralHeader)));

    for (ULONGLONG i = 0; i < cEntries; ++i)
    {
        hr = ParseEntry (directory.data() + ib, directory.size() - ib, cbEntry, entry);
        CHR (hr);

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

HRESULT CZipReader::ParseEntry (const BYTE * pb, size_t cbAvailable, size_t & cbEntry, SArchiveEntry & entry)
{
    HRESULT       hr        = S_OK;
    WORD          wFlags    = 0;
    size_t        cchName   = 0;
    size_t        cbExtra   = 0;
    size_t        cbComment = 0;
    DWORD         dwSize    = 0;
    DWORD         dwSig     = 0;
    SEntrySizes   sizes;



    entry = {};

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

    ParseExtraFields (pb + s_kcbCentralHeader + cchName, cbExtra, dwSize == s_kdwMax32, sizes);

    entry.m_strPath      = sizes.m_strUnicodePath.empty()
                               ? DecodeName (pb + s_kcbCentralHeader, cchName, (wFlags & s_kwFlagUtf8) ? CP_UTF8 : CP_OEMCP)
                               : sizes.m_strUnicodePath;
    entry.m_ullSize      = sizes.m_ullUncompressed;
    entry.m_ftLastWrite  = sizes.m_fHasNtfsTime ? sizes.m_ftNtfsLastWrite
                                                : DosToUtc (Read16 (pb + s_kibDate), Read16 (pb + s_kibTime));
    entry.m_dwAttributes = MapAttributes (Read16 (pb + s_kibVersionMadeBy), Read32 (pb + s_kibExternalAttributes));
    entry.m_fDirectory   = IsFolderName (entry.m_strPath) || (entry.m_dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    entry.m_fEncrypted   = (wFlags & s_kwFlagEncrypted) != 0;



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader::ParseExtraFields
//
//  Applies the ZIP64, NTFS and Info-ZIP Unicode Path extra fields.  The
//  uncompressed size is the first ZIP64 value, and is present only when the
//  header field is 0xFFFFFFFF.  Malformed extra data is ignored.
//
////////////////////////////////////////////////////////////////////////////////

void CZipReader::ParseExtraFields (const BYTE * pb, size_t cb, bool fZip64Size, SEntrySizes & sizes)
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

        if (wId == s_kwExtraZip64 && fZip64Size && cbField >= sizeof (ULONGLONG))
        {
            sizes.m_ullUncompressed = Read64 (pField);
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





