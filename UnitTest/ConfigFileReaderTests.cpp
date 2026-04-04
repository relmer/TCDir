#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/ConfigFileReader.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    ////////////////////////////////////////////////////////////////////////////////
    //
    //  CTestConfigFileReader (mock)
    //
    //  Implements IConfigFileReader for unit tests.  Returns in-memory lines.
    //  Can simulate file-not-found (S_FALSE) and I/O errors (E_FAIL).
    //
    ////////////////////////////////////////////////////////////////////////////////

    class CTestConfigFileReader : public IConfigFileReader
    {
    public:
        void Set (vector<wstring> lines)
        {
            m_lines      = std::move (lines);
            m_hrResult   = S_OK;
            m_fConfigured = true;
        }

        void SetNotFound (void)
        {
            m_hrResult   = S_FALSE;
            m_fConfigured = true;
        }

        void SetError (wstring errorMessage)
        {
            m_hrResult      = E_FAIL;
            m_errorMessage  = std::move (errorMessage);
            m_fConfigured    = true;
        }

        HRESULT ReadLines (const wstring & /*path*/, vector<wstring> & lines, wstring & errorMessage) override
        {
            ASSERT (m_fConfigured);

            if (m_hrResult == S_OK)
            {
                lines = m_lines;
            }
            else if (FAILED (m_hrResult))
            {
                errorMessage = m_errorMessage;
            }

            return m_hrResult;
        }

    private:
        vector<wstring> m_lines;
        wstring         m_errorMessage;
        HRESULT         m_hrResult   = S_FALSE;
        bool            m_fConfigured = false;
    };





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
            istringstream      stream ("");
            CConfigFileReader  reader (&stream);
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines (L"unused", lines, errorMsg);

            Assert::AreEqual (S_OK, hr);
            Assert::IsTrue (errorMsg.empty());
            Assert::AreEqual (static_cast<size_t>(0), lines.size());
        }

        TEST_METHOD (ReadLines_SingleLine_NoNewline)
        {
            istringstream      stream ("hello world");
            CConfigFileReader  reader (&stream);
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines (L"unused", lines, errorMsg);

            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (static_cast<size_t>(1), lines.size());
            Assert::AreEqual (wstring (L"hello world"), lines[0]);
        }

        TEST_METHOD (ReadLines_CrLfSplitting)
        {
            istringstream      stream ("line1\r\nline2\r\nline3");
            CConfigFileReader  reader (&stream);
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines (L"unused", lines, errorMsg);

            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (static_cast<size_t>(3), lines.size());
            Assert::AreEqual (wstring (L"line1"), lines[0]);
            Assert::AreEqual (wstring (L"line2"), lines[1]);
            Assert::AreEqual (wstring (L"line3"), lines[2]);
        }

        TEST_METHOD (ReadLines_LfSplitting)
        {
            istringstream      stream ("line1\nline2\nline3");
            CConfigFileReader  reader (&stream);
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines (L"unused", lines, errorMsg);

            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (static_cast<size_t>(3), lines.size());
            Assert::AreEqual (wstring (L"line1"), lines[0]);
            Assert::AreEqual (wstring (L"line2"), lines[1]);
            Assert::AreEqual (wstring (L"line3"), lines[2]);
        }

        TEST_METHOD (ReadLines_CrSplitting)
        {
            istringstream      stream ("line1\rline2\rline3");
            CConfigFileReader  reader (&stream);
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines (L"unused", lines, errorMsg);

            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (static_cast<size_t>(3), lines.size());
            Assert::AreEqual (wstring (L"line1"), lines[0]);
            Assert::AreEqual (wstring (L"line2"), lines[1]);
            Assert::AreEqual (wstring (L"line3"), lines[2]);
        }

        TEST_METHOD (ReadLines_Utf8Bom_Stripped)
        {
            string             raw = "\xEF\xBB\xBFhello";
            istringstream      stream (raw);
            CConfigFileReader  reader (&stream);
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines (L"unused", lines, errorMsg);

            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (static_cast<size_t>(1), lines.size());
            Assert::AreEqual (wstring (L"hello"), lines[0]);
        }

        TEST_METHOD (ReadLines_Utf16LeBom_Rejected)
        {
            string             raw = "\xFF\xFEhello";
            istringstream      stream (raw);
            CConfigFileReader  reader (&stream);
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines (L"unused", lines, errorMsg);

            Assert::AreEqual (E_FAIL, hr);
            Assert::IsTrue (errorMsg.find (L"UTF-16 LE") != wstring::npos);
        }

        TEST_METHOD (ReadLines_Utf16BeBom_Rejected)
        {
            string             raw = "\xFE\xFFhello";
            istringstream      stream (raw);
            CConfigFileReader  reader (&stream);
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines (L"unused", lines, errorMsg);

            Assert::AreEqual (E_FAIL, hr);
            Assert::IsTrue (errorMsg.find (L"UTF-16 BE") != wstring::npos);
        }

        TEST_METHOD (ReadLines_Utf8Content_ConvertsCorrectly)
        {
            // "café" in UTF-8
            string             raw = "caf\xC3\xA9";
            istringstream      stream (raw);
            CConfigFileReader  reader (&stream);
            vector<wstring>    lines;
            wstring            errorMsg;



            HRESULT hr = reader.ReadLines (L"unused", lines, errorMsg);

            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (static_cast<size_t>(1), lines.size());
            Assert::AreEqual (wstring (L"caf\x00E9"), lines[0]);
        }

        TEST_METHOD (MockReader_NotFound_ReturnsSFalse)
        {
            CTestConfigFileReader mock;
            vector<wstring>       lines;
            wstring               errorMsg;



            mock.SetNotFound();

            HRESULT hr = mock.ReadLines (L"test", lines, errorMsg);

            Assert::AreEqual (S_FALSE, hr);
        }

        TEST_METHOD (MockReader_Error_ReturnsEFail)
        {
            CTestConfigFileReader mock;
            vector<wstring>       lines;
            wstring               errorMsg;



            mock.SetError (L"Access denied");

            HRESULT hr = mock.ReadLines (L"test", lines, errorMsg);

            Assert::AreEqual (E_FAIL, hr);
            Assert::AreEqual (wstring (L"Access denied"), errorMsg);
        }

        TEST_METHOD (MockReader_Lines_ReturnsThem)
        {
            CTestConfigFileReader mock;
            vector<wstring>       lines;
            wstring               errorMsg;



            mock.Set ({ L"line1", L"line2" });

            HRESULT hr = mock.ReadLines (L"test", lines, errorMsg);

            Assert::AreEqual (S_OK, hr);
            Assert::AreEqual (static_cast<size_t>(2), lines.size());
            Assert::AreEqual (wstring (L"line1"), lines[0]);
            Assert::AreEqual (wstring (L"line2"), lines[1]);
        }
    };
}
