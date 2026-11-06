#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/ArchiveStatistics.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(ArchiveStatisticsTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        static SArchiveEntry File (const wstring & strPath, ULONGLONG ullSize, ULONGLONG ullTime = 0, bool fEncrypted = false)
        {
            SArchiveEntry entry;

            entry.m_strPath                    = strPath;
            entry.m_ullSize                    = ullSize;
            entry.m_ftLastWrite.dwLowDateTime  = static_cast<DWORD> (ullTime);
            entry.m_ftLastWrite.dwHighDateTime = static_cast<DWORD> (ullTime >> 32);
            entry.m_fEncrypted                 = fEncrypted;
            return entry;
        }

        static SArchiveEntry Folder (const wstring & strPath, ULONGLONG ullTime = 0)
        {
            SArchiveEntry entry = File (strPath, 0, ullTime);

            entry.m_fDirectory = true;
            return entry;
        }

        static SArchiveStatistics Compute (vector<SArchiveEntry> entries, bool fListingEncrypted = false)
        {
            SArchiveReadResult   result;
            SArchiveStatistics   statistics;

            result.m_vEntries          = move (entries);
            result.m_fListingEncrypted = fListingEncrypted;

            Assert::AreEqual (S_OK, CArchiveStatistics::Compute (result, statistics));
            return statistics;
        }

        static ULONGLONG ToTicks (const optional<FILETIME> & ft)
        {
            Assert::IsTrue (ft.has_value());
            return (static_cast<ULONGLONG> (ft->dwHighDateTime) << 32) | ft->dwLowDateTime;
        }




        TEST_METHOD(Compute_EmptyArchive)
        {
            SArchiveStatistics statistics = Compute ({});

            Assert::AreEqual (0ull, statistics.m_cFiles);
            Assert::AreEqual (0ull, statistics.m_cFolders);
            Assert::AreEqual (0ull, statistics.m_ullUncompressed);
            Assert::IsTrue   (statistics.m_strLargestPath.empty());
            Assert::IsTrue   (statistics.m_strDeepestPath.empty());
            Assert::IsFalse  (statistics.m_ftOldest.has_value());
            Assert::IsTrue   (statistics.m_vExtensions.empty());
        }




        TEST_METHOD(Compute_CountsFilesAndExplicitAndImpliedFolders)
        {
            SArchiveStatistics statistics = Compute ({
                File   (L"readme.txt", 10),
                Folder (L"docs/"),
                File   (L"docs/guide.md", 20),
                File   (L"src/lib/util.cpp", 30),
            });

            Assert::AreEqual (3ull, statistics.m_cFiles);
            Assert::AreEqual (3ull, statistics.m_cFolders);      // docs, src, src\lib
            Assert::AreEqual (60ull, statistics.m_ullUncompressed);
        }




        TEST_METHOD(Compute_CountsNestedArchivesBySuffix)
        {
            SArchiveStatistics statistics = Compute ({
                File (L"inner.zip", 1),
                File (L"pkg/data.tar.gz", 1),
                File (L"notes.txt", 1),
            });

            Assert::AreEqual (2ull, statistics.m_cNestedArchives);
        }




        TEST_METHOD(Compute_LargestFile_FirstOnTie)
        {
            SArchiveStatistics statistics = Compute ({
                File (L"a.bin", 50),
                File (L"b.bin", 50),
                File (L"dir/c.bin", 10),
            });

            Assert::AreEqual (L"a.bin", statistics.m_strLargestPath.c_str());
            Assert::AreEqual (50ull, statistics.m_ullLargestSize);
        }




        TEST_METHOD(Compute_DeepestPath_CountsComponents)
        {
            SArchiveStatistics statistics = Compute ({
                File (L"top.txt", 1),
                File (L"a/b/c/deep.txt", 1),
                File (L"a/shallow.txt", 1),
            });

            Assert::AreEqual (L"a\\b\\c\\deep.txt", statistics.m_strDeepestPath.c_str());
            Assert::AreEqual (4u, statistics.m_cDeepestDepth);
        }




        TEST_METHOD(Compute_DateRange_SkipsEntriesWithoutTime)
        {
            SArchiveStatistics statistics = Compute ({
                File   (L"old.txt", 1, 1000),
                File   (L"none.txt", 1, 0),
                File   (L"new.txt", 1, 5000),
                Folder (L"dir/", 3000),
            });

            Assert::AreEqual (1000ull, ToTicks (statistics.m_ftOldest));
            Assert::AreEqual (5000ull, ToTicks (statistics.m_ftNewest));
        }




        TEST_METHOD(Compute_DateRange_NoTimestamps)
        {
            SArchiveStatistics statistics = Compute ({ File (L"a.txt", 1), File (L"b.txt", 1) });

            Assert::IsFalse (statistics.m_ftOldest.has_value());
            Assert::IsFalse (statistics.m_ftNewest.has_value());
        }




        TEST_METHOD(Compute_EncryptionCounts)
        {
            SArchiveStatistics statistics = Compute ({ File (L"a.txt", 1, 0, true), File (L"b.txt", 1) }, true);

            Assert::AreEqual (1ull, statistics.m_cEncrypted);
            Assert::IsTrue   (statistics.m_fListingEncrypted);
        }




        TEST_METHOD(Compute_ExtensionGroups_LowercaseSortedBySizeThenExtension)
        {
            SArchiveStatistics statistics = Compute ({
                File (L"a.TXT", 5),
                File (L"b.txt", 5),
                File (L"c.cpp", 10),
                File (L"Makefile", 10),
                File (L".gitignore", 1),
                File (L"z.h", 2),
            });

            const vector<SExtensionGroup> & groups = statistics.m_vExtensions;

            Assert::AreEqual (size_t (4), groups.size());
            Assert::AreEqual (L"(none)", groups[0].m_strExtension.c_str());   // 11 bytes: Makefile and .gitignore
            Assert::AreEqual (2ull, groups[0].m_cFiles);
            Assert::AreEqual (L".cpp", groups[1].m_strExtension.c_str());
            Assert::AreEqual (L".txt", groups[2].m_strExtension.c_str());
            Assert::AreEqual (2ull, groups[2].m_cFiles);
            Assert::AreEqual (10ull, groups[2].m_ullSize);
            Assert::AreEqual (L".h", groups[3].m_strExtension.c_str());
        }
    };
}
