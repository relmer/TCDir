#pragma once

#include "ArchiveDetails.h"
#include "ByteSource.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoDetails
//
//  Reads the volume values of a disc image for --Info: labels and IDs,
//  dates, sector counts, Rock Ridge, El Torito boot entries, and the UDF
//  revision and label.  The parsers take sector bytes, so tests supply
//  synthetic sectors.
//
////////////////////////////////////////////////////////////////////////////////

class CIsoDetails
{
public:
    static void               Read                  (IByteSource & source, SDiscDetails & disc);

    static void               ParseVolumeDescriptor (const BYTE * pSector, SDiscDetails & disc, ULONG & ulRootExtent, ULONG & ulCatalog, bool & fEnd);
    static bool               HasRockRidge          (const BYTE * pDirectory, size_t cb);
    static void               ParseBootCatalog      (const BYTE * pCatalog, size_t cb, vector<BYTE> & vPlatforms);
    static void               ParseLogicalVolume    (const BYTE * pSector, SDiscDetails & disc);
    static optional<FILETIME> ParseVolumeDate       (const BYTE * pDate);
    static wstring            DecodeText            (const BYTE * p, size_t cb, bool fUcs2);

    static constexpr BYTE     s_kPlatformBios         = 0x00;
    static constexpr BYTE     s_kPlatformPowerPc      = 0x01;
    static constexpr BYTE     s_kPlatformMac          = 0x02;
    static constexpr BYTE     s_kPlatformUefi         = 0xEF;

private:
    static void               ReadIso9660           (IByteSource & source, SDiscDetails & disc);
    static void               ReadUdf               (IByteSource & source, SDiscDetails & disc);
    static void               ParsePrimaryFields    (const BYTE * pSector, bool fJoliet, SDiscDetails & disc);
    static void               AddPlatform           (BYTE bPlatform, vector<BYTE> & vPlatforms);
    static int                Digits                (const BYTE * p, size_t cch);

    //
    // ISO 9660 volume descriptor layout (ECMA-119 8.4)
    //

    static constexpr ULONG    s_kFirstDescriptor      = 16;
    static constexpr ULONG    s_kMaxDescriptors       = 64;
    static constexpr BYTE     s_kTypeBootRecord       = 0;
    static constexpr BYTE     s_kTypePrimary          = 1;
    static constexpr BYTE     s_kTypeSupplementary    = 2;
    static constexpr BYTE     s_kTypeTerminator       = 255;
    static constexpr size_t   s_kibStandardId         = 1;
    static constexpr size_t   s_kcchStandardId        = 5;
    static constexpr size_t   s_kibSystemId           = 8;
    static constexpr size_t   s_kcchSystemId          = 32;
    static constexpr size_t   s_kibVolumeId           = 40;
    static constexpr size_t   s_kcchVolumeId          = 32;
    static constexpr size_t   s_kibVolumeSpaceSize    = 80;
    static constexpr size_t   s_kibEscapeSequences    = 88;
    static constexpr size_t   s_kibLogicalBlockSize   = 128;
    static constexpr size_t   s_kibRootRecord         = 156;
    static constexpr size_t   s_kibRecordExtent       = 2;
    static constexpr size_t   s_kibPublisherId        = 318;
    static constexpr size_t   s_kibPreparerId         = 446;
    static constexpr size_t   s_kibApplicationId      = 574;
    static constexpr size_t   s_kcchLongId            = 128;
    static constexpr size_t   s_kibCreationDate       = 813;
    static constexpr size_t   s_kibModificationDate   = 830;
    static constexpr size_t   s_kibExpirationDate     = 847;
    static constexpr size_t   s_kibBootSystemId       = 7;
    static constexpr size_t   s_kibBootCatalog        = 71;
    static constexpr char     s_kszElTorito[]         = "EL TORITO SPECIFICATION";

    //
    // Volume dates: "YYYYMMDDHHMMSScc" digits, then the UTC offset in
    // 15-minute units
    //

    static constexpr size_t   s_kcchYear              = 4;
    static constexpr size_t   s_kcchDateField         = 2;
    static constexpr size_t   s_kcchDateDigits        = 16;
    static constexpr size_t   s_kibCentiseconds       = 14;
    static constexpr int      s_kMsPerCentisecond     = 10;
    static constexpr int      s_kMinutesPerOffsetUnit = 15;
    static constexpr int      s_kDecimalBase          = 10;

    //
    // Directory entries and the System Use Sharing Protocol (Rock Ridge)
    //

    static constexpr size_t   s_kibRecordIdLength     = 32;
    static constexpr size_t   s_kibRecordId           = 33;
    static constexpr size_t   s_kcbSuspHeader         = 4;
    static constexpr size_t   s_kibSuspLength         = 2;
    static constexpr size_t   s_kcchSuspSignature     = 2;

    //
    // El Torito boot catalog entries
    //

    static constexpr size_t   s_kcbCatalogEntry       = 32;
    static constexpr BYTE     s_kHeaderValidation     = 0x01;
    static constexpr BYTE     s_kHeaderSection        = 0x90;
    static constexpr BYTE     s_kHeaderFinalSection   = 0x91;
    static constexpr size_t   s_kibPlatform           = 1;

    //
    // UDF anchor and logical volume descriptor (ECMA-167, UDF 2.60)
    //

    static constexpr ULONG    s_kAnchorSector         = 256;
    static constexpr USHORT   s_kTagAnchor            = 2;
    static constexpr USHORT   s_kTagLogicalVolume     = 6;
    static constexpr USHORT   s_kTagTerminating       = 8;
    static constexpr size_t   s_kibAnchorMainVds      = 16;
    static constexpr ULONG    s_kMaxVdsSectors        = 64;
    static constexpr size_t   s_kibLvdIdentifier      = 84;
    static constexpr size_t   s_kcbLvdIdentifier      = 128;
    static constexpr size_t   s_kibLvdDomainId        = 216;
    static constexpr size_t   s_kibRegIdSuffix        = 24;      // After the flags byte and the 23-byte identifier
    static constexpr size_t   s_kibLvdDomainSuffix    = s_kibLvdDomainId + s_kibRegIdSuffix;
};
