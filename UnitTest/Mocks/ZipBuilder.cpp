#include "pch.h"
#include "ZipBuilder.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::AddFile
//
////////////////////////////////////////////////////////////////////////////////

CZipBuilder & CZipBuilder::AddFile (const string & name, ULONGLONG ullSize)
{
    SEntryOptions options;



    options.m_strName = name;
    options.m_ullSize = ullSize;

    return AddEntry (options);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::AddFolder
//
//  Folder entries in a zip are entries whose name ends with '/'.
//
////////////////////////////////////////////////////////////////////////////////

CZipBuilder & CZipBuilder::AddFolder (const string & name)
{
    SEntryOptions options;



    options.m_strName = name;

    return AddEntry (options);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::AddEntry
//
////////////////////////////////////////////////////////////////////////////////

CZipBuilder & CZipBuilder::AddEntry (const SEntryOptions & options)
{
    m_vEntries.push_back (options);

    return *this;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::SetComment
//
////////////////////////////////////////////////////////////////////////////////

CZipBuilder & CZipBuilder::SetComment (const string & comment)
{
    m_strComment = comment;

    return *this;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::SetZip64EndRecord
//
//  Emits the ZIP64 end record and locator, and stores 0xFFFF and
//  0xFFFFFFFF markers in the classic end record so that a reader has to
//  use the ZIP64 values.
//
////////////////////////////////////////////////////////////////////////////////

CZipBuilder & CZipBuilder::SetZip64EndRecord (bool fZip64EndRecord)
{
    m_fZip64EndRecord = fZip64EndRecord;

    return *this;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::SetReportedEntryCount
//
//  Replaces the entry count that Build writes after the central directory,
//  so that the directory can contain fewer entries than the count indicates.
//
////////////////////////////////////////////////////////////////////////////////

CZipBuilder & CZipBuilder::SetReportedEntryCount (ULONGLONG cEntries)
{
    m_fReportedCountSet = true;
    m_cReportedEntries  = cEntries;

    return *this;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::SetPrefix
//
//  Places cbPrefix zero bytes, in place of a self-extractor program, before
//  the first local header.  Offsets in the directory account for them.
//
////////////////////////////////////////////////////////////////////////////////

CZipBuilder & CZipBuilder::SetPrefix (size_t cbPrefix)
{
    m_cbPrefix = cbPrefix;

    return *this;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::SetDiskNumbers
//
////////////////////////////////////////////////////////////////////////////////

CZipBuilder & CZipBuilder::SetDiskNumbers (WORD wDisk, WORD wDirectoryDisk)
{
    m_wDisk          = wDisk;
    m_wDirectoryDisk = wDirectoryDisk;

    return *this;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::Build
//
//  Layout: local headers, central directory, optional ZIP64 end record and
//  locator, end record, archive comment.
//
////////////////////////////////////////////////////////////////////////////////

vector<BYTE> CZipBuilder::Build() const
{
    vector<BYTE>        bytes (m_cbPrefix, 0);
    vector<ULONGLONG>   localOffsets;
    ULONGLONG           ullDirectoryOffset = 0;
    ULONGLONG           ullDirectorySize   = 0;
    ULONGLONG           cEntries           = m_fReportedCountSet ? m_cReportedEntries : m_vEntries.size();



    for (const SEntryOptions & options : m_vEntries)
    {
        localOffsets.push_back (bytes.size());
        AppendLocalHeader (bytes, options);
    }

    ullDirectoryOffset = bytes.size();

    for (size_t i = 0; i < m_vEntries.size(); ++i)
    {
        AppendCentralHeader (bytes, m_vEntries[i], localOffsets[i]);
    }

    ullDirectorySize = bytes.size() - ullDirectoryOffset;

    if (m_fZip64EndRecord)
    {
        AppendZip64End (bytes, cEntries, ullDirectorySize, ullDirectoryOffset);
    }

    AppendEndRecord (bytes, cEntries, ullDirectorySize, ullDirectoryOffset);

    return bytes;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::DosDate
//
////////////////////////////////////////////////////////////////////////////////

WORD CZipBuilder::DosDate (WORD wYear, WORD wMonth, WORD wDay)
{
    return static_cast<WORD> (((wYear - s_kwDosYearBase) << s_kcDosYearShift) | (wMonth << s_kcDosMonthShift) | wDay);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::DosTime
//
//  MS-DOS times have two-second resolution.
//
////////////////////////////////////////////////////////////////////////////////

WORD CZipBuilder::DosTime (WORD wHour, WORD wMinute, WORD wSecond)
{
    return static_cast<WORD> ((wHour << s_kcDosHourShift) | (wMinute << s_kcDosMinuteShift) | (wSecond / s_kwDosSecondScale));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::UsesZip64Size
//
////////////////////////////////////////////////////////////////////////////////

bool CZipBuilder::UsesZip64Size (const SEntryOptions & options)
{
    return options.m_fZip64Size || options.m_ullSize >= s_kdwMax32;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::FlagsOf
//
////////////////////////////////////////////////////////////////////////////////

WORD CZipBuilder::FlagsOf (const SEntryOptions & options)
{
    WORD wFlags = 0;



    wFlags |= options.m_fUtf8 ? s_kwFlagUtf8 : 0;
    wFlags |= (options.m_fEncrypted || options.m_bAesStrength != 0 || options.m_fStrongEncryption) ? s_kwFlagEncrypted : 0;
    wFlags |= options.m_fStrongEncryption ? s_kwFlagStrongEncryption : 0;

    return wFlags;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::MethodOf
//
////////////////////////////////////////////////////////////////////////////////

WORD CZipBuilder::MethodOf (const SEntryOptions & options)
{
    return options.m_bAesStrength != 0 ? s_kwMethodAes : options.m_wMethod;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::CompressedSizeOf
//
////////////////////////////////////////////////////////////////////////////////

ULONGLONG CZipBuilder::CompressedSizeOf (const SEntryOptions & options)
{
    return options.m_ullCompressedSize.value_or (options.m_ullSize);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::AppendLocalHeader
//
//  The reader parses only the central directory, so the local header has
//  no extra fields and no entry data follows it.
//
////////////////////////////////////////////////////////////////////////////////

void CZipBuilder::AppendLocalHeader (vector<BYTE> & bytes, const SEntryOptions & options)
{
    Append32    (bytes, s_kdwLocalSignature);
    Append16    (bytes, s_kwSpecVersion);
    Append16    (bytes, FlagsOf (options));
    Append16    (bytes, MethodOf (options));
    Append16    (bytes, options.m_wDosTime);
    Append16    (bytes, options.m_wDosDate);
    Append32    (bytes, 0);                                         // CRC-32
    Append32    (bytes, 0);                                         // Compressed size
    Append32    (bytes, 0);                                         // Uncompressed size
    Append16    (bytes, static_cast<WORD> (options.m_strName.size()));
    Append16    (bytes, 0);                                         // Extra field length
    AppendBytes (bytes, options.m_strName);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::AppendCentralHeader
//
////////////////////////////////////////////////////////////////////////////////

void CZipBuilder::AppendCentralHeader (vector<BYTE> & bytes, const SEntryOptions & options, ULONGLONG ullLocalOffset)
{
    vector<BYTE>   extra;
    bool           fZip64       = UsesZip64Size (options);
    DWORD          dwSize       = fZip64 ? s_kdwMax32 : static_cast<DWORD> (options.m_ullSize);
    DWORD          dwCompressed = fZip64 ? s_kdwMax32 : static_cast<DWORD> (CompressedSizeOf (options));
    WORD           wVersion     = static_cast<WORD> ((options.m_bHost << s_kcHostShift) | options.m_bVersion);



    AppendExtraFields (extra, options);

    Append32    (bytes, s_kdwCentralSignature);
    Append16    (bytes, wVersion);
    Append16    (bytes, fZip64 ? s_kwSpecVersionZip64 : s_kwSpecVersion);
    Append16    (bytes, FlagsOf (options));
    Append16    (bytes, MethodOf (options));
    Append16    (bytes, options.m_wDosTime);
    Append16    (bytes, options.m_wDosDate);
    Append32    (bytes, 0);                                         // CRC-32
    Append32    (bytes, dwCompressed);
    Append32    (bytes, dwSize);                                    // Uncompressed size
    Append16    (bytes, static_cast<WORD> (options.m_strName.size()));
    Append16    (bytes, static_cast<WORD> (extra.size()));
    Append16    (bytes, 0);                                         // Comment length
    Append16    (bytes, 0);                                         // Disk number start
    Append16    (bytes, 0);                                         // Internal attributes
    Append32    (bytes, options.m_dwExternalAttributes);
    Append32    (bytes, static_cast<DWORD> (ullLocalOffset));
    AppendBytes (bytes, options.m_strName);
    AppendBytes (bytes, extra);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::AppendExtraFields
//
//  The ZIP64 field contains the uncompressed size, then the compressed
//  size, matching the two 0xFFFFFFFF header fields.  The Unicode Path CRC
//  is left zero.
//
////////////////////////////////////////////////////////////////////////////////

void CZipBuilder::AppendExtraFields (vector<BYTE> & extra, const SEntryOptions & options)
{
    if (UsesZip64Size (options))
    {
        Append16 (extra, s_kwExtraZip64);
        Append16 (extra, static_cast<WORD> (sizeof (ULONGLONG) * 2));
        Append64 (extra, options.m_ullSize);
        Append64 (extra, CompressedSizeOf (options));
    }

    if (options.m_bAesStrength != 0)
    {
        Append16 (extra, s_kwExtraAes);
        Append16 (extra, s_kcbAesExtra);
        Append16 (extra, s_kwAesVersion);
        Append16 (extra, s_kwAesVendor);
        extra.push_back (options.m_bAesStrength);
        Append16 (extra, options.m_wMethod);
    }

    if (options.m_fNtfsTime)
    {
        Append16 (extra, s_kwExtraNtfs);
        Append16 (extra, static_cast<WORD> (sizeof (DWORD) + sizeof (WORD) * 2 + s_kcbNtfsTimes));
        Append32 (extra, 0);                                            // Reserved
        Append16 (extra, s_kwNtfsTimesTag);
        Append16 (extra, s_kcbNtfsTimes);
        Append64 (extra, options.m_ullNtfsLastWrite);
        Append64 (extra, options.m_ullNtfsLastWrite);                   // Last access
        Append64 (extra, options.m_ullNtfsLastWrite);                   // Creation
    }

    if (!options.m_strUnicodePath.empty())
    {
        Append16    (extra, s_kwExtraUnicodePath);
        Append16    (extra, static_cast<WORD> (sizeof (BYTE) + sizeof (DWORD) + options.m_strUnicodePath.size()));
        extra.push_back (s_kbUnicodePathVersion);
        Append32    (extra, 0);                                         // CRC-32 of the header name
        AppendBytes (extra, options.m_strUnicodePath);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::AppendZip64End
//
//  Appends the ZIP64 end record and the locator that points to it.
//
////////////////////////////////////////////////////////////////////////////////

void CZipBuilder::AppendZip64End (vector<BYTE> & bytes, ULONGLONG cEntries, ULONGLONG ullSize, ULONGLONG ullOffset)
{
    ULONGLONG ullRecordOffset = bytes.size();



    Append32 (bytes, s_kdwZip64EndSignature);
    Append64 (bytes, s_kcbZip64EndRemainder);
    Append16 (bytes, s_kwSpecVersionZip64);                             // Version made by
    Append16 (bytes, s_kwSpecVersionZip64);                             // Minimum version to extract
    Append32 (bytes, 0);                                                // This disk
    Append32 (bytes, 0);                                                // Disk with the central directory
    Append64 (bytes, cEntries);                                         // Entries on this disk
    Append64 (bytes, cEntries);                                         // Total entries
    Append64 (bytes, ullSize);
    Append64 (bytes, ullOffset);

    Append32 (bytes, s_kdwZip64LocatorSignature);
    Append32 (bytes, 0);                                                // Disk with the ZIP64 end record
    Append64 (bytes, ullRecordOffset);
    Append32 (bytes, 1);                                                // Total disks
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::AppendEndRecord
//
////////////////////////////////////////////////////////////////////////////////

void CZipBuilder::AppendEndRecord (vector<BYTE> & bytes, ULONGLONG cEntries, ULONGLONG ullSize, ULONGLONG ullOffset) const
{
    WORD    wCount   = m_fZip64EndRecord ? s_kwMax16 : static_cast<WORD> (cEntries);
    DWORD   dwSize   = m_fZip64EndRecord ? s_kdwMax32 : static_cast<DWORD> (ullSize);
    DWORD   dwOffset = m_fZip64EndRecord ? s_kdwMax32 : static_cast<DWORD> (ullOffset);



    Append32    (bytes, s_kdwEndSignature);
    Append16    (bytes, m_wDisk);                                       // This disk
    Append16    (bytes, m_wDirectoryDisk);                              // Disk with the central directory
    Append16    (bytes, wCount);                                        // Entries on this disk
    Append16    (bytes, wCount);                                        // Total entries
    Append32    (bytes, dwSize);
    Append32    (bytes, dwOffset);
    Append16    (bytes, static_cast<WORD> (m_strComment.size()));
    AppendBytes (bytes, m_strComment);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::Append16
//
//  Zip fields are little-endian.
//
////////////////////////////////////////////////////////////////////////////////

void CZipBuilder::Append16 (vector<BYTE> & bytes, WORD w)
{
    for (size_t i = 0; i < sizeof (w); ++i)
    {
        bytes.push_back (static_cast<BYTE> (w >> (i * CHAR_BIT)));
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::Append32
//
////////////////////////////////////////////////////////////////////////////////

void CZipBuilder::Append32 (vector<BYTE> & bytes, DWORD dw)
{
    for (size_t i = 0; i < sizeof (dw); ++i)
    {
        bytes.push_back (static_cast<BYTE> (dw >> (i * CHAR_BIT)));
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::Append64
//
////////////////////////////////////////////////////////////////////////////////

void CZipBuilder::Append64 (vector<BYTE> & bytes, ULONGLONG ull)
{
    for (size_t i = 0; i < sizeof (ull); ++i)
    {
        bytes.push_back (static_cast<BYTE> (ull >> (i * CHAR_BIT)));
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::AppendBytes
//
////////////////////////////////////////////////////////////////////////////////

void CZipBuilder::AppendBytes (vector<BYTE> & bytes, const string & text)
{
    bytes.insert (bytes.end(), text.begin(), text.end());
}





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder::AppendBytes
//
////////////////////////////////////////////////////////////////////////////////

void CZipBuilder::AppendBytes (vector<BYTE> & bytes, const vector<BYTE> & data)
{
    bytes.insert (bytes.end(), data.begin(), data.end());
}





