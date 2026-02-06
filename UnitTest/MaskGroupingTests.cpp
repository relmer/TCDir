#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/MaskGrouper.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    ////////////////////////////////////////////////////////////////////////////
    //
    //  MaskGroupingTests
    //
    //  Tests for GroupMasksByDirectory() which converts a list of masks into
    //  groups by target directory.
    //
    //  Input:  { L"*.cpp", L"*.h", L"foo\\*.txt", L"bar\\" }
    //  Output: vector<pair<path, vector<wstring>>> groups
    //  [
    //    { "C:\\cwd\\",      { "*.cpp", "*.h" } },   // pure masks grouped
    //    { "C:\\cwd\\foo\\", { "*.txt" } },          // directory-qualified
    //    { "C:\\cwd\\bar\\", { "*" } }               // directory only
    //  ]
    //
    ////////////////////////////////////////////////////////////////////////////

    TEST_CLASS(MaskGroupingTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  IsPureMask tests
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(IsPureMask_SimpleWildcard_ReturnsTrue)
        {
            Assert::IsTrue (CMaskGrouper::IsPureMask (L"*.cpp"));
            Assert::IsTrue (CMaskGrouper::IsPureMask (L"*.h"));
            Assert::IsTrue (CMaskGrouper::IsPureMask (L"*"));
            Assert::IsTrue (CMaskGrouper::IsPureMask (L"foo.txt"));
            Assert::IsTrue (CMaskGrouper::IsPureMask (L"*.?pp"));
        }

        TEST_METHOD(IsPureMask_WithBackslash_ReturnsFalse)
        {
            Assert::IsFalse (CMaskGrouper::IsPureMask (L"foo\\*.cpp"));
            Assert::IsFalse (CMaskGrouper::IsPureMask (L"sub\\file.txt"));
            Assert::IsFalse (CMaskGrouper::IsPureMask (L"a\\b\\c.txt"));
        }

        TEST_METHOD(IsPureMask_WithForwardSlash_ReturnsFalse)
        {
            Assert::IsFalse (CMaskGrouper::IsPureMask (L"foo/*.cpp"));
            Assert::IsFalse (CMaskGrouper::IsPureMask (L"sub/file.txt"));
        }

        TEST_METHOD(IsPureMask_DotSlashPrefix_ReturnsFalse)
        {
            // .\*.cpp has a path separator, so not pure
            Assert::IsFalse (CMaskGrouper::IsPureMask (L".\\*.cpp"));
            Assert::IsFalse (CMaskGrouper::IsPureMask (L"./file.txt"));
        }

        TEST_METHOD(IsPureMask_AbsolutePath_ReturnsFalse)
        {
            Assert::IsFalse (CMaskGrouper::IsPureMask (L"C:\\foo\\*.cpp"));
            Assert::IsFalse (CMaskGrouper::IsPureMask (L"D:\\file.txt"));
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  GroupMasksByDirectory tests
        //
        ////////////////////////////////////////////////////////////////////////

        TEST_METHOD(GroupMasks_SinglePureMask_OneGroupWithOneMask)
        {
            list<wstring> masks = { L"*.cpp" };
            
            auto groups = CMaskGrouper::GroupMasksByDirectory (masks);
            
            Assert::AreEqual (1ull, groups.size(), L"Should have 1 group");
            Assert::AreEqual (1ull, groups[0].second.size(), L"Group should have 1 mask");
            Assert::AreEqual (wstring (L"*.cpp"), groups[0].second[0].wstring());
        }

        TEST_METHOD(GroupMasks_MultiplePureMasks_OneGroupWithAllMasks)
        {
            list<wstring> masks = { L"*.cpp", L"*.h", L"*.hpp" };
            
            auto groups = CMaskGrouper::GroupMasksByDirectory (masks);
            
            Assert::AreEqual (1ull, groups.size(), L"Should have 1 group");
            Assert::AreEqual (3ull, groups[0].second.size(), L"Group should have 3 masks");
            Assert::AreEqual (wstring (L"*.cpp"), groups[0].second[0].wstring());
            Assert::AreEqual (wstring (L"*.h"),   groups[0].second[1].wstring());
            Assert::AreEqual (wstring (L"*.hpp"), groups[0].second[2].wstring());
        }

        TEST_METHOD(GroupMasks_SingleDirectoryQualifiedMask_OneGroup)
        {
            list<wstring> masks = { L"foo\\*.cpp" };
            
            auto groups = CMaskGrouper::GroupMasksByDirectory (masks);
            
            Assert::AreEqual (1ull, groups.size(), L"Should have 1 group");
            Assert::AreEqual (1ull, groups[0].second.size(), L"Group should have 1 mask");
            Assert::AreEqual (wstring (L"*.cpp"), groups[0].second[0].wstring());
            
            // Directory path should end with foo
            wstring dirStr = groups[0].first.wstring();
            Assert::IsTrue (dirStr.find (L"foo") != wstring::npos, L"Dir should contain 'foo'");
        }

        TEST_METHOD(GroupMasks_MixedPureAndDirectoryQualified_MultipleGroups)
        {
            // tcdir *.cpp foo\*.h
            // Expected: group1 = (cwd, {*.cpp}), group2 = (foo, {*.h})
            list<wstring> masks = { L"*.cpp", L"foo\\*.h" };
            
            auto groups = CMaskGrouper::GroupMasksByDirectory (masks);
            
            Assert::AreEqual (2ull, groups.size(), L"Should have 2 groups");
            
            // First group: pure mask in CWD
            Assert::AreEqual (1ull, groups[0].second.size());
            Assert::AreEqual (wstring (L"*.cpp"), groups[0].second[0].wstring());
            
            // Second group: directory-qualified
            Assert::AreEqual (1ull, groups[1].second.size());
            Assert::AreEqual (wstring (L"*.h"), groups[1].second[0].wstring());
        }

        TEST_METHOD(GroupMasks_SameDirectoryDifferentMasks_CombinedIntoOneGroup)
        {
            // tcdir foo\*.cpp foo\*.h
            // Expected: group1 = (foo, {*.cpp, *.h})
            list<wstring> masks = { L"foo\\*.cpp", L"foo\\*.h" };
            
            auto groups = CMaskGrouper::GroupMasksByDirectory (masks);
            
            Assert::AreEqual (1ull, groups.size(), L"Should have 1 group (same directory)");
            Assert::AreEqual (2ull, groups[0].second.size(), L"Group should have 2 masks");
            Assert::AreEqual (wstring (L"*.cpp"), groups[0].second[0].wstring());
            Assert::AreEqual (wstring (L"*.h"),   groups[0].second[1].wstring());
        }

        TEST_METHOD(GroupMasks_DirectoryOnlyNoMask_UsesStarMask)
        {
            // tcdir bar\ (directory only, no mask)
            list<wstring> masks = { L"bar\\" };
            
            auto groups = CMaskGrouper::GroupMasksByDirectory (masks);
            
            Assert::AreEqual (1ull, groups.size());
            Assert::AreEqual (1ull, groups[0].second.size());
            Assert::AreEqual (wstring (L"*"), groups[0].second[0].wstring(), L"Should default to *");
        }

        TEST_METHOD(GroupMasks_EmptyInput_ReturnsCurrentDirWithStar)
        {
            list<wstring> masks;
            
            auto groups = CMaskGrouper::GroupMasksByDirectory (masks);
            
            Assert::AreEqual (1ull, groups.size(), L"Should have 1 group for CWD");
            Assert::AreEqual (1ull, groups[0].second.size());
            Assert::AreEqual (wstring (L"*"), groups[0].second[0].wstring(), L"Should default to *");
        }

        TEST_METHOD(GroupMasks_OrderPreserved_FirstSeenDirectoryFirst)
        {
            // tcdir foo\*.cpp *.h bar\*.txt
            // Expected order: foo, cwd, bar (order of first occurrence)
            list<wstring> masks = { L"foo\\*.cpp", L"*.h", L"bar\\*.txt" };
            
            auto groups = CMaskGrouper::GroupMasksByDirectory (masks);
            
            Assert::AreEqual (3ull, groups.size(), L"Should have 3 groups");
            
            // First group should be foo
            Assert::IsTrue (groups[0].first.wstring().find (L"foo") != wstring::npos, 
                           L"First group should be 'foo' directory");
            
            // Second group should be CWD (with *.h)
            Assert::AreEqual (wstring (L"*.h"), groups[1].second[0].wstring());
            
            // Third group should be bar
            Assert::IsTrue (groups[2].first.wstring().find (L"bar") != wstring::npos,
                           L"Third group should be 'bar' directory");
        }

        TEST_METHOD(GroupMasks_MultipleDifferentDirectories_SeparateGroups)
        {
            // tcdir foo bar (directories only, with trailing slashes)
            list<wstring> masks = { L"foo\\", L"bar\\" };
            
            auto groups = CMaskGrouper::GroupMasksByDirectory (masks);
            
            Assert::AreEqual (2ull, groups.size(), L"Should have 2 groups");
        }

        TEST_METHOD(GroupMasks_DuplicateMasksInSameDir_AllIncluded)
        {
            // tcdir *.cpp *.cpp (user specified same mask twice)
            // We include both - deduplication happens at file enumeration time
            list<wstring> masks = { L"*.cpp", L"*.cpp" };
            
            auto groups = CMaskGrouper::GroupMasksByDirectory (masks);
            
            Assert::AreEqual (1ull, groups.size());
            Assert::AreEqual (2ull, groups[0].second.size(), L"Both masks included");
        }

    };
}
