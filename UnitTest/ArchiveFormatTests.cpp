#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/ArchiveFormat.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(ArchiveFormatTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        TEST_METHOD(FromFileName_Zip_ReturnsZip)
        {
            AssertFormat (EArchiveFormat::Zip, L"photos.zip", false);
            AssertFormat (EArchiveFormat::Zip, L"photos.zip", true);
        }




        TEST_METHOD(FromFileName_UppercaseIso_ReturnsIso)
        {
            AssertFormat (EArchiveFormat::Iso, L"INSTALL.ISO", false);
        }




        TEST_METHOD(FromFileName_TarGz_ReturnsLibArchive)
        {
            AssertFormat (EArchiveFormat::LibArchive, L"source.tar.gz", true);
        }




        TEST_METHOD(FromFileName_Tgz_ReturnsLibArchive)
        {
            AssertFormat (EArchiveFormat::LibArchive, L"source.tgz", true);
        }




        TEST_METHOD(FromFileName_SevenZip_RequiresLibArchive)
        {
            AssertFormat (EArchiveFormat::LibArchive, L"backup.7z", true);
            AssertFormat (EArchiveFormat::None,       L"backup.7z", false);
        }




        TEST_METHOD(FromFileName_TarGzWithoutLibArchive_ReturnsNone)
        {
            AssertFormat (EArchiveFormat::None, L"source.tar.gz", false);
        }




        TEST_METHOD(FromFileName_BareGz_ReturnsNone)
        {
            AssertFormat (EArchiveFormat::None, L"server.log.gz", true);
        }




        TEST_METHOD(FromFileName_Docx_ReturnsNone)
        {
            AssertFormat (EArchiveFormat::None, L"report.docx", true);
        }




        TEST_METHOD(FromFileName_ExtensionOnly_ReturnsNone)
        {
            AssertFormat (EArchiveFormat::None, L".zip", true);
            AssertFormat (EArchiveFormat::None, L".7z",  true);
        }




    private:

        ////////////////////////////////////////////////////////////////////////////////
        //
        //  ArchiveFormatTests::AssertFormat
        //
        ////////////////////////////////////////////////////////////////////////////////

        static void AssertFormat (EArchiveFormat targetFormat, wstring_view fileName, bool fLibArchiveAvailable)
        {
            EArchiveFormat resultFormat = CArchiveFormat::FromFileName (fileName, fLibArchiveAvailable);



            Assert::AreEqual (static_cast<int> (targetFormat), static_cast<int> (resultFormat), wstring (fileName).c_str());
        }
    };
}
