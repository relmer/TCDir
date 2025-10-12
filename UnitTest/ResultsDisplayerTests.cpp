#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/ResultsDisplayerNormal.h"
#include "../TCDirCore/ResultsDisplayerWide.h"
#include "../TCDirCore/Config.h"
#include "../TCDirCore/Console.h"
#include "../TCDirCore/CommandLine.h"
#include "../TCDirCore/DirectoryInfo.h"







using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{
    TEST_CLASS(ResultsDisplayerTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            // Setup EHM to use unit test framework's assertion mechanism
            SetupEhmForUnitTests();
        }





        TEST_METHOD(Normal_FormatNumber_Samples)
        {
            struct NormalProbe : public CResultsDisplayerNormal
            {
                NormalProbe(std::shared_ptr<CCommandLine> a, std::shared_ptr<CConsole> b, std::shared_ptr<CConfig> c)
                    : CResultsDisplayerNormal(a, b, c) {}
                void DisplayFileResults(const CDirectoryInfo &) override {}
                LPCWSTR Wrap(ULONGLONG n) { return FormatNumberWithSeparators(n); }
            };

            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);
            NormalProbe np(cmd, con, cfg);



            std::wstring s0     = np.Wrap(0ull);
            std::wstring s1234  = np.Wrap(1234ull);
            std::wstring s1m    = np.Wrap(1000000ull);

            Assert::AreEqual(L"0",         s0.c_str());
            Assert::AreEqual(L"1,234",     s1234.c_str());
            Assert::AreEqual(L"1,000,000", s1m.c_str());
        }





        TEST_METHOD(Wide_FormatNumber_Samples)
        {
            struct WideProbe : public CResultsDisplayerWide
            {
                WideProbe(std::shared_ptr<CCommandLine> a, std::shared_ptr<CConsole> b, std::shared_ptr<CConfig> c)
                    : CResultsDisplayerWide(a, b, c) {}
                void DisplayFileResults(const CDirectoryInfo &) override {}
                LPCWSTR Wrap(ULONGLONG n) { return FormatNumberWithSeparators(n); }
            };

            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);
            WideProbe wp(cmd, con, cfg);



            std::wstring s9     = wp.Wrap(999ull);
            std::wstring s10k   = wp.Wrap(10000ull);

            Assert::AreEqual(L"999",    s9.c_str());
            Assert::AreEqual(L"10,000", s10k.c_str());
        }

    };
}
