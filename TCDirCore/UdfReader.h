#pragma once

#include "ArchiveEntry.h"
#include "ByteSource.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader
//
//  Lists the files in a UDF disc image that uses a single physical partition.
//
////////////////////////////////////////////////////////////////////////////////

class CUdfReader
{
public:
    static HRESULT Read           (IByteSource & source, SArchiveReadResult & result);
    static HRESULT DecodeOstaName (const BYTE * p, size_t cb, wstring & strName);

private:
    struct SExtent
    {
        ULONG   m_cb         = 0;
        ULONG   m_ulLocation = 0;
    };

    struct SLongAd
    {
        ULONG    m_cb          = 0;
        BYTE     m_bType       = 0;
        ULONG    m_ulBlock     = 0;
        USHORT   m_usPartition = 0;
    };

    struct SVolume
    {
        unordered_map<USHORT, ULONG>   m_mapPartitionStart;
        vector<USHORT>                 m_vMapPartitions;
        vector<ULONG>                  m_vPartitionStarts;
        SLongAd                        m_fileSet;
        bool                           m_fLogicalVolume = false;
    };

    struct SFileEntry
    {
        BYTE           m_bFileType   = 0;
        USHORT         m_usIcbFlags  = 0;
        ULONGLONG      m_ullSize     = 0;
        FILETIME       m_ftModified  = {};
        USHORT         m_usPartition = 0;
        vector<BYTE>   m_vAllocation;
    };

    struct SFileId
    {
        BYTE      m_bCharacteristics = 0;
        SLongAd   m_icb;
        wstring   m_strName;
    };

    struct SEntryLayout
    {
        size_t   m_ibModified;
        size_t   m_ibEaLength;
        size_t   m_ibAdLength;
        size_t   m_ibExtendedAttributes;
    };

    struct SWalkState
    {
        SWalkState (IByteSource & source, const SVolume & volume, SArchiveReadResult & result) :
            m_source (source), m_volume (volume), m_result (result) {}

        IByteSource              & m_source;
        const SVolume            & m_volume;
        SArchiveReadResult       & m_result;
        unordered_set<ULONGLONG>   m_setVisited;
    };

    static constexpr ULONG        s_kVrsFirstSector            = 16;
    static constexpr ULONG        s_kMaxVrsSectors             = 64;
    static constexpr DWORD        s_kcbVrsHeader               = 7;
    static constexpr size_t       s_kibVrsId                   = 1;
    static constexpr size_t       s_kcchVrsId                  = 5;
    static constexpr ULONG        s_kAnchorSector              = 256;
    static constexpr ULONG        s_kMaxVdsSectors             = 64;
    static constexpr UINT         s_kMaxDepth                  = 128;
    static constexpr ULONGLONG    s_kcbMaxDirectory            = 16 * 1024 * 1024;
    static constexpr ULONG        s_kNoLocation                = ULONG_MAX;
    static constexpr int          s_kVisitedPartitionShift     = 32;

    static constexpr size_t       s_kcbTag                     = 16;
    static constexpr size_t       s_kibTagChecksum             = 4;
    static constexpr size_t       s_kibTagLocation             = 12;
    static constexpr USHORT       s_kTagAnchor                 = 2;
    static constexpr USHORT       s_kTagPartition              = 5;
    static constexpr USHORT       s_kTagLogicalVolume          = 6;
    static constexpr USHORT       s_kTagTerminating            = 8;
    static constexpr USHORT       s_kTagFileSet                = 256;
    static constexpr USHORT       s_kTagFileId                 = 257;
    static constexpr USHORT       s_kTagFileEntry              = 261;
    static constexpr USHORT       s_kTagExtendedFileEntry      = 266;

    static constexpr size_t       s_kibAnchorMainVds           = 16;
    static constexpr size_t       s_kibPartitionNumber         = 22;
    static constexpr size_t       s_kibPartitionStart          = 188;
    static constexpr size_t       s_kibLvdBlockSize            = 212;
    static constexpr size_t       s_kibLvdFileSet              = 248;
    static constexpr size_t       s_kibLvdMapTableLength       = 264;
    static constexpr size_t       s_kibLvdMapCount             = 268;
    static constexpr size_t       s_kibLvdMaps                 = 440;
    static constexpr size_t       s_kcbMapHeader               = 2;
    static constexpr BYTE         s_kMapTypePhysical           = 1;
    static constexpr BYTE         s_kMapTypeIdentified         = 2;
    static constexpr BYTE         s_kcbPhysicalMap             = 6;
    static constexpr size_t       s_kibPhysicalMapPartition    = 4;
    static constexpr size_t       s_kibFsdRootIcb              = 400;

    static constexpr size_t       s_kibIcbFileType             = 27;
    static constexpr size_t       s_kibIcbFlags                = 34;
    static constexpr size_t       s_kibInformationLength       = 56;
    static constexpr BYTE         s_kFileTypeDirectory         = 4;
    static constexpr USHORT       s_kIcbAllocationMask         = 0x0007;
    static constexpr USHORT       s_kIcbShortAd                = 0;
    static constexpr USHORT       s_kIcbLongAd                 = 1;
    static constexpr USHORT       s_kIcbEmbedded               = 3;

    static constexpr size_t       s_kibAdPosition              = 4;
    static constexpr size_t       s_kibLongAdPartition         = 8;
    static constexpr size_t       s_kcbShortAd                 = 8;
    static constexpr size_t       s_kcbLongAd                  = 16;
    static constexpr ULONG        s_kExtentLengthMask          = 0x3FFFFFFF;
    static constexpr int          s_kExtentTypeShift           = 30;
    static constexpr BYTE         s_kExtentRecorded            = 0;
    static constexpr BYTE         s_kExtentContinuation        = 3;

    static constexpr size_t       s_kibFidCharacteristics      = 18;
    static constexpr size_t       s_kibFidNameLength           = 19;
    static constexpr size_t       s_kibFidIcb                  = 20;
    static constexpr size_t       s_kibFidImplUseLength        = 36;
    static constexpr size_t       s_kcbFidHeader               = 38;
    static constexpr size_t       s_kcbFidAlignment            = 4;
    static constexpr BYTE         s_kFidHidden                 = 0x01;
    static constexpr BYTE         s_kFidDirectory              = 0x02;
    static constexpr BYTE         s_kFidDeleted                = 0x04;
    static constexpr BYTE         s_kFidParent                 = 0x08;
    static constexpr BYTE         s_kCompression8              = 8;
    static constexpr BYTE         s_kCompression16             = 16;

    static constexpr USHORT       s_kTimezoneMask              = 0x0FFF;
    static constexpr int          s_kTimezoneSignBit           = 0x0800;
    static constexpr int          s_kTimezoneRange             = 0x1000;
    static constexpr int          s_kTimezoneUnspecified       = -2047;
    static constexpr size_t       s_kibTimestampYear           = 2;
    static constexpr size_t       s_kibTimestampMonth          = 4;
    static constexpr size_t       s_kibTimestampDay            = 5;
    static constexpr size_t       s_kibTimestampHour           = 6;
    static constexpr size_t       s_kibTimestampMinute         = 7;
    static constexpr size_t       s_kibTimestampSecond         = 8;
    static constexpr size_t       s_kibTimestampCentiseconds   = 9;
    static constexpr WORD         s_kMsPerCentisecond          = 10;

    static constexpr SEntryLayout s_kFileEntryLayout           = { 84, 168, 172, 176 };
    static constexpr SEntryLayout s_kExtendedFileEntryLayout   = { 92, 208, 212, 216 };

    static HRESULT FindRecognitionSequence (IByteSource & source);
    static void    ClassifyVrsDescriptor   (const BYTE * pDescriptor, bool & fBea, bool & fNsr, bool & fEnd);
    static bool    IsTagValid              (const BYTE * p, size_t cb, ULONG ulLocation);
    static HRESULT VerifyTag               (const vector<BYTE> & vBlock, USHORT usTagId, ULONG ulLocation);
    static SLongAd ParseLongAd             (const BYTE * p);
    static SLongAd ParseShortAd            (const BYTE * p, USHORT usPartition);
    static HRESULT ReadAnchor              (IByteSource & source, SExtent & vds);
    static HRESULT ReadVolumeDescriptors   (IByteSource & source, const SExtent & vds, SVolume & volume);
    static HRESULT ParseVolumeDescriptor   (const vector<BYTE> & vSector, ULONG ulSector, SVolume & volume, bool & fEnd);
    static HRESULT ParseLogicalVolume      (const vector<BYTE> & vSector, SVolume & volume);
    static HRESULT ParsePartitionMaps      (const BYTE * pMaps, ULONG cbMaps, ULONG cMaps, SVolume & volume);
    static HRESULT ResolvePartitions       (SVolume & volume);
    static HRESULT ReadBlock               (IByteSource & source, const SVolume & volume, const SLongAd & ad, ULONG cb, vector<BYTE> & vBlock);
    static HRESULT ReadFileSet             (IByteSource & source, const SVolume & volume, SLongAd & rootIcb);
    static HRESULT ReadFileEntry           (SWalkState & state, const SLongAd & icb, SFileEntry & fe);
    static HRESULT ParseFileEntry          (const vector<BYTE> & vBlock, const SEntryLayout & layout, SFileEntry & fe);
    static HRESULT ConvertTimestamp        (const BYTE * p, FILETIME & ft);
    static HRESULT ReadDirectoryData       (SWalkState & state, const SFileEntry & fe, vector<BYTE> & vData);
    static HRESULT ReadAllocatedExtents    (SWalkState & state, const SFileEntry & fe, bool fLong, vector<BYTE> & vData);
    static HRESULT ParseFileIdentifiers    (const vector<BYTE> & vData, vector<SFileId> & vFileIds);
    static HRESULT ParseFileIdentifier     (const BYTE * p, size_t cbAvailable, SFileId & fileId, size_t & cbFileId);
    static HRESULT ReadDirectory           (SWalkState & state, const SLongAd & icb, const SFileEntry & fe, const wstring & strPrefix, UINT cDepth);
    static HRESULT AddEntry                (SWalkState & state, const SFileId & fileId, const wstring & strPrefix, UINT cDepth);
};
