#include "pch.h"
#include "EhmTestHelper.h"
#include "Mocks/ZipBuilder.h"
#include "../TCDirCore/ArchiveDetails.h"
#include "../TCDirCore/ZipReader.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(ZipReaderTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        TEST_METHOD(Read_EndRecordWithoutComment_ReadsEntries)
        {
            CZipBuilder          builder;
            SArchiveReadResult   result;



            builder.AddFile ("a.txt", 10).AddFile ("b.txt", 20);

            ReadZip (builder, result);

            Assert::AreEqual (size_t (2),     result.m_vEntries.size());
            Assert::AreEqual (L"a.txt",       result.m_vEntries[0].m_strPath.c_str());
            Assert::AreEqual (ULONGLONG (10), result.m_vEntries[0].m_ullSize);
            Assert::AreEqual (L"b.txt",       result.m_vEntries[1].m_strPath.c_str());
            Assert::AreEqual (ULONGLONG (20), result.m_vEntries[1].m_ullSize);
            Assert::IsFalse  (result.m_fListingEncrypted);
        }




        TEST_METHOD(Read_EndRecordWithComment_ReadsEntries)
        {
            CZipBuilder          builder;
            SArchiveReadResult   result;



            builder.AddFile ("a.txt", 10).SetComment ("Archive comment");

            ReadZip (builder, result);

            Assert::AreEqual (size_t (1), result.m_vEntries.size());
            Assert::AreEqual (L"a.txt",   result.m_vEntries[0].m_strPath.c_str());
        }




        TEST_METHOD(Read_EmptyArchive_ReturnsNoEntries)
        {
            CZipBuilder          builder;
            SArchiveReadResult   result;



            ReadZip (builder, result);

            Assert::IsTrue (result.m_vEntries.empty());
        }




        TEST_METHOD(Read_Zip64_ReadsCountOffsetAndSize)
        {
            CZipBuilder          builder;
            SArchiveReadResult   result;



            builder.AddFile ("small.txt", 10)
                   .AddFile ("huge.bin", 0x140000000ull)
                   .SetZip64EndRecord (true);

            ReadZip (builder, result);

            Assert::AreEqual (size_t (2),              result.m_vEntries.size());
            Assert::AreEqual (L"small.txt",            result.m_vEntries[0].m_strPath.c_str());
            Assert::AreEqual (ULONGLONG (10),          result.m_vEntries[0].m_ullSize);
            Assert::AreEqual (L"huge.bin",             result.m_vEntries[1].m_strPath.c_str());
            Assert::AreEqual (ULONGLONG (0x140000000), result.m_vEntries[1].m_ullSize);
        }




        TEST_METHOD(Read_Utf8Flag_DecodesUtf8Name)
        {
            CZipBuilder                  builder;
            CZipBuilder::SEntryOptions   options;
            SArchiveReadResult           result;



            options.m_strName = "caf\xC3\xA9.txt";
            options.m_fUtf8   = true;
            builder.AddEntry (options);

            ReadZip (builder, result);

            Assert::AreEqual (L"caf\x00E9.txt", result.m_vEntries[0].m_strPath.c_str());
        }




        TEST_METHOD(Read_AsciiNameWithoutFlag_DecodesName)
        {
            CZipBuilder          builder;
            SArchiveReadResult   result;



            builder.AddFile ("docs/readme.txt");

            ReadZip (builder, result);

            Assert::AreEqual (L"docs/readme.txt", result.m_vEntries[0].m_strPath.c_str());
        }




        TEST_METHOD(Read_UnicodePathExtra_OverridesHeaderName)
        {
            CZipBuilder                  builder;
            CZipBuilder::SEntryOptions   options;
            SArchiveReadResult           result;



            options.m_strName        = "fallback.txt";
            options.m_strUnicodePath = "caf\xC3\xA9.txt";
            builder.AddEntry (options);

            ReadZip (builder, result);

            Assert::AreEqual (L"caf\x00E9.txt", result.m_vEntries[0].m_strPath.c_str());
        }




        TEST_METHOD(Read_NtfsExtra_UsesUtcTimeExactly)
        {
            CZipBuilder                  builder;
            CZipBuilder::SEntryOptions   options;
            SArchiveReadResult           result;



            options.m_strName          = "file.txt";
            options.m_wDosDate         = CZipBuilder::DosDate (2001, 2, 3);
            options.m_wDosTime         = CZipBuilder::DosTime (4, 5, 6);
            options.m_fNtfsTime        = true;
            options.m_ullNtfsLastWrite = 0x01DA123456789ABCull;
            builder.AddEntry (options);

            ReadZip (builder, result);

            Assert::AreEqual (DWORD (0x56789ABC), result.m_vEntries[0].m_ftLastWrite.dwLowDateTime);
            Assert::AreEqual (DWORD (0x01DA1234), result.m_vEntries[0].m_ftLastWrite.dwHighDateTime);
        }




        TEST_METHOD(Read_DosTime_ProducesNonZeroTime)
        {
            CZipBuilder                  builder;
            CZipBuilder::SEntryOptions   options;
            SArchiveReadResult           result;



            options.m_strName  = "file.txt";
            options.m_wDosDate = CZipBuilder::DosDate (2001, 2, 3);
            options.m_wDosTime = CZipBuilder::DosTime (4, 5, 6);
            builder.AddEntry (options);

            ReadZip (builder, result);

            Assert::AreNotEqual (DWORD (0), result.m_vEntries[0].m_ftLastWrite.dwHighDateTime);
        }




        TEST_METHOD(DosToSystemTime_SplitsBitFields)
        {
            WORD         wDate = CZipBuilder::DosDate (2024, 12, 31);
            WORD         wTime = CZipBuilder::DosTime (23, 59, 58);
            SYSTEMTIME   st    = CZipReader::DosToSystemTime (wDate, wTime);



            Assert::AreEqual (WORD (2024), st.wYear);
            Assert::AreEqual (WORD (12),   st.wMonth);
            Assert::AreEqual (WORD (31),   st.wDay);
            Assert::AreEqual (WORD (23),   st.wHour);
            Assert::AreEqual (WORD (59),   st.wMinute);
            Assert::AreEqual (WORD (58),   st.wSecond);
            Assert::AreEqual (WORD (0),    st.wMilliseconds);
        }




        TEST_METHOD(DosToSystemTime_ZeroDate_IsYear1980)
        {
            SYSTEMTIME st = CZipReader::DosToSystemTime (0, 0);



            Assert::AreEqual (WORD (1980), st.wYear);
            Assert::AreEqual (WORD (0),    st.wMonth);
            Assert::AreEqual (WORD (0),    st.wDay);
        }




        TEST_METHOD(MapAttributes_DosHosts_KeepDosAttributes)
        {
            DWORD dwDos = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_ARCHIVE;



            Assert::AreEqual (dwDos,                                                    CZipReader::MapAttributes (MadeBy (0),  dwDos | FILE_ATTRIBUTE_NORMAL));
            Assert::AreEqual (DWORD (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM),    CZipReader::MapAttributes (MadeBy (10), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM));
            Assert::AreEqual (DWORD (FILE_ATTRIBUTE_DIRECTORY),                         CZipReader::MapAttributes (MadeBy (14), FILE_ATTRIBUTE_DIRECTORY));
        }




        TEST_METHOD(MapAttributes_UnixDirectory_IsDirectory)
        {
            DWORD dwAttrs = CZipReader::MapAttributes (MadeBy (3), 040755u << 16);



            Assert::AreEqual (DWORD (FILE_ATTRIBUTE_DIRECTORY), dwAttrs);
        }




        TEST_METHOD(MapAttributes_UnixNoOwnerWrite_IsReadOnly)
        {
            DWORD dwReadOnly = CZipReader::MapAttributes (MadeBy (3), 0100444u << 16);
            DWORD dwWritable = CZipReader::MapAttributes (MadeBy (3), 0100644u << 16);



            Assert::AreEqual (DWORD (FILE_ATTRIBUTE_READONLY), dwReadOnly);
            Assert::AreEqual (DWORD (0),                       dwWritable);
        }




        TEST_METHOD(MapAttributes_OtherHost_ReturnsZero)
        {
            Assert::AreEqual (DWORD (0), CZipReader::MapAttributes (MadeBy (19), FILE_ATTRIBUTE_READONLY | (040555u << 16)));
            Assert::AreEqual (DWORD (0), CZipReader::MapAttributes (MadeBy (3),  FILE_ATTRIBUTE_READONLY));
        }




        TEST_METHOD(Read_TrailingSlash_IsFolder)
        {
            CZipBuilder          builder;
            SArchiveReadResult   result;



            builder.AddFolder ("docs/").AddFile ("docs/a.txt");

            ReadZip (builder, result);

            Assert::IsTrue  (result.m_vEntries[0].m_fDirectory);
            Assert::IsFalse (result.m_vEntries[1].m_fDirectory);
        }




        TEST_METHOD(Read_UnixDirectoryMode_IsFolder)
        {
            CZipBuilder                  builder;
            CZipBuilder::SEntryOptions   options;
            SArchiveReadResult           result;



            options.m_strName              = "bin";
            options.m_bHost                = 3;
            options.m_dwExternalAttributes = 040755u << 16;
            builder.AddEntry (options);

            ReadZip (builder, result);

            Assert::IsTrue (result.m_vEntries[0].m_fDirectory);
        }




        TEST_METHOD(Read_EncryptionFlag_SetsEncrypted)
        {
            CZipBuilder                  builder;
            CZipBuilder::SEntryOptions   options;
            SArchiveReadResult           result;



            options.m_strName    = "secret.txt";
            options.m_fEncrypted = true;
            builder.AddEntry (options).AddFile ("plain.txt");

            ReadZip (builder, result);

            Assert::IsTrue  (result.m_vEntries[0].m_fEncrypted);
            Assert::IsFalse (result.m_vEntries[1].m_fEncrypted);
        }




        TEST_METHOD(Read_FewerEntriesThanReported_ReturnsBadFormat)
        {
            CZipBuilder builder;



            builder.AddFile ("a.txt").AddFile ("b.txt").SetReportedEntryCount (3);

            AssertBadFormat (builder.Build());
        }




        TEST_METHOD(Read_TruncatedFile_ReturnsBadFormat)
        {
            CZipBuilder    builder;
            vector<BYTE>   bytes;
            vector<BYTE>   truncated;



            builder.AddFile ("a.txt").AddFile ("b.txt");
            bytes = builder.Build();

            // Cut the end record and the tail of the second central directory header,
            // then append the end record again
            truncated.assign (bytes.begin(), bytes.end() - 30);
            truncated.insert (truncated.end(), bytes.end() - 22, bytes.end());

            AssertBadFormat (truncated);
        }




        TEST_METHOD(Read_NonZipBytes_ReturnsBadFormat)
        {
            vector<BYTE> bytes (4096);



            for (size_t i = 0; i < bytes.size(); ++i)
            {
                bytes[i] = static_cast<BYTE> (i * 7);
            }

            AssertBadFormat (bytes);
        }




        TEST_METHOD(Read_ShorterThanEndRecord_ReturnsBadFormat)
        {
            AssertBadFormat (vector<BYTE> (21, 0));
            AssertBadFormat (vector<BYTE>());
        }




        TEST_METHOD(Read_ManyEntries_ParsesAll)
        {
            static constexpr size_t s_kcEntries = 10000;

            CZipBuilder          builder;
            SArchiveReadResult   result;
            char                 szName[32]  = {};



            for (size_t i = 0; i < s_kcEntries; ++i)
            {
                sprintf_s (szName, "dir%zu/file%05zu.txt", i % 10, i);
                builder.AddFile (szName, i);
            }

            ReadZip (builder, result);

            Assert::AreEqual (s_kcEntries,              result.m_vEntries.size());
            Assert::AreEqual (L"dir0/file00000.txt",    result.m_vEntries[0].m_strPath.c_str());
            Assert::AreEqual (L"dir9/file09999.txt",    result.m_vEntries[s_kcEntries - 1].m_strPath.c_str());
            Assert::AreEqual (ULONGLONG (9999),         result.m_vEntries[s_kcEntries - 1].m_ullSize);
        }




    private:

        ////////////////////////////////////////////////////////////////////////////////
        //
        //  ZipReaderTests::ReadZip
        //
        //  Reads the archive that builder produces and asserts success.
        //
        ////////////////////////////////////////////////////////////////////////////////

        static void ReadZip (const CZipBuilder & builder, SArchiveReadResult & result)
        {
            CMemoryByteSource   source (builder.Build());
            HRESULT             hr     = S_OK;



            hr = CZipReader::Read (source, result);

            Assert::AreEqual (S_OK, hr);
        }




        ////////////////////////////////////////////////////////////////////////////////
        //
        //  ZipReaderTests::AssertBadFormat
        //
        ////////////////////////////////////////////////////////////////////////////////

        static void AssertBadFormat (vector<BYTE> bytes)
        {
            CMemoryByteSource    source (move (bytes));
            SArchiveReadResult   result;
            HRESULT              hr     = S_OK;



            hr = CZipReader::Read (source, result);

            Assert::AreEqual (HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT), hr);
        }




        ////////////////////////////////////////////////////////////////////////////////
        //
        //  ZipReaderTests::MadeBy
        //
        //  Returns a "version made by" value for host.
        //
        ////////////////////////////////////////////////////////////////////////////////

        static WORD MadeBy (BYTE bHost)
        {
            static constexpr WORD s_kwSpecVersion = 20;
            static constexpr int  s_kcHostShift   = 8;



            return static_cast<WORD> ((bHost << s_kcHostShift) | s_kwSpecVersion);
        }




        //
        //  Details for --Info
        //

        static SZipDetails ReadDetails (const CZipBuilder & builder)
        {
            SArchiveReadResult result;



            result.m_detailsPtr = make_shared<SArchiveDetails>();
            ReadZip (builder, result);

            return result.m_detailsPtr->m_zip;
        }

        static CZipBuilder::SEntryOptions Entry (const string & name, ULONGLONG ullSize, WORD wMethod, ULONGLONG ullCompressed)
        {
            CZipBuilder::SEntryOptions options;



            options.m_strName           = name;
            options.m_ullSize           = ullSize;
            options.m_wMethod           = wMethod;
            options.m_ullCompressedSize = ullCompressed;
            return options;
        }




        TEST_METHOD(Details_NotCollectedWithoutPointer)
        {
            CZipBuilder          builder;
            SArchiveReadResult   result;



            builder.AddFile ("a.txt", 10);
            ReadZip (builder, result);

            Assert::IsNull (result.m_detailsPtr.get());
        }




        TEST_METHOD(Details_MethodGroups)
        {
            CZipBuilder builder;



            builder.AddEntry (Entry ("a.txt", 100, 8, 40))
                   .AddEntry (Entry ("b.txt", 200, 8, 60))
                   .AddEntry (Entry ("c.bin", 50, 0, 50))
                   .AddEntry (Entry ("d.xz",  80, 95, 20))
                   .AddFolder ("docs/");

            SZipDetails zip = ReadDetails (builder);

            Assert::AreEqual (size_t (3), zip.m_mapMethods.size());
            Assert::AreEqual (2ull,   zip.m_mapMethods[8].m_cFiles);
            Assert::AreEqual (100ull, zip.m_mapMethods[8].m_ullCompressed);
            Assert::AreEqual (300ull, zip.m_mapMethods[8].m_ullUncompressed);
            Assert::AreEqual (1ull,   zip.m_mapMethods[0].m_cFiles);
            Assert::AreEqual (1ull,   zip.m_mapMethods[95].m_cFiles);
        }




        TEST_METHOD(Details_EncryptionTypes)
        {
            CZipBuilder                builder;
            CZipBuilder::SEntryOptions crypto = Entry ("crypto.txt", 1, 8, 1);
            CZipBuilder::SEntryOptions aes128 = Entry ("aes128.txt", 1, 8, 1);
            CZipBuilder::SEntryOptions aes256 = Entry ("aes256.txt", 1, 12, 1);
            CZipBuilder::SEntryOptions strong = Entry ("strong.txt", 1, 8, 1);



            crypto.m_fEncrypted        = true;
            aes128.m_bAesStrength      = 1;
            aes256.m_bAesStrength      = 3;
            strong.m_fStrongEncryption = true;

            builder.AddEntry (crypto).AddEntry (aes128).AddEntry (aes256).AddEntry (strong).AddFile ("plain.txt", 1);

            SZipDetails zip = ReadDetails (builder);

            Assert::AreEqual (1ull, zip.m_mapEncryption[EZipEncryption::ZipCrypto]);
            Assert::AreEqual (1ull, zip.m_mapEncryption[EZipEncryption::Aes128]);
            Assert::AreEqual (1ull, zip.m_mapEncryption[EZipEncryption::Aes256]);
            Assert::AreEqual (1ull, zip.m_mapEncryption[EZipEncryption::PkwareStrong]);
            Assert::AreEqual (1ull, zip.m_mapEncryption[EZipEncryption::None]);
            Assert::AreEqual (1ull, zip.m_mapMethods[12].m_cFiles);          // AES entries group by their real method
            Assert::IsTrue   (zip.m_mapMethods.find (99) == zip.m_mapMethods.end());
        }




        TEST_METHOD(Details_Zip64AndCompressedSizeFromExtra)
        {
            CZipBuilder                builder;
            CZipBuilder::SEntryOptions big = Entry ("big.bin", 0x100000000ull, 8, 0x80000000ull);



            builder.AddEntry (big);

            SZipDetails zip = ReadDetails (builder);

            Assert::IsTrue   (zip.m_fZip64);
            Assert::AreEqual (0x80000000ull, zip.m_mapMethods[8].m_ullCompressed);
        }




        TEST_METHOD(Details_Zip64EndRecord)
        {
            CZipBuilder builder;



            builder.AddFile ("a.txt", 1).SetZip64EndRecord (true);

            Assert::IsTrue (ReadDetails (builder).m_fZip64);
        }




        TEST_METHOD(Details_FlagsCreatorPrefixAndDisks)
        {
            CZipBuilder                builder;
            CZipBuilder::SEntryOptions unixEntry = Entry ("u.txt", 1, 0, 1);



            unixEntry.m_bHost    = 3;
            unixEntry.m_bVersion = 63;
            unixEntry.m_fUtf8    = true;

            builder.AddEntry (unixEntry).AddFile ("a.txt", 1).AddFile ("b.txt", 1).SetPrefix (512).SetDiskNumbers (1, 1);

            SZipDetails zip = ReadDetails (builder);

            Assert::IsTrue   (zip.m_fUtf8Paths);
            Assert::IsTrue   (zip.m_fMultiDisk);
            Assert::IsFalse  (zip.m_fZip64);
            Assert::AreEqual (512ull, zip.m_ullPrefixSize);
            Assert::AreEqual (2ull, zip.m_mapCreators[MadeBy (0)]);
            Assert::AreEqual (1ull, zip.m_mapCreators[static_cast<WORD> ((3 << 8) | 63)]);
        }




        TEST_METHOD(Details_NoPrefix)
        {
            CZipBuilder builder;



            builder.AddFile ("a.txt", 1);

            Assert::AreEqual (0ull, ReadDetails (builder).m_ullPrefixSize);
            Assert::IsFalse  (ReadDetails (builder).m_fMultiDisk);
        }




        TEST_METHOD(Details_CommentUtf8AndCodePage437)
        {
            CZipBuilder utf8;
            CZipBuilder dos;



            utf8.AddFile ("a.txt", 1).SetComment ("caf\xC3\xA9");
            dos.AddFile  ("a.txt", 1).SetComment ("caf\x82");

            Assert::AreEqual (L"caf\x00E9", ReadDetails (utf8).m_strComment.c_str());
            Assert::AreEqual (L"caf\x00E9", ReadDetails (dos).m_strComment.c_str());
        }
    };
}
