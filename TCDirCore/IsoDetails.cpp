#include "pch.h"

#include "IsoDetails.h"

#include "DiscImage.h"
#include "IsoReader.h"
#include "UdfReader.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoDetails::Read
//
//  Each value is optional.  When a structure fails to read, its values are
//  unset and the report doesn't fail.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoDetails::Read (IByteSource & source, SDiscDetails & disc)
{
    ReadIso9660 (source, disc);
    ReadUdf (source, disc);

    if (disc.m_ullSectorCount == 0)
    {
        disc.m_ulSectorSize   = CDiscImage::s_kcbSector;
        disc.m_ullSectorCount = source.Size() / CDiscImage::s_kcbSector;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoDetails::ReadIso9660
//
//  Reads the volume descriptors, then the root directory for Rock Ridge and
//  the boot catalog for El Torito.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoDetails::ReadIso9660 (IByteSource & source, SDiscDetails & disc)
{
    HRESULT        hr           = S_OK;
    vector<BYTE>   vSector;
    ULONG          ulRootExtent = 0;
    ULONG          ulCatalog    = 0;
    bool           fEnd         = false;



    for (ULONG iSector = 0; iSector < s_kMaxDescriptors && !fEnd; ++iSector)
    {
        hr = CDiscImage::ReadBytes (source, s_kFirstDescriptor + iSector, CDiscImage::s_kcbSector, vSector);
        CHR (hr);

        ParseVolumeDescriptor (vSector.data(), disc, ulRootExtent, ulCatalog, fEnd);
    }

    if (ulRootExtent != 0)
    {
        hr = CDiscImage::ReadBytes (source, ulRootExtent, CDiscImage::s_kcbSector, vSector);
        CHR (hr);

        disc.m_fRockRidge = HasRockRidge (vSector.data(), vSector.size());
    }

    if (ulCatalog != 0)
    {
        hr = CDiscImage::ReadBytes (source, ulCatalog, CDiscImage::s_kcbSector, vSector);
        CHR (hr);

        ParseBootCatalog (vSector.data(), vSector.size(), disc.m_vBootPlatforms);
    }



Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoDetails::ReadUdf
//
//  Follows the anchor at sector 256 to the main volume descriptor sequence
//  and reads the logical volume descriptor.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoDetails::ReadUdf (IByteSource & source, SDiscDetails & disc)
{
    HRESULT        hr         = S_OK;
    vector<BYTE>   vSector;
    ULONG          cbVds      = 0;
    ULONG          ulVds      = 0;
    USHORT         usTag      = 0;
    ULONG          cSectors   = 0;



    hr = CDiscImage::ReadBytes (source, s_kAnchorSector, CDiscImage::s_kcbSector, vSector);
    CHR (hr);

    usTag = CDiscImage::ReadU16 (vSector.data());
    BAIL_OUT_IF (usTag != s_kTagAnchor, S_OK);

    cbVds    = CDiscImage::ReadU32 (vSector.data() + s_kibAnchorMainVds);
    ulVds    = CDiscImage::ReadU32 (vSector.data() + s_kibAnchorMainVds + sizeof (ULONG));
    cSectors = min (cbVds / CDiscImage::s_kcbSector, s_kMaxVdsSectors);

    for (ULONG iSector = 0; iSector < cSectors; ++iSector)
    {
        hr = CDiscImage::ReadBytes (source, (ULONGLONG) ulVds + iSector, CDiscImage::s_kcbSector, vSector);
        CHR (hr);

        usTag = CDiscImage::ReadU16 (vSector.data());

        if (usTag == s_kTagTerminating)
        {
            break;
        }

        if (usTag == s_kTagLogicalVolume)
        {
            ParseLogicalVolume (vSector.data(), disc);
        }
    }



Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoDetails::ParseVolumeDescriptor
//
//  Primary and Joliet descriptors supply labels, IDs, dates and sizes; the
//  boot record supplies the El Torito catalog sector.  A sector without the
//  CD001 identifier, or the set terminator, sets fEnd.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoDetails::ParseVolumeDescriptor (
    const BYTE     * pSector,
    SDiscDetails   & disc,
    ULONG          & ulRootExtent,
    ULONG          & ulCatalog,
    bool           & fEnd)
{
    BYTE   bType = pSector[0];
    bool   fIso  = memcmp (pSector + s_kibStandardId, "CD001", s_kcchStandardId) == 0;



    fEnd = !fIso || bType == s_kTypeTerminator;

    if (fEnd)
    {
        return;
    }

    if (bType == s_kTypePrimary && !disc.m_fIso9660)
    {
        disc.m_fIso9660       = true;
        disc.m_ulSectorSize   = CDiscImage::ReadU16 (pSector + s_kibLogicalBlockSize);
        disc.m_ullSectorCount = CDiscImage::ReadU32 (pSector + s_kibVolumeSpaceSize);
        disc.m_ftCreated      = ParseVolumeDate (pSector + s_kibCreationDate);
        disc.m_ftModified     = ParseVolumeDate (pSector + s_kibModificationDate);
        disc.m_ftExpires      = ParseVolumeDate (pSector + s_kibExpirationDate);
        ulRootExtent          = CDiscImage::ReadU32 (pSector + s_kibRootRecord + s_kibRecordExtent);

        ParsePrimaryFields (pSector, false, disc);
    }
    else if (bType == s_kTypeSupplementary && !disc.m_fJoliet && CIsoReader::IsJolietEscape (pSector + s_kibEscapeSequences))
    {
        disc.m_fJoliet = true;

        ParsePrimaryFields (pSector, true, disc);
    }
    else if (bType == s_kTypeBootRecord && memcmp (pSector + s_kibBootSystemId, s_kszElTorito, sizeof (s_kszElTorito) - 1) == 0)
    {
        ulCatalog = CDiscImage::ReadU32 (pSector + s_kibBootCatalog);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoDetails::ParsePrimaryFields
//
//  Joliet values replace the primary ones when they aren't blank, because
//  the primary descriptor is limited to uppercase d-characters.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoDetails::ParsePrimaryFields (const BYTE * pSector, bool fJoliet, SDiscDetails & disc)
{
    struct SField
    {
        size_t                   m_ib;
        size_t                   m_cb;
        wstring SDiscDetails::*  m_pstr;
    };

    static constexpr SField s_krgFields[] =
    {
        { s_kibSystemId,      s_kcchSystemId, &SDiscDetails::m_strSystemId    },
        { s_kibVolumeId,      s_kcchVolumeId, &SDiscDetails::m_strVolumeLabel },
        { s_kibPublisherId,   s_kcchLongId,   &SDiscDetails::m_strPublisher   },
        { s_kibPreparerId,    s_kcchLongId,   &SDiscDetails::m_strPreparer    },
        { s_kibApplicationId, s_kcchLongId,   &SDiscDetails::m_strApplication },
    };

    wstring strValue;



    for (const SField & field : s_krgFields)
    {
        strValue = DecodeText (pSector + field.m_ib, field.m_cb, fJoliet);

        if (!strValue.empty())
        {
            disc.*(field.m_pstr) = strValue;
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoDetails::HasRockRidge
//
//  Rock Ridge images start the system use area of the root "." record with
//  a SUSP "SP" entry, followed by Rock Ridge entries such as "RR", "PX" or
//  "NM".
//
////////////////////////////////////////////////////////////////////////////////

bool CIsoDetails::HasRockRidge (const BYTE * pDirectory, size_t cb)
{
    size_t   cbRecord = pDirectory[0];
    size_t   cchId    = pDirectory[s_kibRecordIdLength];
    size_t   ib       = s_kibRecordId + cchId + ((cchId % 2 == 0) ? 1 : 0);
    bool     fSusp    = false;
    bool     fFound   = false;



    cbRecord = min (cbRecord, cb);

    while (!fFound && ib + s_kcbSuspHeader <= cbRecord)
    {
        size_t cbEntry = pDirectory[ib + s_kibSuspLength];

        if (cbEntry < s_kcbSuspHeader)
        {
            break;
        }

        if (memcmp (pDirectory + ib, "SP", s_kcchSuspSignature) == 0)
        {
            fSusp = true;
        }
        else if (fSusp && (memcmp (pDirectory + ib, "RR", s_kcchSuspSignature) == 0 ||
                           memcmp (pDirectory + ib, "PX", s_kcchSuspSignature) == 0 ||
                           memcmp (pDirectory + ib, "NM", s_kcchSuspSignature) == 0))
        {
            fFound = true;
        }

        ib += cbEntry;
    }

    return fFound;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoDetails::ParseBootCatalog
//
//  The validation entry contains the platform of the default boot entry.
//  Each section header that follows contains the platform of its entries.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoDetails::ParseBootCatalog (const BYTE * pCatalog, size_t cb, vector<BYTE> & vPlatforms)
{
    BYTE bHeader = 0;



    vPlatforms.clear();

    if (cb < s_kcbCatalogEntry || pCatalog[0] != s_kHeaderValidation)
    {
        return;
    }

    AddPlatform (pCatalog[s_kibPlatform], vPlatforms);

    for (size_t ib = s_kcbCatalogEntry * 2; ib + s_kcbCatalogEntry <= cb; ib += s_kcbCatalogEntry)
    {
        bHeader = pCatalog[ib];

        if (bHeader == s_kHeaderSection || bHeader == s_kHeaderFinalSection)
        {
            AddPlatform (pCatalog[ib + s_kibPlatform], vPlatforms);
        }

        if (bHeader == s_kHeaderFinalSection)
        {
            break;
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoDetails::ParseLogicalVolume
//
//  The UDF revision is the first field of the domain identifier suffix; the
//  label is the logical volume identifier, a dstring whose last byte is
//  its used length.
//
////////////////////////////////////////////////////////////////////////////////

void CIsoDetails::ParseLogicalVolume (const BYTE * pSector, SDiscDetails & disc)
{
    HRESULT        hr      = S_OK;
    const BYTE   * pLabel  = pSector + s_kibLvdIdentifier;
    size_t         cbLabel = min<size_t> (pLabel[s_kcbLvdIdentifier - 1], s_kcbLvdIdentifier - 1);
    wstring        strLabel;



    disc.m_wUdfRevision = CDiscImage::ReadU16 (pSector + s_kibLvdDomainSuffix);

    hr = CUdfReader::DecodeOstaName (pLabel, cbLabel, strLabel);

    if (SUCCEEDED (hr) && !strLabel.empty())
    {
        disc.m_strVolumeLabel = strLabel;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoDetails::ParseVolumeDate
//
//  Returns an empty optional for an unset date, which is all '0' digits or
//  all zero bytes.
//
////////////////////////////////////////////////////////////////////////////////

optional<FILETIME> CIsoDetails::ParseVolumeDate (const BYTE * pDate)
{
    HRESULT              hr      = S_OK;
    SYSTEMTIME           st      = {};
    FILETIME             ft      = {};
    optional<FILETIME>   result;
    size_t               ib      = 0;
    int                  iOffset = static_cast<signed char> (pDate[s_kcchDateDigits]);



    for (ib = 0; ib < s_kcchDateDigits && (pDate[ib] == '0' || pDate[ib] == 0); ++ib)
    {
    }

    if (ib == s_kcchDateDigits)
    {
        return result;
    }

    st.wYear         = static_cast<WORD> (Digits (pDate, s_kcchYear));
    st.wMonth        = static_cast<WORD> (Digits (pDate + s_kcchYear, s_kcchDateField));
    st.wDay          = static_cast<WORD> (Digits (pDate + s_kcchYear + s_kcchDateField, s_kcchDateField));
    st.wHour         = static_cast<WORD> (Digits (pDate + s_kcchYear + s_kcchDateField * 2, s_kcchDateField));
    st.wMinute       = static_cast<WORD> (Digits (pDate + s_kcchYear + s_kcchDateField * 3, s_kcchDateField));
    st.wSecond       = static_cast<WORD> (Digits (pDate + s_kcchYear + s_kcchDateField * 4, s_kcchDateField));
    st.wMilliseconds = static_cast<WORD> (Digits (pDate + s_kibCentiseconds, s_kcchDateField) * s_kMsPerCentisecond);

    hr = CDiscImage::ToUtcFileTime (st, iOffset * s_kMinutesPerOffsetUnit, ft);

    if (SUCCEEDED (hr))
    {
        result = ft;
    }

    return result;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoDetails::DecodeText
//
//  Decodes an a-character or d-character field (one byte per character) or
//  a Joliet field (big-endian UCS-2), without trailing spaces or nulls.
//
////////////////////////////////////////////////////////////////////////////////

wstring CIsoDetails::DecodeText (const BYTE * p, size_t cb, bool fUcs2)
{
    wstring strText;



    if (fUcs2)
    {
        for (size_t ib = 0; ib + 1 < cb; ib += sizeof (wchar_t))
        {
            strText += static_cast<wchar_t> ((p[ib] << CDiscImage::s_kBitsPerByte) | p[ib + 1]);
        }
    }
    else
    {
        strText.assign (p, p + cb);
    }

    while (!strText.empty() && (strText.back() == L' ' || strText.back() == L'\0'))
    {
        strText.pop_back();
    }

    return strText;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoDetails::AddPlatform
//
////////////////////////////////////////////////////////////////////////////////

void CIsoDetails::AddPlatform (BYTE bPlatform, vector<BYTE> & vPlatforms)
{
    if (find (vPlatforms.begin(), vPlatforms.end(), bPlatform) == vPlatforms.end())
    {
        vPlatforms.push_back (bPlatform);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CIsoDetails::Digits
//
////////////////////////////////////////////////////////////////////////////////

int CIsoDetails::Digits (const BYTE * p, size_t cch)
{
    int iValue = 0;



    for (size_t i = 0; i < cch; ++i)
    {
        iValue = iValue * s_kDecimalBase + ((p[i] >= '0' && p[i] <= '9') ? p[i] - '0' : 0);
    }

    return iValue;
}





