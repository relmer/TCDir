#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/ConfigFileReader.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    ////////////////////////////////////////////////////////////////////////////////
    //
    //  ConfigFileReaderTests
    //
    ////////////////////////////////////////////////////////////////////////////////

    TEST_CLASS (ConfigFileReaderTests)
    {
    public:

        TEST_METHOD (ReadLines_EmptyStream_ReturnsEmptyVector)
        {
            CConfigFileReader  reader;
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines ("", lines, errorMsg);

            Assert::AreEqual (S_OK, hr);
            Assert::IsTrue (errorMsg.empty());
            Assert::AreEqual (static_cast<size_t>(0), lines.size());
        }

        TEST_METHOD (ReadLines_SingleLine_NoNewline)
        {
            CConfigFileReader  reader;
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines ("hello world", lines, errorMsg);

            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (static_cast<size_t>(1), lines.size());
            Assert::AreEqual (wstring (L"hello world"), lines[0]);
        }

        TEST_METHOD (ReadLines_CrLfSplitting)
        {
            CConfigFileReader  reader;
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines ("line1\r\nline2\r\nline3", lines, errorMsg);

            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (static_cast<size_t>(3), lines.size());
            Assert::AreEqual (wstring (L"line1"), lines[0]);
            Assert::AreEqual (wstring (L"line2"), lines[1]);
            Assert::AreEqual (wstring (L"line3"), lines[2]);
        }

        TEST_METHOD (ReadLines_LfSplitting)
        {
            CConfigFileReader  reader;
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines ("line1\nline2\nline3", lines, errorMsg);

            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (static_cast<size_t>(3), lines.size());
            Assert::AreEqual (wstring (L"line1"), lines[0]);
            Assert::AreEqual (wstring (L"line2"), lines[1]);
            Assert::AreEqual (wstring (L"line3"), lines[2]);
        }

        TEST_METHOD (ReadLines_CrSplitting)
        {
            CConfigFileReader  reader;
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines ("line1\rline2\rline3", lines, errorMsg);

            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (static_cast<size_t>(3), lines.size());
            Assert::AreEqual (wstring (L"line1"), lines[0]);
            Assert::AreEqual (wstring (L"line2"), lines[1]);
            Assert::AreEqual (wstring (L"line3"), lines[2]);
        }

        TEST_METHOD (ReadLines_Utf8Bom_Stripped)
        {
            string             raw = "\xEF\xBB\xBFhello";
            CConfigFileReader  reader;
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines (raw, lines, errorMsg);

            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (static_cast<size_t>(1), lines.size());
            Assert::AreEqual (wstring (L"hello"), lines[0]);
        }

        TEST_METHOD (ReadLines_Utf16LeBom_Rejected)
        {
            string             raw = "\xFF\xFEhello";
            CConfigFileReader  reader;
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines (raw, lines, errorMsg);

            Assert::AreEqual (E_FAIL, hr);
            Assert::IsTrue (errorMsg.find (L"UTF-16 LE") != wstring::npos);
        }

        TEST_METHOD (ReadLines_Utf16BeBom_Rejected)
        {
            string             raw = "\xFE\xFFhello";
            CConfigFileReader  reader;
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines (raw, lines, errorMsg);

            Assert::AreEqual (E_FAIL, hr);
            Assert::IsTrue (errorMsg.find (L"UTF-16 BE") != wstring::npos);
        }

        TEST_METHOD (ReadLines_Utf8Content_ConvertsCorrectly)
        {
            // "café" in UTF-8
            string             raw = "caf\xC3\xA9";
            CConfigFileReader  reader;
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines (raw, lines, errorMsg);

            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (static_cast<size_t>(1), lines.size());
            Assert::AreEqual (wstring (L"caf\x00E9"), lines[0]);
        }
    };
}
