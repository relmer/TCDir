#include "pch.h"
#include "EhmTestHelper.h"
#include "Mocks/MockArchiveFileAccess.h"
#include "../TCDirCore/ArchiveFileSystem.h"
#include "../TCDirCore/ArchiveInfoBuilder.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(ArchiveInfoBuilderTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        static shared_ptr<CMockArchiveFileAccess> MakeAccess()
        {
            auto accessPtr = make_shared<CMockArchiveFileAccess>();

            accessPtr->AddDirectory (L"C:\\Root");
            accessPtr->AddArchive   (L"C:\\Root\\a.zip", {
                CMockArchiveFileAccess::File   (L"readme.txt", 100),
                CMockArchiveFileAccess::Folder (L"docs/"),
                CMockArchiveFileAccess::File   (L"docs/inner.zip", 40),
            });
            accessPtr->AddArchive   (L"C:\\Root\\a.zip\\docs\\inner.zip", {
                CMockArchiveFileAccess::File (L"x.txt", 7),
            });
            return accessPtr;
        }




        TEST_METHOD(Build_ReportsSizeStatisticsAndDetails)
        {
            auto                accessPtr = MakeAccess();
            CArchiveFileSystem  fs          (accessPtr);
            CArchiveInfoBuilder builder     (fs, false);
            SArchiveReport      report;
            SArchiveDetails     details;

            details.m_zip.m_fZip64 = true;
            accessPtr->SetDetails (L"C:\\Root\\a.zip", details);

            Assert::AreEqual (S_OK, builder.Build (L"C:\\Root\\a.zip", 1234, report));
            Assert::AreEqual (L"C:\\Root\\a.zip", report.m_path.c_str());
            Assert::AreEqual (1234ull, report.m_ullSizeOnDisk);
            Assert::AreEqual (2ull, report.m_statistics.m_cFiles);
            Assert::AreEqual (1ull, report.m_statistics.m_cFolders);
            Assert::AreEqual (1ull, report.m_statistics.m_cNestedArchives);
            Assert::IsTrue   (report.m_details.m_zip.m_fZip64);
            Assert::IsTrue   (report.m_details.m_format == EArchiveFormat::Zip);
            Assert::IsFalse  (report.m_verify.has_value());
        }




        TEST_METHOD(Build_NestedArchive)
        {
            auto                accessPtr = MakeAccess();
            CArchiveFileSystem  fs          (accessPtr);
            CArchiveInfoBuilder builder     (fs, false);
            SArchiveReport      report;

            Assert::AreEqual (S_OK, builder.Build (L"C:\\Root\\a.zip\\docs\\inner.zip", 40, report));
            Assert::AreEqual (1ull, report.m_statistics.m_cFiles);
            Assert::AreEqual (40ull, report.m_ullSizeOnDisk);
        }




        TEST_METHOD(Build_ReadError_StoredInReport)
        {
            auto                accessPtr = MakeAccess();
            CArchiveFileSystem  fs          (accessPtr);
            CArchiveInfoBuilder builder     (fs, false);
            SArchiveReport      report;

            accessPtr->AddBrokenArchive (L"C:\\Root\\bad.zip", HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

            Assert::IsTrue   (FAILED (builder.Build (L"C:\\Root\\bad.zip", 10, report)));
            Assert::IsTrue   (FAILED (report.m_hr));
            Assert::AreEqual (L" is not a valid archive", report.m_strError.c_str());
            Assert::AreEqual (10ull, report.m_ullSizeOnDisk);
        }




        TEST_METHOD(Build_EncryptedListing_NoStatistics)
        {
            auto                accessPtr = MakeAccess();
            CArchiveFileSystem  fs          (accessPtr);
            CArchiveInfoBuilder builder     (fs, true);
            SArchiveReport      report;

            accessPtr->AddEncryptedListingArchive (L"C:\\Root\\secret.7z");

            Assert::AreEqual (S_OK, builder.Build (L"C:\\Root\\secret.7z", 99, report));
            Assert::IsTrue   (report.m_statistics.m_fListingEncrypted);
            Assert::AreEqual (0ull, report.m_statistics.m_cFiles);
            Assert::IsFalse  (report.m_verify.has_value());
            Assert::AreEqual (0, accessPtr->m_cVerifies.load());
        }




        TEST_METHOD(Build_Verify_OnlyWithFlag)
        {
            auto                accessPtr = MakeAccess();
            CArchiveFileSystem  fs          (accessPtr);
            CArchiveInfoBuilder builder     (fs, true);
            SArchiveReport      report;
            SVerifyResult       verify;

            verify.m_cPassed = 2;
            accessPtr->SetVerifyResult (L"C:\\Root\\a.zip", S_OK, verify);

            Assert::AreEqual (S_OK, builder.Build (L"C:\\Root\\a.zip", 1, report));
            Assert::IsTrue   (report.m_verify.has_value());
            Assert::AreEqual (2ull, report.m_verify->m_cPassed);
        }




        TEST_METHOD(Build_VerifyFailure_StoredInVerifyResult)
        {
            auto                accessPtr = MakeAccess();
            CArchiveFileSystem  fs          (accessPtr);
            CArchiveInfoBuilder builder     (fs, true);
            SArchiveReport      report;

            accessPtr->SetVerifyResult (L"C:\\Root\\a.zip", HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED), {});
            accessPtr->m_fLibArchiveAvailable = true;

            Assert::AreEqual (S_OK, builder.Build (L"C:\\Root\\a.zip", 1, report));
            Assert::IsTrue   (report.m_verify.has_value());
            Assert::IsTrue   (report.m_verify->m_eCoverage == EChecksumCoverage::Unavailable);
            Assert::IsFalse  (report.m_verify->m_strError.empty());
        }
    };
}
