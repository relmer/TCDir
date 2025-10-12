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

            Assert::IsTrue(comp(fileSmall, fileLarge));
            Assert::IsFalse(comp(fileLarge, fileSmall));

            cmd->m_sortdirection = CCommandLine::ESortDirection::SD_DESCENDING;

            Assert::IsFalse(comp(fileSmall, fileLarge));
            Assert::IsTrue(comp(fileLarge, fileSmall));
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

            Assert::IsTrue(comp(entryDir, entryFile));
            Assert::IsFalse(comp(entryFile, entryDir));
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
            Assert::IsTrue(comp(a, b));
            Assert::IsFalse(comp(b, a));
        }

    };
}
