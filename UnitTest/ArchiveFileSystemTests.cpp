#include "pch.h"
#include "EhmTestHelper.h"
#include "Mocks/MockArchiveFileAccess.h"

#include "../TCDirCore/ArchiveFileSystem.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(ArchiveFileSystemTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  MakeFindData
        //
        ////////////////////////////////////////////////////////////////////////

        static WIN32_FIND_DATA MakeFindData (LPCWSTR pszName, DWORD dwAttributes)
        {
            WIN32_FIND_DATA wfd = {};

            wcscpy_s (wfd.cFileName, pszName);
            wfd.dwFileAttributes = dwAttributes;
            return wfd;
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  MakeFileSystem
        //
        //  C:\Root\release.zip contains readme.txt and docs\guide.md, and
        //  C:\Root\folder.zip is a real directory.
        //
        ////////////////////////////////////////////////////////////////////////

        static shared_ptr<CMockArchiveFileAccess> MakeAccess()
        {
            auto accessPtr = make_shared<CMockArchiveFileAccess>();

            accessPtr->AddDirectory (L"C:\\Root");
            accessPtr->AddDirectory (L"C:\\Root\\folder.zip");
            accessPtr->AddFile      (L"C:\\Root\\notes.txt");
            accessPtr->AddArchive   (L"C:\\Root\\release.zip",
                                     { CMockArchiveFileAccess::File (L"readme.txt",    3),
                                       CMockArchiveFileAccess::File (L"docs/guide.md", 4) });
            return accessPtr;
        }




        TEST_METHOD(IsArchiveFile_RecognizedExtension_True)
        {
            CArchiveFileSystem fs (MakeAccess());

            Assert::IsTrue (fs.IsArchiveFile (MakeFindData (L"release.ZIP", FILE_ATTRIBUTE_ARCHIVE)));
            Assert::IsTrue (fs.IsArchiveFile (MakeFindData (L"src.tar.gz",  FILE_ATTRIBUTE_NORMAL)));
        }




        TEST_METHOD(IsArchiveFile_DirectoryOrOtherExtension_False)
        {
            CArchiveFileSystem fs (MakeAccess());

            Assert::IsFalse (fs.IsArchiveFile (MakeFindData (L"folder.zip", FILE_ATTRIBUTE_DIRECTORY)));
            Assert::IsFalse (fs.IsArchiveFile (MakeFindData (L"report.docx", FILE_ATTRIBUTE_ARCHIVE)));
        }




        TEST_METHOD(IsArchiveFile_CloudPlaceholder_False)
        {
            CArchiveFileSystem fs (MakeAccess());

            Assert::IsFalse (fs.IsArchiveFile (MakeFindData (L"a.zip", FILE_ATTRIBUTE_OFFLINE)));
            Assert::IsFalse (fs.IsArchiveFile (MakeFindData (L"b.zip", FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS)));
        }




        TEST_METHOD(IsArchiveFile_LibArchiveFormatWithoutDll_False)
        {
            auto accessPtr = MakeAccess();

            accessPtr->m_fLibArchiveAvailable = false;

            CArchiveFileSystem fs (accessPtr);

            Assert::IsFalse (fs.IsArchiveFile (MakeFindData (L"a.7z", FILE_ATTRIBUTE_ARCHIVE)));
            Assert::IsTrue  (fs.IsArchiveFile (MakeFindData (L"a.zip", FILE_ATTRIBUTE_ARCHIVE)));
        }




        TEST_METHOD(TrySplit_ArchiveRoot)
        {
            CArchiveFileSystem fs (MakeAccess());
            SArchiveLocation   location;

            Assert::IsTrue (fs.TrySplit (L"C:\\Root\\release.zip", location));
            Assert::AreEqual (L"C:\\Root\\release.zip", location.m_archivePath.c_str());
            Assert::IsTrue (location.m_strInnerPath.empty());
        }




        TEST_METHOD(TrySplit_InnerPath)
        {
            CArchiveFileSystem fs (MakeAccess());
            SArchiveLocation   location;

            Assert::IsTrue (fs.TrySplit (L"C:\\Root\\release.zip\\docs\\api\\", location));
            Assert::AreEqual (L"C:\\Root\\release.zip", location.m_archivePath.c_str());
            Assert::AreEqual (L"docs\\api", location.m_strInnerPath.c_str());
        }




        TEST_METHOD(TrySplit_RealDirectoryWithArchiveExtension_False)
        {
            CArchiveFileSystem fs (MakeAccess());
            SArchiveLocation   location;

            Assert::IsFalse (fs.TrySplit (L"C:\\Root\\folder.zip", location));
            Assert::IsFalse (fs.TrySplit (L"C:\\Root\\folder.zip\\sub", location));
        }




        TEST_METHOD(TrySplit_NonArchiveFileOrMissing_False)
        {
            CArchiveFileSystem fs (MakeAccess());
            SArchiveLocation   location;

            Assert::IsFalse (fs.TrySplit (L"C:\\Root\\notes.txt\\x", location));
            Assert::IsFalse (fs.TrySplit (L"C:\\Missing\\a.zip", location));
        }




        TEST_METHOD(Resolve_OnlyRegisteredArchives)
        {
            CArchiveFileSystem fs (MakeAccess());
            SArchiveLocation   location;

            Assert::IsFalse (fs.Resolve (L"C:\\Root\\release.zip\\docs", location));

            fs.RegisterArchive (L"C:\\Root\\RELEASE.zip");

            Assert::IsTrue (fs.Resolve (L"c:\\root\\release.zip\\docs", location));
            Assert::AreEqual (L"docs", location.m_strInnerPath.c_str());
            Assert::IsTrue (fs.IsRegisteredArchive (L"C:\\Root\\release.zip"));
        }




        TEST_METHOD(IsDirectory_RootFolderAndFile)
        {
            CArchiveFileSystem fs (MakeAccess());

            Assert::IsTrue  (fs.IsDirectory (L"C:\\Root\\release.zip"));
            Assert::IsTrue  (fs.IsDirectory (L"C:\\Root\\release.zip\\DOCS"));
            Assert::IsFalse (fs.IsDirectory (L"C:\\Root\\release.zip\\readme.txt"));
            Assert::IsFalse (fs.IsDirectory (L"C:\\Root\\release.zip\\missing"));
        }




        TEST_METHOD(IsDirectory_BrokenArchiveRoot_True)
        {
            auto accessPtr = MakeAccess();

            accessPtr->AddBrokenArchive (L"C:\\Root\\bad.zip", HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

            CArchiveFileSystem fs (accessPtr);

            Assert::IsTrue  (fs.IsDirectory (L"C:\\Root\\bad.zip"));
            Assert::IsFalse (fs.IsDirectory (L"C:\\Root\\bad.zip\\sub"));
        }




        TEST_METHOD(GetTree_ParsesOnce)
        {
            auto                             accessPtr = MakeAccess();
            CArchiveFileSystem               fs          (accessPtr);
            shared_ptr<const CArchiveTree>   treePtr;
            wstring                          strError;

            Assert::IsTrue (SUCCEEDED (fs.GetTree (L"C:\\Root\\release.zip", treePtr, strError)));
            Assert::IsTrue (SUCCEEDED (fs.GetTree (L"c:\\root\\RELEASE.ZIP", treePtr, strError)));

            Assert::AreEqual (1, accessPtr->m_cReads.load());
            Assert::IsNotNull (treePtr->FindFolder (L"docs"));
            Assert::IsTrue (accessPtr->m_lastFormat == EArchiveFormat::Zip);
        }




        TEST_METHOD(GetTree_ConcurrentCallsParseOnce)
        {
            auto                 accessPtr = MakeAccess();
            CArchiveFileSystem   fs          (accessPtr);
            vector<thread>       threads;
            atomic<int>          cSucceeded  = 0;

            for (int i = 0; i < 8; ++i)
            {
                threads.emplace_back ([&] ()
                {
                    shared_ptr<const CArchiveTree> treePtr;
                    wstring                        strError;

                    if (SUCCEEDED (fs.GetTree (L"C:\\Root\\release.zip", treePtr, strError)) && treePtr)
                    {
                        ++cSucceeded;
                    }
                });
            }

            for (auto & t : threads)
            {
                t.join();
            }

            Assert::AreEqual (8, cSucceeded.load());
            Assert::AreEqual (1, accessPtr->m_cReads.load());
        }




        TEST_METHOD(GetTree_FailureCachedWithMessage)
        {
            auto accessPtr = MakeAccess();

            accessPtr->AddBrokenArchive (L"C:\\Root\\bad.zip", HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

            CArchiveFileSystem               fs (accessPtr);
            shared_ptr<const CArchiveTree>   treePtr;
            wstring                          strError;
            HRESULT                          hr = fs.GetTree (L"C:\\Root\\bad.zip", treePtr, strError);

            Assert::IsTrue (hr == HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));
            Assert::AreEqual (L" is not a valid archive", strError.c_str());

            hr = fs.GetTree (L"C:\\Root\\bad.zip", treePtr, strError);

            Assert::IsTrue (FAILED (hr));
            Assert::AreEqual (1, accessPtr->m_cReads.load());
        }




        TEST_METHOD(GetTree_EncryptedListing)
        {
            auto accessPtr = MakeAccess();

            accessPtr->AddEncryptedListingArchive (L"C:\\Root\\secret.7z");

            CArchiveFileSystem               fs (accessPtr);
            shared_ptr<const CArchiveTree>   treePtr;
            wstring                          strError;
            HRESULT                          hr = fs.GetTree (L"C:\\Root\\secret.7z", treePtr, strError);

            Assert::IsTrue (hr == HRESULT_FROM_WIN32 (ERROR_FILE_ENCRYPTED));
            Assert::IsTrue (strError.find (L"is encrypted") != wstring::npos);
        }




        TEST_METHOD(GetTree_OfflinePlaceholder_NotOpened)
        {
            auto accessPtr = MakeAccess();

            accessPtr->AddArchive (L"C:\\Root\\cloud.zip", {}, FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS);

            CArchiveFileSystem               fs (accessPtr);
            SArchiveLocation                 location;
            shared_ptr<const CArchiveTree>   treePtr;
            wstring                          strError;

            Assert::IsTrue (fs.TrySplit (L"C:\\Root\\cloud.zip", location));

            HRESULT hr = fs.GetTree (L"C:\\Root\\cloud.zip", treePtr, strError);

            Assert::IsTrue (hr == HRESULT_FROM_WIN32 (ERROR_FILE_OFFLINE));
            Assert::AreEqual (L" is not available offline", strError.c_str());
            Assert::AreEqual (0, accessPtr->m_cReads.load());
        }




        TEST_METHOD(GetTree_LibArchiveFormatWithoutDll_Windows11Message)
        {
            auto accessPtr = MakeAccess();

            accessPtr->m_fLibArchiveAvailable = false;
            accessPtr->AddArchive (L"C:\\Root\\a.7z", {});

            CArchiveFileSystem               fs (accessPtr);
            SArchiveLocation                 location;
            shared_ptr<const CArchiveTree>   treePtr;
            wstring                          strError;

            Assert::IsTrue (fs.TrySplit (L"C:\\Root\\a.7z", location));

            HRESULT hr = fs.GetTree (L"C:\\Root\\a.7z", treePtr, strError);

            Assert::IsTrue (hr == HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED));
            Assert::IsTrue (strError.find (L"Windows 11 23H2") != wstring::npos);
            Assert::AreEqual (0, accessPtr->m_cReads.load());
        }




        TEST_METHOD(FormatError_Variants)
        {
            Assert::AreEqual (L" uses an unsupported format variant",
                              CArchiveFileSystem::FormatError (HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED), EArchiveFormat::Iso, true).c_str());

            wstring strIoError = CArchiveFileSystem::FormatError (HRESULT_FROM_WIN32 (ERROR_ACCESS_DENIED), EArchiveFormat::Zip, true);

            Assert::IsTrue (strIoError.starts_with (L": "));
            Assert::IsFalse (iswspace (strIoError.back()));
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  MakeNestedAccess
        //
        //  C:\Root\outer.zip contains pkg\inner.zip, which contains
        //  docs\guide.md and deep.zip; deep.zip contains deep.txt.
        //
        ////////////////////////////////////////////////////////////////////////

        static shared_ptr<CMockArchiveFileAccess> MakeNestedAccess (ULONGLONG cbInner = 100)
        {
            auto accessPtr = make_shared<CMockArchiveFileAccess>();

            accessPtr->AddDirectory (L"C:\\Root");
            accessPtr->AddArchive   (L"C:\\Root\\outer.zip",
                                     { CMockArchiveFileAccess::File (L"pkg/inner.zip", cbInner) });
            accessPtr->AddArchive   (L"C:\\Root\\outer.zip\\pkg\\inner.zip",
                                     { CMockArchiveFileAccess::File (L"docs/guide.md", 4),
                                       CMockArchiveFileAccess::File (L"deep.zip",      50) });
            accessPtr->AddArchive   (L"C:\\Root\\outer.zip\\pkg\\inner.zip\\deep.zip",
                                     { CMockArchiveFileAccess::File (L"deep.txt", 9) });
            return accessPtr;
        }




        TEST_METHOD(IsNestedArchiveFile_SizeLimitAndLibArchive)
        {
            auto               accessPtr = make_shared<CMockArchiveFileAccess>();
            CArchiveFileSystem fs          (accessPtr);
            WIN32_FIND_DATA    wfd       = MakeFindData (L"inner.zip", 0);

            wfd.nFileSizeLow = 0x10000000;          // 256 MB, the limit
            Assert::IsTrue (fs.IsNestedArchiveFile (wfd));

            wfd.nFileSizeLow = 0x10000001;
            Assert::IsFalse (fs.IsNestedArchiveFile (wfd));

            wfd.nFileSizeLow  = 0;
            wfd.nFileSizeHigh = 1;
            Assert::IsFalse (fs.IsNestedArchiveFile (wfd));

            accessPtr->m_fLibArchiveAvailable = false;

            CArchiveFileSystem fsWithoutDll (accessPtr);

            wfd.nFileSizeHigh = 0;
            wfd.nFileSizeLow  = 1000;
            Assert::IsTrue  (fsWithoutDll.IsArchiveFile (wfd));
            Assert::IsFalse (fsWithoutDll.IsNestedArchiveFile (wfd));
        }




        TEST_METHOD(TrySplit_NestedArchive_LocationInInnerArchive)
        {
            CArchiveFileSystem fs (MakeNestedAccess());
            SArchiveLocation   location;

            Assert::IsTrue   (fs.TrySplit (L"C:\\Root\\outer.zip\\pkg\\inner.zip\\docs", location));
            Assert::AreEqual (L"C:\\Root\\outer.zip\\pkg\\inner.zip", location.m_archivePath.c_str());
            Assert::AreEqual (L"docs", location.m_strInnerPath.c_str());
            Assert::IsTrue   (fs.IsDirectory (L"C:\\Root\\outer.zip\\pkg\\inner.zip"));
            Assert::IsTrue   (fs.IsDirectory (L"C:\\Root\\outer.zip\\pkg\\inner.zip\\docs"));
        }




        TEST_METHOD(TrySplit_TwoLevels_LocationInInnermostArchive)
        {
            auto               accessPtr = MakeNestedAccess();
            CArchiveFileSystem fs          (accessPtr);
            SArchiveLocation   location;

            shared_ptr<const CArchiveTree> treePtr;
            wstring                        strError;

            Assert::IsTrue   (fs.TrySplit (L"C:\\Root\\outer.zip\\pkg\\inner.zip\\deep.zip", location));
            Assert::AreEqual (L"C:\\Root\\outer.zip\\pkg\\inner.zip\\deep.zip", location.m_archivePath.c_str());
            Assert::IsTrue   (location.m_strInnerPath.empty());

            Assert::IsTrue   (SUCCEEDED (fs.GetTree (location.m_archivePath, treePtr, strError)));
            Assert::AreEqual (size_t (1), treePtr->Root().m_vChildren.size());
            Assert::IsTrue   (accessPtr->m_cbLastExtractMax > 0);
        }




        TEST_METHOD(TrySplit_NestedWithoutLibArchive_StopsAtOuterArchive)
        {
            auto accessPtr = MakeNestedAccess();

            accessPtr->m_fLibArchiveAvailable = false;

            CArchiveFileSystem fs (accessPtr);
            SArchiveLocation   location;

            Assert::IsTrue   (fs.TrySplit (L"C:\\Root\\outer.zip\\pkg\\inner.zip\\docs", location));
            Assert::AreEqual (L"C:\\Root\\outer.zip", location.m_archivePath.c_str());
            Assert::IsFalse  (fs.IsDirectory (L"C:\\Root\\outer.zip\\pkg\\inner.zip\\docs"));
            Assert::AreEqual (0, accessPtr->m_cExtracts.load());
        }




        TEST_METHOD(TrySplit_NestedTooLarge_StopsAtOuterArchive)
        {
            auto               accessPtr = MakeNestedAccess (0x100000000ull);
            CArchiveFileSystem fs          (accessPtr);
            SArchiveLocation   location;

            Assert::IsTrue   (fs.TrySplit (L"C:\\Root\\outer.zip\\pkg\\inner.zip", location));
            Assert::AreEqual (L"C:\\Root\\outer.zip", location.m_archivePath.c_str());
            Assert::AreEqual (L"pkg\\inner.zip", location.m_strInnerPath.c_str());
            Assert::AreEqual (0, accessPtr->m_cExtracts.load());
        }




        TEST_METHOD(GetTree_NestedExtractFailure_CachedWithMessage)
        {
            auto               accessPtr = MakeNestedAccess();
            CArchiveFileSystem fs          (accessPtr);
            SArchiveLocation   location;

            shared_ptr<const CArchiveTree> treePtr;
            wstring                        strError;

            accessPtr->AddExtractFailure (L"C:\\Root\\outer.zip\\pkg\\inner.zip", HRESULT_FROM_WIN32 (ERROR_FILE_TOO_LARGE));

            Assert::IsTrue (fs.TrySplit (L"C:\\Root\\outer.zip\\pkg\\inner.zip", location));
            Assert::IsTrue (FAILED (fs.GetTree (location.m_archivePath, treePtr, strError)));
            Assert::AreEqual (L" is too large to browse inside another archive", strError.c_str());

            Assert::IsTrue (FAILED (fs.GetTree (location.m_archivePath, treePtr, strError)));
            Assert::AreEqual (1, accessPtr->m_cExtracts.load());
        }
    };
}
