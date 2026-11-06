#include "pch.h"
#include "EhmTestHelper.h"
#include "Mocks/FileSystemMock.h"
#include "Mocks/MockArchiveFileAccess.h"

#include "../TCDirCore/ArchiveFileSystem.h"
#include "../TCDirCore/ArchiveInfoFinder.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(ArchiveInfoFinderTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        //
        //  C:\Root
        //      a.zip            contains docs\inner.7z and pkg.zip (pkg.zip contains deep.7z)
        //      b.7z
        //      notes.txt
        //      sub\c.zip
        //      sub\bad.zip      unreadable
        //

        struct SFixture
        {
            MockFileTree                         m_tree;
            shared_ptr<CMockArchiveFileAccess>   m_accessPtr = make_shared<CMockArchiveFileAccess>();
            shared_ptr<CArchiveFileSystem>       m_archiveFsPtr;
            vector<wstring>                      m_vReported;
            vector<ULONGLONG>                    m_vSizes;
            vector<wstring>                      m_vErrors;

            SFixture()
            {
                m_tree.AddDirectory (L"C:\\Root");
                m_tree.AddFile      (L"C:\\Root\\a.zip",       300);
                m_tree.AddFile      (L"C:\\Root\\b.7z",        200);
                m_tree.AddFile      (L"C:\\Root\\notes.txt",   10);
                m_tree.AddDirectory (L"C:\\Root\\sub");
                m_tree.AddFile      (L"C:\\Root\\sub\\c.zip",  100);
                m_tree.AddFile      (L"C:\\Root\\sub\\bad.zip", 50);

                m_accessPtr->AddDirectory (L"C:\\Root");
                m_accessPtr->AddDirectory (L"C:\\Root\\sub");
                m_accessPtr->AddFile      (L"C:\\Root\\notes.txt");
                m_accessPtr->AddArchive   (L"C:\\Root\\a.zip", {
                    CMockArchiveFileAccess::File (L"readme.txt",     1),
                    CMockArchiveFileAccess::File (L"docs/inner.7z",  20),
                    CMockArchiveFileAccess::File (L"pkg.zip",        30),
                });
                m_accessPtr->AddArchive   (L"C:\\Root\\a.zip\\docs\\inner.7z", { CMockArchiveFileAccess::File (L"x.txt", 1) });
                m_accessPtr->AddArchive   (L"C:\\Root\\a.zip\\pkg.zip",        { CMockArchiveFileAccess::File (L"deep.7z", 5) });
                m_accessPtr->AddArchive   (L"C:\\Root\\a.zip\\pkg.zip\\deep.7z", { CMockArchiveFileAccess::File (L"y.txt", 1) });
                m_accessPtr->AddArchive   (L"C:\\Root\\b.7z",      { CMockArchiveFileAccess::File (L"z.txt", 1) });
                m_accessPtr->AddArchive   (L"C:\\Root\\sub\\c.zip", { CMockArchiveFileAccess::File (L"w.txt", 1) });
                m_accessPtr->AddBrokenArchive (L"C:\\Root\\sub\\bad.zip", HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

                m_archiveFsPtr = make_shared<CArchiveFileSystem> (m_accessPtr);
            }

            bool Find (const wstring & strTarget, bool fRecurse)
            {
                CArchiveInfoFinder finder (m_archiveFsPtr,
                                           fRecurse,
                                           [this] (const filesystem::path & path, ULONGLONG ullSize) { m_vReported.push_back (path.wstring()); m_vSizes.push_back (ullSize); },
                                           [this] (const filesystem::path & path, const wstring &)   { m_vErrors.push_back (path.wstring()); });

                return finder.Find (strTarget);
            }
        };

        static void AssertReported (const vector<wstring> & wanted, const vector<wstring> & actual)
        {
            Assert::AreEqual (wanted.size(), actual.size());

            for (size_t i = 0; i < wanted.size(); ++i)
            {
                Assert::AreEqual (wanted[i].c_str(), actual[i].c_str());
            }
        }




        TEST_METHOD(Find_ArchivePath_ReportsIt)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock (fixture.m_tree);

            Assert::IsTrue   (fixture.Find (L"C:\\Root\\a.zip", false));
            AssertReported   ({ L"C:\\Root\\a.zip" }, fixture.m_vReported);
            Assert::AreEqual (300ull, fixture.m_vSizes[0]);
        }




        TEST_METHOD(Find_Mask_SkipsNonArchivesInNameOrder)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock (fixture.m_tree);

            Assert::IsTrue (fixture.Find (L"C:\\Root\\*", false));
            AssertReported ({ L"C:\\Root\\a.zip", L"C:\\Root\\b.7z" }, fixture.m_vReported);
        }




        TEST_METHOD(Find_Folder_SearchedAsFolderStar)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock (fixture.m_tree);

            Assert::IsTrue (fixture.Find (L"C:\\Root\\sub", false));
            AssertReported ({ L"C:\\Root\\sub\\bad.zip", L"C:\\Root\\sub\\c.zip" }, fixture.m_vReported);
        }




        TEST_METHOD(Find_NoArchives_ReturnsFalse)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock (fixture.m_tree);

            Assert::IsFalse (fixture.Find (L"C:\\Root\\*.txt", false));
            Assert::IsFalse (fixture.Find (L"C:\\Root\\notes.txt", false));
            Assert::IsFalse (fixture.Find (L"C:\\Root\\missing.zip", false));
            Assert::IsTrue  (fixture.m_vReported.empty());
        }




        TEST_METHOD(Find_NestedArchivePath_ReportsIt)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock (fixture.m_tree);

            Assert::IsTrue   (fixture.Find (L"C:\\Root\\a.zip\\docs\\inner.7z", false));
            AssertReported   ({ L"C:\\Root\\a.zip\\docs\\inner.7z" }, fixture.m_vReported);
            Assert::AreEqual (20ull, fixture.m_vSizes[0]);
        }




        TEST_METHOD(Find_MaskInsideArchive)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock (fixture.m_tree);

            Assert::IsTrue (fixture.Find (L"C:\\Root\\a.zip\\*.zip", false));
            AssertReported ({ L"C:\\Root\\a.zip\\pkg.zip" }, fixture.m_vReported);
        }




        TEST_METHOD(Find_WithoutRecurse_NestedArchivesNotReported)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock (fixture.m_tree);

            Assert::IsTrue (fixture.Find (L"C:\\Root\\*.zip", false));
            AssertReported ({ L"C:\\Root\\a.zip" }, fixture.m_vReported);
        }




        TEST_METHOD(Find_Recurse_ArchivesNestedAfterParentThenSubfolders)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock (fixture.m_tree);

            Assert::IsTrue (fixture.Find (L"C:\\Root\\*.zip", true));
            AssertReported ({ L"C:\\Root\\a.zip",
                              L"C:\\Root\\a.zip\\pkg.zip",
                              L"C:\\Root\\sub\\bad.zip",
                              L"C:\\Root\\sub\\c.zip" }, fixture.m_vReported);
        }




        TEST_METHOD(Find_Recurse_NonMatchingArchivesSearched)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock (fixture.m_tree);

            Assert::IsTrue (fixture.Find (L"C:\\Root\\*.7z", true));
            AssertReported ({ L"C:\\Root\\a.zip\\pkg.zip\\deep.7z",
                              L"C:\\Root\\a.zip\\docs\\inner.7z",
                              L"C:\\Root\\b.7z" }, fixture.m_vReported);
            AssertReported ({ L"C:\\Root\\sub\\bad.zip" }, fixture.m_vErrors);
        }




        TEST_METHOD(Find_Recurse_ExplicitArchiveReportsAllNested)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock (fixture.m_tree);

            Assert::IsTrue (fixture.Find (L"C:\\Root\\a.zip", true));
            AssertReported ({ L"C:\\Root\\a.zip",
                              L"C:\\Root\\a.zip\\pkg.zip",
                              L"C:\\Root\\a.zip\\pkg.zip\\deep.7z",
                              L"C:\\Root\\a.zip\\docs\\inner.7z" }, fixture.m_vReported);
        }




        TEST_METHOD(Find_Recurse_ReportedBrokenArchiveHasNoSeparateError)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock (fixture.m_tree);

            Assert::IsTrue (fixture.Find (L"C:\\Root\\sub\\*.zip", true));
            AssertReported ({ L"C:\\Root\\sub\\bad.zip", L"C:\\Root\\sub\\c.zip" }, fixture.m_vReported);
            Assert::IsTrue (fixture.m_vErrors.empty());
        }
    };
}
