#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/ResultsDisplayerWithHeaderAndFooter.h"
#include "../TCDirCore/Config.h"
#include "../TCDirCore/Console.h"
#include "../TCDirCore/CommandLine.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(ResultsDisplayerWithHeaderAndFooterTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            // Setup EHM to use unit test framework's assertion mechanism
            SetupEhmForUnitTests();
        }

        TEST_METHOD(FormatNumberWithSeparators_Samples)
        {
            struct DisplayerProbe : public CResultsDisplayerWithHeaderAndFooter
            {
                DisplayerProbe(std::shared_ptr<CCommandLine> a, std::shared_ptr<CConsole> b, std::shared_ptr<CConfig> c)
                    : CResultsDisplayerWithHeaderAndFooter(a, b, c) {}
                void    DisplayFileResults(const CDirectoryInfo &) override {}
                wstring WrapFormat(ULONGLONG n) { return FormatNumberWithSeparators(n); }
            };

            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);
            DisplayerProbe dp(cmd, con, cfg);

            std::wstring s0        = dp.WrapFormat(0ull);
            std::wstring s1        = dp.WrapFormat(1ull);
            std::wstring s1234     = dp.WrapFormat(1234ull);
            std::wstring s1000000  = dp.WrapFormat(1000000ull);

            Assert::AreEqual(L"0",           s0.c_str());
            Assert::AreEqual(L"1",           s1.c_str());
            Assert::AreEqual(L"1,234",       s1234.c_str());
            Assert::AreEqual(L"1,000,000",   s1000000.c_str());
        }

        TEST_METHOD(GetStringLengthOfMaxFileSize_Samples)
        {
            struct DisplayerProbe : public CResultsDisplayerWithHeaderAndFooter
            {
                DisplayerProbe(std::shared_ptr<CCommandLine> a, std::shared_ptr<CConsole> b, std::shared_ptr<CConfig> c)
                    : CResultsDisplayerWithHeaderAndFooter(a, b, c) {}
                void  DisplayFileResults(const CDirectoryInfo &) override {}
                UINT  WrapLen(ULONGLONG n) { ULARGE_INTEGER u; u.QuadPart = n; return GetStringLengthOfMaxFileSize(u); }
            };

            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);
            DisplayerProbe dp(cmd, con, cfg);

            Assert::AreEqual(1u, dp.WrapLen(0ull));           // "0"
            Assert::AreEqual(3u, dp.WrapLen(999ull));         // "999"
            Assert::AreEqual(5u, dp.WrapLen(1000ull));        // "1,000"
            Assert::AreEqual(9u, dp.WrapLen(1000000ull));     // "1,000,000"
        }

    };
}
