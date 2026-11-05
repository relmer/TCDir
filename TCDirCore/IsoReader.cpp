#include "pch.h"

#include "IsoReader.h"

#include "DiscImage.h"
#include "UdfReader.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoReader::Read
//
//  Lists every file and folder in a disc image.  The UDF volume is used when
//  present, because UDF images often contain an ISO 9660 "bridge" tree
//  with only a README.  When UDF is absent or uses an unsupported
//  partition type, the Joliet tree is used, then the primary tree.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CIsoReader::Read (
    IByteSource        & source,
    SArchiveReadResult & result)
{
    HRESULT              hr              = S_OK;
    HRESULT              hrUdf           = S_OK;
    SArchiveReadResult   udfResult;
    bool                 fUdfUnsupported = false;
    bool                 fFallBack       = false;



    result = {};

    hrUdf           = CUdfReader::Read (source, udfResult);
    fUdfUnsupported = hrUdf == HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED);
    fFallBack       = fUdfUnsupported || hrUdf == HRESULT_FROM_WIN32 (ERROR_NOT_FOUND);

    CBREx (SUCCEEDED (hrUdf) || fFallBack, hrUdf);

    if (SUCCEEDED (hrUdf))
    {
        result = std::move (udfResult);
    }
    else
    {
        hr = ReadIso9660Tree (source, fUdfUnsupported, result);
        CHR (hr);
    }



Error:
    if (FAILED (hr))
    {
        result = {};
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoReader::ReadIso9660Tree
//
//  Lists the Joliet tree when present, otherwise the primary tree.  When the
//  image has an unsupported UDF volume and an empty ISO 9660 tree, returns
//  ERROR_NOT_SUPPORTED, since an empty listing would hide the real content.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CIsoReader::ReadIso9660Tree (
    IByteSource        & source,
    bool                 fUdfUnsupported,
    SArchiveReadResult & result)
{
    HRESULT        hr = S_OK;
    SVolumeRoots   roots;
    SWalkState     state (source, result);



    hr = FindVolumeRoots (source, roots);
    CHR (hr);

    state.m_fJoliet = roots.m_fJoliet;

    hr = ReadDirectory (state, roots.m_fJoliet ? roots.m_joliet : roots.m_primary, L"", 0);
    CHR (hr);

    CBREx (!fUdfUnsupported || !result.m_vEntries.empty(), HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED));



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoReader::FindVolumeRoots
//
//  Reads the volume descriptors that start at sector 16 up to the set
//  terminator, and returns the primary and Joliet root directories.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CIsoReader::FindVolumeRoots (
    IByteSource  & source,
    SVolumeRoots & roots)
{
    HRESULT        hr      = S_OK;
    vector<BYTE>   vSector;
    ULONG          iSector = 0;
    bool           fEnd    = false;



    for (iSector = 0; iSector < s_kMaxVolumeDescriptors && !fEnd; iSector++)
    {
        hr = CDiscImage::ReadBytes (source, s_kFirstVolumeDescriptor + iSector, CDiscImage::s_kcbSector, vSector);
        CHR (hr);

        hr = ParseVolumeDescriptor (vSector, roots, fEnd);
        CHR (hr);
    }

    CBREx (roots.m_fPrimary, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoReader::ParseVolumeDescriptor
//
//  Stores the root of a primary or Joliet volume descriptor in roots.  A sector
//  without the CD001 identifier, or the set terminator, sets fEnd.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CIsoReader::ParseVolumeDescriptor (
    const vector<BYTE> & vSector,
    SVolumeRoots       & roots,
    bool               & fEnd)
{
    HRESULT hr      = S_OK;
    BYTE    bType   = vSector[0];
    bool    fIso    = memcmp (vSector.data() + s_kibStandardId, "CD001", s_kcchStandardId) == 0;
    bool    fJoliet = false;



    fEnd = !fIso || bType == s_kTypeTerminator;
    BAIL_OUT_IF (fEnd, S_OK);

    if (bType == s_kTypePrimary && !roots.m_fPrimary)
    {
        hr = ParseRootRecord (vSector, false, roots.m_primary);
        CHR (hr);

        roots.m_fPrimary = true;
    }
    else if (bType == s_kTypeSupplementary && !roots.m_fJoliet)
    {
        fJoliet = IsJolietEscape (vSector.data() + s_kibEscapeSequences);
        BAIL_OUT_IF (!fJoliet, S_OK);

        hr = ParseRootRecord (vSector, true, roots.m_joliet);
        CHR (hr);

        roots.m_fJoliet = true;
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoReader::ParseRootRecord
//
//  Decodes the root directory record embedded in a volume descriptor.  Only
//  2048-byte logical blocks are supported.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CIsoReader::ParseRootRecord (
    const vector<BYTE> & vSector,
    bool                 fJoliet,
    SDirectoryRecord   & root)
{
    HRESULT hr      = S_OK;
    USHORT  cbBlock = CDiscImage::ReadU16 (vSector.data() + s_kibLogicalBlockSize);



    CBREx (cbBlock == CDiscImage::s_kcbSector, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    hr = ParseDirectoryRecord (vSector.data() + s_kibRootRecord, s_kcbRootRecord, fJoliet, root);
    CHR (hr);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoReader::IsJolietEscape
//
//  Returns true for the UCS-2 level 1, 2 and 3 escape sequences.
//
////////////////////////////////////////////////////////////////////////////////

bool CIsoReader::IsJolietEscape (const BYTE * pEscape)
{
    BYTE bLevel = pEscape[s_kcchJolietPrefix];



    return memcmp (pEscape, "%/", s_kcchJolietPrefix) == 0 && (bLevel == '@' || bLevel == 'C' || bLevel == 'E');
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoReader::ReadDirectory
//
//  Adds every entry in a directory, then recurses into its subdirectories.
//  Each directory extent is visited once, so a cycle returns
//  ERROR_BAD_FORMAT instead of looping.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CIsoReader::ReadDirectory (
    SWalkState             & state,
    const SDirectoryRecord & directory,
    const wstring          & strPrefix,
    UINT                     cDepth)
{
    HRESULT                    hr        = S_OK;
    bool                       fInserted = false;
    vector<BYTE>               vData;
    vector<SDirectoryRecord>   vRecords;
    vector<size_t>             vSubdirectories;



    CBREx (cDepth <= s_kMaxDepth, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));
    CBREx (directory.m_cbData <= s_kcbMaxDirectory, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    fInserted = state.m_setVisited.insert (directory.m_ulExtent).second;
    CBREx (fInserted, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    hr = CDiscImage::ReadBytes (state.m_source, directory.m_ulExtent, directory.m_cbData, vData);
    CHR (hr);

    hr = ParseDirectoryExtent (vData, state.m_fJoliet, vRecords);
    CHR (hr);

    AppendEntries (state, vRecords, strPrefix, vSubdirectories);

    for (size_t iRecord : vSubdirectories)
    {
        hr = ReadDirectory (state, vRecords[iRecord], strPrefix + vRecords[iRecord].m_strName + L"\\", cDepth + 1);
        CHR (hr);
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoReader::ParseDirectoryExtent
//
//  Decodes each directory record in an extent, omitting the self and
//  parent entries.  A directory record never crosses a sector boundary, so
//  a zero length byte marks the unused tail of a sector.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CIsoReader::ParseDirectoryExtent (
    const vector<BYTE>       & vData,
    bool                       fJoliet,
    vector<SDirectoryRecord> & vRecords)
{
    HRESULT            hr          = S_OK;
    size_t             ib          = 0;
    size_t             cbRecord    = 0;
    size_t             cbAvailable = 0;
    SDirectoryRecord   record;



    vRecords.clear();

    while (ib < vData.size())
    {
        cbRecord = vData[ib];

        if (cbRecord == 0)
        {
            ib = (ib / CDiscImage::s_kcbSector + 1) * CDiscImage::s_kcbSector;
            continue;
        }

        cbAvailable = min (CDiscImage::s_kcbSector - ib % CDiscImage::s_kcbSector, vData.size() - ib);

        hr = ParseDirectoryRecord (vData.data() + ib, cbAvailable, fJoliet, record);
        CHR (hr);

        if (!record.m_fSelfOrParent)
        {
            vRecords.push_back (record);
        }

        ib += cbRecord;
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoReader::ParseDirectoryRecord
//
//  Decodes one directory record.  Both-endian fields are read from their
//  little-endian half.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CIsoReader::ParseDirectoryRecord (
    const BYTE       * pRecord,
    size_t             cbAvailable,
    bool               fJoliet,
    SDirectoryRecord & record)
{
    HRESULT hr       = S_OK;
    size_t  cbRecord = 0;
    size_t  cbId     = 0;



    CBREx (cbAvailable >= s_kcbMinRecord, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    cbRecord = pRecord[0];
    cbId     = pRecord[s_kibRecordIdLength];

    CBREx (cbRecord >= s_kcbMinRecord && cbRecord <= cbAvailable, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));
    CBREx (cbId > 0 && s_kibRecordId + cbId <= cbRecord, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    record.m_ulExtent      = CDiscImage::ReadU32 (pRecord + s_kibRecordExtent) + pRecord[s_kibRecordXarLength];
    record.m_cbData        = CDiscImage::ReadU32 (pRecord + s_kibRecordDataLength);
    record.m_bFlags        = pRecord[s_kibRecordFlags];
    record.m_fSelfOrParent = cbId == 1 && (pRecord[s_kibRecordId] == s_kIdSelf || pRecord[s_kibRecordId] == s_kIdParent);
    record.m_strName.clear();

    hr = ConvertRecordingDate (pRecord + s_kibRecordDate, record.m_ftRecorded);
    IGNORE_RETURN_VALUE (hr, S_OK);

    BAIL_OUT_IF (record.m_fSelfOrParent, S_OK);

    hr = DecodeName (pRecord + s_kibRecordId, cbId, fJoliet, record.m_strName);
    CHR (hr);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoReader::DecodeName
//
//  Decodes a file identifier: single-byte characters in the primary tree,
//  big-endian UCS-2 in the Joliet tree.  The ";1" version suffix and a
//  trailing '.' are removed.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CIsoReader::DecodeName (
    const BYTE * pId,
    size_t       cbId,
    bool         fJoliet,
    wstring    & strName)
{
    HRESULT hr = S_OK;
    size_t  ib = 0;



    strName.clear();

    if (fJoliet)
    {
        CBREx (cbId % sizeof (wchar_t) == 0, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

        for (ib = 0; ib < cbId; ib += sizeof (wchar_t))
        {
            strName += (wchar_t) ((pId[ib] << CDiscImage::s_kBitsPerByte) | pId[ib + 1]);
        }
    }
    else
    {
        for (ib = 0; ib < cbId; ib++)
        {
            strName += (wchar_t) pId[ib];
        }
    }

    RemoveVersionSuffix (strName);

    CBREx (!strName.empty(), HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoReader::RemoveVersionSuffix
//
//  Turns "README.TXT;1" into "README.TXT" and "NOEXT.;1" into "NOEXT".
//
////////////////////////////////////////////////////////////////////////////////

void CIsoReader::RemoveVersionSuffix (wstring & strName)
{
    size_t ichSemicolon = strName.rfind (L';');



    if (ichSemicolon != wstring::npos)
    {
        strName.erase (ichSemicolon);
    }

    if (!strName.empty() && strName.back() == L'.')
    {
        strName.pop_back();
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoReader::ConvertRecordingDate
//
//  Converts the 7-byte recording date to a UTC FILETIME.  An all-zero or
//  invalid date returns a failure and a zero FILETIME.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CIsoReader::ConvertRecordingDate (
    const BYTE * pDate,
    FILETIME   & ft)
{
    HRESULT          hr   = S_OK;
    SRecordingDate   date = {};
    SYSTEMTIME       st   = {};



    memcpy (&date, pDate, sizeof (date));

    st.wYear   = (WORD) (s_kYearBase + date.m_bYear);
    st.wMonth  = date.m_bMonth;
    st.wDay    = date.m_bDay;
    st.wHour   = date.m_bHour;
    st.wMinute = date.m_bMinute;
    st.wSecond = date.m_bSecond;

    hr = CDiscImage::ToUtcFileTime (st, date.m_chOffset * s_kMinutesPerOffsetUnit, ft);
    CHR (hr);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoReader::AppendEntries
//
//  Adds an entry per directory record and returns the indexes of the
//  subdirectories.  A file larger than one extent is stored as a run of
//  directory entries with the same name, all but the last with the
//  multi-extent flag; their sizes are summed into one entry.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoReader::AppendEntries (
    SWalkState                     & state,
    const vector<SDirectoryRecord> & vRecords,
    const wstring                  & strPrefix,
    vector<size_t>                 & vSubdirectories)
{
    vector<SArchiveEntry>  & vEntries      = state.m_result.m_vEntries;
    const SDirectoryRecord * pRecord       = nullptr;
    bool                     fContinuation = false;
    size_t                   iRecord       = 0;
    wstring                  strPath;



    vSubdirectories.clear();

    for (iRecord = 0; iRecord < vRecords.size(); iRecord++)
    {
        pRecord = &vRecords[iRecord];
        strPath = strPrefix + pRecord->m_strName;

        if (fContinuation && !vEntries.empty() && vEntries.back().m_strPath == strPath)
        {
            vEntries.back().m_ullSize += pRecord->m_cbData;
        }
        else
        {
            vEntries.push_back (MakeEntry (*pRecord, strPath));

            if (pRecord->m_bFlags & s_kFlagDirectory)
            {
                vSubdirectories.push_back (iRecord);
            }
        }

        fContinuation = (pRecord->m_bFlags & s_kFlagMultiExtent) != 0;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoReader::MakeEntry
//
//  Every entry is read-only because a disc image is read-only media.
//
////////////////////////////////////////////////////////////////////////////////

SArchiveEntry CIsoReader::MakeEntry (
    const SDirectoryRecord & record,
    const wstring          & strPath)
{
    SArchiveEntry entry;



    entry.m_strPath      = strPath;
    entry.m_fDirectory   = (record.m_bFlags & s_kFlagDirectory) != 0;
    entry.m_ullSize      = entry.m_fDirectory ? 0 : record.m_cbData;
    entry.m_ftLastWrite  = record.m_ftRecorded;
    entry.m_dwAttributes = FILE_ATTRIBUTE_READONLY;

    if (record.m_bFlags & s_kFlagHidden)
    {
        entry.m_dwAttributes |= FILE_ATTRIBUTE_HIDDEN;
    }

    if (entry.m_fDirectory)
    {
        entry.m_dwAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    }

    return entry;
}





