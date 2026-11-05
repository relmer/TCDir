#pragma once

#include "ArchiveEntry.h"
#include "ByteSource.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CZipReader
//
//  Reads the entry list of a .zip file from its central directory.  Entry
//  data is never read or decompressed.
//
////////////////////////////////////////////////////////////////////////////////

class CZipReader
{
public:
    static HRESULT    Read                (IByteSource & source, SArchiveReadResult & result);

    static SYSTEMTIME DosToSystemTime     (WORD wDate, WORD wTime);
    static DWORD      MapAttributes       (WORD wVersionMadeBy, DWORD dwExternalAttributes);
    static wstring    DecodeName          (const BYTE * pb, size_t cb, UINT codePage);

private:
    struct SDirectoryLocation
    {
        ULONGLONG m_ullOffset  = 0;
        ULONGLONG m_ullSize    = 0;
        ULONGLONG m_cEntries   = 0;
    };

    struct SEntrySizes
    {
        ULONGLONG m_ullUncompressed = 0;
        bool      m_fHasNtfsTime    = false;
        FILETIME  m_ftNtfsLastWrite = {};
        wstring   m_strUnicodePath;
    };

    static HRESULT    FindEndRecord       (IByteSource & source, ULONGLONG & ullEndOffset, SDirectoryLocation & location);
    static HRESULT    ReadZip64EndRecord  (IByteSource & source, ULONGLONG ullEndOffset, SDirectoryLocation & location);
    static HRESULT    ParseDirectory      (const vector<BYTE> & directory, ULONGLONG cEntries, SArchiveReadResult & result);
    static HRESULT    ParseEntry          (const BYTE * pb, size_t cbAvailable, size_t & cbEntry, SArchiveEntry & entry);
    static void       ParseExtraFields    (const BYTE * pb, size_t cb, bool fZip64Size, SEntrySizes & sizes);
    static void       ParseNtfsExtra      (const BYTE * pb, size_t cb, SEntrySizes & sizes);
    static FILETIME   DosToUtc            (WORD wDate, WORD wTime);
    static bool       IsFolderName        (const wstring & name);

    static WORD       Read16              (const BYTE * pb);
    static DWORD      Read32              (const BYTE * pb);
    static ULONGLONG  Read64              (const BYTE * pb);

    //
    // Record signatures and layouts (APPNOTE.TXT 4.3)
    //

    static constexpr DWORD  s_kdwEndSignature            = 0x06054B50;
    static constexpr DWORD  s_kdwZip64LocatorSignature   = 0x07064B50;
    static constexpr DWORD  s_kdwZip64EndSignature       = 0x06064B50;
    static constexpr DWORD  s_kdwCentralSignature        = 0x02014B50;

    static constexpr size_t s_kcbEndRecord               = 22;
    static constexpr size_t s_kcbMaxComment              = 0xFFFF;
    static constexpr size_t s_kcbZip64Locator            = 20;
    static constexpr size_t s_kcbZip64EndRecord          = 56;
    static constexpr size_t s_kcbCentralHeader           = 46;
    static constexpr size_t s_kcbExtraHeader             = 4;
    static constexpr size_t s_kcbNtfsReserved            = 4;
    static constexpr size_t s_kcbFileTime                = 8;

    static constexpr size_t s_kibEndEntryCount           = 10;
    static constexpr size_t s_kibEndDirectorySize        = 12;
    static constexpr size_t s_kibEndDirectoryOffset      = 16;
    static constexpr size_t s_kibEndCommentLength        = 20;
    static constexpr size_t s_kibLocatorEndOffset        = 8;
    static constexpr size_t s_kibZip64EntryCount         = 32;
    static constexpr size_t s_kibZip64DirectorySize      = 40;
    static constexpr size_t s_kibZip64DirectoryOffset    = 48;

    static constexpr size_t s_kibVersionMadeBy           = 4;
    static constexpr size_t s_kibFlags                   = 8;
    static constexpr size_t s_kibTime                    = 12;
    static constexpr size_t s_kibDate                    = 14;
    static constexpr size_t s_kibUncompressedSize        = 24;
    static constexpr size_t s_kibNameLength              = 28;
    static constexpr size_t s_kibExtraLength             = 30;
    static constexpr size_t s_kibCommentLength           = 32;
    static constexpr size_t s_kibExternalAttributes      = 38;

    static constexpr WORD   s_kwFlagEncrypted            = 0x0001;
    static constexpr WORD   s_kwFlagUtf8                 = 0x0800;
    static constexpr WORD   s_kwExtraZip64               = 0x0001;
    static constexpr WORD   s_kwExtraNtfs                = 0x000A;
    static constexpr WORD   s_kwExtraUnicodePath         = 0x7075;
    static constexpr WORD   s_kwNtfsTimesTag             = 0x0001;
    static constexpr BYTE   s_kbUnicodePathVersion       = 1;
    static constexpr size_t s_kcbUnicodePathHeader       = 5;      // Version byte plus CRC-32 of the header name

    static constexpr DWORD  s_kdwMax32                   = 0xFFFFFFFF;
    static constexpr WORD   s_kwMax16                    = 0xFFFF;
    static constexpr DWORD  s_kcbMaxDirectory            = 0x7FFFFFFF;

    //
    // "Version made by" host systems and their attribute layouts
    //

    static constexpr int    s_kcHostShift                = 8;
    static constexpr BYTE   s_kbHostMsDos                = 0;
    static constexpr BYTE   s_kbHostUnix                 = 3;
    static constexpr BYTE   s_kbHostNtfs                 = 10;
    static constexpr BYTE   s_kbHostVfat                 = 14;
    static constexpr int    s_kcUnixModeShift            = 16;
    static constexpr DWORD  s_kdwUnixTypeMask            = 0170000;
    static constexpr DWORD  s_kdwUnixDirectory           = 0040000;
    static constexpr DWORD  s_kdwUnixOwnerWrite          = 0000200;
    static constexpr DWORD  s_kdwDosAttributeMask        = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM |
                                                           FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE;

    //
    // MS-DOS date and time bit fields
    //

    static constexpr WORD   s_kwDosYearBase              = 1980;
    static constexpr int    s_kcDosYearShift             = 9;
    static constexpr int    s_kcDosMonthShift            = 5;
    static constexpr WORD   s_kwDosMonthMask             = 0x0F;
    static constexpr WORD   s_kwDosDayMask               = 0x1F;
    static constexpr int    s_kcDosHourShift             = 11;
    static constexpr int    s_kcDosMinuteShift           = 5;
    static constexpr WORD   s_kwDosMinuteMask            = 0x3F;
    static constexpr WORD   s_kwDosSecondMask            = 0x1F;
    static constexpr WORD   s_kwDosSecondScale           = 2;
};
