#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/ArchiveTree.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(ArchiveTreeTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        TEST_METHOD(Build_EmptyEntryList_ContainsOnlyRoot)
        {
            unique_ptr<CArchiveTree>   tree;
            HRESULT                    hr   = S_OK;



            hr = CArchiveTree::Build ({}, tree);

            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (size_t (1), tree->NodeCount());
            Assert::IsTrue   (tree->Root().m_vChildren.empty());
            Assert::IsTrue   (tree->Root().m_fDirectory);
            Assert::IsTrue   (tree->FindFolder (L"") == &tree->Root());
        }




        TEST_METHOD(Build_NestedFile_CreatesImpliedFolders)
        {
            unique_ptr<CArchiveTree>      tree     = BuildTree ({ MakeFile (L"a/b/c.txt", 10, 1) });
            const CArchiveTree::SNode   * pFolderA = tree->FindFolder (L"a");
            const CArchiveTree::SNode   * pFolderB = tree->FindFolder (L"a/b");



            Assert::IsNotNull (pFolderA);
            Assert::IsNotNull (pFolderB);
            Assert::IsTrue    (pFolderA->m_fImplied);
            Assert::IsTrue    (pFolderB->m_fImplied);
            Assert::AreEqual  (DWORD (FILE_ATTRIBUTE_DIRECTORY), pFolderA->m_dwAttributes);
            Assert::AreEqual  (size_t (1), pFolderB->m_vChildren.size());
            Assert::AreEqual  (L"c.txt", ChildNames (*tree, *pFolderB, L"*")[0].c_str());
        }




        TEST_METHOD(Build_ExplicitFolderAfterImplied_UpdatesTimeAndAttributes)
        {
            unique_ptr<CArchiveTree>      tree;
            const CArchiveTree::SNode   * pFolder = nullptr;
            SArchiveEntry                 folder  = MakeFolder (L"dir/", 5);



            folder.m_dwAttributes = FILE_ATTRIBUTE_HIDDEN;

            tree    = BuildTree ({ MakeFile (L"dir/a.txt", 1, 9), folder });
            pFolder = tree->FindFolder (L"dir");

            Assert::IsNotNull (pFolder);
            Assert::IsFalse   (pFolder->m_fImplied);
            Assert::AreEqual  (DWORD (5), pFolder->m_ftLastWrite.dwLowDateTime);
            Assert::AreEqual  (DWORD (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_DIRECTORY), pFolder->m_dwAttributes);
            Assert::AreEqual  (size_t (1), tree->Root().m_vChildren.size());
        }




        TEST_METHOD(Build_ImpliedFolder_GetsNewestChildTime)
        {
            unique_ptr<CArchiveTree>      tree     = BuildTree ({ MakeFile (L"x/y/a.txt", 1, 50),
                                                                  MakeFile (L"x/b.txt",   1, 30),
                                                                  MakeFile (L"x/y/c.txt", 1, 20) });
            const CArchiveTree::SNode   * pFolderX = tree->FindFolder (L"x");
            const CArchiveTree::SNode   * pFolderY = tree->FindFolder (L"x/y");



            Assert::AreEqual (DWORD (50), pFolderY->m_ftLastWrite.dwLowDateTime);
            Assert::AreEqual (DWORD (50), pFolderX->m_ftLastWrite.dwLowDateTime);
        }




        TEST_METHOD(Build_BothSeparators_ShareFolders)
        {
            unique_ptr<CArchiveTree>      tree    = BuildTree ({ MakeFile (L"dir/a.txt",  1, 1),
                                                                 MakeFile (L"dir\\b.txt", 1, 1) });
            const CArchiveTree::SNode   * pFolder = tree->FindFolder (L"dir");



            Assert::IsNotNull (pFolder);
            Assert::AreEqual  (size_t (2), pFolder->m_vChildren.size());
            Assert::IsTrue    (tree->FindFolder (L"dir\\") == pFolder);
        }




        TEST_METHOD(SplitPath_DropsDotsDrivesAndRoots)
        {
            vector<wstring>   components;
            bool              fHasPath   = false;



            fHasPath = CArchiveTree::SplitPath (L"C:\\x\\..\\y\\.\\z.txt", components);

            Assert::IsTrue   (fHasPath);
            Assert::AreEqual (size_t (3), components.size());
            Assert::AreEqual (L"x",     components[0].c_str());
            Assert::AreEqual (L"y",     components[1].c_str());
            Assert::AreEqual (L"z.txt", components[2].c_str());

            fHasPath = CArchiveTree::SplitPath (L"//abs/file.txt", components);

            Assert::IsTrue   (fHasPath);
            Assert::AreEqual (size_t (2), components.size());
            Assert::AreEqual (L"abs", components[0].c_str());

            fHasPath = CArchiveTree::SplitPath (L"../.", components);

            Assert::IsFalse  (fHasPath);
            Assert::IsTrue   (components.empty());
        }




        TEST_METHOD(Build_SkippedComponents_StayInsideRoot)
        {
            unique_ptr<CArchiveTree> tree = BuildTree ({ MakeFile (L"../../evil.txt",  1, 1),
                                                         MakeFile (L"C:/tools/x.dll",  1, 1),
                                                         MakeFile (L"/abs/./file.txt", 1, 1),
                                                         MakeFile (L"..",              1, 1) });
            vector<wstring>          names = ChildNames (*tree, tree->Root(), L"*");



            Assert::AreEqual (size_t (3), names.size());
            Assert::AreEqual (L"evil.txt", names[0].c_str());
            Assert::AreEqual (L"tools",    names[1].c_str());
            Assert::AreEqual (L"abs",      names[2].c_str());
            Assert::IsNotNull (tree->FindFolder (L"abs"));
        }




        TEST_METHOD(Build_DuplicateFiles_AreKept)
        {
            unique_ptr<CArchiveTree> tree  = BuildTree ({ MakeFile (L"dup.txt", 1, 1),
                                                          MakeFile (L"dup.txt", 2, 2) });
            vector<wstring>          names = ChildNames (*tree, tree->Root(), L"*");



            Assert::AreEqual (size_t (2), names.size());
            Assert::AreEqual (L"dup.txt", names[0].c_str());
            Assert::AreEqual (L"dup.txt", names[1].c_str());
        }




        TEST_METHOD(FindFolder_IsCaseInsensitive)
        {
            unique_ptr<CArchiveTree>      tree    = BuildTree ({ MakeFile (L"Docs/a.txt", 1, 1),
                                                                 MakeFile (L"docs/b.txt", 1, 1) });
            const CArchiveTree::SNode   * pFolder = tree->FindFolder (L"DOCS");



            Assert::IsNotNull (pFolder);
            Assert::AreEqual  (L"Docs", pFolder->m_strName.c_str());
            Assert::AreEqual  (size_t (2), pFolder->m_vChildren.size());
            Assert::AreEqual  (size_t (1), tree->Root().m_vChildren.size());
        }




        TEST_METHOD(FindFolder_ThroughFile_ReturnsNull)
        {
            unique_ptr<CArchiveTree> tree = BuildTree ({ MakeFile (L"a/file.txt", 1, 1) });



            Assert::IsNull (tree->FindFolder (L"a/file.txt"));
            Assert::IsNull (tree->FindFolder (L"a/file.txt/x"));
            Assert::IsNull (tree->FindFolder (L"missing"));
        }




        TEST_METHOD(EnumerateChildren_FiltersBySpecInArchiveOrder)
        {
            unique_ptr<CArchiveTree> tree  = BuildTree ({ MakeFile (L"c.txt",     1, 1),
                                                          MakeFile (L"b.log",     1, 1),
                                                          MakeFile (L"a.txt",     1, 1),
                                                          MakeFile (L"sub/x.txt", 1, 1) });
            vector<wstring>          names = ChildNames (*tree, tree->Root(), L"*.txt");



            Assert::AreEqual (size_t (2), names.size());
            Assert::AreEqual (L"c.txt", names[0].c_str());
            Assert::AreEqual (L"a.txt", names[1].c_str());
        }




        TEST_METHOD(EnumerateChildren_CallbackFalse_StopsEnumeration)
        {
            unique_ptr<CArchiveTree> tree   = BuildTree ({ MakeFile (L"a.txt", 1, 1),
                                                           MakeFile (L"b.txt", 1, 1),
                                                           MakeFile (L"c.txt", 1, 1) });
            int                      cCalls = 0;



            tree->EnumerateChildren (tree->Root(), L"*", [&cCalls] (const WIN32_FIND_DATA &)
            {
                ++cCalls;
                return false;
            });

            Assert::AreEqual (1, cCalls);
        }




        TEST_METHOD(ToFindData_LargeFile_SplitsSize)
        {
            CArchiveTree::SNode node;
            WIN32_FIND_DATA     wfd  = {};



            node.m_strName     = L"big.bin";
            node.m_ullSize     = 0x123456789ull;
            node.m_ftLastWrite = { 0x11111111, 0x22222222 };

            CArchiveTree::ToFindData (node, wfd);

            Assert::AreEqual (DWORD (0x1),        wfd.nFileSizeHigh);
            Assert::AreEqual (DWORD (0x23456789), wfd.nFileSizeLow);
            Assert::AreEqual (L"big.bin",         wfd.cFileName);
        }




        TEST_METHOD(ToFindData_AllTimesEqualLastWrite)
        {
            CArchiveTree::SNode node;
            WIN32_FIND_DATA     wfd  = {};



            node.m_strName     = L"file.txt";
            node.m_ftLastWrite = { 0x11111111, 0x22222222 };

            CArchiveTree::ToFindData (node, wfd);

            Assert::AreEqual (0L, CompareFileTime (&node.m_ftLastWrite, &wfd.ftLastWriteTime));
            Assert::AreEqual (0L, CompareFileTime (&node.m_ftLastWrite, &wfd.ftCreationTime));
            Assert::AreEqual (0L, CompareFileTime (&node.m_ftLastWrite, &wfd.ftLastAccessTime));
        }




        TEST_METHOD(ToFindData_NoAttributes_ReturnsNormal)
        {
            CArchiveTree::SNode node;
            WIN32_FIND_DATA     wfd  = {};



            node.m_strName = L"file.txt";

            CArchiveTree::ToFindData (node, wfd);

            Assert::AreEqual (DWORD (FILE_ATTRIBUTE_NORMAL), wfd.dwFileAttributes);
        }




        TEST_METHOD(ToFindData_Folder_HasDirectoryAttributeAndNoSize)
        {
            CArchiveTree::SNode node;
            WIN32_FIND_DATA     wfd  = {};



            node.m_strName      = L"folder";
            node.m_fDirectory   = true;
            node.m_ullSize      = 1234;
            node.m_dwAttributes = FILE_ATTRIBUTE_READONLY;

            CArchiveTree::ToFindData (node, wfd);

            Assert::AreEqual (DWORD (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_DIRECTORY), wfd.dwFileAttributes);
            Assert::AreEqual (DWORD (0), wfd.nFileSizeLow);
            Assert::AreEqual (DWORD (0), wfd.nFileSizeHigh);
        }




        TEST_METHOD(ToFindData_LongName_IsTruncated)
        {
            CArchiveTree::SNode node;
            WIN32_FIND_DATA     wfd  = {};



            node.m_strName = wstring (MAX_PATH * 2, L'n');

            CArchiveTree::ToFindData (node, wfd);

            Assert::AreEqual (size_t (MAX_PATH - 1), wcsnlen (wfd.cFileName, MAX_PATH));
            Assert::AreEqual (L'\0', wfd.cFileName[MAX_PATH - 1]);
        }




    private:

        ////////////////////////////////////////////////////////////////////////////////
        //
        //  ArchiveTreeTests::MakeFile
        //
        //  The low part of the last-write time is dwTime, so tests can compare
        //  times through dwLowDateTime.
        //
        ////////////////////////////////////////////////////////////////////////////////

        static SArchiveEntry MakeFile (const wstring & path, ULONGLONG ullSize, DWORD dwTime)
        {
            SArchiveEntry entry;



            entry.m_strPath                   = path;
            entry.m_ullSize                   = ullSize;
            entry.m_ftLastWrite.dwLowDateTime = dwTime;

            return entry;
        }




        ////////////////////////////////////////////////////////////////////////////////
        //
        //  ArchiveTreeTests::MakeFolder
        //
        ////////////////////////////////////////////////////////////////////////////////

        static SArchiveEntry MakeFolder (const wstring & path, DWORD dwTime)
        {
            SArchiveEntry entry = MakeFile (path, 0, dwTime);



            entry.m_fDirectory = true;

            return entry;
        }




        ////////////////////////////////////////////////////////////////////////////////
        //
        //  ArchiveTreeTests::BuildTree
        //
        ////////////////////////////////////////////////////////////////////////////////

        static unique_ptr<CArchiveTree> BuildTree (const vector<SArchiveEntry> & entries)
        {
            unique_ptr<CArchiveTree>   tree;
            HRESULT                    hr   = S_OK;



            hr = CArchiveTree::Build (entries, tree);

            Assert::AreEqual (S_OK, hr);

            return tree;
        }




        ////////////////////////////////////////////////////////////////////////////////
        //
        //  ArchiveTreeTests::ChildNames
        //
        //  Returns the names of the children of folder that match spec, in
        //  enumeration order.
        //
        ////////////////////////////////////////////////////////////////////////////////

        static vector<wstring> ChildNames (const CArchiveTree & tree, const CArchiveTree::SNode & folder, wstring_view spec)
        {
            vector<wstring> names;



            tree.EnumerateChildren (folder, spec, [&names] (const WIN32_FIND_DATA & wfd)
            {
                names.push_back (wfd.cFileName);
                return true;
            });

            return names;
        }
    };
}
