#include "pch.h"
#include "EhmTestHelper.h"
#include "Mocks/TestConsole.h"
#include "../TCDirCore/ArchiveInfoDisplayer.h"
#include "../TCDirCore/Config.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(ArchiveInfoDisplayerTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        static SArchiveReport MakeReport()
        {
            SArchiveReport report;

            report.m_path                         = L"C:\\Root\\a.zip";
            report.m_ullSizeOnDisk                = 50;
            report.m_details.m_format             = EArchiveFormat::Zip;
            report.m_statistics.m_cFiles          = 9;
            report.m_statistics.m_cFolders        = 4;
            report.m_statistics.m_cNestedArchives = 1;
            report.m_statistics.m_ullUncompressed = 200;
            report.m_statistics.m_strLargestPath  = L"big.bin";
            report.m_statistics.m_ullLargestSize  = 120;
            report.m_statistics.m_strDeepestPath  = L"src\\lib\\util.cpp";
            report.m_statistics.m_cDeepestDepth   = 3;
            report.m_statistics.m_vExtensions     = { { L".bin", 1, 120 }, { L".cpp", 12, 80 } };
            return report;
        }

        static const SInfoLine * FindLine (const vector<SInfoLine> & lines, LPCWSTR pszLabel)
        {
            for (const SInfoLine & line : lines)
            {
                if (line.m_strLabel == pszLabel)
                {
                    return &line;
                }
            }

            return nullptr;
        }

        static wstring ValueOf (const vector<SInfoLine> & lines, LPCWSTR pszLabel)
        {
            const SInfoLine * pLine = FindLine (lines, pszLabel);

            Assert::IsNotNull (pLine, pszLabel);
            return pLine->m_strValue;
        }




        TEST_METHOD(FormatCount_SingularAndPlural)
        {
            Assert::AreEqual (L"1 file",   CArchiveInfoDisplayer::FormatCount (1, L"file", L"files").c_str());
            Assert::AreEqual (L"0 files",  CArchiveInfoDisplayer::FormatCount (0, L"file", L"files").c_str());
            Assert::AreEqual (L"12 files", CArchiveInfoDisplayer::FormatCount (12, L"file", L"files").c_str());
            Assert::AreEqual (L"1 byte",   CArchiveInfoDisplayer::FormatBytes (1).c_str());
        }




        TEST_METHOD(FormatRatio_RoundsToPercent)
        {
            Assert::AreEqual (L"25%",  CArchiveInfoDisplayer::FormatRatio (50, 200).c_str());
            Assert::AreEqual (L"33%",  CArchiveInfoDisplayer::FormatRatio (1, 3).c_str());
            Assert::AreEqual (L"67%",  CArchiveInfoDisplayer::FormatRatio (2, 3).c_str());
            Assert::AreEqual (L"150%", CArchiveInfoDisplayer::FormatRatio (300, 200).c_str());
        }




        TEST_METHOD(BuildLines_CommonValues)
        {
            vector<SInfoLine> lines;



            CArchiveInfoDisplayer::BuildLines (MakeReport(), lines);

            Assert::AreEqual (L"ZIP",                                  ValueOf (lines, L"Format").c_str());       // No entries, so no creator
            Assert::AreEqual (L"50 bytes",                             ValueOf (lines, L"Size on disk").c_str());
            Assert::AreEqual (L"9 files, 4 folders, 1 nested archive", ValueOf (lines, L"Contents").c_str());
            Assert::AreEqual (L"200 bytes (25%)",                      ValueOf (lines, L"Uncompressed").c_str());
            Assert::AreEqual (L"big.bin (120 bytes)",                  ValueOf (lines, L"Largest file").c_str());
            Assert::AreEqual (L"src\\lib\\util.cpp (3 levels)",        ValueOf (lines, L"Deepest path").c_str());
            Assert::AreEqual (L"none",                                 ValueOf (lines, L"Dates").c_str());
            Assert::AreEqual (L"None",                                 ValueOf (lines, L"Encryption").c_str());
        }




        TEST_METHOD(BuildLines_EmptyArchive_NoneValues)
        {
            SArchiveReport    report;
            vector<SInfoLine> lines;



            report.m_details.m_format = EArchiveFormat::Zip;
            CArchiveInfoDisplayer::BuildLines (report, lines);

            Assert::AreEqual (L"0 files, 0 folders", ValueOf (lines, L"Contents").c_str());
            Assert::AreEqual (L"0 bytes",            ValueOf (lines, L"Uncompressed").c_str());
            Assert::AreEqual (L"none",               ValueOf (lines, L"Largest file").c_str());
            Assert::AreEqual (L"none",               ValueOf (lines, L"Deepest path").c_str());
            Assert::IsNull   (FindLine (lines, L"Extensions"));
        }




        TEST_METHOD(BuildLines_DateRange)
        {
            SArchiveReport    report = MakeReport();
            vector<SInfoLine> lines;
            FILETIME          ftOld  = { 0x10000000, 0x01D00000 };
            FILETIME          ftNew  = { 0x20000000, 0x01DC0000 };



            report.m_statistics.m_ftOldest = ftOld;
            report.m_statistics.m_ftNewest = ftNew;
            CArchiveInfoDisplayer::BuildLines (report, lines);

            Assert::AreEqual ((CArchiveInfoDisplayer::FormatDateTime (ftOld) + L" to " + CArchiveInfoDisplayer::FormatDateTime (ftNew)).c_str(),
                              ValueOf (lines, L"Dates").c_str());
            Assert::AreEqual (size_t (19), CArchiveInfoDisplayer::FormatDateTime (ftOld).size());   // "MM/dd/yyyy hh:mm tt"
        }




        TEST_METHOD(BuildLines_EncryptedEntries)
        {
            SArchiveReport    report = MakeReport();
            vector<SInfoLine> lines;



            report.m_details.m_format        = EArchiveFormat::LibArchive;
            report.m_statistics.m_cEncrypted = 2;
            CArchiveInfoDisplayer::BuildLines (report, lines);

            Assert::AreEqual (L"2 encrypted entries", ValueOf (lines, L"Encryption").c_str());
        }




        TEST_METHOD(BuildLines_EncryptedListing_OnlySizeAndFormat)
        {
            SArchiveReport    report = MakeReport();
            vector<SInfoLine> lines;



            report.m_statistics.m_fListingEncrypted = true;
            CArchiveInfoDisplayer::BuildLines (report, lines);

            Assert::AreEqual (size_t (3), lines.size());
            Assert::IsNotNull (FindLine (lines, L"Format"));
            Assert::IsNotNull (FindLine (lines, L"Size on disk"));
            Assert::IsTrue    (ValueOf (lines, L"Encryption").find (L"listing is encrypted") != wstring::npos);
        }




        TEST_METHOD(BuildLines_ExtensionRowsAligned)
        {
            vector<SInfoLine> lines;
            size_t            iFirst = 0;



            CArchiveInfoDisplayer::BuildLines (MakeReport(), lines);

            for (iFirst = 0; iFirst < lines.size() && lines[iFirst].m_strLabel != L"Extensions"; ++iFirst)
            {
            }

            Assert::IsTrue   (iFirst + 1 < lines.size());
            Assert::AreEqual (L".bin   1 file     120 bytes", lines[iFirst].m_strValue.c_str());
            Assert::AreEqual (L"",                            lines[iFirst + 1].m_strLabel.c_str());
            Assert::AreEqual (L".cpp   12 files    80 bytes", lines[iFirst + 1].m_strValue.c_str());
        }




        TEST_METHOD(BuildLines_ManyExtensions_Capped)
        {
            SArchiveReport    report = MakeReport();
            vector<SInfoLine> lines;



            report.m_statistics.m_vExtensions.clear();

            for (int i = 0; i < 25; ++i)
            {
                report.m_statistics.m_vExtensions.push_back ({ format (L".e{}", i), 1, 1 });
            }

            CArchiveInfoDisplayer::BuildLines (report, lines);

            Assert::AreEqual (L"and 5 more extensions", lines.back().m_strValue.c_str());
        }




        TEST_METHOD(FormatSummary_Totals)
        {
            SInfoTotals totals = { 3, 90, 400 };



            Assert::AreEqual (L"3 archives, 90 bytes on disk, 400 bytes uncompressed", CArchiveInfoDisplayer::FormatSummary (totals).c_str());
        }




        TEST_METHOD(Display_PrintsHeadingLabelsAndSummary)
        {
            shared_ptr<CConfig>   configPtr  = make_shared<CConfig>();
            CCapturingConsole     console;
            SArchiveReport        report     = MakeReport();



            console.Initialize (configPtr);

            CArchiveInfoDisplayer displayer (console);

            displayer.Display (report);
            displayer.DisplaySummary();
            console.Flush();

            Assert::IsTrue  (console.m_strCaptured.find (L"C:\\Root\\a.zip") != wstring::npos);
            Assert::IsTrue  (console.m_strCaptured.find (L"   Size on disk   ") != wstring::npos);
            Assert::IsTrue  (console.m_strCaptured.find (L"9 files, 4 folders, 1 nested archive") != wstring::npos);
            Assert::IsTrue  (console.m_strCaptured.find (L"archives,") == wstring::npos);   // One block: no summary
            Assert::AreEqual (1ull, displayer.Totals().m_cArchives);

            displayer.Display (report);
            displayer.DisplaySummary();
            console.Flush();

            Assert::IsTrue (console.m_strCaptured.find (L"2 archives, 100 bytes on disk, 400 bytes uncompressed") != wstring::npos);
        }




        TEST_METHOD(Display_ReadError_PrintsErrorLine)
        {
            shared_ptr<CConfig>   configPtr = make_shared<CConfig>();
            CCapturingConsole     console;
            SArchiveReport        report;



            console.Initialize (configPtr);

            CArchiveInfoDisplayer displayer (console);

            report.m_path     = L"C:\\Root\\bad.zip";
            report.m_hr       = HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT);
            report.m_strError = L" is not a valid archive";

            displayer.Display (report);
            console.Flush();

            Assert::IsTrue   (console.m_strCaptured.find (L"Error:") != wstring::npos);
            Assert::IsTrue   (console.m_strCaptured.find (L" is not a valid archive") != wstring::npos);
            Assert::AreEqual (0ull, displayer.Totals().m_cArchives);
        }




        //
        //  ZIP rows
        //

        static vector<SInfoLine> ZipLines (const SZipDetails & zip)
        {
            SArchiveReport    report = MakeReport();
            vector<SInfoLine> lines;



            report.m_details.m_zip = zip;
            CArchiveInfoDisplayer::BuildLines (report, lines);

            return lines;
        }

        static vector<wstring> RowsUnder (const vector<SInfoLine> & lines, LPCWSTR pszLabel)
        {
            vector<wstring> rows;
            size_t          i    = 0;



            for (i = 0; i < lines.size() && lines[i].m_strLabel != pszLabel; ++i)
            {
            }

            for (; i < lines.size() && (rows.empty() || lines[i].m_strLabel.empty()); ++i)
            {
                rows.push_back (lines[i].m_strValue);
            }

            return rows;
        }




        TEST_METHOD(Zip_FormatLine_CreatorAndUtf8)
        {
            SZipDetails zip;



            zip.m_mapCreators[static_cast<WORD> ((10 << 8) | 20)] = 3;
            zip.m_mapCreators[static_cast<WORD> ((3 << 8) | 63)]  = 1;
            zip.m_fUtf8Paths                                      = true;

            Assert::AreEqual (L"ZIP, made by NTFS 2.0, UTF-8 paths", ValueOf (ZipLines (zip), L"Format").c_str());
        }




        TEST_METHOD(Zip_MethodRowsAligned)
        {
            SZipDetails       zip;
            vector<wstring>   rows;



            zip.m_mapMethods[8]  = { 8, 102, 254 };
            zip.m_mapMethods[0]  = { 1, 235, 235 };
            zip.m_mapMethods[42] = { 1, 1, 1 };

            rows = RowsUnder (ZipLines (zip), L"Methods");

            Assert::AreEqual (size_t (3), rows.size());
            Assert::AreEqual (L"Deflate     8 files   102 of 254 bytes", rows[0].c_str());
            Assert::AreEqual (L"Store       1 file    235 of 235 bytes", rows[1].c_str());
            Assert::AreEqual (L"Method 42   1 file    1 of 1 byte",      rows[2].c_str());
        }




        TEST_METHOD(Zip_EncryptionPerType)
        {
            SZipDetails       zip;
            vector<wstring>   rows;



            zip.m_mapEncryption[EZipEncryption::None]      = 5;
            zip.m_mapEncryption[EZipEncryption::ZipCrypto] = 1;
            zip.m_mapEncryption[EZipEncryption::Aes256]    = 3;

            rows = RowsUnder (ZipLines (zip), L"Encryption");

            Assert::AreEqual (size_t (2), rows.size());
            Assert::AreEqual (L"ZipCrypto (1 entry)",  rows[0].c_str());
            Assert::AreEqual (L"AES-256 (3 entries)",  rows[1].c_str());
        }




        TEST_METHOD(Zip_OptionalRowsOmittedWhenFalse)
        {
            vector<SInfoLine> lines = ZipLines ({});



            Assert::IsNull (FindLine (lines, L"ZIP64"));
            Assert::IsNull (FindLine (lines, L"Disks"));
            Assert::IsNull (FindLine (lines, L"Prefix"));
            Assert::IsNull (FindLine (lines, L"Comment"));
            Assert::AreEqual (L"None", ValueOf (lines, L"Encryption").c_str());
        }




        TEST_METHOD(Zip_OptionalRowsAndMultiLineComment)
        {
            SZipDetails       zip;
            vector<SInfoLine> lines;
            vector<wstring>   comment;



            zip.m_fZip64        = true;
            zip.m_fMultiDisk    = true;
            zip.m_ullPrefixSize = 512;
            zip.m_strComment    = L"First line\r\nSecond line";

            lines   = ZipLines (zip);
            comment = RowsUnder (lines, L"Comment");

            Assert::AreEqual (L"Yes", ValueOf (lines, L"ZIP64").c_str());
            Assert::IsNotNull (FindLine (lines, L"Disks"));
            Assert::AreEqual (L"512 bytes before the archive data (self-extracting)", ValueOf (lines, L"Prefix").c_str());
            Assert::AreEqual (size_t (2), comment.size());
            Assert::AreEqual (L"First line",  comment[0].c_str());
            Assert::AreEqual (L"Second line", comment[1].c_str());
        }
    };
}
