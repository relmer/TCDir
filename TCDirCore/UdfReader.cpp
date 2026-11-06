#include "pch.h"

#include "UdfReader.h"

#include "DiscImage.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::Read
//
//  Lists every file and folder in a UDF image.  Returns ERROR_NOT_FOUND when
//  the image has no UDF volume recognition sequence, ERROR_NOT_SUPPORTED for
//  virtual, sparable or metadata partitions, and ERROR_BAD_FORMAT for
//  corrupt structures.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::Read (
    IByteSource        & source,
    SArchiveReadResult & result)
{
    HRESULT      hr      = S_OK;
    SExtent      vds;
    SVolume      volume;
    SLongAd      rootIcb;
    SFileEntry   rootEntry;
    SWalkState   state (source, volume, result);



    result.Reset();

    hr = FindRecognitionSequence (source);
    CHR (hr);

    hr = ReadAnchor (source, vds);
    CHR (hr);

    hr = ReadVolumeDescriptors (source, vds, volume);
    CHR (hr);

    hr = ReadFileSet (source, volume, rootIcb);
    CHR (hr);

    hr = ReadFileEntry (state, rootIcb, rootEntry);
    CHR (hr);

    hr = ReadDirectory (state, rootIcb, rootEntry, L"", 0);
    CHR (hr);



Error:
    if (FAILED (hr))
    {
        result.Reset();
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::FindRecognitionSequence
//
//  Scans the volume recognition sequence that starts at sector 16 for BEA01
//  followed by NSR02 or NSR03.  An image that ends inside the sequence has no
//  UDF volume.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::FindRecognitionSequence (IByteSource & source)
{
    HRESULT   hr                        = S_OK;
    BYTE      rgbHeader[s_kcbVrsHeader] = {};
    bool      fBea                      = false;
    bool      fNsr                      = false;
    bool      fEnd                      = false;
    ULONG     ulSector                  = 0;



    for (ulSector = s_kVrsFirstSector; ulSector < s_kVrsFirstSector + s_kMaxVrsSectors && !fEnd; ulSector++)
    {
        hr = source.ReadAt ((ULONGLONG) ulSector * CDiscImage::s_kcbSector, rgbHeader, sizeof (rgbHeader));

        if (hr == HRESULT_FROM_WIN32 (ERROR_HANDLE_EOF))
        {
            hr = S_OK;
            break;
        }

        CHR (hr);

        ClassifyVrsDescriptor (rgbHeader, fBea, fNsr, fEnd);
    }

    CBREx (fNsr, HRESULT_FROM_WIN32 (ERROR_NOT_FOUND));



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ClassifyVrsDescriptor
//
//  Updates the scan state for one volume structure descriptor.  An
//  identifier outside the ECMA-167 and ISO 9660 set ends the sequence.
//
////////////////////////////////////////////////////////////////////////////////

void CUdfReader::ClassifyVrsDescriptor (
    const BYTE * pDescriptor,
    bool       & fBea,
    bool       & fNsr,
    bool       & fEnd)
{
    const char * pszId = (const char *) pDescriptor + s_kibVrsId;



    if (memcmp (pszId, "BEA01", s_kcchVrsId) == 0)
    {
        fBea = true;
    }
    else if (memcmp (pszId, "NSR02", s_kcchVrsId) == 0 || memcmp (pszId, "NSR03", s_kcchVrsId) == 0)
    {
        fNsr = fNsr || fBea;
    }
    else if (memcmp (pszId, "TEA01", s_kcchVrsId) == 0)
    {
        fEnd = true;
    }
    else if (memcmp (pszId, "CD001", s_kcchVrsId) != 0 &&
             memcmp (pszId, "CDW02", s_kcchVrsId) != 0 &&
             memcmp (pszId, "BOOT2", s_kcchVrsId) != 0)
    {
        fEnd = true;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::IsTagValid
//
//  Checks a descriptor tag's checksum, which is the byte sum of the 16 tag
//  bytes excluding the checksum byte.  ulLocation is the tag location to
//  match, or s_kNoLocation to skip that check.
//
////////////////////////////////////////////////////////////////////////////////

bool CUdfReader::IsTagValid (
    const BYTE * p,
    size_t       cb,
    ULONG        ulLocation)
{
    bool   fValid = false;
    BYTE   bSum   = 0;
    size_t ib     = 0;



    if (cb >= s_kcbTag)
    {
        for (ib = 0; ib < s_kcbTag; ib++)
        {
            bSum = (BYTE) (bSum + ((ib == s_kibTagChecksum) ? 0 : p[ib]));
        }

        fValid = (bSum == p[s_kibTagChecksum]) &&
                 (ulLocation == s_kNoLocation || CDiscImage::ReadU32 (p + s_kibTagLocation) == ulLocation);
    }

    return fValid;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::VerifyTag
//
//  Fails with ERROR_BAD_FORMAT unless the block starts with a valid tag
//  with identifier usTagId and location ulLocation.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::VerifyTag (
    const vector<BYTE> & vBlock,
    USHORT               usTagId,
    ULONG                ulLocation)
{
    HRESULT hr     = S_OK;
    bool    fValid = IsTagValid (vBlock.data(), vBlock.size(), ulLocation);



    CBREx (fValid, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    fValid = CDiscImage::ReadU16 (vBlock.data()) == usTagId;
    CBREx (fValid, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ParseLongAd
//
//  Decodes a long_ad: extent length and type, logical block and partition
//  reference number.
//
////////////////////////////////////////////////////////////////////////////////

CUdfReader::SLongAd CUdfReader::ParseLongAd (const BYTE * p)
{
    SLongAd ad;
    ULONG   ulLength = CDiscImage::ReadU32 (p);



    ad.m_cb          = ulLength & s_kExtentLengthMask;
    ad.m_bType       = (BYTE) (ulLength >> s_kExtentTypeShift);
    ad.m_ulBlock     = CDiscImage::ReadU32 (p + s_kibAdPosition);
    ad.m_usPartition = CDiscImage::ReadU16 (p + s_kibLongAdPartition);

    return ad;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ParseShortAd
//
//  Decodes a short_ad, whose extent is in the partition of its file entry.
//
////////////////////////////////////////////////////////////////////////////////

CUdfReader::SLongAd CUdfReader::ParseShortAd (
    const BYTE * p,
    USHORT       usPartition)
{
    SLongAd ad;
    ULONG   ulLength = CDiscImage::ReadU32 (p);



    ad.m_cb          = ulLength & s_kExtentLengthMask;
    ad.m_bType       = (BYTE) (ulLength >> s_kExtentTypeShift);
    ad.m_ulBlock     = CDiscImage::ReadU32 (p + s_kibAdPosition);
    ad.m_usPartition = usPartition;

    return ad;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ReadAnchor
//
//  Reads the Anchor Volume Descriptor Pointer at sector 256 and returns the
//  extent of the Main Volume Descriptor Sequence.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::ReadAnchor (
    IByteSource & source,
    SExtent     & vds)
{
    HRESULT        hr = S_OK;
    vector<BYTE>   vSector;



    hr = CDiscImage::ReadBytes (source, s_kAnchorSector, CDiscImage::s_kcbSector, vSector);
    CHR (hr);

    hr = VerifyTag (vSector, s_kTagAnchor, s_kAnchorSector);
    CHR (hr);

    vds.m_cb         = CDiscImage::ReadU32 (vSector.data() + s_kibAnchorMainVds);
    vds.m_ulLocation = CDiscImage::ReadU32 (vSector.data() + s_kibAnchorMainVds + sizeof (ULONG));

    CBREx (vds.m_cb >= CDiscImage::s_kcbSector, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ReadVolumeDescriptors
//
//  Walks the Main Volume Descriptor Sequence up to the Terminating
//  Descriptor, collecting partition starts and the logical volume.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::ReadVolumeDescriptors (
    IByteSource   & source,
    const SExtent & vds,
    SVolume       & volume)
{
    HRESULT        hr       = S_OK;
    vector<BYTE>   vSector;
    ULONG          cSectors = min (vds.m_cb / CDiscImage::s_kcbSector, s_kMaxVdsSectors);
    ULONG          iSector  = 0;
    bool           fEnd     = false;



    for (iSector = 0; iSector < cSectors && !fEnd; iSector++)
    {
        hr = CDiscImage::ReadBytes (source, (ULONGLONG) vds.m_ulLocation + iSector, CDiscImage::s_kcbSector, vSector);
        CHR (hr);

        hr = ParseVolumeDescriptor (vSector, vds.m_ulLocation + iSector, volume, fEnd);
        CHR (hr);
    }

    CBREx (volume.m_fLogicalVolume, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    hr = ResolvePartitions (volume);
    CHR (hr);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ParseVolumeDescriptor
//
//  Stores a Partition Descriptor or Logical Volume Descriptor in volume;
//  sets fEnd at the Terminating Descriptor.  Other descriptor types are
//  skipped.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::ParseVolumeDescriptor (
    const vector<BYTE> & vSector,
    ULONG                ulSector,
    SVolume            & volume,
    bool               & fEnd)
{
    HRESULT hr          = S_OK;
    bool    fValid      = IsTagValid (vSector.data(), vSector.size(), ulSector);
    USHORT  usTagId     = 0;
    USHORT  usPartition = 0;



    CBREx (fValid, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    usTagId = CDiscImage::ReadU16 (vSector.data());

    if (usTagId == s_kTagPartition)
    {
        usPartition = CDiscImage::ReadU16 (vSector.data() + s_kibPartitionNumber);
        volume.m_mapPartitionStart[usPartition] = CDiscImage::ReadU32 (vSector.data() + s_kibPartitionStart);
    }
    else if (usTagId == s_kTagLogicalVolume && !volume.m_fLogicalVolume)
    {
        hr = ParseLogicalVolume (vSector, volume);
        CHR (hr);
    }
    else if (usTagId == s_kTagTerminating)
    {
        fEnd = true;
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ParseLogicalVolume
//
//  Reads the block size, the File Set Descriptor location and the partition
//  maps from a Logical Volume Descriptor.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::ParseLogicalVolume (
    const vector<BYTE> & vSector,
    SVolume            & volume)
{
    HRESULT      hr      = S_OK;
    const BYTE * p       = vSector.data();
    ULONG        cbBlock = CDiscImage::ReadU32 (p + s_kibLvdBlockSize);
    ULONG        cbMaps  = CDiscImage::ReadU32 (p + s_kibLvdMapTableLength);
    ULONG        cMaps   = CDiscImage::ReadU32 (p + s_kibLvdMapCount);



    CBREx (cbBlock == CDiscImage::s_kcbSector, HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED));
    CBREx (cbMaps <= CDiscImage::s_kcbSector - s_kibLvdMaps, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    volume.m_fileSet = ParseLongAd (p + s_kibLvdFileSet);

    hr = ParsePartitionMaps (p + s_kibLvdMaps, cbMaps, cMaps, volume);
    CHR (hr);

    volume.m_fLogicalVolume = true;



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ParsePartitionMaps
//
//  Accepts only type 1 (physical) maps.  Type 2 maps are the virtual,
//  sparable and metadata partitions, which return ERROR_NOT_SUPPORTED.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::ParsePartitionMaps (
    const BYTE * pMaps,
    ULONG        cbMaps,
    ULONG        cMaps,
    SVolume    & volume)
{
    HRESULT hr    = S_OK;
    ULONG   ib    = 0;
    ULONG   iMap  = 0;
    BYTE    bType = 0;
    BYTE    cbMap = 0;



    volume.m_vMapPartitions.clear();

    for (iMap = 0; iMap < cMaps; iMap++)
    {
        CBREx (ib + s_kcbMapHeader <= cbMaps, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

        bType = pMaps[ib];
        cbMap = pMaps[ib + 1];

        CBREx (cbMap >= s_kcbMapHeader && ib + cbMap <= cbMaps, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));
        CBREx (bType != s_kMapTypeIdentified, HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED));
        CBREx (bType == s_kMapTypePhysical && cbMap == s_kcbPhysicalMap, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

        volume.m_vMapPartitions.push_back (CDiscImage::ReadU16 (pMaps + ib + s_kibPhysicalMapPartition));
        ib += cbMap;
    }

    CBREx (!volume.m_vMapPartitions.empty(), HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ResolvePartitions
//
//  Converts each partition map's partition number to the starting sector
//  from its Partition Descriptor.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::ResolvePartitions (SVolume & volume)
{
    HRESULT                                        hr     = S_OK;
    bool                                           fFound = false;
    unordered_map<USHORT, ULONG>::const_iterator   it;



    volume.m_vPartitionStarts.clear();

    for (USHORT usPartition : volume.m_vMapPartitions)
    {
        it = volume.m_mapPartitionStart.find (usPartition);

        fFound = it != volume.m_mapPartitionStart.end();
        CBREx (fFound, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

        volume.m_vPartitionStarts.push_back (it->second);
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ReadBlock
//
//  Reads cb bytes at a partition-relative logical block.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::ReadBlock (
    IByteSource    & source,
    const SVolume  & volume,
    const SLongAd  & ad,
    ULONG            cb,
    vector<BYTE>   & vBlock)
{
    HRESULT   hr        = S_OK;
    ULONGLONG ullSector = 0;



    CBREx (ad.m_usPartition < volume.m_vPartitionStarts.size(), HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    ullSector = (ULONGLONG) volume.m_vPartitionStarts[ad.m_usPartition] + ad.m_ulBlock;

    hr = CDiscImage::ReadBytes (source, ullSector, cb, vBlock);
    CHR (hr);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ReadFileSet
//
//  Reads the File Set Descriptor and returns the root directory ICB.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::ReadFileSet (
    IByteSource   & source,
    const SVolume & volume,
    SLongAd       & rootIcb)
{
    HRESULT        hr = S_OK;
    vector<BYTE>   vBlock;



    hr = ReadBlock (source, volume, volume.m_fileSet, CDiscImage::s_kcbSector, vBlock);
    CHR (hr);

    hr = VerifyTag (vBlock, s_kTagFileSet, volume.m_fileSet.m_ulBlock);
    CHR (hr);

    rootIcb = ParseLongAd (vBlock.data() + s_kibFsdRootIcb);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ReadFileEntry
//
//  Reads the File Entry or Extended File Entry at an ICB.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::ReadFileEntry (
    SWalkState    & state,
    const SLongAd & icb,
    SFileEntry    & fe)
{
    HRESULT        hr      = S_OK;
    vector<BYTE>   vBlock;
    bool           fValid  = false;
    USHORT         usTagId = 0;



    hr = ReadBlock (state.m_source, state.m_volume, icb, CDiscImage::s_kcbSector, vBlock);
    CHR (hr);

    fValid = IsTagValid (vBlock.data(), vBlock.size(), icb.m_ulBlock);
    CBREx (fValid, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    usTagId = CDiscImage::ReadU16 (vBlock.data());
    CBREx (usTagId == s_kTagFileEntry || usTagId == s_kTagExtendedFileEntry, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    fe.m_usPartition = icb.m_usPartition;

    hr = ParseFileEntry (vBlock,
                         (usTagId == s_kTagFileEntry) ? s_kFileEntryLayout : s_kExtendedFileEntryLayout,
                         fe);
    CHR (hr);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ParseFileEntry
//
//  Decodes the file type, size, modification time and allocation
//  descriptors.  The two entry types differ only in field offsets.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::ParseFileEntry (
    const vector<BYTE> & vBlock,
    const SEntryLayout & layout,
    SFileEntry         & fe)
{
    HRESULT      hr   = S_OK;
    const BYTE * p    = vBlock.data();
    ULONG        cbEa = CDiscImage::ReadU32 (p + layout.m_ibEaLength);
    ULONG        cbAd = CDiscImage::ReadU32 (p + layout.m_ibAdLength);
    size_t       ibAd = 0;



    CBREx (cbEa <= vBlock.size() && cbAd <= vBlock.size(), HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));
    CBREx (layout.m_ibExtendedAttributes + cbEa + cbAd <= vBlock.size(), HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    ibAd = layout.m_ibExtendedAttributes + cbEa;

    fe.m_bFileType  = p[s_kibIcbFileType];
    fe.m_usIcbFlags = CDiscImage::ReadU16 (p + s_kibIcbFlags);
    fe.m_ullSize    = CDiscImage::ReadU64 (p + s_kibInformationLength);
    fe.m_vAllocation.assign (p + ibAd, p + ibAd + cbAd);

    hr = ConvertTimestamp (p + layout.m_ibModified, fe.m_ftModified);
    IGNORE_RETURN_VALUE (hr, S_OK);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ConvertTimestamp
//
//  Converts an ECMA-167 timestamp to a UTC FILETIME.  The low 12 bits of the
//  first field are a signed offset from UTC in minutes; -2047 means the
//  offset is unknown, which is treated as UTC.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::ConvertTimestamp (
    const BYTE * p,
    FILETIME   & ft)
{
    HRESULT      hr      = S_OK;
    SYSTEMTIME   st      = {};
    int          iOffset = CDiscImage::ReadU16 (p) & s_kTimezoneMask;



    if (iOffset & s_kTimezoneSignBit)
    {
        iOffset -= s_kTimezoneRange;
    }

    if (iOffset == s_kTimezoneUnspecified)
    {
        iOffset = 0;
    }

    st.wYear         = CDiscImage::ReadU16 (p + s_kibTimestampYear);
    st.wMonth        = p[s_kibTimestampMonth];
    st.wDay          = p[s_kibTimestampDay];
    st.wHour         = p[s_kibTimestampHour];
    st.wMinute       = p[s_kibTimestampMinute];
    st.wSecond       = p[s_kibTimestampSecond];
    st.wMilliseconds = (WORD) (p[s_kibTimestampCentiseconds] * s_kMsPerCentisecond);

    hr = CDiscImage::ToUtcFileTime (st, iOffset, ft);
    CHR (hr);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ReadDirectoryData
//
//  Returns a directory's data stream: the File Identifier Descriptors.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::ReadDirectoryData (
    SWalkState       & state,
    const SFileEntry & fe,
    vector<BYTE>     & vData)
{
    HRESULT hr      = S_OK;
    USHORT  usAlloc = fe.m_usIcbFlags & s_kIcbAllocationMask;



    vData.clear();

    CBREx (fe.m_ullSize <= s_kcbMaxDirectory, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    if (usAlloc == s_kIcbEmbedded)
    {
        CBREx (fe.m_ullSize <= fe.m_vAllocation.size(), HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));
        vData.assign (fe.m_vAllocation.begin(), fe.m_vAllocation.begin() + (size_t) fe.m_ullSize);
    }
    else
    {
        CBREx (usAlloc == s_kIcbShortAd || usAlloc == s_kIcbLongAd, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

        hr = ReadAllocatedExtents (state, fe, usAlloc == s_kIcbLongAd, vData);
        CHR (hr);
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ReadAllocatedExtents
//
//  Concatenates the data extents listed by short_ad or long_ad
//  descriptors until the information length is read.  Allocation extent
//  continuations return ERROR_NOT_SUPPORTED.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::ReadAllocatedExtents (
    SWalkState       & state,
    const SFileEntry & fe,
    bool               fLong,
    vector<BYTE>     & vData)
{
    HRESULT        hr           = S_OK;
    size_t         cbDescriptor = fLong ? s_kcbLongAd : s_kcbShortAd;
    size_t         ib           = 0;
    SLongAd        ad;
    vector<BYTE>   vExtent;



    for (ib = 0; ib + cbDescriptor <= fe.m_vAllocation.size() && vData.size() < fe.m_ullSize; ib += cbDescriptor)
    {
        ad = fLong ? ParseLongAd (fe.m_vAllocation.data() + ib)
                   : ParseShortAd (fe.m_vAllocation.data() + ib, fe.m_usPartition);

        if (ad.m_cb == 0)
        {
            break;
        }

        CBREx (ad.m_bType != s_kExtentContinuation, HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED));
        CBREx (ad.m_bType == s_kExtentRecorded, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));
        CBREx (vData.size() + ad.m_cb <= s_kcbMaxDirectory, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

        hr = ReadBlock (state.m_source, state.m_volume, ad, ad.m_cb, vExtent);
        CHR (hr);

        vData.insert (vData.end(), vExtent.begin(), vExtent.end());
    }

    CBREx (vData.size() >= fe.m_ullSize, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    vData.resize ((size_t) fe.m_ullSize);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ParseFileIdentifiers
//
//  Decodes every File Identifier Descriptor in a directory's data, omitting
//  the parent and deleted entries.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::ParseFileIdentifiers (
    const vector<BYTE> & vData,
    vector<SFileId>    & vFileIds)
{
    HRESULT hr       = S_OK;
    size_t  ib       = 0;
    size_t  cbFileId = 0;
    SFileId fileId;



    vFileIds.clear();

    while (ib < vData.size())
    {
        hr = ParseFileIdentifier (vData.data() + ib, vData.size() - ib, fileId, cbFileId);
        CHR (hr);

        ib += cbFileId;

        if (fileId.m_bCharacteristics & (s_kFidParent | s_kFidDeleted))
        {
            continue;
        }

        CBREx (!fileId.m_strName.empty(), HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

        vFileIds.push_back (fileId);
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ParseFileIdentifier
//
//  Decodes one File Identifier Descriptor and returns its padded length.
//  Descriptors are padded to a multiple of 4 bytes; the padding of the last
//  descriptor may be cut off by the end of the directory data.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::ParseFileIdentifier (
    const BYTE * p,
    size_t       cbAvailable,
    SFileId    & fileId,
    size_t     & cbFileId)
{
    HRESULT hr     = S_OK;
    bool    fValid = false;
    size_t  cbName = 0;
    size_t  cbImpl = 0;
    size_t  cbUsed = 0;



    CBREx (cbAvailable >= s_kcbFidHeader, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    fValid = IsTagValid (p, cbAvailable, s_kNoLocation) && CDiscImage::ReadU16 (p) == s_kTagFileId;
    CBREx (fValid, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    fileId.m_bCharacteristics = p[s_kibFidCharacteristics];
    fileId.m_icb              = ParseLongAd (p + s_kibFidIcb);
    cbName                    = p[s_kibFidNameLength];
    cbImpl                    = CDiscImage::ReadU16 (p + s_kibFidImplUseLength);
    cbUsed                    = s_kcbFidHeader + cbImpl + cbName;

    CBREx (cbUsed <= cbAvailable, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    hr = DecodeOstaName (p + s_kcbFidHeader + cbImpl, cbName, fileId.m_strName);
    CHR (hr);

    cbFileId = min ((cbUsed + s_kcbFidAlignment - 1) / s_kcbFidAlignment * s_kcbFidAlignment, cbAvailable);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::DecodeOstaName
//
//  Decodes OSTA compressed Unicode: a compression ID byte (8 for one byte per
//  character, 16 for big-endian UCS-2) followed by the characters.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::DecodeOstaName (
    const BYTE * p,
    size_t       cb,
    wstring    & strName)
{
    HRESULT hr = S_OK;
    size_t  ib = 0;



    strName.clear();

    BAIL_OUT_IF (cb == 0, S_OK);

    if (p[0] == s_kCompression8)
    {
        for (ib = 1; ib < cb; ib++)
        {
            strName += (wchar_t) p[ib];
        }
    }
    else
    {
        CBREx (p[0] == s_kCompression16 && (cb - 1) % sizeof (wchar_t) == 0, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

        for (ib = 1; ib < cb; ib += sizeof (wchar_t))
        {
            strName += (wchar_t) ((p[ib] << CDiscImage::s_kBitsPerByte) | p[ib + 1]);
        }
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::ReadDirectory
//
//  Adds every entry in a directory, recursing into subdirectories.  Each
//  directory ICB is visited once, so a cycle or a shared directory returns
//  ERROR_BAD_FORMAT instead of looping.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::ReadDirectory (
    SWalkState       & state,
    const SLongAd    & icb,
    const SFileEntry & fe,
    const wstring    & strPrefix,
    UINT               cDepth)
{
    HRESULT           hr        = S_OK;
    ULONGLONG         ullKey    = ((ULONGLONG) icb.m_usPartition << s_kVisitedPartitionShift) | icb.m_ulBlock;
    bool              fInserted = false;
    vector<BYTE>      vData;
    vector<SFileId>   vFileIds;



    CBREx (cDepth <= s_kMaxDepth, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));
    CBREx (fe.m_bFileType == s_kFileTypeDirectory, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    fInserted = state.m_setVisited.insert (ullKey).second;
    CBREx (fInserted, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    hr = ReadDirectoryData (state, fe, vData);
    CHR (hr);

    hr = ParseFileIdentifiers (vData, vFileIds);
    CHR (hr);

    for (const SFileId & fileId : vFileIds)
    {
        hr = AddEntry (state, fileId, strPrefix, cDepth);
        CHR (hr);
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CUdfReader::AddEntry
//
//  Adds the entry for one File Identifier Descriptor and, for a directory,
//  its contents.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CUdfReader::AddEntry (
    SWalkState    & state,
    const SFileId & fileId,
    const wstring & strPrefix,
    UINT            cDepth)
{
    HRESULT         hr = S_OK;
    SFileEntry      fe;
    SArchiveEntry   entry;



    hr = ReadFileEntry (state, fileId.m_icb, fe);
    CHR (hr);

    entry.m_strPath      = strPrefix + fileId.m_strName;
    entry.m_fDirectory   = (fileId.m_bCharacteristics & s_kFidDirectory) != 0;
    entry.m_ullSize      = entry.m_fDirectory ? 0 : fe.m_ullSize;
    entry.m_ftLastWrite  = fe.m_ftModified;
    entry.m_dwAttributes = FILE_ATTRIBUTE_READONLY;

    if (fileId.m_bCharacteristics & s_kFidHidden)
    {
        entry.m_dwAttributes |= FILE_ATTRIBUTE_HIDDEN;
    }

    if (entry.m_fDirectory)
    {
        entry.m_dwAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    }

    state.m_result.m_vEntries.push_back (entry);

    if (entry.m_fDirectory)
    {
        hr = ReadDirectory (state, fileId.m_icb, fe, entry.m_strPath + L"\\", cDepth + 1);
        CHR (hr);
    }



Error:
    return hr;
}





