#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/IconMapping.h"
#include "../TCDirCore/Config.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(IconMappingTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }



        //
        //  Scenario 1: BMP code point produces count=1
        //

        TEST_METHOD(CodePointToWideChars_BMP_ProducesCount1)
        {
            constexpr WideCharPair pair = CodePointToWideChars (NfIcon::CustomCpp);

            Assert::AreEqual (1u, pair.count);
            Assert::AreEqual (static_cast<wchar_t>(0xE61D), pair.chars[0]);
        }



        TEST_METHOD(CodePointToWideChars_BMP_AllNfIconsInBmpRange)
        {
            //
            //  Verify a selection of BMP-range NfIcon constants produce count=1
            //

            Assert::AreEqual (1u, CodePointToWideChars (NfIcon::CustomC).count);
            Assert::AreEqual (1u, CodePointToWideChars (NfIcon::CustomFolder).count);
            Assert::AreEqual (1u, CodePointToWideChars (NfIcon::SetiConfig).count);
            Assert::AreEqual (1u, CodePointToWideChars (NfIcon::FaFile).count);
            Assert::AreEqual (1u, CodePointToWideChars (NfIcon::OctFileBinary).count);
            Assert::AreEqual (1u, CodePointToWideChars (NfIcon::CodFileSymlinkDir).count);
        }



        //
        //  Scenario 2: Supplementary code points produce count=2 with correct surrogates
        //

        TEST_METHOD(CodePointToWideChars_Supplementary_ProducesCount2)
        {
            constexpr WideCharPair pair = CodePointToWideChars (NfIcon::MdApplication);

            Assert::AreEqual (2u, pair.count);
            Assert::AreEqual (static_cast<wchar_t>(0xDB82), pair.chars[0]);
            Assert::AreEqual (static_cast<wchar_t>(0xDCC6), pair.chars[1]);
        }



        TEST_METHOD(CodePointToWideChars_AllMaterialDesignCodePoints)
        {
            //
            //  Verify every Material Design code point in NfIcon produces count=2
            //

            Assert::AreEqual (2u, CodePointToWideChars (NfIcon::MdApplication).count);
            Assert::AreEqual (2u, CodePointToWideChars (NfIcon::MdCloudCheck).count);
            Assert::AreEqual (2u, CodePointToWideChars (NfIcon::MdCloudOutline).count);
            Assert::AreEqual (2u, CodePointToWideChars (NfIcon::MdFileDocument).count);
            Assert::AreEqual (2u, CodePointToWideChars (NfIcon::MdFilePowerpoint).count);
            Assert::AreEqual (2u, CodePointToWideChars (NfIcon::MdPin).count);
            Assert::AreEqual (2u, CodePointToWideChars (NfIcon::MdTestTube).count);
        }



        TEST_METHOD(CodePointToWideChars_MdCloudOutline_ExactSurrogates)
        {
            //
            //  MdCloudOutline = 0xF0163
            //  offset = 0xF0163 - 0x10000 = 0xE0163
            //  high = 0xD800 + (0xE0163 >> 10) = 0xD800 + 0x380 = 0xDB80
            //  low  = 0xDC00 + (0xE0163 & 0x3FF) = 0xDC00 + 0x163 = 0xDD63
            //

            constexpr WideCharPair pair = CodePointToWideChars (NfIcon::MdCloudOutline);

            Assert::AreEqual (static_cast<wchar_t>(0xDB80), pair.chars[0]);
            Assert::AreEqual (static_cast<wchar_t>(0xDD63), pair.chars[1]);
        }



        //
        //  Edge cases: surrogates rejected, zero produces count=0
        //

        TEST_METHOD(CodePointToWideChars_SurrogateRejected)
        {
            Assert::AreEqual (0u, CodePointToWideChars (0xD800).count);
            Assert::AreEqual (0u, CodePointToWideChars (0xDBFF).count);
            Assert::AreEqual (0u, CodePointToWideChars (0xDC00).count);
            Assert::AreEqual (0u, CodePointToWideChars (0xDFFF).count);
        }



        TEST_METHOD(CodePointToWideChars_ZeroRejected)
        {
            Assert::AreEqual (0u, CodePointToWideChars (0).count);
        }



        TEST_METHOD(CodePointToWideChars_OutOfRangeRejected)
        {
            Assert::AreEqual (0u, CodePointToWideChars (0x110000).count);
            Assert::AreEqual (0u, CodePointToWideChars (0xFFFFFFFF).count);
        }



        //
        //  Scenario 3: Every extension in default table has non-zero code point
        //

        TEST_METHOD(DefaultExtensionTable_AllEntriesHaveNonZeroCodePoint)
        {
            for (size_t i = 0; i < g_cDefaultExtensionIcons; i++)
            {
                Assert::IsNotNull (g_rgDefaultExtensionIcons[i].m_pszKey,
                                   L"Extension key must not be null");
                Assert::IsTrue (g_rgDefaultExtensionIcons[i].m_codePoint != 0,
                                g_rgDefaultExtensionIcons[i].m_pszKey);
            }
        }



        TEST_METHOD(DefaultExtensionTable_NoDuplicateKeys)
        {
            unordered_set<wstring> seen;

            for (size_t i = 0; i < g_cDefaultExtensionIcons; i++)
            {
                wstring key = g_rgDefaultExtensionIcons[i].m_pszKey;
                Assert::IsTrue (seen.insert (key).second,
                                (L"Duplicate extension key: " + key).c_str ());
            }
        }



        //
        //  Scenario 4: Every well-known dir entry has non-zero code point, no duplicates
        //

        TEST_METHOD(DefaultWellKnownDirTable_AllEntriesHaveNonZeroCodePoint)
        {
            for (size_t i = 0; i < g_cDefaultWellKnownDirIcons; i++)
            {
                Assert::IsNotNull (g_rgDefaultWellKnownDirIcons[i].m_pszKey,
                                   L"Dir name key must not be null");
                Assert::IsTrue (g_rgDefaultWellKnownDirIcons[i].m_codePoint != 0,
                                g_rgDefaultWellKnownDirIcons[i].m_pszKey);
            }
        }



        TEST_METHOD(DefaultWellKnownDirTable_NoDuplicateKeys)
        {
            unordered_set<wstring> seen;

            for (size_t i = 0; i < g_cDefaultWellKnownDirIcons; i++)
            {
                wstring key = g_rgDefaultWellKnownDirIcons[i].m_pszKey;
                Assert::IsTrue (seen.insert (key).second,
                                (L"Duplicate dir name key: " + key).c_str ());
            }
        }



        //
        //  Scenario 5: Table completeness — every extension in color table
        //  has a matching entry in the icon table
        //

        TEST_METHOD(DefaultExtensionTable_CoversColorTable)
        {
            //
            //  Build a set of icon table extensions for fast lookup
            //

            unordered_set<wstring> iconExtensions;
            for (size_t i = 0; i < g_cDefaultExtensionIcons; i++)
            {
                iconExtensions.insert (g_rgDefaultExtensionIcons[i].m_pszKey);
            }

            //
            //  Check that every extension in the color default table (s_rgTextAttrs)
            //  also has an icon entry. We get the color table via CConfig::Initialize
            //  and then check its extension map.
            //

            CConfig config;
            config.Initialize (0x07);  // Default grey-on-black

            for (const auto & [ext, attr] : config.m_mapExtensionToTextAttr)
            {
                Assert::IsTrue (iconExtensions.count (ext) > 0,
                                (L"Color table extension missing from icon table: " + ext).c_str ());
            }
        }



        //
        //  Attribute precedence array
        //

        TEST_METHOD(AttributePrecedenceArray_HasExpectedCount)
        {
            Assert::AreEqual (static_cast<size_t>(9), g_cAttributePrecedenceOrder);
        }



        TEST_METHOD(AttributePrecedenceArray_ReparsePointIsHighestPriority)
        {
            Assert::AreEqual (static_cast<DWORD>(FILE_ATTRIBUTE_REPARSE_POINT),
                              g_rgAttributePrecedenceOrder[0].m_dwAttribute);
        }



        TEST_METHOD(AttributePrecedenceArray_ArchiveIsLowestPriority)
        {
            Assert::AreEqual (static_cast<DWORD>(FILE_ATTRIBUTE_ARCHIVE),
                              g_rgAttributePrecedenceOrder[g_cAttributePrecedenceOrder - 1].m_dwAttribute);
        }

    };
}
