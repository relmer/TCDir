#include "pch.h"

#include "ArchiveInfoDisplayer.h"

#include "ArchiveDetailLines.h"
#include "Console.h"
#include "DirectoryLister.h"
#include "ResultsDisplayerWithHeaderAndFooter.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer::CArchiveInfoDisplayer
//
////////////////////////////////////////////////////////////////////////////////

CArchiveInfoDisplayer::CArchiveInfoDisplayer (CConsole & console) :
    m_console (console)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer::Display
//
//  Prints one block: the archive path, then its labeled rows.  An archive
//  that couldn't be read prints the error line a listing prints.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveInfoDisplayer::Display (const SArchiveReport & report)
{
    vector<SInfoLine>   lines;
    size_t              cchLabel = 0;



    if (FAILED (report.m_hr))
    {
        CDirectoryLister::PrintArchiveError (m_console, report.m_path, report.m_strError);
        return;
    }

    ++m_totals.m_cArchives;
    m_totals.m_ullSizeOnDisk   += report.m_ullSizeOnDisk;
    m_totals.m_ullUncompressed += report.m_statistics.m_ullUncompressed;

    BuildLines (report, lines);
    cchLabel = LabelWidth (lines);

    m_console.Printf (CConfig::Information,          L" ");
    m_console.Printf (CConfig::InformationHighlight, L"%s\n\n", report.m_path.c_str());

    for (const SInfoLine & line : lines)
    {
        m_console.Printf (CConfig::Information, L"   %-*s", static_cast<int> (cchLabel), line.m_strLabel.c_str());
        m_console.Printf (line.m_attrValue,     L"%s\n", line.m_strValue.c_str());
    }

    m_console.Printf (CConfig::Information, L"\n");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer::DisplaySummary
//
//  Prints the combined totals when more than one archive was reported.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveInfoDisplayer::DisplaySummary()
{
    if (m_totals.m_cArchives > 1)
    {
        m_console.Printf (CConfig::Information, L" %s\n\n", FormatSummary (m_totals).c_str());
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer::BuildLines
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveInfoDisplayer::BuildLines (const SArchiveReport & report, vector<SInfoLine> & lines)
{
    lines.clear();

    AddFormatLines (report, lines);
    lines.push_back ({ L"Size on disk", FormatBytes (report.m_ullSizeOnDisk) });

    if (report.m_statistics.m_fListingEncrypted)
    {
        lines.push_back ({ L"Encryption", L"The listing is encrypted; entries can't be read without the password" });
        return;
    }

    AddContentLines (report, lines);
    AddEncryptionLines (report, lines);
    CArchiveDetailLines::AddDetails (report, lines);
    AddExtensionLines (report.m_statistics, lines);

    if (report.m_verify.has_value())
    {
        AddVerifyLines (report.m_verify.value(), lines);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer::AddFormatLines
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveInfoDisplayer::AddFormatLines (const SArchiveReport & report, vector<SInfoLine> & lines)
{
    lines.push_back ({ L"Format", CArchiveDetailLines::FormatName (report.m_details) });
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer::AddContentLines
//
//  Contents, uncompressed size, largest file, deepest path and date range.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveInfoDisplayer::AddContentLines (const SArchiveReport & report, vector<SInfoLine> & lines)
{
    const SArchiveStatistics & statistics = report.m_statistics;
    wstring                    strContents;
    wstring                    strUncompressed;



    strContents = FormatCount (statistics.m_cFiles, L"file", L"files") + L", " +
                  FormatCount (statistics.m_cFolders, L"folder", L"folders");

    if (statistics.m_cNestedArchives > 0)
    {
        strContents += L", " + FormatCount (statistics.m_cNestedArchives, L"nested archive", L"nested archives");
    }

    strUncompressed = FormatBytes (statistics.m_ullUncompressed);

    //
    // Disc images store files in whole sectors next to file system
    // structures, so the ratio of image size to file size means nothing.
    //

    if (statistics.m_ullUncompressed > 0 && report.m_details.m_format != EArchiveFormat::Iso)
    {
        strUncompressed += L" (" + FormatRatio (report.m_ullSizeOnDisk, statistics.m_ullUncompressed) + L")";
    }

    lines.push_back ({ L"Contents",     strContents });
    lines.push_back ({ L"Uncompressed", strUncompressed });

    lines.push_back ({ L"Largest file",
                       statistics.m_cFiles == 0
                           ? wstring (L"none")
                           : format (L"{} ({})", statistics.m_strLargestPath, FormatBytes (statistics.m_ullLargestSize)) });

    lines.push_back ({ L"Deepest path",
                       statistics.m_cDeepestDepth == 0
                           ? wstring (L"none")
                           : format (L"{} ({})", statistics.m_strDeepestPath, FormatCount (statistics.m_cDeepestDepth, L"level", L"levels")) });

    lines.push_back ({ L"Dates", FormatDateRange (statistics) });
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer::AddEncryptionLines
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveInfoDisplayer::AddEncryptionLines (const SArchiveReport & report, vector<SInfoLine> & lines)
{
    ULONGLONG cEncrypted = report.m_statistics.m_cEncrypted;



    if (CArchiveDetailLines::AddEncryption (report, lines))
    {
        return;
    }

    lines.push_back ({ L"Encryption",
                       cEncrypted == 0 ? wstring (L"None") : FormatCount (cEncrypted, L"encrypted entry", L"encrypted entries") });
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer::AddExtensionLines
//
//  One row per extension, largest first, with the count and size columns
//  aligned.  For archives with many extensions, only the largest ones get a
//  row, followed by a count of the rest.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveInfoDisplayer::AddExtensionLines (const SArchiveStatistics & statistics, vector<SInfoLine> & lines)
{
    size_t            cRows      = min (statistics.m_vExtensions.size(), s_kcMaxExtensionRows);
    size_t            cchExt     = 0;
    size_t            cchCount   = 0;
    size_t            cchBytes   = 0;
    vector<wstring>   vCounts;
    vector<wstring>   vBytes;
    vector<wstring>   rows;



    for (size_t i = 0; i < cRows; ++i)
    {
        const SExtensionGroup & group = statistics.m_vExtensions[i];

        vCounts.push_back (FormatCount (group.m_cFiles, L"file", L"files"));
        vBytes.push_back  (FormatBytes (group.m_ullSize));

        cchExt   = max (cchExt,   group.m_strExtension.size());
        cchCount = max (cchCount, vCounts.back().size());
        cchBytes = max (cchBytes, vBytes.back().size());
    }

    for (size_t i = 0; i < cRows; ++i)
    {
        rows.push_back (format (L"{:<{}}   {:<{}}   {:>{}}",
                                statistics.m_vExtensions[i].m_strExtension, cchExt,
                                vCounts[i], cchCount,
                                vBytes[i], cchBytes));
    }

    if (statistics.m_vExtensions.size() > cRows)
    {
        rows.push_back (L"and " + FormatCount (statistics.m_vExtensions.size() - cRows, L"more extension", L"more extensions"));
    }

    AddRows (L"Extensions", rows, lines);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer::AddVerifyLines
//
//  The counts and what the checksums cover, then one row per damaged entry
//  in the error color.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveInfoDisplayer::AddVerifyLines (const SVerifyResult & verify, vector<SInfoLine> & lines)
{
    static constexpr LPCWSTR s_krgCoverage[] =
    {
        L"checked against a checksum per entry",              // PerEntry
        L"checked by decompressing the stream",               // Stream
        L"the format doesn't store data checksums; entries were read",   // HeadersOnly
        L"the format doesn't store checksums; entries were read",        // None
    };

    size_t  iCoverage = static_cast<size_t> (verify.m_eCoverage);
    wstring strError  = verify.m_strError;



    if (verify.m_eCoverage == EChecksumCoverage::Unavailable)
    {
        strError.erase (0, strError.find_first_not_of (L": "));
        lines.push_back ({ L"Verify", L"Not available" + (strError.empty() ? wstring() : L": " + strError) });
        return;
    }

    lines.push_back ({ L"Verify", format (L"{} passed, {} damaged, {} skipped ({})",
                                          CResultsDisplayerWithHeaderAndFooter::FormatNumberWithSeparators (verify.m_cPassed),
                                          CResultsDisplayerWithHeaderAndFooter::FormatNumberWithSeparators (verify.m_cDamaged),
                                          CResultsDisplayerWithHeaderAndFooter::FormatNumberWithSeparators (verify.m_cSkipped),
                                          (iCoverage < ARRAYSIZE (s_krgCoverage)) ? s_krgCoverage[iCoverage] : L"") });

    for (const SDamagedEntry & damaged : verify.m_vDamaged)
    {
        lines.push_back ({ L"",
                           format (L"{}: {}", damaged.m_strPath.empty() ? wstring (L"(archive)") : damaged.m_strPath, damaged.m_strReason),
                           CConfig::Error });
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer::AddRows
//
//  Adds rows under one label: the label on the first row only.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveInfoDisplayer::AddRows (LPCWSTR pszLabel, const vector<wstring> & rows, vector<SInfoLine> & lines)
{
    for (size_t i = 0; i < rows.size(); ++i)
    {
        lines.push_back ({ i == 0 ? wstring (pszLabel) : wstring(), rows[i] });
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer::FormatDateRange
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveInfoDisplayer::FormatDateRange (const SArchiveStatistics & statistics)
{
    wstring strRange = L"none";



    if (statistics.m_ftOldest.has_value() && statistics.m_ftNewest.has_value())
    {
        strRange = FormatDateTime (statistics.m_ftOldest.value()) + L" to " + FormatDateTime (statistics.m_ftNewest.value());
    }

    return strRange;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer::FormatSummary
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveInfoDisplayer::FormatSummary (const SInfoTotals & totals)
{
    return format (L"{}, {} on disk, {} uncompressed",
                   FormatCount (totals.m_cArchives, L"archive", L"archives"),
                   FormatBytes (totals.m_ullSizeOnDisk),
                   FormatBytes (totals.m_ullUncompressed));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer::FormatCount
//
//  The count with digit grouping, then the singular or plural noun.
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveInfoDisplayer::FormatCount (ULONGLONG cItems, LPCWSTR pszSingular, LPCWSTR pszPlural)
{
    return CResultsDisplayerWithHeaderAndFooter::FormatNumberWithSeparators (cItems) + L" " + (cItems == 1 ? pszSingular : pszPlural);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer::FormatBytes
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveInfoDisplayer::FormatBytes (ULONGLONG cb)
{
    return FormatCount (cb, L"byte", L"bytes");
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer::FormatRatio
//
//  Stored size as a rounded percentage of the uncompressed size.
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveInfoDisplayer::FormatRatio (ULONGLONG ullStored, ULONGLONG ullUncompressed)
{
    ULONGLONG ullPercent = (ullStored * s_kullPercent + ullUncompressed / 2) / ullUncompressed;



    return format (L"{}%", ullPercent);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer::FormatDateTime
//
//  Local date and time in the listing's format.
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveInfoDisplayer::FormatDateTime (const FILETIME & ft)
{
    HRESULT      hr                     = S_OK;
    BOOL         fSuccess               = FALSE;
    SYSTEMTIME   st                     = {};
    SYSTEMTIME   stLocal                = {};
    WCHAR        szDate[s_kcchDateTime] = {};
    WCHAR        szTime[s_kcchDateTime] = {};
    wstring      strResult;



    fSuccess = FileTimeToSystemTime (&ft, &st);
    CWRA (fSuccess);

    fSuccess = SystemTimeToTzSpecificLocalTime (nullptr, &st, &stLocal);
    CWRA (fSuccess);

    fSuccess = GetDateFormatEx (LOCALE_NAME_USER_DEFAULT, 0, &stLocal, L"MM/dd/yyyy", szDate, ARRAYSIZE (szDate), nullptr);
    CWRA (fSuccess);

    fSuccess = GetTimeFormatEx (LOCALE_NAME_USER_DEFAULT, 0, &stLocal, L"hh:mm tt", szTime, ARRAYSIZE (szTime));
    CWRA (fSuccess);

    strResult = format (L"{} {}", szDate, szTime);



Error:
    return strResult;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer::LabelWidth
//
////////////////////////////////////////////////////////////////////////////////

size_t CArchiveInfoDisplayer::LabelWidth (const vector<SInfoLine> & lines)
{
    size_t cchMax = 0;



    for (const SInfoLine & line : lines)
    {
        cchMax = max (cchMax, line.m_strLabel.size());
    }

    return cchMax + s_kcchLabelGap;
}





