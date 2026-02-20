#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/CommandLine.h"
#include "../TCDirCore/FileComparator.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(FileComparatorTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            // Setup EHM to use unit test framework's assertion mechanism
            SetupEhmForUnitTests();
        }





        TEST_METHOD(SizeOrderingAscDesc)
        {
            auto cmd = std::make_shared<CCommandLine>();

            cmd->m_rgSortPreference[0] = CCommandLine::ESortOrder::SO_SIZE;
            cmd->m_sortorder           = CCommandLine::ESortOrder::SO_SIZE;
            cmd->m_sortdirection       = CCommandLine::ESortDirection::SD_ASCENDING;

            FileComparator comp(cmd);

            WIN32_FIND_DATA fileSmall = {};
            WIN32_FIND_DATA fileLarge = {};
            StringCchCopyW(fileSmall.cFileName, ARRAYSIZE(fileSmall.cFileName), L"a");
            StringCchCopyW(fileLarge.cFileName, ARRAYSIZE(fileLarge.cFileName), L"b");
            fileSmall.nFileSizeHigh = 0; fileSmall.nFileSizeLow = 100;
            fileLarge.nFileSizeHigh = 0; fileLarge.nFileSizeLow = 200;

            Assert::IsTrue (comp(fileSmall, fileLarge));
            Assert::IsFalse (comp(fileLarge, fileSmall));

            cmd->m_sortdirection = CCommandLine::ESortDirection::SD_DESCENDING;

            Assert::IsFalse (comp(fileSmall, fileLarge));
            Assert::IsTrue (comp(fileLarge, fileSmall));
        }





        TEST_METHOD(DirectoryFirst)
        {
            auto cmd = std::make_shared<CCommandLine>();

            cmd->m_rgSortPreference[0] = CCommandLine::ESortOrder::SO_NAME;
            cmd->m_sortorder           = CCommandLine::ESortOrder::SO_NAME;
            cmd->m_sortdirection       = CCommandLine::ESortDirection::SD_ASCENDING;

            FileComparator comp(cmd);

            WIN32_FIND_DATA entryDir  = {};
            WIN32_FIND_DATA entryFile = {};
            StringCchCopyW(entryDir.cFileName,  ARRAYSIZE(entryDir.cFileName),   L"zzz");
            StringCchCopyW(entryFile.cFileName, ARRAYSIZE(entryFile.cFileName),  L"aaa");
            entryDir.dwFileAttributes  = FILE_ATTRIBUTE_DIRECTORY;
            entryFile.dwFileAttributes = 0;

            Assert::IsTrue (comp(entryDir, entryFile));
            Assert::IsFalse (comp(entryFile, entryDir));
        }





        TEST_METHOD(PrimarySortsAndReverseBehavior)
        {
            auto cmd = std::make_shared<CCommandLine>();

            cmd->m_rgSortPreference[0] = CCommandLine::ESortOrder::SO_NAME;
            cmd->m_sortorder           = CCommandLine::ESortOrder::SO_NAME;
            cmd->m_sortdirection       = CCommandLine::ESortDirection::SD_DESCENDING;

            FileComparator comp(cmd);

            // Same name => falls back; verify reverse applies only to primary attribute (not to fallback).
            WIN32_FIND_DATA a = {};
            WIN32_FIND_DATA b = {};
            StringCchCopyW(a.cFileName, ARRAYSIZE(a.cFileName), L"same");
            StringCchCopyW(b.cFileName, ARRAYSIZE(b.cFileName), L"same");

            // Equal times and extensions; different sizes so fallback ends on size.
            a.ftLastWriteTime.dwHighDateTime = 0; a.ftLastWriteTime.dwLowDateTime = 0;
            b.ftLastWriteTime.dwHighDateTime = 0; b.ftLastWriteTime.dwLowDateTime = 0;
            a.nFileSizeHigh = 0; a.nFileSizeLow = 100;
            b.nFileSizeHigh = 0; b.nFileSizeLow = 200;

            // Even though direction is DESC for primary (name), because names are equal,
            // comparison falls back, and reverse should NOT apply. Smaller comes first.
            Assert::IsTrue (comp(a, b));
            Assert::IsFalse (comp(b, a));
        }




        TEST_METHOD(DateOrderingDefaultWriteTime)
        {
            auto cmd = std::make_shared<CCommandLine>();

            cmd->m_rgSortPreference[0] = CCommandLine::ESortOrder::SO_DATE;
            cmd->m_sortorder           = CCommandLine::ESortOrder::SO_DATE;
            cmd->m_sortdirection       = CCommandLine::ESortDirection::SD_ASCENDING;
            // m_timeField defaults to TF_WRITTEN

            FileComparator comp(cmd);

            WIN32_FIND_DATA older = {};
            WIN32_FIND_DATA newer = {};
            StringCchCopyW(older.cFileName, ARRAYSIZE(older.cFileName), L"older");
            StringCchCopyW(newer.cFileName, ARRAYSIZE(newer.cFileName), L"newer");

            // Set last write times (older < newer)
            older.ftLastWriteTime.dwHighDateTime = 0; older.ftLastWriteTime.dwLowDateTime = 100;
            newer.ftLastWriteTime.dwHighDateTime = 0; newer.ftLastWriteTime.dwLowDateTime = 200;

            Assert::IsTrue (comp(older, newer));
            Assert::IsFalse (comp(newer, older));
        }




        TEST_METHOD(DateOrderingCreationTime)
        {
            auto cmd = std::make_shared<CCommandLine>();

            cmd->m_rgSortPreference[0] = CCommandLine::ESortOrder::SO_DATE;
            cmd->m_sortorder           = CCommandLine::ESortOrder::SO_DATE;
            cmd->m_sortdirection       = CCommandLine::ESortDirection::SD_ASCENDING;
            cmd->m_timeField           = CCommandLine::ETimeField::TF_CREATION;

            FileComparator comp(cmd);

            WIN32_FIND_DATA older = {};
            WIN32_FIND_DATA newer = {};
            StringCchCopyW(older.cFileName, ARRAYSIZE(older.cFileName), L"older");
            StringCchCopyW(newer.cFileName, ARRAYSIZE(newer.cFileName), L"newer");

            // Set creation times (older < newer) - opposite write times to prove we use creation
            older.ftCreationTime.dwHighDateTime   = 0; older.ftCreationTime.dwLowDateTime   = 100;
            newer.ftCreationTime.dwHighDateTime   = 0; newer.ftCreationTime.dwLowDateTime   = 200;
            older.ftLastWriteTime.dwHighDateTime  = 0; older.ftLastWriteTime.dwLowDateTime  = 200;  // Intentionally reversed
            newer.ftLastWriteTime.dwHighDateTime  = 0; newer.ftLastWriteTime.dwLowDateTime  = 100;

            Assert::IsTrue (comp(older, newer));
            Assert::IsFalse (comp(newer, older));
        }




        TEST_METHOD(DateOrderingAccessTime)
        {
            auto cmd = std::make_shared<CCommandLine>();

            cmd->m_rgSortPreference[0] = CCommandLine::ESortOrder::SO_DATE;
            cmd->m_sortorder           = CCommandLine::ESortOrder::SO_DATE;
            cmd->m_sortdirection       = CCommandLine::ESortDirection::SD_ASCENDING;
            cmd->m_timeField           = CCommandLine::ETimeField::TF_ACCESS;

            FileComparator comp(cmd);

            WIN32_FIND_DATA older = {};
            WIN32_FIND_DATA newer = {};
            StringCchCopyW(older.cFileName, ARRAYSIZE(older.cFileName), L"older");
            StringCchCopyW(newer.cFileName, ARRAYSIZE(newer.cFileName), L"newer");

            // Set access times (older < newer) - opposite write times to prove we use access
            older.ftLastAccessTime.dwHighDateTime = 0; older.ftLastAccessTime.dwLowDateTime = 100;
            newer.ftLastAccessTime.dwHighDateTime = 0; newer.ftLastAccessTime.dwLowDateTime = 200;
            older.ftLastWriteTime.dwHighDateTime  = 0; older.ftLastWriteTime.dwLowDateTime  = 200;  // Intentionally reversed
            newer.ftLastWriteTime.dwHighDateTime  = 0; newer.ftLastWriteTime.dwLowDateTime  = 100;

            Assert::IsTrue (comp(older, newer));
            Assert::IsFalse (comp(newer, older));
        }




        TEST_METHOD(DateOrderingDescendingCreationTime)
        {
            auto cmd = std::make_shared<CCommandLine>();

            cmd->m_rgSortPreference[0] = CCommandLine::ESortOrder::SO_DATE;
            cmd->m_sortorder           = CCommandLine::ESortOrder::SO_DATE;
            cmd->m_sortdirection       = CCommandLine::ESortDirection::SD_DESCENDING;
            cmd->m_timeField           = CCommandLine::ETimeField::TF_CREATION;

            FileComparator comp(cmd);

            WIN32_FIND_DATA older = {};
            WIN32_FIND_DATA newer = {};
            StringCchCopyW(older.cFileName, ARRAYSIZE(older.cFileName), L"older");
            StringCchCopyW(newer.cFileName, ARRAYSIZE(newer.cFileName), L"newer");

            older.ftCreationTime.dwHighDateTime = 0; older.ftCreationTime.dwLowDateTime = 100;
            newer.ftCreationTime.dwHighDateTime = 0; newer.ftCreationTime.dwLowDateTime = 200;

            // Descending: newer should come before older
            Assert::IsFalse (comp(older, newer));
            Assert::IsTrue (comp(newer, older));
        }




        TEST_METHOD(InterleavedSort_DirectoriesNotGroupedFirst)
        {
            auto cmd = std::make_shared<CCommandLine>();

            cmd->m_rgSortPreference[0] = CCommandLine::ESortOrder::SO_NAME;
            cmd->m_sortorder           = CCommandLine::ESortOrder::SO_NAME;
            cmd->m_sortdirection       = CCommandLine::ESortDirection::SD_ASCENDING;

            FileComparator comp(cmd, true /* fInterleavedSort */);

            WIN32_FIND_DATA entryDir  = {};
            WIN32_FIND_DATA entryFile = {};
            StringCchCopyW(entryDir.cFileName,  ARRAYSIZE(entryDir.cFileName),   L"zzz");
            StringCchCopyW(entryFile.cFileName, ARRAYSIZE(entryFile.cFileName),  L"aaa");
            entryDir.dwFileAttributes  = FILE_ATTRIBUTE_DIRECTORY;
            entryFile.dwFileAttributes = 0;

            // In interleaved mode, "aaa" (file) sorts before "zzz" (dir) by name
            Assert::IsFalse (comp(entryDir, entryFile));
            Assert::IsTrue (comp(entryFile, entryDir));
        }




        TEST_METHOD(InterleavedSort_SortsByNameNotType)
        {
            auto cmd = std::make_shared<CCommandLine>();

            cmd->m_rgSortPreference[0] = CCommandLine::ESortOrder::SO_NAME;
            cmd->m_sortorder           = CCommandLine::ESortOrder::SO_NAME;
            cmd->m_sortdirection       = CCommandLine::ESortDirection::SD_ASCENDING;

            FileComparator comp(cmd, true /* fInterleavedSort */);

            WIN32_FIND_DATA dir1  = {};
            WIN32_FIND_DATA file1 = {};
            WIN32_FIND_DATA dir2  = {};
            WIN32_FIND_DATA file2 = {};
            StringCchCopyW(dir1.cFileName,  ARRAYSIZE(dir1.cFileName),  L"bbb");
            StringCchCopyW(file1.cFileName, ARRAYSIZE(file1.cFileName), L"aaa");
            StringCchCopyW(dir2.cFileName,  ARRAYSIZE(dir2.cFileName),  L"ddd");
            StringCchCopyW(file2.cFileName, ARRAYSIZE(file2.cFileName), L"ccc");
            dir1.dwFileAttributes  = FILE_ATTRIBUTE_DIRECTORY;
            dir2.dwFileAttributes  = FILE_ATTRIBUTE_DIRECTORY;
            file1.dwFileAttributes = 0;
            file2.dwFileAttributes = 0;

            // Verify full sort produces: aaa(file), bbb(dir), ccc(file), ddd(dir)
            std::vector<WIN32_FIND_DATA> entries = { dir2, file1, dir1, file2 };
            std::sort(entries.begin(), entries.end(), comp);

            Assert::AreEqual(std::wstring(L"aaa"), std::wstring(entries[0].cFileName));
            Assert::AreEqual(std::wstring(L"bbb"), std::wstring(entries[1].cFileName));
            Assert::AreEqual(std::wstring(L"ccc"), std::wstring(entries[2].cFileName));
            Assert::AreEqual(std::wstring(L"ddd"), std::wstring(entries[3].cFileName));
        }




        TEST_METHOD(InterleavedSort_NonInterleavedGroupsDirsFirst)
        {
            auto cmd = std::make_shared<CCommandLine>();

            cmd->m_rgSortPreference[0] = CCommandLine::ESortOrder::SO_NAME;
            cmd->m_sortorder           = CCommandLine::ESortOrder::SO_NAME;
            cmd->m_sortdirection       = CCommandLine::ESortDirection::SD_ASCENDING;

            FileComparator comp(cmd, false /* fInterleavedSort - default */);

            WIN32_FIND_DATA dir1  = {};
            WIN32_FIND_DATA file1 = {};
            WIN32_FIND_DATA dir2  = {};
            WIN32_FIND_DATA file2 = {};
            StringCchCopyW(dir1.cFileName,  ARRAYSIZE(dir1.cFileName),  L"bbb");
            StringCchCopyW(file1.cFileName, ARRAYSIZE(file1.cFileName), L"aaa");
            StringCchCopyW(dir2.cFileName,  ARRAYSIZE(dir2.cFileName),  L"ddd");
            StringCchCopyW(file2.cFileName, ARRAYSIZE(file2.cFileName), L"ccc");
            dir1.dwFileAttributes  = FILE_ATTRIBUTE_DIRECTORY;
            dir2.dwFileAttributes  = FILE_ATTRIBUTE_DIRECTORY;
            file1.dwFileAttributes = 0;
            file2.dwFileAttributes = 0;

            // Non-interleaved: dirs first, then files. bbb(dir), ddd(dir), aaa(file), ccc(file)
            std::vector<WIN32_FIND_DATA> entries = { dir2, file1, dir1, file2 };
            std::sort(entries.begin(), entries.end(), comp);

            Assert::AreEqual(std::wstring(L"bbb"), std::wstring(entries[0].cFileName));
            Assert::AreEqual(std::wstring(L"ddd"), std::wstring(entries[1].cFileName));
            Assert::AreEqual(std::wstring(L"aaa"), std::wstring(entries[2].cFileName));
            Assert::AreEqual(std::wstring(L"ccc"), std::wstring(entries[3].cFileName));
        }

    };
}


