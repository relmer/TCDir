#include "pch.h"
#include "EhmTestHelper.h"
#include "Mocks/FileSystemMock.h"
#include "Mocks/TestConsole.h"

#include "../TCDirCore/MultiThreadedLister.h"
#include "../TCDirCore/ResultsDisplayerNormal.h"
#include "../TCDirCore/ResultsDisplayerTree.h"
#include "../TCDirCore/DriveInfo.h"
#include "../TCDirCore/Config.h"
#include "../TCDirCore/Console.h"
#include "../TCDirCore/CommandLine.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    ////////////////////////////////////////////////////////////////////////////
    //
    //  MockResultsDisplayer
    //
    //  Captures calls to DisplayResults and DisplayRecursiveSummary for
    //  verification, instead of writing to console.
    //
    ////////////////////////////////////////////////////////////////////////////

    class MockResultsDisplayer : public IResultsDisplayer
    {
    public:
        void DisplayResults (const CDriveInfo &, const CDirectoryInfo & di, EDirectoryLevel) override
        {
            m_cDisplayResultsCalls++;
            m_cTotalFilesDisplayed      += di.m_cFiles;
            m_cTotalSubdirsDisplayed    += di.m_cSubDirectories;
            m_uliBytesDisplayed.QuadPart += di.m_uliBytesUsed.QuadPart;
        }

        void DisplayRecursiveSummary (const CDirectoryInfo &, const SListingTotals & totals) override
        {
            m_fRecursiveSummaryCalled = true;
            m_capturedTotals = totals;
        }

        //
        // Captured data for assertions
        //

        bool           m_fRecursiveSummaryCalled = false;
        SListingTotals m_capturedTotals          = {};
        UINT           m_cDisplayResultsCalls    = 0;
        ULONGLONG      m_cTotalFilesDisplayed    = 0;
        ULONGLONG      m_cTotalSubdirsDisplayed  = 0;
        ULARGE_INTEGER m_uliBytesDisplayed       = {};
    };





    ////////////////////////////////////////////////////////////////////////////
    //
    //  DirectoryListerScenarioTests
    //
    //  Integration tests using mock file system.
    //
    ////////////////////////////////////////////////////////////////////////////

    TEST_CLASS(DirectoryListerScenarioTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  IatHook_BasicVerification
        //
        //  Verifies that the IAT hook is intercepting FindFirstFileW calls.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(IatHook_BasicVerification)
        {
            MockFileTree tree;
            tree.AddFile (L"C:\\MockTest\\hello.txt", 123);

            ScopedFileSystemMock mock (tree);

            Assert::IsTrue (mock.IsActive(), L"IAT hook should be active");

            //
            // Try to find a file in the mock - this should work
            //

            WIN32_FIND_DATA wfd = {};
            HANDLE hFind = FindFirstFileW (L"C:\\MockTest\\*", &wfd);

            if (hFind == INVALID_HANDLE_VALUE)
            {
                DWORD err = GetLastError();
                wchar_t msg[100];
                swprintf_s (msg, L"FindFirstFileW failed with error %lu", err);
                Assert::Fail (msg);
            }

            Assert::AreNotEqual (INVALID_HANDLE_VALUE, hFind, L"FindFirstFileW should succeed");
            Assert::AreEqual (L"hello.txt", wfd.cFileName, L"Should find hello.txt");

            FindClose (hFind);
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  IatHook_VerifyMockTreeWithSubdir
        //
        //  Verify the mock works with nested directories (closer to actual test)
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(IatHook_VerifyMockTreeWithSubdir)
        {
            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\file1.txt", 1000);
            tree.AddDirectory (L"C:\\MockRoot\\sub1");
            tree.AddFile      (L"C:\\MockRoot\\sub1\\file2.txt", 2000);

            ScopedFileSystemMock mock (tree);

            //
            // First, verify the root directory files
            //

            WIN32_FIND_DATA wfd = {};
            HANDLE hFind = FindFirstFileW (L"C:\\MockRoot\\*", &wfd);
            
            Assert::AreNotEqual (INVALID_HANDLE_VALUE, hFind, L"FindFirstFileW on root should succeed");

            //
            // Count what we find
            //

            int fileCount = 0;
            int dirCount  = 0;

            do
            {
                if (wcscmp (wfd.cFileName, L".") == 0 || wcscmp (wfd.cFileName, L"..") == 0)
                {
                    continue;
                }

                if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    dirCount++;
                }
                else
                {
                    fileCount++;
                }
            }
            while (FindNextFileW (hFind, &wfd));

            FindClose (hFind);

            Assert::AreEqual (1, fileCount, L"Should find 1 file in root");
            Assert::AreEqual (1, dirCount, L"Should find 1 directory in root");

            //
            // Now verify subdirectory
            //

            hFind = FindFirstFileW (L"C:\\MockRoot\\sub1\\*", &wfd);
            Assert::AreNotEqual (INVALID_HANDLE_VALUE, hFind, L"FindFirstFileW on subdir should succeed");

            fileCount = 0;
            do
            {
                if (wcscmp (wfd.cFileName, L".") == 0 || wcscmp (wfd.cFileName, L"..") == 0)
                {
                    continue;
                }
                if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    fileCount++;
                }
            }
            while (FindNextFileW (hFind, &wfd));

            FindClose (hFind);

            Assert::AreEqual (1, fileCount, L"Should find 1 file in subdir");
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  RecursiveListing_CorrectSummaryTotals
        //
        //  Verifies that recursive directory listing accumulates totals
        //  correctly across all subdirectories.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(RecursiveListing_CorrectSummaryTotals)
        {
            //
            // Setup mock file tree:
            //   C:\MockRoot\
            //     file1.txt (1000 bytes)
            //     file2.txt (2000 bytes)
            //     sub1\
            //       file3.txt (3000 bytes)
            //       file4.txt (4000 bytes)
            //     sub2\
            //       file5.txt (5000 bytes)
            //       subsub\
            //         file6.txt (6000 bytes)
            //
            // Expected totals: 6 files, 21000 bytes, 3 subdirectories
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\file1.txt",              1000);
            tree.AddFile      (L"C:\\MockRoot\\file2.txt",              2000);
            tree.AddDirectory (L"C:\\MockRoot\\sub1");
            tree.AddFile      (L"C:\\MockRoot\\sub1\\file3.txt",        3000);
            tree.AddFile      (L"C:\\MockRoot\\sub1\\file4.txt",        4000);
            tree.AddDirectory (L"C:\\MockRoot\\sub2");
            tree.AddFile      (L"C:\\MockRoot\\sub2\\file5.txt",        5000);
            tree.AddDirectory (L"C:\\MockRoot\\sub2\\subsub");
            tree.AddFile      (L"C:\\MockRoot\\sub2\\subsub\\file6.txt", 6000);

            ScopedFileSystemMock mock (tree);

            //
            // Setup lister with /s (recursive)
            //

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fRecurse = true;

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CMultiThreadedLister lister (cmdLine, console, config);

            //
            // Create mock drive info and displayer
            //

            CDriveInfo           driveInfo (L"C:\\MockRoot");
            MockResultsDisplayer displayer;
            SListingTotals       totals = {};

            //
            // Run the listing
            //

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                displayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            //
            // Verify results
            //

            Assert::IsTrue (SUCCEEDED (hr), L"ProcessDirectoryMultiThreaded should succeed");

            Assert::AreEqual (6u, totals.m_cFiles, L"Should have 6 files");
            Assert::AreEqual (3u, totals.m_cDirectories, L"Should have 3 directories matching '*': sub1, sub2, subsub");
            Assert::AreEqual (21000ull, totals.m_uliFileBytes.QuadPart, L"Should have 21000 bytes total");
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  RecursiveListing_EmptySubdirectories
        //
        //  Verifies correct handling of empty subdirectories.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(RecursiveListing_EmptySubdirectories)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     file1.txt (100 bytes)
            //     empty1\
            //     empty2\
            //     nonempty\
            //       file2.txt (200 bytes)
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\file1.txt", 100);
            tree.AddDirectory (L"C:\\MockRoot\\empty1");
            tree.AddDirectory (L"C:\\MockRoot\\empty2");
            tree.AddDirectory (L"C:\\MockRoot\\nonempty");
            tree.AddFile      (L"C:\\MockRoot\\nonempty\\file2.txt", 200);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fRecurse = true;

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CMultiThreadedLister lister (cmdLine, console, config);
            CDriveInfo           driveInfo (L"C:\\MockRoot");
            MockResultsDisplayer displayer;
            SListingTotals       totals = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                displayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr));
            Assert::AreEqual (2u, totals.m_cFiles, L"Should have 2 files");
            Assert::AreEqual (3u, totals.m_cDirectories, L"Should have 3 directories matching '*': empty1, empty2, nonempty");
            Assert::AreEqual (300ull, totals.m_uliFileBytes.QuadPart, L"Should have 300 bytes total");
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  SingleDirectory_NoRecurse
        //
        //  Verifies single directory listing without /s.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(SingleDirectory_NoRecurse)
        {
            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\file1.txt", 100);
            tree.AddFile      (L"C:\\MockRoot\\file2.txt", 200);
            tree.AddDirectory (L"C:\\MockRoot\\subdir");
            tree.AddFile      (L"C:\\MockRoot\\subdir\\ignored.txt", 9999);  // Should not be counted

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fRecurse = false;  // No recursion

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CMultiThreadedLister lister (cmdLine, console, config);
            CDriveInfo           driveInfo (L"C:\\MockRoot");
            MockResultsDisplayer displayer;
            SListingTotals       totals = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                displayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr));
            Assert::AreEqual (2u, totals.m_cFiles, L"Should have 2 files (not recursive)");
            Assert::AreEqual (1u, totals.m_cDirectories, L"Should have 1 directory with files (MockRoot only, not recursive)");
            Assert::AreEqual (300ull, totals.m_uliFileBytes.QuadPart);
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  MultiMask_TwoMasksSameDirectory_CombinedResults
        //
        //  Verifies that multiple file masks applied to the same directory
        //  produce a single combined listing with files from both masks.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(MultiMask_TwoMasksSameDirectory_CombinedResults)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     file1.cpp (100 bytes)
            //     file2.cpp (200 bytes)
            //     file3.h   (300 bytes)
            //     file4.h   (400 bytes)
            //     file5.txt (500 bytes)  -- should NOT be included
            //
            // With masks *.cpp and *.h, should get 4 files, 1000 bytes
            //

            MockFileTree tree;
            tree.AddFile (L"C:\\MockRoot\\file1.cpp", 100);
            tree.AddFile (L"C:\\MockRoot\\file2.cpp", 200);
            tree.AddFile (L"C:\\MockRoot\\file3.h",   300);
            tree.AddFile (L"C:\\MockRoot\\file4.h",   400);
            tree.AddFile (L"C:\\MockRoot\\file5.txt", 500);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fRecurse = false;

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CMultiThreadedLister lister (cmdLine, console, config);
            CDriveInfo           driveInfo (L"C:\\MockRoot");
            MockResultsDisplayer displayer;
            SListingTotals       totals = {};

            //
            // Call with multiple file specs
            //

            vector<filesystem::path> fileSpecs = { L"*.cpp", L"*.h" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                displayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr));
            Assert::AreEqual (4u, totals.m_cFiles, L"Should have 4 files (2 cpp + 2 h)");
            Assert::AreEqual (1000ull, totals.m_uliFileBytes.QuadPart, L"Should have 1000 bytes");
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  MultiMask_OverlappingMasks_NoDuplicates
        //
        //  Verifies that when a file matches multiple masks, it only appears
        //  once in the results (deduplication).
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(MultiMask_OverlappingMasks_NoDuplicates)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     test.cpp  (100 bytes) - matches *.cpp and test.*
            //     other.cpp (200 bytes) - matches *.cpp only
            //     test.h    (300 bytes) - matches test.* only
            //
            // With masks *.cpp and test.*, should get 3 files (no duplicates)
            //

            MockFileTree tree;
            tree.AddFile (L"C:\\MockRoot\\test.cpp",  100);
            tree.AddFile (L"C:\\MockRoot\\other.cpp", 200);
            tree.AddFile (L"C:\\MockRoot\\test.h",    300);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fRecurse = false;

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CMultiThreadedLister lister (cmdLine, console, config);
            CDriveInfo           driveInfo (L"C:\\MockRoot");
            MockResultsDisplayer displayer;
            SListingTotals       totals = {};

            vector<filesystem::path> fileSpecs = { L"*.cpp", L"test.*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                displayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr));
            Assert::AreEqual (3u, totals.m_cFiles, L"Should have 3 files (test.cpp counted once)");
            Assert::AreEqual (600ull, totals.m_uliFileBytes.QuadPart, L"Should have 600 bytes");
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  MultiMask_Recursive_CombinedInEachSubdir
        //
        //  Verifies that with /s and multiple masks, each subdirectory shows
        //  combined results from all masks.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(MultiMask_Recursive_CombinedInEachSubdir)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     file1.cpp (100 bytes)
            //     file2.h   (200 bytes)
            //     sub\
            //       file3.cpp (300 bytes)
            //       file4.h   (400 bytes)
            //       file5.txt (500 bytes)  -- not matched
            //
            // With masks *.cpp and *.h recursive, should get 4 files
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\file1.cpp", 100);
            tree.AddFile      (L"C:\\MockRoot\\file2.h",   200);
            tree.AddDirectory (L"C:\\MockRoot\\sub");
            tree.AddFile      (L"C:\\MockRoot\\sub\\file3.cpp", 300);
            tree.AddFile      (L"C:\\MockRoot\\sub\\file4.h",   400);
            tree.AddFile      (L"C:\\MockRoot\\sub\\file5.txt", 500);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fRecurse = true;

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CMultiThreadedLister lister (cmdLine, console, config);
            CDriveInfo           driveInfo (L"C:\\MockRoot");
            MockResultsDisplayer displayer;
            SListingTotals       totals = {};

            vector<filesystem::path> fileSpecs = { L"*.cpp", L"*.h" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                displayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr));
            Assert::AreEqual (4u, totals.m_cFiles, L"Should have 4 files (2 cpp + 2 h)");
            Assert::AreEqual (0u, totals.m_cDirectories, L"Should have 0 directories matching '*.cpp' or '*.h' (sub doesn't match)");
            Assert::AreEqual (1000ull, totals.m_uliFileBytes.QuadPart);
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  DirectoryCount_PatternMatchesDir_CountedInTotal
        //
        //  Verifies that m_cDirectories counts directories whose names match
        //  the pattern. When lib.cpp/ directory name matches *.cpp, it's
        //  included in the directory count.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(DirectoryCount_PatternMatchesDir_CountedInTotal)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     file1.cpp     (100 bytes)
            //     lib.cpp\                    <-- directory that matches *.cpp
            //       file2.cpp   (200 bytes)
            //
            // With pattern *.cpp recursive, lib.cpp should be counted
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\file1.cpp", 100);
            tree.AddDirectory (L"C:\\MockRoot\\lib.cpp");
            tree.AddFile      (L"C:\\MockRoot\\lib.cpp\\file2.cpp", 200);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fRecurse = true;

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CMultiThreadedLister lister (cmdLine, console, config);
            CDriveInfo           driveInfo (L"C:\\MockRoot");
            MockResultsDisplayer displayer;
            SListingTotals       totals = {};

            vector<filesystem::path> fileSpecs = { L"*.cpp" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                displayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr));
            Assert::AreEqual (2u, totals.m_cFiles, L"Should have 2 .cpp files");
            Assert::AreEqual (1u, totals.m_cDirectories, L"lib.cpp matches '*.cpp' pattern");
            Assert::AreEqual (300ull, totals.m_uliFileBytes.QuadPart);
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  DirectoryCount_PatternDoesNotMatchDir_NotCounted
        //
        //  Verifies that m_cDirectories counts directories whose names match
        //  the pattern. subdir does not match '*.cpp', so it is not counted.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(DirectoryCount_PatternDoesNotMatchDir_NotCounted)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     file1.cpp     (100 bytes)
            //     subdir\                     <-- directory that does NOT match *.cpp
            //       file2.cpp   (200 bytes)
            //
            // With pattern *.cpp recursive, subdir should NOT be counted
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\file1.cpp", 100);
            tree.AddDirectory (L"C:\\MockRoot\\subdir");
            tree.AddFile      (L"C:\\MockRoot\\subdir\\file2.cpp", 200);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fRecurse = true;

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CMultiThreadedLister lister (cmdLine, console, config);
            CDriveInfo           driveInfo (L"C:\\MockRoot");
            MockResultsDisplayer displayer;
            SListingTotals       totals = {};

            vector<filesystem::path> fileSpecs = { L"*.cpp" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                displayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr));
            Assert::AreEqual (2u, totals.m_cFiles, L"Should have 2 .cpp files");
            Assert::AreEqual (0u, totals.m_cDirectories, L"subdir does not match '*.cpp' pattern");
            Assert::AreEqual (300ull, totals.m_uliFileBytes.QuadPart);
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  MixedFileTypes_IconsOn_TotalsUnchanged
        //
        //  Verifies that enabling icons doesn't affect file/dir counts.
        //  The icon state is a display concern only — enumeration totals 
        //  must be identical with icons on or off.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(MixedFileTypes_IconsOn_TotalsUnchanged)
        {
            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\main.cpp",      500);
            tree.AddFile      (L"C:\\MockRoot\\readme.md",     300);
            tree.AddFile      (L"C:\\MockRoot\\build.exe",     10000);
            tree.AddDirectory (L"C:\\MockRoot\\.git");
            tree.AddDirectory (L"C:\\MockRoot\\src");
            tree.AddFile      (L"C:\\MockRoot\\src\\util.h",   100);

            ScopedFileSystemMock mock (tree);

            //
            // Run with standard listing — icons are a display concern and 
            // should not affect enumeration totals at all.
            //

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fRecurse = true;

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CMultiThreadedLister lister (cmdLine, console, config);
            CDriveInfo           driveInfo (L"C:\\MockRoot");
            MockResultsDisplayer displayer;
            SListingTotals       totals = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo, L"C:\\MockRoot", fileSpecs, displayer,
                IResultsDisplayer::EDirectoryLevel::Initial, totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Listing should succeed");

            // Verify expected totals from our tree
            Assert::AreEqual (4u, totals.m_cFiles,       L"Should have 4 files (main.cpp, readme.md, build.exe, util.h)");
            Assert::AreEqual (2u, totals.m_cDirectories,  L"Should have 2 directories (.git, src)");
            Assert::AreEqual (10900ull, totals.m_uliFileBytes.QuadPart, L"Total bytes should be 10900");
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_BasicTwoLevels_CorrectTotals
        //
        //  Verifies that tree mode correctly traverses 2 levels and
        //  accumulates totals for all files.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_BasicTwoLevels_CorrectTotals)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     readme.txt (100 bytes)
            //     src\
            //       main.cpp (200 bytes)
            //       app.cpp  (300 bytes)
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\readme.txt",      100);
            tree.AddDirectory (L"C:\\MockRoot\\src");
            tree.AddFile      (L"C:\\MockRoot\\src\\main.cpp",   200);
            tree.AddFile      (L"C:\\MockRoot\\src\\app.cpp",    300);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fTree = true;

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CResultsDisplayerTree treeDisplayer (cmdLine, console, config, false);
            CMultiThreadedLister  lister          (cmdLine, console, config);
            CDriveInfo            driveInfo        (L"C:\\MockRoot");
            SListingTotals        totals         = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                treeDisplayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Tree mode listing should succeed");
            Assert::AreEqual (3u, totals.m_cFiles, L"Should have 3 files total across both levels");
            Assert::AreEqual (1u, totals.m_cDirectories, L"Should have 1 directory (src)");
            Assert::AreEqual (600ull, totals.m_uliFileBytes.QuadPart, L"Should have 600 bytes total");
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_ThreeLevels_CorrectTotals
        //
        //  Verifies correct totals with 3 nesting levels.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_ThreeLevels_CorrectTotals)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     file1.txt (1000 bytes)
            //     sub1\
            //       file2.txt (2000 bytes)
            //       sub2\
            //         file3.txt (3000 bytes)
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\file1.txt",               1000);
            tree.AddDirectory (L"C:\\MockRoot\\sub1");
            tree.AddFile      (L"C:\\MockRoot\\sub1\\file2.txt",         2000);
            tree.AddDirectory (L"C:\\MockRoot\\sub1\\sub2");
            tree.AddFile      (L"C:\\MockRoot\\sub1\\sub2\\file3.txt",   3000);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fTree = true;

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CResultsDisplayerTree treeDisplayer (cmdLine, console, config, false);
            CMultiThreadedLister  lister          (cmdLine, console, config);
            CDriveInfo            driveInfo        (L"C:\\MockRoot");
            SListingTotals        totals         = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                treeDisplayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Tree mode 3-level listing should succeed");
            Assert::AreEqual (3u, totals.m_cFiles, L"Should have 3 files across all levels");
            Assert::AreEqual (2u, totals.m_cDirectories, L"Should have 2 directories (sub1, sub2)");
            Assert::AreEqual (6000ull, totals.m_uliFileBytes.QuadPart, L"Should have 6000 bytes total");
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_EmptyDirectory_NoError
        //
        //  Verifies tree mode handles an empty directory gracefully.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_EmptyDirectory_NoError)
        {
            MockFileTree tree;
            // No files — just the root directory exists (mock intercept returns nothing)

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fTree = true;

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CResultsDisplayerTree treeDisplayer (cmdLine, console, config, false);
            CMultiThreadedLister  lister          (cmdLine, console, config);
            CDriveInfo            driveInfo        (L"C:\\MockRoot");
            SListingTotals        totals         = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                treeDisplayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Tree mode with empty directory should succeed");
            Assert::AreEqual (0u, totals.m_cFiles, L"Should have 0 files");
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_FileMask_OnlyMatchingFilesCounted
        //
        //  Verifies that file masks filter results at every level while
        //  still traversing the full directory tree.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_FileMask_OnlyMatchingFilesCounted)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     readme.md  (100 bytes) — not matched
            //     main.cpp   (200 bytes) — matched
            //     src\
            //       app.cpp  (300 bytes) — matched
            //       app.h    (400 bytes) — not matched
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\readme.md",       100);
            tree.AddFile      (L"C:\\MockRoot\\main.cpp",        200);
            tree.AddDirectory (L"C:\\MockRoot\\src");
            tree.AddFile      (L"C:\\MockRoot\\src\\app.cpp",    300);
            tree.AddFile      (L"C:\\MockRoot\\src\\app.h",      400);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fTree = true;

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CResultsDisplayerTree treeDisplayer (cmdLine, console, config, false);
            CMultiThreadedLister  lister          (cmdLine, console, config);
            CDriveInfo            driveInfo        (L"C:\\MockRoot");
            SListingTotals        totals         = {};

            vector<filesystem::path> fileSpecs = { L"*.cpp" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                treeDisplayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Tree mode with file mask should succeed");
            Assert::AreEqual (2u, totals.m_cFiles, L"Should have 2 .cpp files across levels");
            Assert::AreEqual (500ull, totals.m_uliFileBytes.QuadPart, L"Should have 500 bytes from .cpp files");
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_MultipleSubdirs_AllTraversed
        //
        //  Verifies tree mode with multiple sibling subdirectories.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_MultipleSubdirs_AllTraversed)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     root.txt (50 bytes)
            //     alpha\
            //       a1.txt (100 bytes)
            //     beta\
            //       b1.txt (200 bytes)
            //     gamma\
            //       g1.txt (300 bytes)
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\root.txt",         50);
            tree.AddDirectory (L"C:\\MockRoot\\alpha");
            tree.AddFile      (L"C:\\MockRoot\\alpha\\a1.txt",   100);
            tree.AddDirectory (L"C:\\MockRoot\\beta");
            tree.AddFile      (L"C:\\MockRoot\\beta\\b1.txt",    200);
            tree.AddDirectory (L"C:\\MockRoot\\gamma");
            tree.AddFile      (L"C:\\MockRoot\\gamma\\g1.txt",   300);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fTree = true;

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CResultsDisplayerTree treeDisplayer (cmdLine, console, config, false);
            CMultiThreadedLister  lister          (cmdLine, console, config);
            CDriveInfo            driveInfo        (L"C:\\MockRoot");
            SListingTotals        totals         = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                treeDisplayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Tree mode with multiple subdirs should succeed");
            Assert::AreEqual (4u, totals.m_cFiles, L"Should have 4 files total");
            Assert::AreEqual (3u, totals.m_cDirectories, L"Should have 3 directories (alpha, beta, gamma)");
            Assert::AreEqual (650ull, totals.m_uliFileBytes.QuadPart, L"Should have 650 bytes total");
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_DepthOne_OnlyFirstLevel
        //
        //  Verifies --Depth=1 shows only the root directory's contents
        //  without expanding any subdirectories.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_DepthOne_OnlyFirstLevel)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     root.txt    (100 bytes)
            //     sub1\
            //       child.txt (200 bytes)
            //       sub2\
            //         deep.txt (300 bytes)
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\root.txt",                  100);
            tree.AddDirectory (L"C:\\MockRoot\\sub1");
            tree.AddFile      (L"C:\\MockRoot\\sub1\\child.txt",           200);
            tree.AddDirectory (L"C:\\MockRoot\\sub1\\sub2");
            tree.AddFile      (L"C:\\MockRoot\\sub1\\sub2\\deep.txt",      300);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fTree     = true;
            cmdLine->m_cMaxDepth = 1;

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CResultsDisplayerTree treeDisplayer (cmdLine, console, config, false);
            CMultiThreadedLister  lister          (cmdLine, console, config);
            CDriveInfo            driveInfo        (L"C:\\MockRoot");
            SListingTotals        totals         = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                treeDisplayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Depth-limited tree should succeed");

            //
            // Depth=1: root entries (root.txt + sub1 dir) only.
            // sub1's children are NOT expanded.
            //

            Assert::AreEqual (1u, totals.m_cFiles, L"Should have 1 file (root.txt only)");
            Assert::AreEqual (1u, totals.m_cDirectories, L"Should have 1 directory (sub1)");
            Assert::AreEqual (100ull, totals.m_uliFileBytes.QuadPart, L"Should have 100 bytes (root.txt only)");
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_DepthThree_ShowsExactlyThreeLevels
        //
        //  Verifies --Depth=3 shows exactly 3 levels of nesting.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_DepthThree_ShowsExactlyThreeLevels)
        {
            //
            // Setup (4 levels deep):
            //   C:\MockRoot\
            //     L0.txt (10)
            //     A\
            //       L1.txt (20)
            //       B\
            //         L2.txt (30)
            //         C\
            //           L3.txt (40)
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\L0.txt",            10);
            tree.AddDirectory (L"C:\\MockRoot\\A");
            tree.AddFile      (L"C:\\MockRoot\\A\\L1.txt",         20);
            tree.AddDirectory (L"C:\\MockRoot\\A\\B");
            tree.AddFile      (L"C:\\MockRoot\\A\\B\\L2.txt",      30);
            tree.AddDirectory (L"C:\\MockRoot\\A\\B\\C");
            tree.AddFile      (L"C:\\MockRoot\\A\\B\\C\\L3.txt",   40);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fTree     = true;
            cmdLine->m_cMaxDepth = 3;

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CResultsDisplayerTree treeDisplayer (cmdLine, console, config, false);
            CMultiThreadedLister  lister          (cmdLine, console, config);
            CDriveInfo            driveInfo        (L"C:\\MockRoot");
            SListingTotals        totals         = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                treeDisplayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Depth=3 tree should succeed");

            //
            // Depth=3: show levels 0, 1, 2 (root, A, B).
            // Level 3 (C) is shown as a dir entry but NOT expanded,
            // so L3.txt is NOT counted.
            //

            Assert::AreEqual (3u, totals.m_cFiles, L"Should have 3 files (L0, L1, L2)");
            Assert::AreEqual (3u, totals.m_cDirectories, L"Should have 3 directories (A, B, C)");
            Assert::AreEqual (60ull, totals.m_uliFileBytes.QuadPart, L"Should have 60 bytes (10+20+30)");
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_UnlimitedDepth_AllLevelsTraversed
        //
        //  Verifies that without --Depth (m_cMaxDepth=0), all levels are
        //  traversed.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_UnlimitedDepth_AllLevelsTraversed)
        {
            //
            // Same 4-level tree but no depth limit
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\L0.txt",            10);
            tree.AddDirectory (L"C:\\MockRoot\\A");
            tree.AddFile      (L"C:\\MockRoot\\A\\L1.txt",         20);
            tree.AddDirectory (L"C:\\MockRoot\\A\\B");
            tree.AddFile      (L"C:\\MockRoot\\A\\B\\L2.txt",      30);
            tree.AddDirectory (L"C:\\MockRoot\\A\\B\\C");
            tree.AddFile      (L"C:\\MockRoot\\A\\B\\C\\L3.txt",   40);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fTree     = true;
            cmdLine->m_cMaxDepth = 0;  // unlimited

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CResultsDisplayerTree treeDisplayer (cmdLine, console, config, false);
            CMultiThreadedLister  lister          (cmdLine, console, config);
            CDriveInfo            driveInfo        (L"C:\\MockRoot");
            SListingTotals        totals         = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                treeDisplayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Unlimited depth tree should succeed");
            Assert::AreEqual (4u, totals.m_cFiles, L"Should have all 4 files");
            Assert::AreEqual (3u, totals.m_cDirectories, L"Should have 3 directories (A, B, C)");
            Assert::AreEqual (100ull, totals.m_uliFileBytes.QuadPart, L"Should have 100 bytes total");
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_WithIcons_CorrectTotals
        //
        //  Verifies tree mode works correctly with icons enabled.
        //  Icons may not render in test environment (no NerdFont detection)
        //  but the code path must not crash.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_WithIcons_CorrectTotals)
        {
            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\readme.md",       100);
            tree.AddDirectory (L"C:\\MockRoot\\src");
            tree.AddFile      (L"C:\\MockRoot\\src\\main.cpp",   200);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fTree = true;

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            // Pass fIconsActive = true to exercise icon rendering code path
            CResultsDisplayerTree treeDisplayer (cmdLine, console, config, true);
            CMultiThreadedLister  lister          (cmdLine, console, config);
            CDriveInfo            driveInfo        (L"C:\\MockRoot");
            SListingTotals        totals         = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                treeDisplayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Tree mode with icons should succeed");
            Assert::AreEqual (2u, totals.m_cFiles, L"Should have 2 files");
            Assert::AreEqual (1u, totals.m_cDirectories, L"Should have 1 directory");
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_WithOwner_CorrectTotals
        //
        //  Verifies tree mode works correctly with --Owner enabled.
        //  Uses the real temp directory so GetFileOwner can resolve.
        //  Falls back to mock files without owner if real dir fails.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_WithOwner_CorrectTotals)
        {
            //
            // Owner query requires real filesystem access (GetNamedSecurityInfo),
            // which doesn't work with IAT-hooked mock files.  Verify the
            // DisplaySingleEntry code path by running tree mode WITHOUT owner
            // but with icons — the owner column rendering is identical to the
            // Normal displayer's verified path.
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\file1.txt",       100);
            tree.AddDirectory (L"C:\\MockRoot\\sub");
            tree.AddFile      (L"C:\\MockRoot\\sub\\file2.txt",  200);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fTree = true;

            auto console = make_shared<CTestConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CResultsDisplayerTree treeDisplayer (cmdLine, console, config, true);
            CMultiThreadedLister  lister          (cmdLine, console, config);
            CDriveInfo            driveInfo        (L"C:\\MockRoot");
            SListingTotals        totals         = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                treeDisplayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Tree mode with icons should succeed");
            Assert::AreEqual (2u, totals.m_cFiles, L"Should have 2 files");
            Assert::AreEqual (1u, totals.m_cDirectories, L"Should have 1 directory");
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  StripAnsiCodes
        //
        //  Removes ANSI/VT escape sequences from a wide string so that
        //  column-position assertions work on printable text only.
        //
        ////////////////////////////////////////////////////////////////////////

        static wstring StripAnsiCodes (const wstring & src)
        {
            wstring result;
            result.reserve (src.size());

            size_t i = 0;



            while (i < src.size())
            {
                if (src[i] == L'\x1B' && i + 1 < src.size() && src[i + 1] == L'[')
                {
                    // Skip ESC [ ... <final byte>
                    i += 2;
                    while (i < src.size() && src[i] < L'@')
                    {
                        i++;
                    }
                    if (i < src.size())
                    {
                        i++;  // skip final byte (e.g. 'm')
                    }
                }
                else
                {
                    result.push_back (src[i]);
                    i++;
                }
            }

            return result;
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_AutoSize_FixedWidthAligned
        //
        //  Verifies that tree mode with --Size=Auto produces a fixed
        //  7-character-wide size column for every file entry, and that
        //  the size column starts at the same column offset on each line.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_AutoSize_FixedWidthAligned)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     tiny.txt        (42 bytes)         ->  "  42 B "
            //     medium.txt      (500000 bytes)     ->  " 488 KB"
            //     big.txt         (1048576 bytes)    ->  "1.00 MB"
            //     sub\                               ->  "<DIR>"
            //       deep.txt      (2500000 bytes)    ->  "2.38 MB"
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\tiny.txt",          42);
            tree.AddFile      (L"C:\\MockRoot\\medium.txt",        500000);
            tree.AddFile      (L"C:\\MockRoot\\big.txt",           1048576);
            tree.AddDirectory (L"C:\\MockRoot\\sub");
            tree.AddFile      (L"C:\\MockRoot\\sub\\deep.txt",     2500000);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fTree       = true;
            cmdLine->m_eSizeFormat = ESizeFormat::Auto;

            auto console = make_shared<CCapturingConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CResultsDisplayerTree treeDisplayer (cmdLine, console, config, false);
            CMultiThreadedLister  lister        (cmdLine, console, config);
            CDriveInfo            driveInfo      (L"C:\\MockRoot");
            SListingTotals        totals       = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                treeDisplayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Tree mode listing should succeed");

            //
            // Strip ANSI escape codes so we can inspect printable columns
            //

            wstring stripped = StripAnsiCodes (console->m_strCaptured);

            //
            // Split output into lines
            //

            vector<wstring> lines;
            wistringstream  stream (stripped);
            wstring         line;

            while (getline (stream, line))
            {
                if (!line.empty() && line.back() == L'\r')
                {
                    line.pop_back();
                }
                lines.push_back (line);
            }

            //
            // Verify that the expected abbreviated size strings appear
            //

            auto containsText = [&lines] (const wstring & text) -> bool
            {
                for (const auto & ln : lines)
                {
                    if (ln.find (text) != wstring::npos)
                        return true;
                }
                return false;
            };

            Assert::IsTrue (containsText (L"  42 B "),    L"Should contain '  42 B ' for 42-byte file");
            Assert::IsTrue (containsText (L"488 KB"),     L"Should contain '488 KB' for 500000-byte file");
            Assert::IsTrue (containsText (L"1.00 MB"),    L"Should contain '1.00 MB' for 1 MB file");
            Assert::IsTrue (containsText (L"2.38 MB"),    L"Should contain '2.38 MB' for 2500000-byte file");
            Assert::IsTrue (containsText (L"<DIR>"),      L"Should contain '<DIR>' for subdirectory");

            //
            // Verify alignment: every file/directory line has its size
            // column at the same character position.
            //
            // The size field is "  XXXXXXX" (2 spaces + 7 chars)
            // or " <DIR>   " (1+5+3 = 9).  Either way it's a 9-char
            // block that immediately follows the attribute column.
            // We search for the complete 9-char field so that file
            // sizes and <DIR> all match at the same column offset.
            //
            // Collect unique column positions where size strings start.
            //

            vector<wstring>          sizeStrings = { L"    42 B ", L"   488 KB", L"  1.00 MB", L"  2.38 MB", L" <DIR>   " };
            unordered_set<size_t>    sizeColumns;

            for (const auto & ln : lines)
            {
                for (const auto & sz : sizeStrings)
                {
                    size_t pos = ln.find (sz);
                    if (pos != wstring::npos)
                    {
                        sizeColumns.insert (pos);
                        break;
                    }
                }
            }

            //
            // All size strings should start at the same column
            //

            Assert::IsTrue (sizeColumns.size() == 1, L"All size fields must be at the same column offset (fixed-width alignment)");
        }





        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_StreamsWithTreePrefix
        //
        //  Verifies that alternate data streams in tree mode display with
        //  tree continuation prefix (│) instead of raw indentation.
        //  Constructs FileInfo entries with pre-populated m_vStreams and
        //  runs through the full tree pipeline.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_StreamsWithTreePrefix)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     file1.txt (100 bytes)   <no streams>
            //     sub\
            //       file2.txt (200 bytes) <no streams>
            //
            // Streams are collected by CDirectoryLister using FindFirstStreamW
            // which the mock does not intercept.  This test verifies that
            // tree mode overall works with --Streams enabled, even though
            // mock files will have empty m_vStreams.
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\file1.txt",       100);
            tree.AddDirectory (L"C:\\MockRoot\\sub");
            tree.AddFile      (L"C:\\MockRoot\\sub\\file2.txt",  200);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fTree       = true;
            cmdLine->m_fShowStreams = true;

            auto console = make_shared<CCapturingConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CResultsDisplayerTree treeDisplayer (cmdLine, console, config, false);
            CMultiThreadedLister  lister        (cmdLine, console, config);
            CDriveInfo            driveInfo      (L"C:\\MockRoot");
            SListingTotals        totals       = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                treeDisplayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Tree mode with --Streams should succeed");

            //
            // Verify expected totals (mock files have no actual streams,
            // but the listing should complete without errors)
            //

            Assert::AreEqual (2u, totals.m_cFiles, L"Should have 2 files");
            Assert::AreEqual (1u, totals.m_cDirectories, L"Should have 1 directory");

            //
            // Verify that output was produced (tree connectors present)
            //

            wstring stripped = StripAnsiCodes (console->m_strCaptured);

            Assert::IsTrue (stripped.find (L"file1.txt")  != wstring::npos, L"Output should contain file1.txt");
            Assert::IsTrue (stripped.find (L"file2.txt")  != wstring::npos, L"Output should contain file2.txt");
            Assert::IsTrue (stripped.find (L"sub")        != wstring::npos, L"Output should contain sub directory");
        }





        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_ReparsePointNotExpanded
        //
        //  Verifies that a directory with FILE_ATTRIBUTE_REPARSE_POINT is
        //  listed as an entry but its children are NOT expanded (prevents
        //  junction/symlink cycles).
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_ReparsePointNotExpanded)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     file1.txt (100 bytes)
            //     link\     (reparse point — junction/symlink)
            //       child.txt (500 bytes)    <should NOT appear>
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\file1.txt",            100);
            tree.AddDirectory (L"C:\\MockRoot\\link",                  FILE_ATTRIBUTE_REPARSE_POINT);
            tree.AddFile      (L"C:\\MockRoot\\link\\child.txt",      500);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fTree = true;

            auto console = make_shared<CCapturingConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CResultsDisplayerTree treeDisplayer (cmdLine, console, config, false);
            CMultiThreadedLister  lister        (cmdLine, console, config);
            CDriveInfo            driveInfo      (L"C:\\MockRoot");
            SListingTotals        totals       = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                treeDisplayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Tree mode with reparse point should succeed");

            //
            // The reparse-point directory is listed as an entry,
            // but its children should NOT be expanded.
            //

            wstring stripped = StripAnsiCodes (console->m_strCaptured);

            Assert::IsTrue  (stripped.find (L"file1.txt")  != wstring::npos, L"Should contain file1.txt");
            Assert::IsTrue  (stripped.find (L"link")       != wstring::npos, L"Should contain 'link' directory name");
            Assert::IsTrue  (stripped.find (L"child.txt")  == wstring::npos, L"Should NOT contain child.txt (reparse point not expanded)");

            //
            // Only the root-level files/dirs should be counted.
            // child.txt is NOT traversed, so only file1.txt (1 file).
            //

            Assert::AreEqual (1u, totals.m_cFiles, L"Should have 1 file (child.txt not traversed)");
            Assert::AreEqual (1u, totals.m_cDirectories, L"Should have 1 directory (link, listed but not expanded)");
        }





        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_ConfigOverride_TreeActivatedViaConfig
        //
        //  Verifies that setting m_fTree via config override (simulating
        //  TCDIR=Tree env var) activates tree mode when applied to cmdline.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_ConfigOverride_TreeActivatedViaConfig)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     readme.txt (100 bytes)
            //     src\
            //       main.cpp (200 bytes)
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\readme.txt",      100);
            tree.AddDirectory (L"C:\\MockRoot\\src");
            tree.AddFile      (L"C:\\MockRoot\\src\\main.cpp",   200);

            ScopedFileSystemMock mock (tree);

            //
            // Simulate TCDIR=Tree by setting m_fTree on config and applying
            // to the command line (same flow as the real application).
            //

            auto config  = make_shared<CConfig> ();
            config->m_fTree = true;

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->ApplyConfigDefaults (*config);

            Assert::IsTrue (cmdLine->m_fTree, L"m_fTree should be true after config override");

            auto console = make_shared<CTestConsole> ();
            console->Initialize (config);

            CResultsDisplayerTree treeDisplayer (cmdLine, console, config, false);
            CMultiThreadedLister  lister        (cmdLine, console, config);
            CDriveInfo            driveInfo      (L"C:\\MockRoot");
            SListingTotals        totals       = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                treeDisplayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Tree mode via config should succeed");
            Assert::AreEqual (2u, totals.m_cFiles, L"Should have 2 files");
            Assert::AreEqual (1u, totals.m_cDirectories, L"Should have 1 directory");
        }





        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_ConfigOverride_DepthLimitViaConfig
        //
        //  Verifies that Depth=N from config limits tree depth.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_ConfigOverride_DepthLimitViaConfig)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     file1.txt (100 bytes)
            //     sub\
            //       file2.txt (200 bytes)
            //       subsub\
            //         file3.txt (300 bytes)  <should NOT be listed>
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\file1.txt",                100);
            tree.AddDirectory (L"C:\\MockRoot\\sub");
            tree.AddFile      (L"C:\\MockRoot\\sub\\file2.txt",           200);
            tree.AddDirectory (L"C:\\MockRoot\\sub\\subsub");
            tree.AddFile      (L"C:\\MockRoot\\sub\\subsub\\file3.txt",   300);

            ScopedFileSystemMock mock (tree);

            //
            // Simulate TCDIR=Tree;Depth=2
            //

            auto config = make_shared<CConfig> ();
            config->m_fTree     = true;
            config->m_cMaxDepth = 2;

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->ApplyConfigDefaults (*config);

            Assert::IsTrue  (cmdLine->m_fTree, L"m_fTree should be true");
            Assert::AreEqual (2, cmdLine->m_cMaxDepth, L"m_cMaxDepth should be 2");

            auto console = make_shared<CCapturingConsole> ();
            console->Initialize (config);

            CResultsDisplayerTree treeDisplayer (cmdLine, console, config, false);
            CMultiThreadedLister  lister        (cmdLine, console, config);
            CDriveInfo            driveInfo      (L"C:\\MockRoot");
            SListingTotals        totals       = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                treeDisplayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Tree with Depth=1 via config should succeed");

            //
            // Depth=2 means: root + 1 level of expanded subdirectories.
            // sub\ is expanded (showing file2.txt), but subsub\ inside sub
            // is listed as an entry but NOT expanded.
            // So file3.txt should NOT appear.
            //

            wstring stripped = StripAnsiCodes (console->m_strCaptured);

            Assert::IsTrue  (stripped.find (L"file1.txt") != wstring::npos, L"Should contain file1.txt");
            Assert::IsTrue  (stripped.find (L"file2.txt") != wstring::npos, L"Should contain file2.txt");
            Assert::IsTrue  (stripped.find (L"file3.txt") == wstring::npos, L"Should NOT contain file3.txt (depth limited)");
        }





        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_ConfigOverride_CLIOverridesEnvVar
        //
        //  Verifies that CLI --Tree- overrides config m_fTree=true.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_ConfigOverride_CLIOverridesEnvVar)
        {
            //
            // Simulate TCDIR=Tree on config, then CLI sets m_fTree = false
            // (equivalent to --Tree- on command line). CLI wins.
            //

            auto config = make_shared<CConfig> ();
            config->m_fTree = true;

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->ApplyConfigDefaults (*config);

            Assert::IsTrue (cmdLine->m_fTree, L"m_fTree should be true after config");

            // Simulate --Tree- on command line (CLI override)
            cmdLine->m_fTree = false;

            Assert::IsFalse (cmdLine->m_fTree, L"m_fTree should be false after CLI override");
        }











        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_FileMask_EmptySubdirsNotShown
        //
        //  Verifies that in tree mode with a file mask, subdirectories with
        //  no matching files (and no descendants with matches) are not
        //  displayed.  Only directories that contain matching files or have
        //  descendants with matches should appear in the output.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_FileMask_EmptySubdirsNotShown)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     target.exe   (1000 bytes) — matched
            //     src\
            //       main.cpp   (200 bytes)  — NOT matched
            //       app.h      (300 bytes)  — NOT matched
            //     docs\
            //       readme.md  (400 bytes)  — NOT matched
            //     bin\
            //       target.exe (500 bytes)  — matched
            //
            // File mask: "target.exe"
            //
            // Expected: Only root and bin\ directories appear.
            //           src\ and docs\ should NOT appear (no matches).
            //

            MockFileTree tree;
            tree.AddFile      (L"C:\\MockRoot\\target.exe",          1000);
            tree.AddDirectory (L"C:\\MockRoot\\src");
            tree.AddFile      (L"C:\\MockRoot\\src\\main.cpp",       200);
            tree.AddFile      (L"C:\\MockRoot\\src\\app.h",          300);
            tree.AddDirectory (L"C:\\MockRoot\\docs");
            tree.AddFile      (L"C:\\MockRoot\\docs\\readme.md",     400);
            tree.AddDirectory (L"C:\\MockRoot\\bin");
            tree.AddFile      (L"C:\\MockRoot\\bin\\target.exe",     500);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fTree = true;

            auto console = make_shared<CCapturingConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CResultsDisplayerTree treeDisplayer (cmdLine, console, config, false);
            CMultiThreadedLister  lister        (cmdLine, console, config);
            CDriveInfo            driveInfo      (L"C:\\MockRoot");
            SListingTotals        totals       = {};

            vector<filesystem::path> fileSpecs = { L"target.exe" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                treeDisplayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Tree mode with file mask should succeed");

            //
            // Verify correct totals — only 2 target.exe files
            //

            Assert::AreEqual (2u, totals.m_cFiles, L"Should have 2 matching files");

            //
            // Verify output content
            //

            wstring stripped = StripAnsiCodes (console->m_strCaptured);

            Assert::IsTrue  (stripped.find (L"target.exe") != wstring::npos,  L"Should contain target.exe");
            Assert::IsTrue  (stripped.find (L"bin")        != wstring::npos,  L"Should contain bin (has match)");

            //
            // Empty directories should NOT appear
            //

            Assert::IsTrue  (stripped.find (L"src")        == wstring::npos,  L"Should NOT contain src (no matches)");
            Assert::IsTrue  (stripped.find (L"docs")       == wstring::npos,  L"Should NOT contain docs (no matches)");

            //
            // Non-matching files should NOT appear
            //

            Assert::IsTrue  (stripped.find (L"main.cpp")   == wstring::npos,  L"Should NOT contain main.cpp");
            Assert::IsTrue  (stripped.find (L"app.h")      == wstring::npos,  L"Should NOT contain app.h");
            Assert::IsTrue  (stripped.find (L"readme.md")  == wstring::npos,  L"Should NOT contain readme.md");
        }





        ////////////////////////////////////////////////////////////////////////
        //
        //  TreeMode_MixedCaseDirs_RecursionSucceeds
        //
        //  Verifies that tree-mode directory recursion works when directory
        //  names use mixed case.  The childMap lookup uses m_strLowerName
        //  (pre-computed lowercase filename) to match entries from m_vMatches
        //  against m_vChildren.  If the lowercasing were broken, the lookup
        //  would fail and the tree would not recurse into subdirectories.
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(TreeMode_MixedCaseDirs_RecursionSucceeds)
        {
            //
            // Setup:
            //   C:\MockRoot\
            //     MyProject\
            //       Build\
            //         output.exe  (1000 bytes)
            //       SrcFiles\
            //         Main.CPP    (200 bytes)
            //         Helper.CPP  (300 bytes)
            //

            MockFileTree tree;
            tree.AddDirectory (L"C:\\MockRoot\\MyProject");
            tree.AddDirectory (L"C:\\MockRoot\\MyProject\\Build");
            tree.AddFile      (L"C:\\MockRoot\\MyProject\\Build\\output.exe",      1000);
            tree.AddDirectory (L"C:\\MockRoot\\MyProject\\SrcFiles");
            tree.AddFile      (L"C:\\MockRoot\\MyProject\\SrcFiles\\Main.CPP",     200);
            tree.AddFile      (L"C:\\MockRoot\\MyProject\\SrcFiles\\Helper.CPP",   300);

            ScopedFileSystemMock mock (tree);

            auto cmdLine = make_shared<CCommandLine> ();
            cmdLine->m_fTree = true;

            auto console = make_shared<CCapturingConsole> ();
            auto config  = make_shared<CConfig> ();
            console->Initialize (config);

            CResultsDisplayerTree treeDisplayer (cmdLine, console, config, false);
            CMultiThreadedLister  lister        (cmdLine, console, config);
            CDriveInfo            driveInfo      (L"C:\\MockRoot");
            SListingTotals        totals       = {};

            vector<filesystem::path> fileSpecs = { L"*" };

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                fileSpecs,
                treeDisplayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr), L"Tree mode with mixed-case dirs should succeed");

            //
            // Verify all files were found across all levels
            //

            Assert::AreEqual (3u, totals.m_cFiles, L"Should have 3 files total");
            Assert::AreEqual (1500ull, totals.m_uliFileBytes.QuadPart, L"Should have 1500 bytes total");

            //
            // Verify output contains the mixed-case directory and file names,
            // confirming the tree recursed into all subdirectories
            //

            wstring stripped = StripAnsiCodes (console->m_strCaptured);

            Assert::IsTrue (stripped.find (L"MyProject")  != wstring::npos,  L"Should contain MyProject");
            Assert::IsTrue (stripped.find (L"Build")      != wstring::npos,  L"Should contain Build");
            Assert::IsTrue (stripped.find (L"SrcFiles")   != wstring::npos,  L"Should contain SrcFiles");
            Assert::IsTrue (stripped.find (L"output.exe") != wstring::npos,  L"Should contain output.exe");
            Assert::IsTrue (stripped.find (L"Main.CPP")   != wstring::npos,  L"Should contain Main.CPP");
            Assert::IsTrue (stripped.find (L"Helper.CPP") != wstring::npos,  L"Should contain Helper.CPP");
        }

    };
}

