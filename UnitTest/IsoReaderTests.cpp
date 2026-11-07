#include "pch.h"
#include "EhmTestHelper.h"
#include "Mocks/IsoImageBuilder.h"

#include "../TCDirCore/ByteSource.h"
#include "../TCDirCore/IsoReader.h"
#include "../TCDirCore/UdfReader.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    ////////////////////////////////////////////////////////////////////////////
    //
    //  IsoReaderTests
    //
    //  CIsoReader and CUdfReader against in-memory disc images.
    //
    ////////////////////////////////////////////////////////////////////////////

    TEST_CLASS(IsoReaderTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        TEST_METHOD(Read_PrimaryOnly_ListsEveryEntry)
        {
            SArchiveReadResult    result;
            HRESULT               hr     = ReadIso (CIsoImageBuilder::BuildPrimaryOnly(), result);
            const SArchiveEntry * pNote  = nullptr;
            const SArchiveEntry * pDocs  = nullptr;



            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (s_kcPrimaryEntries, result.m_vEntries.size());

            pDocs = Find (result, L"DOCS");
            pNote = Find (result, L"DOCS\\NOTE.TXT");

            Assert::IsNotNull (pDocs);
            Assert::IsTrue (pDocs->m_fDirectory);
            Assert::IsTrue ((pDocs->m_dwAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
            Assert::IsNotNull (pNote);
            Assert::IsFalse (pNote->m_fDirectory);
            Assert::AreEqual ((ULONGLONG) CIsoImageBuilder::s_kcbNote, pNote->m_ullSize);
        }




        TEST_METHOD(Read_PrimaryOnly_ConvertsRecordingDateToUtc)
        {
            SArchiveReadResult result;
            HRESULT            hr       = ReadIso (CIsoImageBuilder::BuildPrimaryOnly(), result);
            FILETIME           ftExpect = CIsoImageBuilder::ExpectedTime();



            Assert::AreEqual (S_OK, hr);

            for (const SArchiveEntry & entry : result.m_vEntries)
            {
                Assert::AreEqual (0L, CompareFileTime (&ftExpect, &entry.m_ftLastWrite), entry.m_strPath.c_str());
            }
        }




        TEST_METHOD(Read_JolietPresent_PrefersJolietNames)
        {
            SArchiveReadResult    result;
            HRESULT               hr     = ReadIso (CIsoImageBuilder::BuildPrimaryAndJoliet(), result);
            const SArchiveEntry * pInner = nullptr;



            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (s_kcJolietEntries, result.m_vEntries.size());
            Assert::IsNotNull (Find (result, L"ReadMe.txt"));
            Assert::IsNotNull (Find (result, L"Sub Folder"));
            Assert::IsNull (Find (result, L"README.TXT"));

            pInner = Find (result, L"Sub Folder\\Inner File.txt");

            Assert::IsNotNull (pInner);
            Assert::AreEqual ((ULONGLONG) CIsoImageBuilder::s_kcbNote, pInner->m_ullSize);
        }




        TEST_METHOD(Read_VersionSuffixAndTrailingDot_AreRemoved)
        {
            SArchiveReadResult result;
            HRESULT            hr = ReadIso (CIsoImageBuilder::BuildPrimaryOnly(), result);



            Assert::AreEqual (S_OK, hr);
            Assert::IsNotNull (Find (result, L"README.TXT"));
            Assert::IsNotNull (Find (result, L"NOEXT"));

            for (const SArchiveEntry & entry : result.m_vEntries)
            {
                Assert::AreEqual (wstring::npos, entry.m_strPath.find (L';'), entry.m_strPath.c_str());
                Assert::AreNotEqual (L'.', entry.m_strPath.back(), entry.m_strPath.c_str());
            }
        }




        TEST_METHOD(Read_HiddenFlag_SetsHiddenAttribute)
        {
            SArchiveReadResult    result;
            HRESULT               hr      = ReadIso (CIsoImageBuilder::BuildPrimaryOnly(), result);
            const SArchiveEntry * pHidden = nullptr;
            const SArchiveEntry * pReadme = nullptr;



            Assert::AreEqual (S_OK, hr);

            pHidden = Find (result, L"HIDDEN.TXT");
            pReadme = Find (result, L"README.TXT");

            Assert::IsNotNull (pHidden);
            Assert::IsNotNull (pReadme);
            Assert::IsTrue ((pHidden->m_dwAttributes & FILE_ATTRIBUTE_HIDDEN) != 0);
            Assert::IsTrue ((pReadme->m_dwAttributes & FILE_ATTRIBUTE_HIDDEN) == 0);
        }




        TEST_METHOD(Read_EveryEntry_IsReadOnly)
        {
            SArchiveReadResult result;
            HRESULT            hr = ReadIso (CIsoImageBuilder::BuildPrimaryAndJoliet(), result);



            Assert::AreEqual (S_OK, hr);
            Assert::IsFalse (result.m_vEntries.empty());

            for (const SArchiveEntry & entry : result.m_vEntries)
            {
                Assert::IsTrue ((entry.m_dwAttributes & FILE_ATTRIBUTE_READONLY) != 0, entry.m_strPath.c_str());
            }
        }




        TEST_METHOD(Read_MultiExtentFile_SumsExtentSizes)
        {
            SArchiveReadResult    result;
            HRESULT               hr     = ReadIso (CIsoImageBuilder::BuildMultiExtent(), result);
            const SArchiveEntry * pBig   = nullptr;
            const SArchiveEntry * pAfter = nullptr;



            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (s_kcMultiExtentEntries, result.m_vEntries.size());

            pBig   = Find (result, L"BIG.DAT");
            pAfter = Find (result, L"AFTER.TXT");

            Assert::IsNotNull (pBig);
            Assert::IsNotNull (pAfter);
            Assert::AreEqual ((ULONGLONG) CIsoImageBuilder::s_kcbFirstExtent + CIsoImageBuilder::s_kcbLastExtent, pBig->m_ullSize);
            Assert::AreEqual ((ULONGLONG) CIsoImageBuilder::s_kcbNoExtension, pAfter->m_ullSize);
        }




        TEST_METHOD(Read_NestedFolders_ListsDeepAndMultiSectorFolders)
        {
            SArchiveReadResult result;
            HRESULT            hr    = ReadIso (CIsoImageBuilder::BuildNested(), result);
            size_t             cMany = 0;



            Assert::AreEqual (S_OK, hr);
            Assert::IsNotNull (Find (result, L"A\\B\\C"));
            Assert::IsNotNull (Find (result, L"A\\B\\C\\DEEP.TXT"));
            Assert::IsNotNull (Find (result, L"MANY\\FILE00.TXT"));
            Assert::IsNotNull (Find (result, L"MANY\\FILE59.TXT"));

            for (const SArchiveEntry & entry : result.m_vEntries)
            {
                if (entry.m_strPath.starts_with (L"MANY\\"))
                {
                    cMany++;
                }
            }

            Assert::AreEqual ((size_t) CIsoImageBuilder::s_kcManyFiles, cMany);
        }




        TEST_METHOD(Read_CorruptDescriptor_ReturnsBadFormat)
        {
            vector<BYTE>       vImage = CIsoImageBuilder::BuildPrimaryOnly();
            SArchiveReadResult result;
            HRESULT            hr     = S_OK;



            vImage[CIsoImageBuilder::s_kFirstDescriptor * CIsoImageBuilder::s_kcbSector + s_kibStandardId] = s_kbCorrupt;

            hr = ReadIso (vImage, result);

            Assert::AreEqual (HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT), hr);
            Assert::IsTrue (result.m_vEntries.empty());
        }




        TEST_METHOD(Read_CorruptRecordLength_ReturnsBadFormat)
        {
            vector<BYTE>       vImage = CIsoImageBuilder::BuildPrimaryOnly();
            SArchiveReadResult result;
            HRESULT            hr     = S_OK;



            vImage[CIsoImageBuilder::s_kFirstDataSector * CIsoImageBuilder::s_kcbSector] = s_kcbTooShortRecord;

            hr = ReadIso (vImage, result);

            Assert::AreEqual (HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT), hr);
        }




        TEST_METHOD(Read_DirectoryCycle_ReturnsBadFormat)
        {
            SArchiveReadResult result;
            HRESULT            hr = ReadIso (CIsoImageBuilder::BuildCycle(), result);



            Assert::AreEqual (HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT), hr);
        }




        TEST_METHOD(Read_NonIsoBytes_ReturnsBadFormat)
        {
            SArchiveReadResult result;
            HRESULT            hrLarge = ReadIso (vector<BYTE> (s_kcbNonIso, s_kbFill), result);
            HRESULT            hrTiny  = ReadIso (vector<BYTE> (s_kcbTiny, s_kbFill), result);



            Assert::AreEqual (HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT), hrLarge);
            Assert::AreEqual (HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT), hrTiny);
        }




        TEST_METHOD(Read_UdfImage_PrefersUdfOverBridgeTree)
        {
            SArchiveReadResult result;
            HRESULT            hr = ReadIso (CIsoImageBuilder::BuildUdf (EUdfVariant::Standard), result);



            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (s_kcUdfEntries, result.m_vEntries.size());
            Assert::IsNull (Find (result, L"README.TXT"));
            Assert::IsNotNull (Find (result, L"setup.exe"));
        }




        TEST_METHOD(Read_UdfImage_ListsNestedFolders)
        {
            SArchiveReadResult    result;
            HRESULT               hr    = ReadIso (CIsoImageBuilder::BuildUdf (EUdfVariant::Standard), result);
            const SArchiveEntry * pFont = nullptr;
            const SArchiveEntry * pWim  = nullptr;



            Assert::AreEqual (S_OK, hr);

            pFont = Find (result, L"boot\\fonts\\wgl4_boot.ttf");
            pWim  = Find (result, L"Sources\\install.wim");

            Assert::IsNotNull (Find (result, L"boot\\fonts"));
            Assert::IsTrue (Find (result, L"boot\\fonts")->m_fDirectory);
            Assert::IsNotNull (pFont);
            Assert::AreEqual (CIsoImageBuilder::s_kcbFont, pFont->m_ullSize);
            Assert::IsNotNull (pWim);
            Assert::AreEqual (CIsoImageBuilder::s_kcbInstallWim, pWim->m_ullSize);
        }




        TEST_METHOD(Read_UdfImage_Decodes8BitAnd16BitNames)
        {
            SArchiveReadResult    result;
            HRESULT               hr       = ReadIso (CIsoImageBuilder::BuildUdf (EUdfVariant::Standard), result);
            wstring               strPath  = wstring (L"Sources\\") + CIsoImageBuilder::s_kpszUnicodeName;
            const SArchiveEntry * pUnicode = nullptr;



            Assert::AreEqual (S_OK, hr);

            pUnicode = Find (result, strPath.c_str());

            Assert::IsNotNull (Find (result, L"setup.exe"));
            Assert::IsNotNull (pUnicode);
            Assert::AreEqual (CIsoImageBuilder::s_kcbUnicodeFile, pUnicode->m_ullSize);
        }




        TEST_METHOD(Read_UdfImage_ReadsExtendedFileEntries)
        {
            SArchiveReadResult    result;
            HRESULT               hr    = ReadIso (CIsoImageBuilder::BuildUdf (EUdfVariant::Standard), result);
            const SArchiveEntry * pBoot = nullptr;
            const SArchiveEntry * pBcd  = nullptr;
            FILETIME              ft    = CIsoImageBuilder::ExpectedTime();



            Assert::AreEqual (S_OK, hr);

            pBoot = Find (result, L"boot");
            pBcd  = Find (result, L"boot\\BCD");

            Assert::IsNotNull (pBoot);
            Assert::IsTrue (pBoot->m_fDirectory);
            Assert::IsNotNull (pBcd);
            Assert::AreEqual (CIsoImageBuilder::s_kcbBcd, pBcd->m_ullSize);
            Assert::AreEqual (0L, CompareFileTime (&ft, &pBcd->m_ftLastWrite));
        }




        TEST_METHOD(Read_UdfImage_MapsHiddenAndReadOnly)
        {
            SArchiveReadResult    result;
            HRESULT               hr      = ReadIso (CIsoImageBuilder::BuildUdf (EUdfVariant::Standard), result);
            const SArchiveEntry * pHidden = nullptr;
            const SArchiveEntry * pSetup  = nullptr;



            Assert::AreEqual (S_OK, hr);

            pHidden = Find (result, L"boot\\hidden.sys");
            pSetup  = Find (result, L"setup.exe");

            Assert::IsNotNull (pHidden);
            Assert::IsNotNull (pSetup);
            Assert::IsTrue ((pHidden->m_dwAttributes & FILE_ATTRIBUTE_HIDDEN) != 0);
            Assert::IsTrue ((pSetup->m_dwAttributes & FILE_ATTRIBUTE_HIDDEN) == 0);

            for (const SArchiveEntry & entry : result.m_vEntries)
            {
                Assert::IsTrue ((entry.m_dwAttributes & FILE_ATTRIBUTE_READONLY) != 0, entry.m_strPath.c_str());
            }
        }




        TEST_METHOD(Read_UdfMetadataPartition_FallsBackToIsoTree)
        {
            SArchiveReadResult result;
            HRESULT            hr = ReadIso (CIsoImageBuilder::BuildUdf (EUdfVariant::MetadataPartition), result);



            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (size_t (1), result.m_vEntries.size());
            Assert::AreEqual (L"README.TXT", result.m_vEntries[0].m_strPath.c_str());
        }




        TEST_METHOD(Read_UdfMetadataPartitionAndEmptyBridge_ReturnsNotSupported)
        {
            SArchiveReadResult result;
            HRESULT            hr = ReadIso (CIsoImageBuilder::BuildUdf (EUdfVariant::MetadataPartitionEmptyBridge), result);



            Assert::AreEqual (HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED), hr);
        }




        TEST_METHOD(UdfRead_MetadataPartition_ReturnsNotSupported)
        {
            SArchiveReadResult result;
            HRESULT            hr = ReadUdf (CIsoImageBuilder::BuildUdf (EUdfVariant::MetadataPartition), result);



            Assert::AreEqual (HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED), hr);
        }




        TEST_METHOD(UdfRead_NoRecognitionSequence_ReturnsNotFound)
        {
            SArchiveReadResult result;
            HRESULT            hr = ReadUdf (CIsoImageBuilder::BuildPrimaryOnly(), result);



            Assert::AreEqual (HRESULT_FROM_WIN32 (ERROR_NOT_FOUND), hr);
        }




        TEST_METHOD(UdfRead_CorruptAnchorChecksum_ReturnsBadFormat)
        {
            vector<BYTE>       vImage = CIsoImageBuilder::BuildUdf (EUdfVariant::Standard);
            SArchiveReadResult result;
            HRESULT            hrUdf  = S_OK;
            HRESULT            hrIso  = S_OK;



            vImage[CIsoImageBuilder::s_kAnchorSector * CIsoImageBuilder::s_kcbSector] ^= s_kbCorrupt;

            hrUdf = ReadUdf (vImage, result);
            hrIso = ReadIso (vImage, result);

            Assert::AreEqual (HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT), hrUdf);
            Assert::AreEqual (HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT), hrIso);
        }




    private:

        static constexpr size_t   s_kcPrimaryEntries     = 5;
        static constexpr size_t   s_kcJolietEntries      = 3;
        static constexpr size_t   s_kcMultiExtentEntries = 2;
        static constexpr size_t   s_kcUdfEntries         = 9;
        static constexpr size_t   s_kibStandardId        = 1;
        static constexpr BYTE     s_kbCorrupt            = 'X';
        static constexpr BYTE     s_kcbTooShortRecord    = 10;
        static constexpr BYTE     s_kbFill               = 0xAB;
        static constexpr size_t   s_kcbNonIso            = 64 * 1024;
        static constexpr size_t   s_kcbTiny              = 100;




        static HRESULT ReadIso (vector<BYTE> vImage, SArchiveReadResult & result)
        {
            CMemoryByteSource source (std::move (vImage));



            return CIsoReader::Read (source, result);
        }




        static HRESULT ReadUdf (vector<BYTE> vImage, SArchiveReadResult & result)
        {
            CMemoryByteSource source (std::move (vImage));



            return CUdfReader::Read (source, result);
        }




        static const SArchiveEntry * Find (const SArchiveReadResult & result, LPCWSTR pszPath)
        {
            const SArchiveEntry * pFound = nullptr;



            for (const SArchiveEntry & entry : result.m_vEntries)
            {
                if (pFound == nullptr && entry.m_strPath == pszPath)
                {
                    pFound = &entry;
                }
            }

            return pFound;
        }
    };
}





