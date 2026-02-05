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

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                L"*",
                displayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            //
            // Verify results
            //

            Assert::IsTrue (SUCCEEDED (hr), L"ProcessDirectoryMultiThreaded should succeed");

            Assert::AreEqual (6u, totals.m_cFiles, L"Should have 6 files");
            Assert::AreEqual (3u, totals.m_cDirectories, L"Should have 3 subdirectories");
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

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                L"*",
                displayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr));
            Assert::AreEqual (2u, totals.m_cFiles, L"Should have 2 files");
            Assert::AreEqual (3u, totals.m_cDirectories, L"Should have 3 subdirectories (including empty ones)");
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

            HRESULT hr = lister.ProcessDirectoryMultiThreaded (
                driveInfo,
                L"C:\\MockRoot",
                L"*",
                displayer,
                IResultsDisplayer::EDirectoryLevel::Initial,
                totals);

            Assert::IsTrue (SUCCEEDED (hr));
            Assert::AreEqual (2u, totals.m_cFiles, L"Should have 2 files (not recursive)");
            Assert::AreEqual (1u, totals.m_cDirectories, L"Should have 1 subdirectory shown (not recursed into)");
            Assert::AreEqual (300ull, totals.m_uliFileBytes.QuadPart);
        }

    };
}

