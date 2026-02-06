#include "pch.h"
#include "EhmTestHelper.h"
#include "Mocks/FileSystemMock.h"

#include "../TCDirCore/MultiThreadedLister.h"
#include "../TCDirCore/ResultsDisplayerNormal.h"
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

            auto console = make_shared<CConsole> ();
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

            auto console = make_shared<CConsole> ();
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

            auto console = make_shared<CConsole> ();
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

            auto console = make_shared<CConsole> ();
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

            auto console = make_shared<CConsole> ();
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

            auto console = make_shared<CConsole> ();
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

            auto console = make_shared<CConsole> ();
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

            auto console = make_shared<CConsole> ();
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

    };
}

