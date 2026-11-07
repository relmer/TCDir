#include "pch.h"

#include "IsoImageBuilder.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::BuildPrimaryOnly
//
//  Primary tree only: a plain file, a hidden file, a file whose identifier
//  ends in ".;1", and a folder with one file.
//
////////////////////////////////////////////////////////////////////////////////

vector<BYTE> CIsoImageBuilder::BuildPrimaryOnly()
{
    CIsoImageBuilder builder;



    builder.WriteIsoVolume (IsoDirectory (L"", { IsoFile (L"README.TXT;1", s_kcbReadme),
                                                 IsoFile (L"HIDDEN.TXT;1", s_kcbHidden, s_kFlagHidden),
                                                 IsoFile (L"NOEXT.;1",     s_kcbNoExtension),
                                                 IsoDirectory (L"DOCS", { IsoFile (L"NOTE.TXT;1", s_kcbNote) }) }),
                            nullptr,
                            false);

    return builder.m_vImage;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::BuildPrimaryAndJoliet
//
//  A primary tree with 8.3 identifiers and a Joliet tree with long,
//  mixed-case identifiers for the same files.
//
////////////////////////////////////////////////////////////////////////////////

vector<BYTE> CIsoImageBuilder::BuildPrimaryAndJoliet()
{
    CIsoImageBuilder builder;
    SIsoTestNode     primary = IsoDirectory (L"", { IsoFile (L"README.TXT;1", s_kcbReadme),
                                                    IsoDirectory (L"SUBFOL~1", { IsoFile (L"INNERF~1.TXT;1", s_kcbNote) }) });
    SIsoTestNode     joliet  = IsoDirectory (L"", { IsoFile (L"ReadMe.txt;1", s_kcbReadme),
                                                    IsoDirectory (L"Sub Folder", { IsoFile (L"Inner File.txt;1", s_kcbNote) }) });



    builder.WriteIsoVolume (primary, &joliet, false);

    return builder.m_vImage;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::BuildNested
//
//  Folders nested three deep, plus a folder whose directory data fills more
//  than one sector.
//
////////////////////////////////////////////////////////////////////////////////

vector<BYTE> CIsoImageBuilder::BuildNested()
{
    CIsoImageBuilder builder;
    SIsoTestNode     many   = IsoDirectory (L"MANY", {});
    SIsoTestNode     nested = IsoDirectory (L"A", { IsoDirectory (L"B", { IsoDirectory (L"C", { IsoFile (L"DEEP.TXT;1", s_kcbNote) }) }) });
    ULONG            iFile  = 0;



    for (iFile = 0; iFile < s_kcManyFiles; iFile++)
    {
        many.m_vChildren.push_back (IsoFile (format (L"FILE{:02}.TXT;1", iFile).c_str(), s_kcbManyFile));
    }

    builder.WriteIsoVolume (IsoDirectory (L"", { nested, many }), nullptr, false);

    return builder.m_vImage;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::BuildMultiExtent
//
//  A file stored as two extents, followed by an ordinary file.
//
////////////////////////////////////////////////////////////////////////////////

vector<BYTE> CIsoImageBuilder::BuildMultiExtent()
{
    CIsoImageBuilder builder;



    builder.WriteIsoVolume (IsoDirectory (L"", { IsoFile (L"BIG.DAT;1",   s_kcbFirstExtent, s_kFlagMultiExtent),
                                                 IsoFile (L"BIG.DAT;1",   s_kcbLastExtent),
                                                 IsoFile (L"AFTER.TXT;1", s_kcbNoExtension) }),
                            nullptr,
                            false);

    return builder.m_vImage;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::BuildCycle
//
//  A folder whose record links back to the root directory extent, which is
//  always the first data sector.
//
////////////////////////////////////////////////////////////////////////////////

vector<BYTE> CIsoImageBuilder::BuildCycle()
{
    CIsoImageBuilder builder;
    SIsoTestNode     loop = IsoDirectory (L"LOOP", {});



    loop.m_ulExtent = s_kFirstDataSector;

    builder.WriteIsoVolume (IsoDirectory (L"", { loop }), nullptr, false);

    return builder.m_vImage;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::BuildUdf
//
//  A UDF 1.02 volume with an ISO 9660 bridge tree that contains only
//  README.TXT (or nothing, for MetadataPartitionEmptyBridge).
//
////////////////////////////////////////////////////////////////////////////////

vector<BYTE> CIsoImageBuilder::BuildUdf (EUdfVariant variant)
{
    CIsoImageBuilder builder;
    SIsoTestNode     bridge = IsoDirectory (L"", {});



    if (variant != EUdfVariant::MetadataPartitionEmptyBridge)
    {
        bridge.m_vChildren.push_back (IsoFile (L"README.TXT;1", s_kcbReadme));
    }

    builder.WriteIsoVolume (bridge, nullptr, true);
    builder.WriteUdfVolume (UdfTree(), variant);

    return builder.m_vImage;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::ExpectedTime
//
//  Returns the UTC time of every record: 10:30 local time at UTC+1.
//
////////////////////////////////////////////////////////////////////////////////

FILETIME CIsoImageBuilder::ExpectedTime()
{
    SYSTEMTIME st = {};
    FILETIME   ft = {};



    st.wYear   = s_kTestYear;
    st.wMonth  = s_kTestMonth;
    st.wDay    = s_kTestDay;
    st.wHour   = (WORD) (s_kTestHour - s_kTestOffsetMinutes / s_kMinutesPerHour);
    st.wMinute = s_kTestMinute;

    SystemTimeToFileTime (&st, &ft);

    return ft;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::Sector
//
//  Returns the start of a sector, growing the image to cover it.  The
//  pointer is invalidated by the next call that grows the image.
//
////////////////////////////////////////////////////////////////////////////////

BYTE * CIsoImageBuilder::Sector (ULONG ulSector)
{
    size_t cbNeeded = ((size_t) ulSector + 1) * s_kcbSector;



    if (m_vImage.size() < cbNeeded)
    {
        m_vImage.resize (cbNeeded);
    }

    return m_vImage.data() + (size_t) ulSector * s_kcbSector;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::Block
//
//  Returns the start of a partition-relative UDF logical block.
//
////////////////////////////////////////////////////////////////////////////////

BYTE * CIsoImageBuilder::Block (ULONG ulBlock)
{
    return Sector (s_kPartitionStart + ulBlock);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::AllocateSectors
//
////////////////////////////////////////////////////////////////////////////////

ULONG CIsoImageBuilder::AllocateSectors (ULONG cSectors)
{
    ULONG ulSector = m_ulNextSector;



    m_ulNextSector += cSectors;
    Sector (m_ulNextSector - 1);

    return ulSector;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::AllocateBlocks
//
////////////////////////////////////////////////////////////////////////////////

ULONG CIsoImageBuilder::AllocateBlocks (ULONG cBlocks)
{
    ULONG ulBlock = m_ulNextBlock;



    m_ulNextBlock += cBlocks;
    Block (m_ulNextBlock - 1);

    return ulBlock;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::WriteIsoVolume
//
//  Writes the directory trees, then the volume descriptor set starting at
//  sector 16.  For a UDF image the UDF volume recognition sequence follows
//  the set terminator.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::WriteIsoVolume (
    const SIsoTestNode & primaryRoot,
    const SIsoTestNode * pJolietRoot,
    bool                 fUdf)
{
    ULONG ulSector = s_kFirstDescriptor;
    ULONG ulExtent = 0;
    ULONG cbData   = 0;



    WriteIsoDirectory (primaryRoot, false, ulExtent, cbData);
    WriteDescriptor (ulSector++, s_kTypePrimary, ulExtent, cbData);

    if (pJolietRoot != nullptr)
    {
        WriteIsoDirectory (*pJolietRoot, true, ulExtent, cbData);
        WriteDescriptor (ulSector++, s_kTypeSupplementary, ulExtent, cbData);
    }

    WriteDescriptor (ulSector++, s_kTypeTerminator, 0, 0);

    if (fUdf)
    {
        WriteVrs (ulSector);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::WriteDescriptor
//
//  Writes a primary, Joliet supplementary or terminator volume descriptor.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::WriteDescriptor (
    ULONG ulSector,
    BYTE  bType,
    ULONG ulRootExtent,
    ULONG cbRoot)
{
    BYTE * p = Sector (ulSector);



    p[0] = bType;
    memcpy (p + s_kibStandardId, "CD001", s_kcchStandardId);
    p[s_kibDescriptorVersion] = 1;

    if (bType != s_kTypeTerminator)
    {
        PutBoth16 (p + s_kibLogicalBlockSize, (USHORT) s_kcbSector);
        WriteRecord (p + s_kibRootRecord, ulRootExtent, cbRoot, s_kFlagDirectory, { 0 });
    }

    if (bType == s_kTypeSupplementary)
    {
        memcpy (p + s_kibEscapeSequences, "%/E", s_kcchEscapeSequence);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::WriteVrs
//
//  Writes BEA01, NSR02 and TEA01 in consecutive sectors.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::WriteVrs (ULONG ulSector)
{
    static constexpr const char * s_krgpszIds[] = { "BEA01", "NSR02", "TEA01" };

    BYTE * p = nullptr;



    for (const char * pszId : s_krgpszIds)
    {
        p = Sector (ulSector++);

        memcpy (p + s_kibStandardId, pszId, s_kcchStandardId);
        p[s_kibDescriptorVersion] = 1;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::WriteIsoDirectory
//
//  Allocates the directory's extent before its subdirectories, so the root
//  of the first tree is always at s_kFirstDataSector.  The parent record
//  points at the directory itself; readers skip it.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::WriteIsoDirectory (
    const SIsoTestNode & directory,
    bool                 fJoliet,
    ULONG              & ulExtent,
    ULONG              & cbData)
{
    vector<vector<BYTE>> vIds     = { { 0 }, { 1 } };
    vector<size_t>       vOffsets;
    vector<ULONG>        vExtents;
    vector<ULONG>        vSizes;
    BYTE               * p        = nullptr;
    BYTE                 bFlags   = 0;
    size_t               iChild   = 0;



    for (const SIsoTestNode & child : directory.m_vChildren)
    {
        vIds.push_back (EncodeIsoName (child.m_strName, fJoliet));
    }

    vOffsets = LayoutRecords (vIds, cbData);
    ulExtent = AllocateSectors (cbData / s_kcbSector);

    ResolveChildren (directory, fJoliet, vExtents, vSizes);

    p = Sector (ulExtent);

    WriteRecord (p + vOffsets[0], ulExtent, cbData, s_kFlagDirectory, vIds[0]);
    WriteRecord (p + vOffsets[1], ulExtent, cbData, s_kFlagDirectory, vIds[1]);

    for (iChild = 0; iChild < directory.m_vChildren.size(); iChild++)
    {
        bFlags = directory.m_vChildren[iChild].m_bFlags;

        if (directory.m_vChildren[iChild].m_fDirectory)
        {
            bFlags |= s_kFlagDirectory;
        }

        WriteRecord (p + vOffsets[iChild + s_kcSelfAndParent], vExtents[iChild], vSizes[iChild], bFlags, vIds[iChild + s_kcSelfAndParent]);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::ResolveChildren
//
//  Returns the extent and size of each child, writing subdirectories.  Files
//  have no data, so their extent is 0.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::ResolveChildren (
    const SIsoTestNode & directory,
    bool                 fJoliet,
    vector<ULONG>      & vExtents,
    vector<ULONG>      & vSizes)
{
    ULONG ulExtent = 0;
    ULONG cbData   = 0;



    for (const SIsoTestNode & child : directory.m_vChildren)
    {
        ulExtent = 0;
        cbData   = child.m_cbSize;

        if (child.m_fDirectory && child.m_ulExtent != 0)
        {
            ulExtent = child.m_ulExtent;
            cbData   = s_kcbSector;
        }
        else if (child.m_fDirectory)
        {
            WriteIsoDirectory (child, fJoliet, ulExtent, cbData);
        }

        vExtents.push_back (ulExtent);
        vSizes.push_back (cbData);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::WriteUdfVolume
//
//  Writes the anchor, the volume descriptor sequence, the File Set
//  Descriptor and the file tree.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::WriteUdfVolume (
    const SUdfTestNode & root,
    EUdfVariant          variant)
{
    ULONG  ulFileSet = 0;
    ULONG  ulRoot    = 0;
    BYTE * p         = nullptr;



    WriteAnchorAndVds (variant);

    ulFileSet = AllocateBlocks (1);
    ulRoot    = WriteUdfNode (root, s_kParentIsSelf);

    p = Block (ulFileSet);
    PutLongAd (p + s_kibFsdRootIcb, s_kcbSector, ulRoot);
    FinishTag (p, s_kTagFileSet, ulFileSet);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::WriteAnchorAndVds
//
//  Writes the anchor at sector 256 and a volume descriptor sequence of a
//  Partition Descriptor, a Logical Volume Descriptor and a Terminating
//  Descriptor.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::WriteAnchorAndVds (EUdfVariant variant)
{
    BYTE * p = Sector (s_kPartitionStart);



    p = Sector (s_kAnchorSector);
    PutU32 (p + s_kibAnchorMainVds, s_kVdsSectors * s_kcbSector);
    PutU32 (p + s_kibAnchorMainVds + sizeof (ULONG), s_kPartitionSector);
    FinishTag (p, s_kTagAnchor, s_kAnchorSector);

    p = Sector (s_kPartitionSector);
    PutU32 (p + s_kibPartitionStart, s_kPartitionStart);
    PutU32 (p + s_kibPartitionLength, s_kPartitionBlocks);
    FinishTag (p, s_kTagPartition, s_kPartitionSector);

    p = Sector (s_kLogicalVolumeSector);
    WriteLogicalVolume (p, variant);
    FinishTag (p, s_kTagLogicalVolume, s_kLogicalVolumeSector);

    p = Sector (s_kTerminatingSector);
    FinishTag (p, s_kTagTerminating, s_kTerminatingSector);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::WriteLogicalVolume
//
//  Writes a type 1 partition map, followed for the metadata variants by a
//  type 2 "*UDF Metadata Partition" map.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::WriteLogicalVolume (
    BYTE        * p,
    EUdfVariant   variant)
{
    BYTE * pMap = p + s_kibLvdMaps;



    PutU32 (p + s_kibLvdBlockSize, s_kcbSector);
    PutLongAd (p + s_kibLvdFileSet, s_kcbSector, 0);

    pMap[0] = s_kMapTypePhysical;
    pMap[1] = s_kcbPhysicalMap;
    PutU16 (pMap + s_kibMapVolumeSequence, 1);

    PutU32 (p + s_kibLvdMapTableLength, s_kcbPhysicalMap);
    PutU32 (p + s_kibLvdMapCount, 1);

    if (variant != EUdfVariant::Standard)
    {
        pMap   += s_kcbPhysicalMap;
        pMap[0] = s_kMapTypeIdentified;
        pMap[1] = s_kcbIdentifiedMap;
        memcpy (pMap + s_kibMapIdentifier, s_kszMetadataMapId, sizeof (s_kszMetadataMapId) - 1);

        PutU32 (p + s_kibLvdMapTableLength, s_kcbPhysicalMap + s_kcbIdentifiedMap);
        PutU32 (p + s_kibLvdMapCount, 2);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::WriteUdfNode
//
//  Writes a file or directory (and, for a directory, its contents) and
//  returns the block of its File Entry.
//
////////////////////////////////////////////////////////////////////////////////

ULONG CIsoImageBuilder::WriteUdfNode (
    const SUdfTestNode & node,
    ULONG                ulParentBlock)
{
    ULONG          ulBlock = AllocateBlocks (1);
    ULONGLONG      ullSize = node.m_ullSize;
    vector<BYTE>   vFids;
    vector<BYTE>   vAds;



    if (node.m_fDirectory)
    {
        vFids   = BuildFids (node, ulBlock, (ulParentBlock == s_kParentIsSelf) ? ulBlock : ulParentBlock);
        vAds    = StoreDirectoryData (node, vFids);
        ullSize = vFids.size();
    }

    WriteFileEntry (ulBlock, node, ullSize, vAds);

    return ulBlock;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::BuildFids
//
//  Returns a directory's File Identifier Descriptors: the parent entry, then
//  one per child.
//
////////////////////////////////////////////////////////////////////////////////

vector<BYTE> CIsoImageBuilder::BuildFids (
    const SUdfTestNode & directory,
    ULONG                ulSelfBlock,
    ULONG                ulParentBlock)
{
    vector<BYTE> vFids            = EncodeFid (s_kFidParent | s_kFidDirectory, ulParentBlock, {});
    vector<BYTE> vFid;
    ULONG        ulChild          = 0;
    BYTE         bCharacteristics = 0;



    for (const SUdfTestNode & child : directory.m_vChildren)
    {
        ulChild          = WriteUdfNode (child, ulSelfBlock);
        bCharacteristics = (BYTE) ((child.m_fDirectory ? s_kFidDirectory : 0) | (child.m_fHidden ? s_kFidHidden : 0));
        vFid             = EncodeFid (bCharacteristics, ulChild, EncodeOstaName (child.m_strName, child.m_f16BitName));

        vFids.insert (vFids.end(), vFid.begin(), vFid.end());
    }

    return vFids;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::StoreDirectoryData
//
//  Stores directory data as the directory's allocation type requires and
//  returns the bytes for the File Entry's allocation descriptor area.
//
////////////////////////////////////////////////////////////////////////////////

vector<BYTE> CIsoImageBuilder::StoreDirectoryData (
    const SUdfTestNode & directory,
    const vector<BYTE> & vFids)
{
    ULONG          cBlocks = (ULONG) ((vFids.size() + s_kcbSector - 1) / s_kcbSector);
    ULONG          ulData  = 0;
    vector<BYTE>   vAds;



    if (directory.m_bAllocation == s_kAllocEmbedded)
    {
        vAds = vFids;
    }
    else
    {
        ulData = AllocateBlocks (cBlocks);
        memcpy (Block (ulData), vFids.data(), vFids.size());

        vAds.resize ((directory.m_bAllocation == s_kAllocLong) ? s_kcbLongAd : s_kcbShortAd);
        PutU32 (vAds.data(), (ULONG) vFids.size());
        PutU32 (vAds.data() + sizeof (ULONG), ulData);
    }

    return vAds;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::WriteFileEntry
//
//  Writes a File Entry, or an Extended File Entry when m_fExtended is set.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::WriteFileEntry (
    ULONG                ulBlock,
    const SUdfTestNode & node,
    ULONGLONG            ullSize,
    const vector<BYTE> & vAds)
{
    const SUdfLayout & layout = node.m_fExtended ? s_kExtendedLayout : s_kFileEntryLayout;
    BYTE             * p      = Block (ulBlock);



    p[s_kibIcbFileType] = node.m_fDirectory ? s_kFileTypeDirectory : s_kFileTypeFile;
    PutU16 (p + s_kibIcbFlags, node.m_fDirectory ? node.m_bAllocation : s_kAllocShort);
    PutU64 (p + s_kibInformationLength, ullSize);
    WriteTimestamp (p + layout.m_ibModified);
    PutU32 (p + layout.m_ibAdLength, (ULONG) vAds.size());

    if (!vAds.empty())
    {
        memcpy (p + layout.m_ibAds, vAds.data(), vAds.size());
    }

    FinishTag (p, node.m_fExtended ? s_kTagExtendedEntry : s_kTagFileEntry, ulBlock);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::EncodeIsoName
//
//  Returns single-byte characters for the primary tree, big-endian UCS-2
//  for Joliet.
//
////////////////////////////////////////////////////////////////////////////////

vector<BYTE> CIsoImageBuilder::EncodeIsoName (
    const wstring & strName,
    bool            fJoliet)
{
    vector<BYTE> vId;



    for (wchar_t ch : strName)
    {
        if (fJoliet)
        {
            vId.push_back ((BYTE) (ch >> s_kBitsPerByte));
        }

        vId.push_back ((BYTE) ch);
    }

    return vId;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::RecordLength
//
//  Returns the length of a directory record, padded to an even length.
//
////////////////////////////////////////////////////////////////////////////////

size_t CIsoImageBuilder::RecordLength (size_t cbId)
{
    return s_kibRecordId + cbId + (((cbId & 1) == 0) ? 1 : 0);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::LayoutRecords
//
//  Returns the offset of each record, moving a record that would cross a
//  sector boundary to the next sector.  cbData is a whole number of
//  sectors.
//
////////////////////////////////////////////////////////////////////////////////

vector<size_t> CIsoImageBuilder::LayoutRecords (
    const vector<vector<BYTE>> & vIds,
    ULONG                      & cbData)
{
    vector<size_t> vOffsets;
    size_t         ib       = 0;
    size_t         cbRecord = 0;



    for (const vector<BYTE> & vId : vIds)
    {
        cbRecord = RecordLength (vId.size());

        if (ib % s_kcbSector + cbRecord > s_kcbSector)
        {
            ib = (ib / s_kcbSector + 1) * s_kcbSector;
        }

        vOffsets.push_back (ib);
        ib += cbRecord;
    }

    cbData = (ULONG) ((ib + s_kcbSector - 1) / s_kcbSector * s_kcbSector);

    return vOffsets;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::WriteRecord
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::WriteRecord (
    BYTE               * p,
    ULONG                ulExtent,
    ULONG                cbData,
    BYTE                 bFlags,
    const vector<BYTE> & vId)
{
    p[0] = (BYTE) RecordLength (vId.size());
    PutBoth32 (p + s_kibRecordExtent, ulExtent);
    PutBoth32 (p + s_kibRecordDataLength, cbData);
    memcpy (p + s_kibRecordDate, s_krgbRecordingDate, sizeof (s_krgbRecordingDate));
    p[s_kibRecordFlags] = bFlags;
    PutBoth16 (p + s_kibRecordVolumeSequence, 1);
    p[s_kibRecordIdLength] = (BYTE) vId.size();
    memcpy (p + s_kibRecordId, vId.data(), vId.size());
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::EncodeOstaName
//
//  Returns OSTA compressed Unicode: compression ID 8 or 16, then the
//  characters.
//
////////////////////////////////////////////////////////////////////////////////

vector<BYTE> CIsoImageBuilder::EncodeOstaName (
    const wstring & strName,
    bool            f16Bit)
{
    vector<BYTE> vName = { f16Bit ? s_kCompression16 : s_kCompression8 };



    for (wchar_t ch : strName)
    {
        if (f16Bit)
        {
            vName.push_back ((BYTE) (ch >> s_kBitsPerByte));
        }

        vName.push_back ((BYTE) ch);
    }

    return vName;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::EncodeFid
//
//  Returns a File Identifier Descriptor padded to a multiple of 4 bytes.
//
////////////////////////////////////////////////////////////////////////////////

vector<BYTE> CIsoImageBuilder::EncodeFid (
    BYTE                 bCharacteristics,
    ULONG                ulIcbBlock,
    const vector<BYTE> & vName)
{
    size_t         cbUsed = s_kcbFidHeader + vName.size();
    vector<BYTE>   vFid ((cbUsed + s_kcbFidAlignment - 1) / s_kcbFidAlignment * s_kcbFidAlignment);



    PutU16 (vFid.data() + s_kibFidVersion, 1);
    vFid[s_kibFidCharacteristics] = bCharacteristics;
    vFid[s_kibFidNameLength]      = (BYTE) vName.size();
    PutLongAd (vFid.data() + s_kibFidIcb, s_kcbSector, ulIcbBlock);

    if (!vName.empty())
    {
        memcpy (vFid.data() + s_kcbFidHeader, vName.data(), vName.size());
    }

    FinishTag (vFid.data(), s_kTagFileId, 0);

    return vFid;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::WriteTimestamp
//
//  Writes an ECMA-167 timestamp of local time with its UTC offset.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::WriteTimestamp (BYTE * p)
{
    PutU16 (p, (USHORT) (s_kTimestampLocal | s_kTestOffsetMinutes));
    PutU16 (p + s_kibTimestampYear, s_kTestYear);
    p[s_kibTimestampMonth]  = s_kTestMonth;
    p[s_kibTimestampDay]    = s_kTestDay;
    p[s_kibTimestampHour]   = s_kTestHour;
    p[s_kibTimestampMinute] = s_kTestMinute;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::FinishTag
//
//  Fills in a descriptor tag, including its checksum: the byte sum of the
//  tag bytes excluding the checksum byte.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::FinishTag (
    BYTE   * p,
    USHORT   usTagId,
    ULONG    ulLocation)
{
    BYTE   bSum = 0;
    size_t ib   = 0;



    PutU16 (p, usTagId);
    PutU16 (p + s_kibTagVersion, s_kTagVersion);
    PutU32 (p + s_kibTagLocation, ulLocation);

    for (ib = 0; ib < s_kcbTag; ib++)
    {
        bSum = (BYTE) (bSum + ((ib == s_kibTagChecksum) ? 0 : p[ib]));
    }

    p[s_kibTagChecksum] = bSum;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::PutU16
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::PutU16 (
    BYTE   * p,
    USHORT   us)
{
    p[0] = (BYTE) us;
    p[1] = (BYTE) (us >> s_kBitsPerByte);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::PutU32
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::PutU32 (
    BYTE  * p,
    ULONG   ul)
{
    PutU16 (p, (USHORT) ul);
    PutU16 (p + sizeof (USHORT), (USHORT) (ul >> (sizeof (USHORT) * s_kBitsPerByte)));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::PutU64
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::PutU64 (
    BYTE      * p,
    ULONGLONG   ull)
{
    PutU32 (p, (ULONG) ull);
    PutU32 (p + sizeof (ULONG), (ULONG) (ull >> (sizeof (ULONG) * s_kBitsPerByte)));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::PutBoth16
//
//  Writes an ISO 9660 both-endian field: little-endian, then big-endian.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::PutBoth16 (
    BYTE   * p,
    USHORT   us)
{
    PutU16 (p, us);
    PutU16 (p + sizeof (USHORT), _byteswap_ushort (us));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::PutBoth32
//
//  Writes an ISO 9660 both-endian field: little-endian, then big-endian.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::PutBoth32 (
    BYTE  * p,
    ULONG   ul)
{
    PutU32 (p, ul);
    PutU32 (p + sizeof (ULONG), _byteswap_ulong (ul));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::PutLongAd
//
//  Writes a long_ad in partition 0.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoImageBuilder::PutLongAd (
    BYTE  * p,
    ULONG   cb,
    ULONG   ulBlock)
{
    PutU32 (p, cb);
    PutU32 (p + sizeof (ULONG), ulBlock);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::IsoFile
//
////////////////////////////////////////////////////////////////////////////////

SIsoTestNode CIsoImageBuilder::IsoFile (
    LPCWSTR pszName,
    ULONG   cb,
    BYTE    bFlags)
{
    SIsoTestNode node;



    node.m_strName = pszName;
    node.m_cbSize  = cb;
    node.m_bFlags  = bFlags;

    return node;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::IsoDirectory
//
////////////////////////////////////////////////////////////////////////////////

SIsoTestNode CIsoImageBuilder::IsoDirectory (
    LPCWSTR              pszName,
    vector<SIsoTestNode> vChildren)
{
    SIsoTestNode node;



    node.m_strName    = pszName;
    node.m_fDirectory = true;
    node.m_vChildren  = std::move (vChildren);

    return node;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::UdfFile
//
////////////////////////////////////////////////////////////////////////////////

SUdfTestNode CIsoImageBuilder::UdfFile (
    LPCWSTR   pszName,
    ULONGLONG cb)
{
    SUdfTestNode node;



    node.m_strName = pszName;
    node.m_ullSize = cb;

    return node;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::UdfDirectory
//
////////////////////////////////////////////////////////////////////////////////

SUdfTestNode CIsoImageBuilder::UdfDirectory (
    LPCWSTR              pszName,
    BYTE                 bAllocation,
    vector<SUdfTestNode> vChildren)
{
    SUdfTestNode node;



    node.m_strName     = pszName;
    node.m_fDirectory  = true;
    node.m_bAllocation = bAllocation;
    node.m_vChildren   = std::move (vChildren);

    return node;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoImageBuilder::UdfTree
//
//  The root uses short_ad, "Sources" uses long_ad, and "boot" is an
//  Extended File Entry with embedded data.  "boot\BCD" is an Extended File
//  Entry and the Japanese file name uses 16-bit compression.
//
////////////////////////////////////////////////////////////////////////////////

SUdfTestNode CIsoImageBuilder::UdfTree()
{
    SUdfTestNode unicode = UdfFile (s_kpszUnicodeName, s_kcbUnicodeFile);
    SUdfTestNode bcd     = UdfFile (L"BCD", s_kcbBcd);
    SUdfTestNode hidden  = UdfFile (L"hidden.sys", s_kcbHiddenSys);
    SUdfTestNode boot;



    unicode.m_f16BitName = true;
    bcd.m_fExtended      = true;
    hidden.m_fHidden     = true;

    boot = UdfDirectory (L"boot",
                         s_kAllocEmbedded,
                         { bcd, hidden, UdfDirectory (L"fonts", s_kAllocShort, { UdfFile (L"wgl4_boot.ttf", s_kcbFont) }) });
    boot.m_fExtended = true;

    return UdfDirectory (L"",
                         s_kAllocShort,
                         { UdfFile (L"setup.exe", s_kcbSetup),
                           UdfDirectory (L"Sources", s_kAllocLong, { UdfFile (L"install.wim", s_kcbInstallWim), unicode }),
                           boot });
}





