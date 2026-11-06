#include "pch.h"
#include "EhmTestHelper.h"

#include "../TCDirCore/ArchiveFileAccess.h"
#include "../TCDirCore/LibArchiveReader.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(LibArchiveReaderTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        TEST_METHOD(ConvertEntry_RegularFile)
        {
            SLibArchiveEntryInfo info;

            info.m_strPath   = L"src/main.cpp";
            info.m_uFileType = 0100000;
            info.m_fSizeSet  = true;
            info.m_llSize    = 1234;
            info.m_fPermSet  = true;
            info.m_uPerm     = 0644;

            SArchiveEntry entry = CLibArchiveReader::ConvertEntry (info);

            Assert::AreEqual (L"src/main.cpp", entry.m_strPath.c_str());
            Assert::IsFalse  (entry.m_fDirectory);
            Assert::AreEqual (1234ull, entry.m_ullSize);
            Assert::AreEqual (0ul, entry.m_dwAttributes);
        }




        TEST_METHOD(ConvertEntry_Directory)
        {
            SLibArchiveEntryInfo info;

            info.m_strPath   = L"src";
            info.m_uFileType = 0040000;
            info.m_fSizeSet  = true;
            info.m_llSize    = 4096;

            SArchiveEntry entry = CLibArchiveReader::ConvertEntry (info);

            Assert::IsTrue   (entry.m_fDirectory);
            Assert::AreEqual (0ull, entry.m_ullSize);
            Assert::IsTrue   ((entry.m_dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
        }




        TEST_METHOD(ConvertEntry_SizeNotSetOrNegative_Zero)
        {
            SLibArchiveEntryInfo info;

            info.m_uFileType = 0100000;
            info.m_llSize    = 99;

            Assert::AreEqual (0ull, CLibArchiveReader::ConvertEntry (info).m_ullSize);

            info.m_fSizeSet = true;
            info.m_llSize   = -5;

            Assert::AreEqual (0ull, CLibArchiveReader::ConvertEntry (info).m_ullSize);
        }




        TEST_METHOD(ConvertEntry_MtimeToFileTime)
        {
            SLibArchiveEntryInfo info;
            ULARGE_INTEGER       uli   = {};

            // 2000-01-01T00:00:00Z plus half a second
            info.m_fMtimeSet  = true;
            info.m_llMtime    = 946684800;
            info.m_lMtimeNsec = 500000000;

            SArchiveEntry entry = CLibArchiveReader::ConvertEntry (info);

            uli.LowPart  = entry.m_ftLastWrite.dwLowDateTime;
            uli.HighPart = entry.m_ftLastWrite.dwHighDateTime;

            Assert::AreEqual (125911584005000000ull, uli.QuadPart);
        }




        TEST_METHOD(ConvertEntry_MtimeNotSetOrBefore1601_Zero)
        {
            SLibArchiveEntryInfo info;

            info.m_llMtime = 946684800;

            SArchiveEntry entry = CLibArchiveReader::ConvertEntry (info);

            Assert::AreEqual (0ul, entry.m_ftLastWrite.dwLowDateTime);
            Assert::AreEqual (0ul, entry.m_ftLastWrite.dwHighDateTime);

            info.m_fMtimeSet = true;
            info.m_llMtime   = -20000000000LL;

            entry = CLibArchiveReader::ConvertEntry (info);

            Assert::AreEqual (0ul, entry.m_ftLastWrite.dwHighDateTime);
        }




        TEST_METHOD(ConvertEntry_NoOwnerWrite_ReadOnly)
        {
            SLibArchiveEntryInfo info;

            info.m_uFileType = 0100000;
            info.m_fPermSet  = true;
            info.m_uPerm     = 0444;

            Assert::IsTrue ((CLibArchiveReader::ConvertEntry (info).m_dwAttributes & FILE_ATTRIBUTE_READONLY) != 0);

            info.m_fPermSet = false;

            Assert::AreEqual (0ul, CLibArchiveReader::ConvertEntry (info).m_dwAttributes);
        }




        TEST_METHOD(ConvertEntry_EncryptedFlag)
        {
            SLibArchiveEntryInfo info;

            info.m_fEncrypted = true;

            Assert::IsTrue (CLibArchiveReader::ConvertEntry (info).m_fEncrypted);
        }




        ////////////////////////////////////////////////////////////////////////
        //
        //  MakeTar
        //
        //  Builds a ustar image in memory: a header block and padded data per
        //  file, then two zero blocks.
        //
        ////////////////////////////////////////////////////////////////////////

        static vector<BYTE> MakeTar (const vector<pair<string, vector<BYTE>>> & files)
        {
            static constexpr size_t s_kcbBlock = 512;

            vector<BYTE> tar;

            for (const auto & [name, data] : files)
            {
                vector<BYTE> header (s_kcbBlock, 0);
                unsigned     uSum   = 0;
                char         szField[16];

                memcpy (header.data(), name.data(), name.size());
                memcpy (header.data() + 100, "0000644", 7);
                memcpy (header.data() + 108, "0000000", 7);
                memcpy (header.data() + 116, "0000000", 7);
                sprintf_s (szField, "%011o", static_cast<unsigned> (data.size()));
                memcpy (header.data() + 124, szField, 11);
                memcpy (header.data() + 136, "14000000000", 11);
                memset (header.data() + 148, ' ', 8);
                header[156] = '0';
                memcpy (header.data() + 257, "ustar", 6);
                memcpy (header.data() + 263, "00", 2);

                for (BYTE b : header)
                {
                    uSum += b;
                }

                sprintf_s (szField, "%06o", uSum);
                memcpy (header.data() + 148, szField, 7);

                tar.insert (tar.end(), header.begin(), header.end());
                tar.insert (tar.end(), data.begin(), data.end());
                tar.resize ((tar.size() + s_kcbBlock - 1) / s_kcbBlock * s_kcbBlock, 0);
            }

            tar.resize (tar.size() + 2 * s_kcbBlock, 0);
            return tar;
        }

        static vector<BYTE> Bytes (const string & text)
        {
            return vector<BYTE> (text.begin(), text.end());
        }

        static SArchiveSource MemorySource (vector<BYTE> bytes)
        {
            SArchiveSource source;

            source.m_bytesPtr = make_shared<const vector<BYTE>> (move (bytes));
            return source;
        }




        TEST_METHOD(IsSamePath_IgnoresCaseSeparatorsAndDotComponents)
        {
            Assert::IsTrue  (CLibArchiveReader::IsSamePath (L"pkg/Inner.zip",   L"PKG\\inner.zip"));
            Assert::IsTrue  (CLibArchiveReader::IsSamePath (L"./pkg//inner.zip", L"pkg\\inner.zip"));
            Assert::IsFalse (CLibArchiveReader::IsSamePath (L"pkg/inner.zip",   L"inner.zip"));
            Assert::IsFalse (CLibArchiveReader::IsSamePath (L"",                L""));
        }




        TEST_METHOD(ExtractEntry_MemoryTar_ReturnsData)
        {
            vector<BYTE> data;

            if (!CLibArchiveReader::IsAvailable())
            {
                Logger::WriteMessage (L"archiveint.dll is not available; skipped");
                return;
            }

            HRESULT hr = CLibArchiveReader::ExtractEntry (MemorySource (MakeTar ({ { "a.txt", Bytes ("first") }, { "pkg/b.txt", Bytes ("second") } })),
                                                          L"pkg\\B.TXT", 1024, data);

            Assert::IsTrue   (SUCCEEDED (hr));
            Assert::AreEqual (string ("second"), string (data.begin(), data.end()));
        }




        TEST_METHOD(ExtractEntry_MissingOrTooLarge_Errors)
        {
            SArchiveSource source = MemorySource (MakeTar ({ { "a.txt", Bytes ("first") } }));
            vector<BYTE>   data;

            if (!CLibArchiveReader::IsAvailable())
            {
                Logger::WriteMessage (L"archiveint.dll is not available; skipped");
                return;
            }

            Assert::IsTrue (CLibArchiveReader::ExtractEntry (source, L"missing.txt", 1024, data) == HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND));
            Assert::IsTrue (CLibArchiveReader::ExtractEntry (source, L"a.txt", 4, data)          == HRESULT_FROM_WIN32 (ERROR_FILE_TOO_LARGE));
            Assert::IsTrue (data.empty());
        }




        TEST_METHOD(ArchiveFileAccess_NestedTar_ListsInnerEntries)
        {
            CArchiveFileAccess               access;
            shared_ptr<const vector<BYTE>>   innerPtr;
            SArchiveSource                   inner;
            SArchiveReadResult               result;
            vector<BYTE>                     innerTar = MakeTar ({ { "deep.txt", Bytes ("nested") } });

            if (!CLibArchiveReader::IsAvailable())
            {
                Logger::WriteMessage (L"archiveint.dll is not available; skipped");
                return;
            }

            HRESULT hr = access.ExtractEntry (MemorySource (MakeTar ({ { "pkg/inner.tar", innerTar } })), L"pkg\\inner.tar", 1 << 20, innerPtr);

            Assert::IsTrue (SUCCEEDED (hr));
            Assert::IsTrue (*innerPtr == innerTar);

            inner.m_bytesPtr = innerPtr;
            hr = access.Read (inner, EArchiveFormat::LibArchive, result);

            Assert::IsTrue   (SUCCEEDED (hr));
            Assert::AreEqual (size_t (1), result.m_vEntries.size());
            Assert::AreEqual (L"deep.txt", result.m_vEntries[0].m_strPath.c_str());
            Assert::AreEqual (6ull, result.m_vEntries[0].m_ullSize);
        }
    };
}
