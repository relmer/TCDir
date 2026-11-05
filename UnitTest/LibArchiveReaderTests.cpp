#include "pch.h"
#include "EhmTestHelper.h"

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
    };
}
