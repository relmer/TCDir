#pragma once





////////////////////////////////////////////////////////////////////////////////
//
//  SIsoTestNode
//
//  One directory record of a test tree for ISO 9660.  m_strName is the identifier as
//  stored, for example L"README.TXT;1".  A directory with a nonzero
//  m_ulExtent links to that existing extent instead of being written.
//
////////////////////////////////////////////////////////////////////////////////

struct SIsoTestNode
{
    wstring                m_strName;
    bool                   m_fDirectory = false;
    BYTE                   m_bFlags     = 0;
    ULONG                  m_cbSize     = 0;
    ULONG                  m_ulExtent   = 0;
    vector<SIsoTestNode>   m_vChildren;
};





////////////////////////////////////////////////////////////////////////////////
//
//  SUdfTestNode
//
//  One file or directory of a UDF test tree.  m_bAllocation selects how a
//  directory's data is stored: 0 short_ad, 1 long_ad, 3 embedded.
//
////////////////////////////////////////////////////////////////////////////////

struct SUdfTestNode
{
    wstring                m_strName;
    bool                   m_fDirectory  = false;
    bool                   m_fHidden     = false;
    bool                   m_f16BitName  = false;
    bool                   m_fExtended   = false;
    BYTE                   m_bAllocation = 0;
    ULONGLONG              m_ullSize     = 0;
    vector<SUdfTestNode>   m_vChildren;
};





enum class EUdfVariant
{
    Standard,
    MetadataPartition,
    MetadataPartitionEmptyBridge
};





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder
//
//  Builds in-memory ISO 9660, Joliet and UDF disc images for reader tests.
//  Every record is stamped with the time returned by ExpectedTime.
//
////////////////////////////////////////////////////////////////////////////////

class CIsoImageBuilder
{
public:
    static constexpr ULONG       s_kcbSector                = 2048;
    static constexpr ULONG       s_kFirstDescriptor         = 16;
    static constexpr ULONG       s_kFirstDataSector         = 32;
    static constexpr ULONG       s_kAnchorSector            = 256;
    static constexpr ULONG       s_kcbReadme                = 77;
    static constexpr ULONG       s_kcbHidden                = 11;
    static constexpr ULONG       s_kcbNoExtension           = 5;
    static constexpr ULONG       s_kcbNote                  = 300;
    static constexpr ULONG       s_kcbFirstExtent           = 0xFFFFF800;
    static constexpr ULONG       s_kcbLastExtent            = 0x1000;
    static constexpr ULONG       s_kcManyFiles              = 60;
    static constexpr ULONG       s_kcbManyFile              = 9;
    static constexpr ULONGLONG   s_kcbSetup                 = 123456;
    static constexpr ULONGLONG   s_kcbInstallWim            = 5000000000ULL;
    static constexpr ULONGLONG   s_kcbUnicodeFile           = 42;
    static constexpr ULONGLONG   s_kcbBcd                   = 16384;
    static constexpr ULONGLONG   s_kcbHiddenSys             = 10;
    static constexpr ULONGLONG   s_kcbFont                  = 1000;
    static constexpr LPCWSTR     s_kpszUnicodeName          = L"\u65E5\u672C\u8A9E.txt";

    static vector<BYTE> BuildPrimaryOnly();
    static vector<BYTE> BuildPrimaryAndJoliet();
    static vector<BYTE> BuildNested();
    static vector<BYTE> BuildMultiExtent();
    static vector<BYTE> BuildCycle();
    static vector<BYTE> BuildUdf (EUdfVariant variant);
    static FILETIME     ExpectedTime();

private:
    struct SUdfLayout
    {
        size_t   m_ibModified;
        size_t   m_ibAdLength;
        size_t   m_ibAds;
    };

    static constexpr int         s_kBitsPerByte             = 8;
    static constexpr USHORT      s_kTestYear                = 2024;
    static constexpr BYTE        s_kTestMonth               = 3;
    static constexpr BYTE        s_kTestDay                 = 15;
    static constexpr BYTE        s_kTestHour                = 10;
    static constexpr BYTE        s_kTestMinute              = 30;
    static constexpr USHORT      s_kTestOffsetMinutes       = 60;
    static constexpr USHORT      s_kMinutesPerHour          = 60;
    static constexpr USHORT      s_kYearBase                = 1900;
    static constexpr USHORT      s_kMinutesPerOffsetUnit    = 15;

    static constexpr BYTE        s_kTypePrimary             = 1;
    static constexpr BYTE        s_kTypeSupplementary       = 2;
    static constexpr BYTE        s_kTypeTerminator          = 255;
    static constexpr size_t      s_kibStandardId            = 1;
    static constexpr size_t      s_kcchStandardId           = 5;
    static constexpr size_t      s_kibDescriptorVersion     = 6;
    static constexpr size_t      s_kibEscapeSequences       = 88;
    static constexpr size_t      s_kcchEscapeSequence       = 3;
    static constexpr size_t      s_kibLogicalBlockSize      = 128;
    static constexpr size_t      s_kibRootRecord            = 156;
    static constexpr size_t      s_kibRecordExtent          = 2;
    static constexpr size_t      s_kibRecordDataLength      = 10;
    static constexpr size_t      s_kibRecordDate            = 18;
    static constexpr size_t      s_kibRecordFlags           = 25;
    static constexpr size_t      s_kibRecordVolumeSequence  = 28;
    static constexpr size_t      s_kibRecordIdLength        = 32;
    static constexpr size_t      s_kibRecordId              = 33;
    static constexpr size_t      s_kcSelfAndParent          = 2;
    static constexpr BYTE        s_kFlagHidden              = 0x01;
    static constexpr BYTE        s_kFlagDirectory           = 0x02;
    static constexpr BYTE        s_kFlagMultiExtent         = 0x80;
    static constexpr BYTE        s_krgbRecordingDate[]      = { s_kTestYear - s_kYearBase,
                                                                s_kTestMonth,
                                                                s_kTestDay,
                                                                s_kTestHour,
                                                                s_kTestMinute,
                                                                0,
                                                                s_kTestOffsetMinutes / s_kMinutesPerOffsetUnit };

    static constexpr ULONG       s_kPartitionSector         = 257;
    static constexpr ULONG       s_kLogicalVolumeSector     = 258;
    static constexpr ULONG       s_kTerminatingSector       = 259;
    static constexpr ULONG       s_kVdsSectors              = 16;
    static constexpr ULONG       s_kPartitionStart          = 260;
    static constexpr ULONG       s_kPartitionBlocks         = 1000;
    static constexpr ULONG       s_kParentIsSelf            = ULONG_MAX;
    static constexpr size_t      s_kcbTag                   = 16;
    static constexpr size_t      s_kibTagVersion            = 2;
    static constexpr USHORT      s_kTagVersion              = 2;
    static constexpr size_t      s_kibTagChecksum           = 4;
    static constexpr size_t      s_kibTagLocation           = 12;
    static constexpr USHORT      s_kTagAnchor               = 2;
    static constexpr USHORT      s_kTagPartition            = 5;
    static constexpr USHORT      s_kTagLogicalVolume        = 6;
    static constexpr USHORT      s_kTagTerminating          = 8;
    static constexpr USHORT      s_kTagFileSet              = 256;
    static constexpr USHORT      s_kTagFileId               = 257;
    static constexpr USHORT      s_kTagFileEntry            = 261;
    static constexpr USHORT      s_kTagExtendedEntry        = 266;
    static constexpr size_t      s_kibAnchorMainVds         = 16;
    static constexpr size_t      s_kibPartitionStart        = 188;
    static constexpr size_t      s_kibPartitionLength       = 192;
    static constexpr size_t      s_kibLvdBlockSize          = 212;
    static constexpr size_t      s_kibLvdFileSet            = 248;
    static constexpr size_t      s_kibLvdMapTableLength     = 264;
    static constexpr size_t      s_kibLvdMapCount           = 268;
    static constexpr size_t      s_kibLvdMaps               = 440;
    static constexpr BYTE        s_kMapTypePhysical         = 1;
    static constexpr BYTE        s_kMapTypeIdentified       = 2;
    static constexpr BYTE        s_kcbPhysicalMap           = 6;
    static constexpr BYTE        s_kcbIdentifiedMap         = 64;
    static constexpr size_t      s_kibMapVolumeSequence     = 2;
    static constexpr size_t      s_kibMapIdentifier         = 5;
    static constexpr char        s_kszMetadataMapId[]       = "*UDF Metadata Partition";
    static constexpr size_t      s_kibFsdRootIcb            = 400;
    static constexpr size_t      s_kibIcbFileType           = 27;
    static constexpr size_t      s_kibIcbFlags              = 34;
    static constexpr size_t      s_kibInformationLength     = 56;
    static constexpr BYTE        s_kFileTypeDirectory       = 4;
    static constexpr BYTE        s_kFileTypeFile            = 5;
    static constexpr BYTE        s_kAllocShort              = 0;
    static constexpr BYTE        s_kAllocLong               = 1;
    static constexpr BYTE        s_kAllocEmbedded           = 3;
    static constexpr size_t      s_kcbShortAd               = 8;
    static constexpr size_t      s_kcbLongAd                = 16;
    static constexpr size_t      s_kibFidVersion            = 16;
    static constexpr size_t      s_kibFidCharacteristics    = 18;
    static constexpr size_t      s_kibFidNameLength         = 19;
    static constexpr size_t      s_kibFidIcb                = 20;
    static constexpr size_t      s_kcbFidHeader             = 38;
    static constexpr size_t      s_kcbFidAlignment          = 4;
    static constexpr BYTE        s_kFidHidden               = 0x01;
    static constexpr BYTE        s_kFidDirectory            = 0x02;
    static constexpr BYTE        s_kFidParent               = 0x08;
    static constexpr BYTE        s_kCompression8            = 8;
    static constexpr BYTE        s_kCompression16           = 16;
    static constexpr USHORT      s_kTimestampLocal          = 0x1000;
    static constexpr size_t      s_kibTimestampYear         = 2;
    static constexpr size_t      s_kibTimestampMonth        = 4;
    static constexpr size_t      s_kibTimestampDay          = 5;
    static constexpr size_t      s_kibTimestampHour         = 6;
    static constexpr size_t      s_kibTimestampMinute       = 7;
    static constexpr SUdfLayout  s_kFileEntryLayout         = { 84, 172, 176 };
    static constexpr SUdfLayout  s_kExtendedLayout          = { 92, 212, 216 };

    BYTE *                Sector             (ULONG ulSector);
    BYTE *                Block              (ULONG ulBlock);
    ULONG                 AllocateSectors    (ULONG cSectors);
    ULONG                 AllocateBlocks     (ULONG cBlocks);

    void                  WriteIsoVolume     (const SIsoTestNode & primaryRoot, const SIsoTestNode * pJolietRoot, bool fUdf);
    void                  WriteDescriptor    (ULONG ulSector, BYTE bType, ULONG ulRootExtent, ULONG cbRoot);
    void                  WriteVrs           (ULONG ulSector);
    void                  WriteIsoDirectory  (const SIsoTestNode & directory, bool fJoliet, ULONG & ulExtent, ULONG & cbData);
    void                  ResolveChildren    (const SIsoTestNode & directory, bool fJoliet, vector<ULONG> & vExtents, vector<ULONG> & vSizes);

    void                  WriteUdfVolume     (const SUdfTestNode & root, EUdfVariant variant);
    void                  WriteAnchorAndVds  (EUdfVariant variant);
    ULONG                 WriteUdfNode       (const SUdfTestNode & node, ULONG ulParentBlock);
    vector<BYTE>          BuildFids          (const SUdfTestNode & directory, ULONG ulSelfBlock, ULONG ulParentBlock);
    vector<BYTE>          StoreDirectoryData (const SUdfTestNode & directory, const vector<BYTE> & vFids);
    void                  WriteFileEntry     (ULONG ulBlock, const SUdfTestNode & node, ULONGLONG ullSize, const vector<BYTE> & vAds);

    static void           WriteLogicalVolume (BYTE * p, EUdfVariant variant);
    static vector<BYTE>   EncodeIsoName      (const wstring & strName, bool fJoliet);
    static size_t         RecordLength       (size_t cbId);
    static vector<size_t> LayoutRecords      (const vector<vector<BYTE>> & vIds, ULONG & cbData);
    static void           WriteRecord        (BYTE * p, ULONG ulExtent, ULONG cbData, BYTE bFlags, const vector<BYTE> & vId);
    static vector<BYTE>   EncodeOstaName     (const wstring & strName, bool f16Bit);
    static vector<BYTE>   EncodeFid          (BYTE bCharacteristics, ULONG ulIcbBlock, const vector<BYTE> & vName);
    static void           WriteTimestamp     (BYTE * p);
    static void           FinishTag          (BYTE * p, USHORT usTagId, ULONG ulLocation);
    static void           PutU16             (BYTE * p, USHORT us);
    static void           PutU32             (BYTE * p, ULONG ul);
    static void           PutU64             (BYTE * p, ULONGLONG ull);
    static void           PutBoth16          (BYTE * p, USHORT us);
    static void           PutBoth32          (BYTE * p, ULONG ul);
    static void           PutLongAd          (BYTE * p, ULONG cb, ULONG ulBlock);

    static SIsoTestNode   IsoFile            (LPCWSTR pszName, ULONG cb, BYTE bFlags = 0);
    static SIsoTestNode   IsoDirectory       (LPCWSTR pszName, vector<SIsoTestNode> vChildren);
    static SUdfTestNode   UdfFile            (LPCWSTR pszName, ULONGLONG cb);
    static SUdfTestNode   UdfDirectory       (LPCWSTR pszName, BYTE bAllocation, vector<SUdfTestNode> vChildren);
    static SUdfTestNode   UdfTree();

    vector<BYTE>   m_vImage;
    ULONG          m_ulNextSector = s_kFirstDataSector;
    ULONG          m_ulNextBlock  = 0;
};
