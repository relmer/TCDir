#include "pch.h"

#include "ArchiveInfoBuilder.h"

#include "ArchiveFileSystem.h"
#include "ArchiveStatistics.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoBuilder::CArchiveInfoBuilder
//
////////////////////////////////////////////////////////////////////////////////

CArchiveInfoBuilder::CArchiveInfoBuilder (CArchiveFileSystem & archiveFs, bool fVerify) :
    m_archiveFs (archiveFs),
    m_fVerify   (fVerify)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoBuilder::Build
//
//  Fills report for archivePath.  A failure to read the archive is stored in
//  the report (and returned), so the caller prints the error in its place.
//  An encrypted listing has no entries, so only its size and format are
//  reported.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveInfoBuilder::Build (const filesystem::path & archivePath, ULONGLONG ullSizeOnDisk, SArchiveReport & report)
{
    HRESULT              hr = S_OK;
    SArchiveReadResult   result;



    report                 = {};
    report.m_path          = archivePath;
    report.m_ullSizeOnDisk = ullSizeOnDisk;
    result.m_detailsPtr    = make_shared<SArchiveDetails>();

    hr = m_archiveFs.ReadDetails (archivePath, result, report.m_strError);
    CHR (hr);

    report.m_details = *result.m_detailsPtr;

    if (result.m_fListingEncrypted)
    {
        report.m_statistics.m_fListingEncrypted = true;
    }
    else
    {
        hr = CArchiveStatistics::Compute (result, report.m_statistics);
        CHR (hr);

        AddVerification (report);
    }



Error:
    report.m_hr = hr;
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoBuilder::AddVerification
//
//  With --Verify, reads every entry.  A failure to verify is stored in the
//  verify result rather than as a report error, so the statistics print.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveInfoBuilder::AddVerification (SArchiveReport & report)
{
    HRESULT         hr       = S_OK;
    SVerifyResult   verify;
    wstring         strError;



    BAIL_OUT_IF (!m_fVerify, S_OK);

    //
    // libarchive reads only the ISO 9660 tree of a disc image, so an image
    // listed from UDF can't be verified.
    //

    if (report.m_details.m_format == EArchiveFormat::Iso && report.m_details.m_disc.m_eListedTree == EDiscTree::Udf)
    {
        hr       = HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED);
        strError = L"the image is listed from UDF, and verification reads ISO 9660 only";
    }
    else
    {
        hr = m_archiveFs.Verify (report.m_path, verify, strError);
    }

    if (FAILED (hr))
    {
        verify             = {};
        verify.m_eCoverage = EChecksumCoverage::Unavailable;
        verify.m_strError  = strError;
    }

    report.m_verify = move (verify);



Error:
    return;
}




