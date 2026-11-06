#include "pch.h"
#include "EhmTestHelper.h"
#include "Mocks/FileSystemMock.h"
#include "Mocks/MockArchiveFileAccess.h"
#include "Mocks/TestConsole.h"

#include "../TCDirCore/ArchiveFileSystem.h"
#include "../TCDirCore/CommandLine.h"
#include "../TCDirCore/Config.h"
#include "../TCDirCore/Console.h"
#include "../TCDirCore/DriveInfo.h"
#include "../TCDirCore/Flag.h"
#include "../TCDirCore/MultiThreadedLister.h"
#include "../TCDirCore/ResultsDisplayerNormal.h"
#include "../TCDirCore/ResultsDisplayerTree.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    ////////////////////////////////////////////////////////////////////////////
    //
    //  CCapturingResultsDisplayer
    //
    //  Captures the entries of every displayed directory, keyed by the
    //  uppercased directory path.
    //
    ////////////////////////////////////////////////////////////////////////////

    class CCapturingResultsDisplayer : public IResultsDisplayer
    {
    public:
        struct SDirectory
        {
            vector<wstring>     m_vNames;
            vector<DWORD>       m_vAttributes;
            vector<ULONGLONG>   m_vSizes;
            bool                m_fInArchive = false;
        };

        void DisplayResults (const CDriveInfo &, const CDirectoryInfo & di, EDirectoryLevel) override
        {
            lock_guard<mutex>   lock      (m_mutex);
            SDirectory        & directory = m_mapDirectories[Key (di.m_dirPath)];

            directory.m_fInArchive = di.m_fInArchive;

            for (const FileInfo & entry : di.m_vMatches)
            {
                ULARGE_INTEGER uliSize = {};

                uliSize.LowPart  = entry.nFileSizeLow;
                uliSize.HighPart = entry.nFileSizeHigh;

                directory.m_vNames.push_back (entry.cFileName);
                directory.m_vAttributes.push_back (entry.dwFileAttributes);
                directory.m_vSizes.push_back (uliSize.QuadPart);
            }
        }

        void DisplayRecursiveSummary (const CDirectoryInfo &, const SListingTotals & totals) override
        {
            m_totals = totals;
        }

        const SDirectory * Find (const filesystem::path & dirPath) const
        {
            auto it = m_mapDirectories.find (Key (dirPath));

            return (it != m_mapDirectories.end()) ? &it->second : nullptr;
        }

        static DWORD AttributesOf (const SDirectory & directory, LPCWSTR pszName)
        {
            for (size_t i = 0; i < directory.m_vNames.size(); ++i)
            {
                if (directory.m_vNames[i] == pszName)
                {
                    return directory.m_vAttributes[i];
                }
            }

            return INVALID_FILE_ATTRIBUTES;
        }

        static ULONGLONG SizeOf (const SDirectory & directory, LPCWSTR pszName)
        {
            for (size_t i = 0; i < directory.m_vNames.size(); ++i)
            {
                if (directory.m_vNames[i] == pszName)
                {
                    return directory.m_vSizes[i];
                }
            }

            return 0;
        }

        static wstring Key (const filesystem::path & path)
        {
            return CWildcardMatcher::ToUpper (path.lexically_normal().wstring());
        }

        mutex                       m_mutex;
        map<wstring, SDirectory>    m_mapDirectories;
        SListingTotals              m_totals = {};
    };





    ////////////////////////////////////////////////////////////////////////////
    //
    //  CArchiveTestLister
    //
    //  Exposes the single-threaded lister's displayer and totals for tests.
    //
    ////////////////////////////////////////////////////////////////////////////

    class CArchiveTestLister : public CDirectoryLister
    {
    public:
        CArchiveTestLister (shared_ptr<CCommandLine> cmdLinePtr, shared_ptr<CConsole> consolePtr, shared_ptr<CConfig> configPtr) :
            CDirectoryLister (cmdLinePtr, consolePtr, configPtr, make_unique<CCapturingResultsDisplayer>())
        {
        }

        CCapturingResultsDisplayer & Displayer()     { return static_cast<CCapturingResultsDisplayer &> (*m_displayer); }
        const SListingTotals       & Totals() const  { return m_totals; }
    };





    ////////////////////////////////////////////////////////////////////////////
    //
    //  ArchiveListingScenarioTests
    //
    //  Listing scenarios with a mock file system and mock archive contents:
    //
    //    C:\MockRoot\
    //      notes.txt          (10 bytes)
    //      release.zip        (344 bytes on disk)
    //        readme.txt       (3)
    //        docs\guide.md    (4)
    //        docs\api\ref.md  (5)
    //        nested.zip       (7)
    //      sub\
    //        bad.zip          (corrupt)
    //        main.cpp         (20)
    //
    ////////////////////////////////////////////////////////////////////////////

    TEST_CLASS(ArchiveListingScenarioTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        struct SFixture
        {
            MockFileTree                         m_tree;
            shared_ptr<CMockArchiveFileAccess>   m_accessPtr  = make_shared<CMockArchiveFileAccess>();
            shared_ptr<CArchiveFileSystem>       m_archiveFsPtr;
            shared_ptr<CCommandLine>             m_cmdLinePtr = make_shared<CCommandLine>();
            shared_ptr<CCapturingConsole>        m_consolePtr = make_shared<CCapturingConsole>();
            shared_ptr<CConfig>                  m_configPtr  = make_shared<CConfig>();

            SFixture()
            {
                m_tree.AddFile      (L"C:\\MockRoot\\notes.txt",     10);
                m_tree.AddFile      (L"C:\\MockRoot\\release.zip",   344);
                m_tree.AddDirectory (L"C:\\MockRoot\\sub");
                m_tree.AddFile      (L"C:\\MockRoot\\sub\\bad.zip",  50);
                m_tree.AddFile      (L"C:\\MockRoot\\sub\\main.cpp", 20);

                m_accessPtr->AddDirectory     (L"C:\\MockRoot");
                m_accessPtr->AddDirectory     (L"C:\\MockRoot\\sub");
                m_accessPtr->AddArchive       (L"C:\\MockRoot\\release.zip",
                                               { CMockArchiveFileAccess::File (L"readme.txt",      3),
                                                 CMockArchiveFileAccess::File (L"docs/guide.md",   4),
                                                 CMockArchiveFileAccess::File (L"docs/api/ref.md", 5),
                                                 CMockArchiveFileAccess::File (L"nested.zip",      7) });
                m_accessPtr->AddBrokenArchive (L"C:\\MockRoot\\sub\\bad.zip", HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

                m_archiveFsPtr = make_shared<CArchiveFileSystem> (m_accessPtr);
                m_consolePtr->Initialize (m_configPtr);
            }
        };




        static HRESULT RunMultiThreaded (
            SFixture          & fixture,
            IResultsDisplayer & displayer,
            LPCWSTR             pszDir,
            LPCWSTR             pszSpec,
            bool                fArchives,
            SListingTotals    & totals)
        {
            CMultiThreadedLister       lister    (fixture.m_cmdLinePtr, fixture.m_consolePtr, fixture.m_configPtr);
            CDriveInfo                 driveInfo (L"C:\\MockRoot");
            vector<filesystem::path>   fileSpecs = { pszSpec };

            if (fArchives)
            {
                lister.SetArchiveFileSystem (fixture.m_archiveFsPtr);
            }

            return lister.ProcessDirectoryMultiThreaded (driveInfo, pszDir, fileSpecs, displayer,
                                                         IResultsDisplayer::EDirectoryLevel::Initial, totals);
        }




        TEST_METHOD(ArchiveTarget_ListsRootEntries)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock     (fixture.m_tree);
            CArchiveTestLister   lister   (fixture.m_cmdLinePtr, fixture.m_consolePtr, fixture.m_configPtr);

            lister.SetArchiveFileSystem (fixture.m_archiveFsPtr);
            lister.List (MaskGroup (L"C:\\MockRoot\\release.zip", { L"*" }));

            auto pDir = lister.Displayer().Find (L"C:\\MockRoot\\release.zip");

            Assert::IsNotNull (pDir);
            Assert::IsTrue    (pDir->m_fInArchive);
            Assert::AreEqual  (size_t (3), pDir->m_vNames.size());
            Assert::IsTrue    (CFlag::IsSet (CCapturingResultsDisplayer::AttributesOf (*pDir, L"docs"), FILE_ATTRIBUTE_DIRECTORY));
            Assert::IsTrue    (CFlag::IsNotSet (CCapturingResultsDisplayer::AttributesOf (*pDir, L"readme.txt"), FILE_ATTRIBUTE_DIRECTORY));
        }




        TEST_METHOD(AttributeFilter_DirectoriesOnly)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock     (fixture.m_tree);
            CArchiveTestLister   lister   (fixture.m_cmdLinePtr, fixture.m_consolePtr, fixture.m_configPtr);

            fixture.m_cmdLinePtr->m_dwAttributesRequired = FILE_ATTRIBUTE_DIRECTORY;

            lister.SetArchiveFileSystem (fixture.m_archiveFsPtr);
            lister.List (MaskGroup (L"C:\\MockRoot\\release.zip", { L"*" }));

            auto pDir = lister.Displayer().Find (L"C:\\MockRoot\\release.zip");

            Assert::IsNotNull (pDir);
            Assert::AreEqual  (size_t (1), pDir->m_vNames.size());
            Assert::AreEqual  (L"docs", pDir->m_vNames[0].c_str());
        }




        TEST_METHOD(EmptyArchive_ListsNoEntries)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock     (fixture.m_tree);
            CArchiveTestLister   lister   (fixture.m_cmdLinePtr, fixture.m_consolePtr, fixture.m_configPtr);

            fixture.m_accessPtr->AddArchive (L"C:\\MockRoot\\empty.zip", {});

            lister.SetArchiveFileSystem (fixture.m_archiveFsPtr);
            lister.List (MaskGroup (L"C:\\MockRoot\\empty.zip", { L"*" }));

            auto pDir = lister.Displayer().Find (L"C:\\MockRoot\\empty.zip");

            Assert::IsNotNull (pDir);
            Assert::IsTrue    (pDir->m_vNames.empty());
        }




        TEST_METHOD(OwnerAndStreams_ArchiveEntriesListWithoutError)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock     (fixture.m_tree);

            fixture.m_cmdLinePtr->m_fShowOwner   = true;
            fixture.m_cmdLinePtr->m_fShowStreams = true;

            CDirectoryLister lister (fixture.m_cmdLinePtr, fixture.m_consolePtr, fixture.m_configPtr,
                                     make_unique<CResultsDisplayerNormal> (fixture.m_cmdLinePtr, fixture.m_consolePtr, fixture.m_configPtr, false));

            lister.SetArchiveFileSystem (fixture.m_archiveFsPtr);
            lister.List (MaskGroup (L"C:\\MockRoot\\release.zip", { L"*" }));
            fixture.m_consolePtr->Flush();

            Assert::IsTrue (fixture.m_consolePtr->m_strCaptured.find (L"readme.txt") != wstring::npos);
            Assert::IsTrue (fixture.m_consolePtr->m_strCaptured.find (L"Error") == wstring::npos);
        }




        TEST_METHOD(NestedArchive_ListedAsFile)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock     (fixture.m_tree);
            CArchiveTestLister   lister   (fixture.m_cmdLinePtr, fixture.m_consolePtr, fixture.m_configPtr);

            lister.SetArchiveFileSystem (fixture.m_archiveFsPtr);
            lister.List (MaskGroup (L"C:\\MockRoot\\release.zip", { L"*" }));

            auto pDir = lister.Displayer().Find (L"C:\\MockRoot\\release.zip");

            Assert::IsNotNull (pDir);
            Assert::IsTrue    (CFlag::IsNotSet (CCapturingResultsDisplayer::AttributesOf (*pDir, L"nested.zip"), FILE_ATTRIBUTE_DIRECTORY));
        }




        TEST_METHOD(InnerFolderWithMask_ListsMatchesOnly)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock     (fixture.m_tree);
            CArchiveTestLister   lister   (fixture.m_cmdLinePtr, fixture.m_consolePtr, fixture.m_configPtr);

            lister.SetArchiveFileSystem (fixture.m_archiveFsPtr);
            lister.List (MaskGroup (L"C:\\MockRoot\\release.zip\\docs", { L"*.md" }));

            auto pDir = lister.Displayer().Find (L"C:\\MockRoot\\release.zip\\docs");

            Assert::IsNotNull (pDir);
            Assert::AreEqual  (size_t (1), pDir->m_vNames.size());
            Assert::AreEqual  (L"guide.md", pDir->m_vNames[0].c_str());
            Assert::AreEqual  (4ull, pDir->m_vSizes[0]);
        }




        TEST_METHOD(MissingInnerFolder_ReportsDoesNotExist)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock     (fixture.m_tree);
            CArchiveTestLister   lister   (fixture.m_cmdLinePtr, fixture.m_consolePtr, fixture.m_configPtr);

            lister.SetArchiveFileSystem (fixture.m_archiveFsPtr);
            lister.List (MaskGroup (L"C:\\MockRoot\\release.zip\\missing", { L"*" }));
            fixture.m_consolePtr->Flush();

            Assert::IsTrue (fixture.m_consolePtr->m_strCaptured.find (L"does not exist") != wstring::npos);
            Assert::IsNull (lister.Displayer().Find (L"C:\\MockRoot\\release.zip\\missing"));
        }




        TEST_METHOD(WithoutSwitch_InnerPathDoesNotExist)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock     (fixture.m_tree);
            CArchiveTestLister   lister   (fixture.m_cmdLinePtr, fixture.m_consolePtr, fixture.m_configPtr);

            lister.List (MaskGroup (L"C:\\MockRoot\\release.zip\\docs", { L"*" }));
            fixture.m_consolePtr->Flush();

            Assert::IsTrue (fixture.m_consolePtr->m_strCaptured.find (L"does not exist") != wstring::npos);
            Assert::AreEqual (0, fixture.m_accessPtr->m_cReads.load());
        }




        TEST_METHOD(CorruptArchiveTarget_PrintsError)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock     (fixture.m_tree);
            CArchiveTestLister   lister   (fixture.m_cmdLinePtr, fixture.m_consolePtr, fixture.m_configPtr);

            lister.SetArchiveFileSystem (fixture.m_archiveFsPtr);
            lister.List (MaskGroup (L"C:\\MockRoot\\sub\\bad.zip", { L"*" }));
            fixture.m_consolePtr->Flush();

            Assert::IsTrue (fixture.m_consolePtr->m_strCaptured.find (L"is not a valid archive") != wstring::npos);
        }




        TEST_METHOD(DirectoryListing_WithoutSwitch_ArchiveIsFile)
        {
            SFixture                     fixture;
            ScopedFileSystemMock         mock      (fixture.m_tree);
            CCapturingResultsDisplayer   displayer;
            SListingTotals               totals    = {};

            HRESULT hr = RunMultiThreaded (fixture, displayer, L"C:\\MockRoot", L"*", false, totals);

            auto pDir = displayer.Find (L"C:\\MockRoot");

            Assert::IsTrue    (SUCCEEDED (hr));
            Assert::IsNotNull (pDir);
            Assert::IsTrue    (CFlag::IsNotSet (CCapturingResultsDisplayer::AttributesOf (*pDir, L"release.zip"), FILE_ATTRIBUTE_DIRECTORY));
            Assert::AreEqual  (0, fixture.m_accessPtr->m_cReads.load());
        }




        TEST_METHOD(DirectoryListing_WithSwitch_ArchiveIsFile)
        {
            SFixture                     fixture;
            ScopedFileSystemMock         mock      (fixture.m_tree);
            CCapturingResultsDisplayer   displayer;
            SListingTotals               totals    = {};

            HRESULT hr = RunMultiThreaded (fixture, displayer, L"C:\\MockRoot", L"*", true, totals);

            auto pDir = displayer.Find (L"C:\\MockRoot");

            Assert::IsTrue    (SUCCEEDED (hr));
            Assert::IsNotNull (pDir);
            Assert::IsTrue    (CFlag::IsNotSet (CCapturingResultsDisplayer::AttributesOf (*pDir, L"release.zip"), FILE_ATTRIBUTE_DIRECTORY));
            Assert::AreEqual  (344ull, CCapturingResultsDisplayer::SizeOf (*pDir, L"release.zip"));
            Assert::IsFalse   (pDir->m_fInArchive);
            Assert::IsNull    (displayer.Find (L"C:\\MockRoot\\release.zip"));
            Assert::AreEqual  (0, fixture.m_accessPtr->m_cReads.load());
        }





        TEST_METHOD(Recursive_WithSwitch_IncludesArchiveContents)
        {
            SFixture                     fixture;
            ScopedFileSystemMock         mock      (fixture.m_tree);
            CCapturingResultsDisplayer   displayer;
            SListingTotals               totals    = {};

            fixture.m_cmdLinePtr->m_fRecurse = true;

            HRESULT hr = RunMultiThreaded (fixture, displayer, L"C:\\MockRoot", L"*", true, totals);

            auto pRoot = displayer.Find (L"C:\\MockRoot");

            Assert::IsTrue    (SUCCEEDED (hr));
            Assert::IsNotNull (pRoot);
            Assert::IsTrue    (CFlag::IsNotSet (CCapturingResultsDisplayer::AttributesOf (*pRoot, L"release.zip"), FILE_ATTRIBUTE_DIRECTORY));
            Assert::IsNotNull (displayer.Find (L"C:\\MockRoot\\release.zip"));
            Assert::IsNotNull (displayer.Find (L"C:\\MockRoot\\release.zip\\docs\\api"));

            // notes.txt, release.zip, bad.zip, main.cpp, and readme.txt, guide.md, ref.md, nested.zip inside release.zip
            Assert::AreEqual (8u, totals.m_cFiles);
            Assert::AreEqual (10ull + 344 + 50 + 20 + 3 + 4 + 5 + 7, totals.m_uliFileBytes.QuadPart);
        }




        TEST_METHOD(Recursive_WithoutSwitch_SkipsArchiveContents)
        {
            SFixture                     fixture;
            ScopedFileSystemMock         mock      (fixture.m_tree);
            CCapturingResultsDisplayer   displayer;
            SListingTotals               totals    = {};

            fixture.m_cmdLinePtr->m_fRecurse = true;

            HRESULT hr = RunMultiThreaded (fixture, displayer, L"C:\\MockRoot", L"*", false, totals);

            Assert::IsTrue   (SUCCEEDED (hr));
            Assert::IsNull   (displayer.Find (L"C:\\MockRoot\\release.zip"));
            Assert::AreEqual (4u, totals.m_cFiles);
        }




        TEST_METHOD(Recursive_MaskMatchesInsideArchives)
        {
            SFixture                     fixture;
            ScopedFileSystemMock         mock      (fixture.m_tree);
            CCapturingResultsDisplayer   displayer;
            SListingTotals               totals    = {};

            fixture.m_cmdLinePtr->m_fRecurse = true;

            HRESULT hr = RunMultiThreaded (fixture, displayer, L"C:\\MockRoot", L"*.md", true, totals);

            Assert::IsTrue   (SUCCEEDED (hr));
            Assert::AreEqual (2u, totals.m_cFiles);
        }




        TEST_METHOD(Recursive_CorruptArchive_ErrorAndRecursionContinues)
        {
            SFixture                     fixture;
            ScopedFileSystemMock         mock      (fixture.m_tree);
            CCapturingResultsDisplayer   displayer;
            SListingTotals               totals    = {};

            fixture.m_cmdLinePtr->m_fRecurse = true;

            RunMultiThreaded (fixture, displayer, L"C:\\MockRoot", L"*", true, totals);
            fixture.m_consolePtr->Flush();

            Assert::IsTrue    (fixture.m_consolePtr->m_strCaptured.find (L"is not a valid archive") != wstring::npos);
            Assert::IsNotNull (displayer.Find (L"C:\\MockRoot\\release.zip\\docs\\api"));
        }




        TEST_METHOD(RecursiveSingleThreaded_WithSwitch_IncludesArchiveContents)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock     (fixture.m_tree);
            CArchiveTestLister   lister   (fixture.m_cmdLinePtr, fixture.m_consolePtr, fixture.m_configPtr);

            fixture.m_cmdLinePtr->m_fRecurse       = true;
            fixture.m_cmdLinePtr->m_fMultiThreaded = false;

            lister.SetArchiveFileSystem (fixture.m_archiveFsPtr);
            lister.List (MaskGroup (L"C:\\MockRoot\\release.zip", { L"*.md" }));

            Assert::IsNotNull (lister.Displayer().Find (L"C:\\MockRoot\\release.zip\\docs\\api"));
            Assert::AreEqual  (2u, lister.Totals().m_cFiles);
        }




        TEST_METHOD(TreeMode_WithSwitch_DescendsIntoArchive)
        {
            SFixture         fixture;
            ScopedFileSystemMock mock (fixture.m_tree);
            SListingTotals   totals   = {};

            fixture.m_cmdLinePtr->m_fTree = true;

            CResultsDisplayerTree treeDisplayer (fixture.m_cmdLinePtr, fixture.m_consolePtr, fixture.m_configPtr, false);

            HRESULT hr = RunMultiThreaded (fixture, treeDisplayer, L"C:\\MockRoot", L"*", true, totals);
            fixture.m_consolePtr->Flush();

            Assert::IsTrue   (SUCCEEDED (hr));
            Assert::IsTrue   (fixture.m_consolePtr->m_strCaptured.find (L"ref.md") != wstring::npos);
            Assert::AreEqual (8u, totals.m_cFiles);
        }




        TEST_METHOD(TreeMode_MaskMatchesInsideArchive_ArchiveShownAsContainer)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock     (fixture.m_tree);
            SListingTotals       totals   = {};

            fixture.m_cmdLinePtr->m_fTree = true;

            CResultsDisplayerTree treeDisplayer (fixture.m_cmdLinePtr, fixture.m_consolePtr, fixture.m_configPtr, false);

            HRESULT hr = RunMultiThreaded (fixture, treeDisplayer, L"C:\\MockRoot", L"*.md", true, totals);
            fixture.m_consolePtr->Flush();

            const wstring & strOutput = fixture.m_consolePtr->m_strCaptured;

            Assert::IsTrue   (SUCCEEDED (hr));
            Assert::IsTrue   (strOutput.find (L"release.zip") != wstring::npos);
            Assert::IsTrue   (strOutput.find (L"guide.md")    != wstring::npos);
            Assert::IsTrue   (strOutput.find (L"notes.txt")   == wstring::npos);

            // guide.md and ref.md; release.zip is a container only
            Assert::AreEqual (2u, totals.m_cFiles);
        }




        TEST_METHOD(TreeMode_NoMatchesInsideArchive_ArchivePruned)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock     (fixture.m_tree);
            SListingTotals       totals   = {};

            fixture.m_cmdLinePtr->m_fTree = true;

            CResultsDisplayerTree treeDisplayer (fixture.m_cmdLinePtr, fixture.m_consolePtr, fixture.m_configPtr, false);

            HRESULT hr = RunMultiThreaded (fixture, treeDisplayer, L"C:\\MockRoot", L"*.cpp", true, totals);
            fixture.m_consolePtr->Flush();

            const wstring & strOutput = fixture.m_consolePtr->m_strCaptured;

            Assert::IsTrue   (SUCCEEDED (hr));
            Assert::IsTrue   (strOutput.find (L"main.cpp")    != wstring::npos);
            Assert::IsTrue   (strOutput.find (L"release.zip") == wstring::npos);
            Assert::AreEqual (1u, totals.m_cFiles);
        }




        TEST_METHOD(TreeMode_ArchiveMatchesMask_ShownAndCounted)
        {
            SFixture             fixture;
            ScopedFileSystemMock mock     (fixture.m_tree);
            SListingTotals       totals   = {};

            fixture.m_cmdLinePtr->m_fTree = true;

            CResultsDisplayerTree treeDisplayer (fixture.m_cmdLinePtr, fixture.m_consolePtr, fixture.m_configPtr, false);

            HRESULT hr = RunMultiThreaded (fixture, treeDisplayer, L"C:\\MockRoot", L"*.zip", true, totals);
            fixture.m_consolePtr->Flush();

            const wstring & strOutput = fixture.m_consolePtr->m_strCaptured;

            Assert::IsTrue   (SUCCEEDED (hr));
            Assert::IsTrue   (strOutput.find (L"release.zip") != wstring::npos);
            Assert::IsTrue   (strOutput.find (L"nested.zip")  != wstring::npos);

            // release.zip, bad.zip and nested.zip inside release.zip
            Assert::AreEqual (3u, totals.m_cFiles);
        }




        TEST_METHOD(TreeMode_DepthLimit_StopsAtArchive)
        {
            SFixture         fixture;
            ScopedFileSystemMock mock (fixture.m_tree);
            SListingTotals   totals   = {};

            fixture.m_cmdLinePtr->m_fTree     = true;
            fixture.m_cmdLinePtr->m_cMaxDepth = 1;

            CResultsDisplayerTree treeDisplayer (fixture.m_cmdLinePtr, fixture.m_consolePtr, fixture.m_configPtr, false);

            HRESULT hr = RunMultiThreaded (fixture, treeDisplayer, L"C:\\MockRoot", L"*", true, totals);
            fixture.m_consolePtr->Flush();

            Assert::IsTrue (SUCCEEDED (hr));
            Assert::IsTrue (fixture.m_consolePtr->m_strCaptured.find (L"release.zip") != wstring::npos);
            Assert::IsTrue (fixture.m_consolePtr->m_strCaptured.find (L"readme.txt")  == wstring::npos);
        }
    };
}
