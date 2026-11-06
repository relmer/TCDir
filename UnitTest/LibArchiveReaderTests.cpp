#include "pch.h"
#include "EhmTestHelper.h"

#include "../TCDirCore/ArchiveDetails.h"
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

        struct STarFile
        {
            string         m_strName;
            vector<BYTE>   m_data;
            char           m_chType   = '0';
            string         m_strLink;
            string         m_strOwner;
            string         m_strMode  = "0000644";
        };

        static vector<BYTE> MakeTar (const vector<pair<string, vector<BYTE>>> & files)
        {
            vector<STarFile> tarFiles;

            for (const auto & [name, data] : files)
            {
                tarFiles.push_back ({ name, data });
            }

            return MakeTarFiles (tarFiles);
        }

        static vector<BYTE> MakeTarFiles (const vector<STarFile> & files)
        {
            static constexpr size_t s_kcbBlock = 512;

            vector<BYTE> tar;

            for (const STarFile & file : files)
            {
                const string       & name   = file.m_strName;
                const vector<BYTE> & data   = file.m_data;
                vector<BYTE>         header (s_kcbBlock, 0);
                unsigned             uSum   = 0;
                char                 szField[16];

                memcpy (header.data(), name.data(), name.size());
                memcpy (header.data() + 100, file.m_strMode.data(), 7);
                memcpy (header.data() + 108, "0000000", 7);
                memcpy (header.data() + 116, "0000000", 7);
                sprintf_s (szField, "%011o", static_cast<unsigned> (data.size()));
                memcpy (header.data() + 124, szField, 11);
                memcpy (header.data() + 136, "14000000000", 11);
                memset (header.data() + 148, ' ', 8);
                header[156] = file.m_chType;
                memcpy (header.data() + 157, file.m_strLink.data(), file.m_strLink.size());
                memcpy (header.data() + 257, "ustar", 6);
                memcpy (header.data() + 263, "00", 2);
                memcpy (header.data() + 265, file.m_strOwner.data(), file.m_strOwner.size());
                memcpy (header.data() + 297, file.m_strOwner.data(), file.m_strOwner.size());

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

        //
        //  Wraps data in a gzip stream of stored deflate blocks.
        //

        static vector<BYTE> Gzip (const vector<BYTE> & data)
        {
            static constexpr BYTE   s_krgbHeader[] = { 0x1F, 0x8B, 8, 0, 0, 0, 0, 0, 0, 0xFF };
            static constexpr size_t s_kcbMaxStored = 0xFFFF;

            vector<BYTE> gz (begin (s_krgbHeader), end (s_krgbHeader));
            size_t       ib = 0;

            do
            {
                size_t cb     = min (s_kcbMaxStored, data.size() - ib);
                bool   fFinal = ib + cb == data.size();

                gz.push_back (fFinal ? 1 : 0);
                gz.push_back (static_cast<BYTE> (cb));
                gz.push_back (static_cast<BYTE> (cb >> 8));
                gz.push_back (static_cast<BYTE> (~cb));
                gz.push_back (static_cast<BYTE> (~cb >> 8));
                gz.insert (gz.end(), data.begin() + ib, data.begin() + ib + cb);
                ib += cb;
            }
            while (ib < data.size());

            AppendLe32 (gz, Crc32 (data));
            AppendLe32 (gz, static_cast<DWORD> (data.size()));
            return gz;
        }

        static DWORD Crc32 (const vector<BYTE> & data)
        {
            DWORD dwCrc = 0xFFFFFFFF;

            for (BYTE b : data)
            {
                dwCrc ^= b;

                for (int i = 0; i < 8; ++i)
                {
                    dwCrc = (dwCrc >> 1) ^ ((dwCrc & 1) ? 0xEDB88320 : 0);
                }
            }

            return ~dwCrc;
        }

        static void AppendLe32 (vector<BYTE> & bytes, DWORD dw)
        {
            for (int i = 0; i < 4; ++i)
            {
                bytes.push_back (static_cast<BYTE> (dw >> (i * 8)));
            }
        }

        static SLibArchiveDetails ReadLibDetails (vector<BYTE> bytes)
        {
            SArchiveReadResult result;

            result.m_detailsPtr = make_shared<SArchiveDetails>();

            Assert::AreEqual (S_OK, CLibArchiveReader::Read (MemorySource (move (bytes)), result));
            return result.m_detailsPtr->m_libArchive;
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




        TEST_METHOD(Details_Tar_FormatLinksOwnersAndPermissions)
        {
            vector<STarFile>     files;
            SLibArchiveDetails   lib;

            if (!CLibArchiveReader::IsAvailable())
            {
                Logger::WriteMessage (L"archiveint.dll is not available; skipped");
                return;
            }

            files.push_back ({ "a.txt",   Bytes ("one"), '0', "",      "alice", "0000644" });
            files.push_back ({ "b.sh",    Bytes ("two"), '0', "",      "bob",   "0000755" });
            files.push_back ({ "c.txt",   Bytes ("3"),   '0', "",      "alice", "0000644" });
            files.push_back ({ "link",    {},            '2', "a.txt", "alice", "0000777" });
            files.push_back ({ "hard",    {},            '1', "a.txt", "alice", "0000644" });

            lib = ReadLibDetails (MakeTarFiles (files));

            Assert::IsTrue   (lib.m_strFormatName.find (L"ustar") != wstring::npos, lib.m_strFormatName.c_str());
            Assert::IsTrue   (lib.m_vFilters.empty());
            Assert::IsFalse  (lib.m_ullStreamCompressed.has_value());
            Assert::AreEqual (1ull, lib.m_cSymlinks);
            Assert::AreEqual (1ull, lib.m_cHardLinks);
            Assert::AreEqual (4ull, lib.m_mapOwners[L"alice"]);
            Assert::AreEqual (1ull, lib.m_mapOwners[L"bob"]);
            Assert::AreEqual (4ull, lib.m_mapGroups[L"alice"]);
            Assert::AreEqual (3ull, lib.m_mapPermissions[0644]);
            Assert::AreEqual (1ull, lib.m_mapPermissions[0755]);
        }




        TEST_METHOD(Details_TarGz_FilterAndStreamTotals)
        {
            vector<BYTE>         tar;
            vector<BYTE>         gz;
            SLibArchiveDetails   lib;

            if (!CLibArchiveReader::IsAvailable())
            {
                Logger::WriteMessage (L"archiveint.dll is not available; skipped");
                return;
            }

            tar = MakeTar ({ { "a.txt", Bytes ("gzip me") } });
            gz  = Gzip (tar);
            lib = ReadLibDetails (gz);

            Assert::AreEqual (size_t (1), lib.m_vFilters.size());
            Assert::AreEqual (L"gzip", lib.m_vFilters[0].c_str());
            Assert::IsTrue   (lib.m_ullStreamCompressed.has_value());
            Assert::AreEqual (static_cast<ULONGLONG> (tar.size()), lib.m_ullStreamUncompressed.value());
            Assert::IsTrue   (lib.m_ullStreamCompressed.value() > 0);
        }




        TEST_METHOD(Details_NotCollectedWithoutPointer)
        {
            SArchiveReadResult result;

            if (!CLibArchiveReader::IsAvailable())
            {
                Logger::WriteMessage (L"archiveint.dll is not available; skipped");
                return;
            }

            Assert::AreEqual (S_OK, CLibArchiveReader::Read (MemorySource (MakeTar ({ { "a.txt", Bytes ("x") } })), result));
            Assert::IsNull   (result.m_detailsPtr.get());
        }
    };
}
