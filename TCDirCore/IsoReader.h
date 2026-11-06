#pragma once

#include "ArchiveEntry.h"
#include "ByteSource.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoReader
//
//  Lists the files in an .iso disc image from its UDF volume, its Joliet
//  tree or its primary ISO 9660 tree, in that order of preference.
//
////////////////////////////////////////////////////////////////////////////////

class CIsoReader
{
public:
    static HRESULT Read           (IByteSource & source, SArchiveReadResult & result);
    static bool    IsJolietEscape (const BYTE * pEscape);

private:
    struct SDirectoryRecord
    {
        ULONG      m_ulExtent      = 0;
        ULONG      m_cbData        = 0;
        BYTE       m_bFlags        = 0;
        bool       m_fSelfOrParent = false;
        FILETIME   m_ftRecorded    = {};
        wstring    m_strName;
    };

    struct SVolumeRoots
    {
        bool               m_fPrimary = false;
        bool               m_fJoliet  = false;
        SDirectoryRecord   m_primary;
        SDirectoryRecord   m_joliet;
    };

    struct SWalkState
    {
        SWalkState (IByteSource & source, SArchiveReadResult & result) :
            m_source (source), m_result (result) {}

        IByteSource          & m_source;
        SArchiveReadResult   & m_result;
        bool                   m_fJoliet = false;
        unordered_set<ULONG>   m_setVisited;
    };

#pragma pack(push, 1)
    struct SRecordingDate
    {
        BYTE          m_bYear;            // Years since 1900
        BYTE          m_bMonth;
        BYTE          m_bDay;
        BYTE          m_bHour;
        BYTE          m_bMinute;
        BYTE          m_bSecond;
        signed char   m_chOffset;         // Offset from UTC in 15-minute units
    };
#pragma pack(pop)

    static constexpr ULONG    s_kFirstVolumeDescriptor  = 16;
    static constexpr ULONG    s_kMaxVolumeDescriptors   = 64;
    static constexpr BYTE     s_kTypePrimary            = 1;
    static constexpr BYTE     s_kTypeSupplementary      = 2;
    static constexpr BYTE     s_kTypeTerminator         = 255;
    static constexpr size_t   s_kibStandardId           = 1;
    static constexpr size_t   s_kcchStandardId          = 5;
    static constexpr size_t   s_kibEscapeSequences      = 88;
    static constexpr size_t   s_kcchJolietPrefix        = 2;
    static constexpr size_t   s_kibLogicalBlockSize     = 128;
    static constexpr size_t   s_kibRootRecord           = 156;
    static constexpr size_t   s_kcbRootRecord           = 34;

    static constexpr size_t   s_kibRecordXarLength      = 1;
    static constexpr size_t   s_kibRecordExtent         = 2;
    static constexpr size_t   s_kibRecordDataLength     = 10;
    static constexpr size_t   s_kibRecordDate           = 18;
    static constexpr size_t   s_kibRecordFlags          = 25;
    static constexpr size_t   s_kibRecordIdLength       = 32;
    static constexpr size_t   s_kibRecordId             = 33;
    static constexpr size_t   s_kcbMinRecord            = 34;
    static constexpr BYTE     s_kIdSelf                 = 0;
    static constexpr BYTE     s_kIdParent               = 1;

    static constexpr BYTE     s_kFlagHidden             = 0x01;
    static constexpr BYTE     s_kFlagDirectory          = 0x02;
    static constexpr BYTE     s_kFlagMultiExtent        = 0x80;

    static constexpr WORD     s_kYearBase               = 1900;
    static constexpr int      s_kMinutesPerOffsetUnit   = 15;
    static constexpr UINT     s_kMaxDepth               = 128;
    static constexpr ULONG    s_kcbMaxDirectory         = 16 * 1024 * 1024;

    static HRESULT       ReadIso9660Tree       (IByteSource & source, bool fUdfUnsupported, SArchiveReadResult & result);
    static HRESULT       FindVolumeRoots       (IByteSource & source, SVolumeRoots & roots);
    static HRESULT       ParseVolumeDescriptor (const vector<BYTE> & vSector, SVolumeRoots & roots, bool & fEnd);
    static HRESULT       ParseRootRecord       (const vector<BYTE> & vSector, bool fJoliet, SDirectoryRecord & root);
    static HRESULT       ReadDirectory         (SWalkState & state, const SDirectoryRecord & directory, const wstring & strPrefix, UINT cDepth);
    static HRESULT       ParseDirectoryExtent  (const vector<BYTE> & vData, bool fJoliet, vector<SDirectoryRecord> & vRecords);
    static HRESULT       ParseDirectoryRecord  (const BYTE * pRecord, size_t cbAvailable, bool fJoliet, SDirectoryRecord & record);
    static HRESULT       DecodeName            (const BYTE * pId, size_t cbId, bool fJoliet, wstring & strName);
    static void          RemoveVersionSuffix   (wstring & strName);
    static HRESULT       ConvertRecordingDate  (const BYTE * pDate, FILETIME & ft);
    static void          AppendEntries         (SWalkState & state, const vector<SDirectoryRecord> & vRecords, const wstring & strPrefix, vector<size_t> & vSubdirectories);
    static SArchiveEntry MakeEntry             (const SDirectoryRecord & record, const wstring & strPath);
};
