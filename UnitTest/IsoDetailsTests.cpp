#include "pch.h"
#include "EhmTestHelper.h"
#include "Mocks/IsoImageBuilder.h"
#include "../TCDirCore/ByteSource.h"
#include "../TCDirCore/IsoDetails.h"
#include "../TCDirCore/IsoReader.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(IsoDetailsTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        static constexpr size_t s_kcbSector = 2048;

        static void PutText (vector<BYTE> & sector, size_t ib, size_t cb, const string & text)
        {
            memset (sector.data() + ib, ' ', cb);
            memcpy (sector.data() + ib, text.data(), min (cb, text.size()));
        }

        static void PutUcs2 (vector<BYTE> & sector, size_t ib, size_t cb, const wstring & text)
        {
            memset (sector.data() + ib, 0, cb);

            for (size_t i = 0; i < text.size() && (i + 1) * 2 <= cb; ++i)
            {
                sector[ib + i * 2]     = static_cast<BYTE> (text[i] >> 8);
                sector[ib + i * 2 + 1] = static_cast<BYTE> (text[i]);
            }
        }

        static vector<BYTE> Descriptor (BYTE bType)
        {
            vector<BYTE> sector (s_kcbSector, 0);

            sector[0] = bType;
            memcpy (sector.data() + 1, "CD001", 5);
            return sector;
        }

        static vector<BYTE> PrimaryDescriptor()
        {
            vector<BYTE> sector = Descriptor (1);

            PutText (sector, 8,   32,  "WIN32");
            PutText (sector, 40,  32,  "MY_DISC");
            PutText (sector, 318, 128, "");
            PutText (sector, 446, 128, "PREPARER");
            PutText (sector, 574, 128, "MKISOFS");
            sector[80]  = 0x10;                                       // Volume space size 0x2710 = 10000
            sector[81]  = 0x27;
            sector[128] = 0x00;                                       // Logical block size 2048
            sector[129] = 0x08;
            memcpy (sector.data() + 813, "2024031510300000", 16);     // Created 2024-03-15 10:30 at UTC+1
            sector[813 + 16] = 4;
            memset (sector.data() + 830, '0', 16);                    // Modified: unset
            sector[158] = 30;                                         // Root extent 30
            return sector;
        }

        static void ParseAll (const vector<vector<BYTE>> & sectors, SDiscDetails & disc, ULONG & ulRoot, ULONG & ulCatalog)
        {
            bool fEnd = false;

            for (const vector<BYTE> & sector : sectors)
            {
                CIsoDetails::ParseVolumeDescriptor (sector.data(), disc, ulRoot, ulCatalog, fEnd);
            }
        }




        TEST_METHOD(ParseVolumeDescriptor_PrimaryFields)
        {
            SDiscDetails disc;
            ULONG        ulRoot    = 0;
            ULONG        ulCatalog = 0;



            ParseAll ({ PrimaryDescriptor() }, disc, ulRoot, ulCatalog);

            Assert::IsTrue   (disc.m_fIso9660);
            Assert::AreEqual (L"MY_DISC",  disc.m_strVolumeLabel.c_str());
            Assert::AreEqual (L"WIN32",    disc.m_strSystemId.c_str());
            Assert::AreEqual (L"PREPARER", disc.m_strPreparer.c_str());
            Assert::AreEqual (L"MKISOFS",  disc.m_strApplication.c_str());
            Assert::IsTrue   (disc.m_strPublisher.empty());
            Assert::AreEqual (2048ul,      disc.m_ulSectorSize);
            Assert::AreEqual (10000ull,    disc.m_ullSectorCount);
            Assert::AreEqual (30ul,        ulRoot);
            Assert::IsTrue   (disc.m_ftCreated.has_value());
            Assert::IsFalse  (disc.m_ftModified.has_value());
            Assert::IsFalse  (disc.m_ftExpires.has_value());
        }




        TEST_METHOD(ParseVolumeDate_AppliesUtcOffset)
        {
            BYTE                 date[17] = {};
            optional<FILETIME>   ft;
            SYSTEMTIME           st       = {};



            memcpy (date, "2024031510300000", 16);
            date[16] = 4;                                             // UTC+1

            ft = CIsoDetails::ParseVolumeDate (date);

            Assert::IsTrue   (ft.has_value());
            FileTimeToSystemTime (&ft.value(), &st);
            Assert::AreEqual ((WORD) 2024, st.wYear);
            Assert::AreEqual ((WORD) 9,    st.wHour);
            Assert::AreEqual ((WORD) 30,   st.wMinute);

            memset (date, 0, sizeof (date));
            Assert::IsFalse (CIsoDetails::ParseVolumeDate (date).has_value());
        }




        TEST_METHOD(ParseVolumeDescriptor_JolietReplacesPrimaryText)
        {
            SDiscDetails   disc;
            ULONG          ulRoot    = 0;
            ULONG          ulCatalog = 0;
            vector<BYTE>   joliet    = Descriptor (2);



            memcpy (joliet.data() + 88, "%/E", 3);
            PutUcs2 (joliet, 40, 32, L"My Disc");

            ParseAll ({ PrimaryDescriptor(), joliet }, disc, ulRoot, ulCatalog);

            Assert::IsTrue   (disc.m_fJoliet);
            Assert::AreEqual (L"My Disc", disc.m_strVolumeLabel.c_str());
            Assert::AreEqual (L"WIN32",   disc.m_strSystemId.c_str());      // Blank in Joliet, so the primary value applies
        }




        TEST_METHOD(ParseVolumeDescriptor_BootRecordAndTerminator)
        {
            SDiscDetails   disc;
            ULONG          ulRoot    = 0;
            ULONG          ulCatalog = 0;
            bool           fEnd      = false;
            vector<BYTE>   boot      = Descriptor (0);
            vector<BYTE>   other     (s_kcbSector, 0);



            memcpy (boot.data() + 7, "EL TORITO SPECIFICATION", 23);
            boot[71] = 40;

            CIsoDetails::ParseVolumeDescriptor (boot.data(), disc, ulRoot, ulCatalog, fEnd);
            Assert::AreEqual (40ul, ulCatalog);
            Assert::IsFalse  (fEnd);

            CIsoDetails::ParseVolumeDescriptor (Descriptor (255).data(), disc, ulRoot, ulCatalog, fEnd);
            Assert::IsTrue (fEnd);

            CIsoDetails::ParseVolumeDescriptor (other.data(), disc, ulRoot, ulCatalog, fEnd);
            Assert::IsTrue (fEnd);
        }




        TEST_METHOD(HasRockRidge_SpThenRr)
        {
            vector<BYTE> record (64, 0);



            record[0]  = 48;                         // Record length
            record[32] = 1;                          // "." identifier
            memcpy (record.data() + 34, "SP\x07\x01\xBE\xEF\x00", 7);
            memcpy (record.data() + 41, "RR\x05\x01\x81", 5);

            Assert::IsTrue (CIsoDetails::HasRockRidge (record.data(), record.size()));

            memcpy (record.data() + 34, "XX", 2);
            Assert::IsFalse (CIsoDetails::HasRockRidge (record.data(), record.size()));
        }




        TEST_METHOD(ParseBootCatalog_ValidationAndSections)
        {
            vector<BYTE> catalog (s_kcbSector, 0);
            vector<BYTE> platforms;



            catalog[0]       = 0x01;                 // Validation entry: BIOS
            catalog[1]       = 0x00;
            catalog[32]      = 0x88;                 // Default entry
            catalog[64]      = 0x91;                 // Final section header: UEFI
            catalog[64 + 1]  = 0xEF;

            CIsoDetails::ParseBootCatalog (catalog.data(), catalog.size(), platforms);

            Assert::AreEqual (size_t (2), platforms.size());
            Assert::AreEqual ((BYTE) 0x00, platforms[0]);
            Assert::AreEqual ((BYTE) 0xEF, platforms[1]);

            catalog[0] = 0;
            CIsoDetails::ParseBootCatalog (catalog.data(), catalog.size(), platforms);
            Assert::IsTrue (platforms.empty());
        }




        TEST_METHOD(ParseLogicalVolume_RevisionAndLabel)
        {
            vector<BYTE>   sector (s_kcbSector, 0);
            SDiscDetails   disc;



            sector[84]        = 8;                   // Compression ID
            memcpy (sector.data() + 85, "MyUdf", 5);
            sector[84 + 127]  = 6;                   // Used length
            sector[216 + 24]  = 0x50;                // Revision 0x0250
            sector[216 + 25]  = 0x02;

            CIsoDetails::ParseLogicalVolume (sector.data(), disc);

            Assert::AreEqual ((WORD) 0x0250, disc.m_wUdfRevision);
            Assert::AreEqual (L"MyUdf", disc.m_strVolumeLabel.c_str());
        }




        TEST_METHOD(Read_ListedTreeAndSectors)
        {
            struct SCase
            {
                vector<BYTE>   m_image;
                EDiscTree      m_eTree;
                bool           m_fJoliet;
            };

            vector<SCase> cases;

            cases.push_back ({ CIsoImageBuilder::BuildPrimaryOnly(),               EDiscTree::Primary, false });
            cases.push_back ({ CIsoImageBuilder::BuildPrimaryAndJoliet(),          EDiscTree::Joliet,  true  });
            cases.push_back ({ CIsoImageBuilder::BuildUdf (EUdfVariant::Standard), EDiscTree::Udf,     false });



            for (SCase & testCase : cases)
            {
                ULONGLONG           cbImage = testCase.m_image.size();
                CMemoryByteSource   source  (move (testCase.m_image));
                SArchiveReadResult  result;

                result.m_detailsPtr = make_shared<SArchiveDetails>();

                Assert::AreEqual (S_OK, CIsoReader::Read (source, result));
                Assert::IsTrue   (result.m_detailsPtr->m_disc.m_eListedTree == testCase.m_eTree);
                Assert::AreEqual (testCase.m_fJoliet, result.m_detailsPtr->m_disc.m_fJoliet);
                Assert::IsTrue   (result.m_detailsPtr->m_disc.m_ullSectorCount > 0);
                Assert::IsTrue   (result.m_detailsPtr->m_disc.m_ullSectorCount <= cbImage / s_kcbSector);
            }
        }
    };
}
