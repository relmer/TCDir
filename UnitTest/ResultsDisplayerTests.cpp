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
    //
    // Probe class to access protected members of CResultsDisplayerNormal
    //

    struct NormalDisplayerProbe : public CResultsDisplayerNormal
    {
        NormalDisplayerProbe (std::shared_ptr<CCommandLine> cmd, std::shared_ptr<CConsole> con, std::shared_ptr<CConfig> cfg)
            : CResultsDisplayerNormal (cmd, con, cfg) {}

        void DisplayFileResults (const CDirectoryInfo &) override {}

        // Expose protected members for testing
        LPCWSTR      WrapFormatNumber   (ULONGLONG n)                                            { return FormatNumberWithSeparators (n);    }
        ECloudStatus WrapGetCloudStatus (const WIN32_FIND_DATA & wfd, bool fInSyncRoot = false)  { return GetCloudStatus (wfd, fInSyncRoot); }
    };





    //
    // Helper to create a mock WIN32_FIND_DATA with specified attributes
    //

    WIN32_FIND_DATA CreateMockFileData (LPCWSTR pszName, DWORD dwAttributes, ULONGLONG ullSize = 0)
    {
        WIN32_FIND_DATA fd = { 0 };

        fd.dwFileAttributes  = dwAttributes;
        fd.nFileSizeLow      = static_cast<DWORD>(ullSize & 0xFFFFFFFF);
        fd.nFileSizeHigh     = static_cast<DWORD>(ullSize >> 32);

        wcscpy_s (fd.cFileName, pszName);

        return fd;
    }





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
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);



            std::wstring s0     = probe.WrapFormatNumber(0ull);
            std::wstring s1234  = probe.WrapFormatNumber(1234ull);
            std::wstring s1m    = probe.WrapFormatNumber(1000000ull);

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




        //
        // GetCloudStatus tests via probe
        //

        TEST_METHOD(GetCloudStatus_None_RegularFile)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);

            WIN32_FIND_DATA fd = CreateMockFileData(L"regular.txt", FILE_ATTRIBUTE_ARCHIVE);



            ECloudStatus status = probe.WrapGetCloudStatus(fd);

            Assert::AreEqual(static_cast<int>(ECloudStatus::CS_NONE), static_cast<int>(status));
        }




        TEST_METHOD(GetCloudStatus_CloudOnly_Offline)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);

            WIN32_FIND_DATA fd = CreateMockFileData(L"cloud.txt", FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_OFFLINE);



            ECloudStatus status = probe.WrapGetCloudStatus(fd);

            Assert::AreEqual(static_cast<int>(ECloudStatus::CS_CLOUD_ONLY), static_cast<int>(status));
        }




        TEST_METHOD(GetCloudStatus_CloudOnly_RecallOnDataAccess)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);

            WIN32_FIND_DATA fd = CreateMockFileData(L"cloud.txt", FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS);



            ECloudStatus status = probe.WrapGetCloudStatus(fd);

            Assert::AreEqual(static_cast<int>(ECloudStatus::CS_CLOUD_ONLY), static_cast<int>(status));
        }




        TEST_METHOD(GetCloudStatus_CloudOnly_RecallOnOpen)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);

            WIN32_FIND_DATA fd = CreateMockFileData(L"cloud.txt", FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_RECALL_ON_OPEN);



            ECloudStatus status = probe.WrapGetCloudStatus(fd);

            Assert::AreEqual(static_cast<int>(ECloudStatus::CS_CLOUD_ONLY), static_cast<int>(status));
        }




        TEST_METHOD(GetCloudStatus_Local_Unpinned)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);

            WIN32_FIND_DATA fd = CreateMockFileData(L"local.txt", FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_UNPINNED);



            ECloudStatus status = probe.WrapGetCloudStatus(fd);

            Assert::AreEqual(static_cast<int>(ECloudStatus::CS_LOCAL), static_cast<int>(status));
        }




        TEST_METHOD(GetCloudStatus_Pinned)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);

            WIN32_FIND_DATA fd = CreateMockFileData(L"pinned.txt", FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_PINNED);



            ECloudStatus status = probe.WrapGetCloudStatus(fd);

            Assert::AreEqual(static_cast<int>(ECloudStatus::CS_PINNED), static_cast<int>(status));
        }




        TEST_METHOD(GetCloudStatus_PinnedTakesPriority)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);

            // File has both PINNED and OFFLINE - pinned should win
            WIN32_FIND_DATA fd = CreateMockFileData(L"pinned.txt", 
                FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_PINNED | FILE_ATTRIBUTE_OFFLINE);



            ECloudStatus status = probe.WrapGetCloudStatus(fd);

            Assert::AreEqual(static_cast<int>(ECloudStatus::CS_PINNED), static_cast<int>(status));
        }




        //
        // Attribute filtering tests - verify /A: switches filter correctly
        //

        TEST_METHOD(AttributeFilter_CloudOnly_MatchesOffline)
        {
            CCommandLine cl;
            const wchar_t* arg = L"/A:O";
            wchar_t* argv[] = { const_cast<wchar_t*>(arg) };
            HRESULT hr = cl.Parse(1, argv);

            WIN32_FIND_DATA fd = CreateMockFileData(L"cloud.txt", 
                FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_OFFLINE);



            Assert::IsTrue(SUCCEEDED(hr));
            // File should match: has OFFLINE which is part of the O composite mask
            bool matches = (fd.dwFileAttributes & cl.m_dwAttributesRequired) != 0;
            Assert::IsTrue(matches);
        }




        TEST_METHOD(AttributeFilter_CloudOnly_DoesNotMatchLocal)
        {
            CCommandLine cl;
            const wchar_t* arg = L"/A:O";
            wchar_t* argv[] = { const_cast<wchar_t*>(arg) };
            HRESULT hr = cl.Parse(1, argv);

            WIN32_FIND_DATA fd = CreateMockFileData(L"local.txt", 
                FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_UNPINNED);



            Assert::IsTrue(SUCCEEDED(hr));
            // File should NOT match: has UNPINNED but none of the O composite bits
            bool matches = (fd.dwFileAttributes & cl.m_dwAttributesRequired) != 0;
            Assert::IsFalse(matches);
        }




        TEST_METHOD(AttributeFilter_AlwaysLocallyAvailable_MatchesPinnedFile)
        {
            CCommandLine cl;
            const wchar_t* arg = L"/A:V";
            wchar_t* argv[] = { const_cast<wchar_t*>(arg) };
            HRESULT hr = cl.Parse(1, argv);

            WIN32_FIND_DATA fd = CreateMockFileData(L"pinned.txt", 
                FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_PINNED);



            Assert::IsTrue(SUCCEEDED(hr));
            bool matches = (fd.dwFileAttributes & cl.m_dwAttributesRequired) != 0;
            Assert::IsTrue(matches);
        }




        TEST_METHOD(AttributeFilter_LocallyAvailable_MatchesLocalFile)
        {
            CCommandLine cl;
            const wchar_t* arg = L"/A:L";
            wchar_t* argv[] = { const_cast<wchar_t*>(arg) };
            HRESULT hr = cl.Parse(1, argv);

            WIN32_FIND_DATA fd = CreateMockFileData(L"local.txt", 
                FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_UNPINNED);



            Assert::IsTrue(SUCCEEDED(hr));
            bool matches = (fd.dwFileAttributes & cl.m_dwAttributesRequired) != 0;
            Assert::IsTrue(matches);
        }




        TEST_METHOD(AttributeFilter_ExcludeCloudOnly)
        {
            CCommandLine cl;
            const wchar_t* arg = L"/A:-O";
            wchar_t* argv[] = { const_cast<wchar_t*>(arg) };
            HRESULT hr = cl.Parse(1, argv);

            WIN32_FIND_DATA fdCloud = CreateMockFileData(L"cloud.txt", 
                FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_OFFLINE);
            WIN32_FIND_DATA fdLocal = CreateMockFileData(L"local.txt", 
                FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_UNPINNED);



            Assert::IsTrue(SUCCEEDED(hr));
            
            // Cloud file should be excluded (has OFFLINE)
            bool cloudExcluded = (fdCloud.dwFileAttributes & cl.m_dwAttributesExcluded) != 0;
            Assert::IsTrue(cloudExcluded);

            // Local file should NOT be excluded (no cloud-only attributes)
            bool localExcluded = (fdLocal.dwFileAttributes & cl.m_dwAttributesExcluded) != 0;
            Assert::IsFalse(localExcluded);
        }




        TEST_METHOD(GetCloudStatus_BothPinnedAndUnpinned_PinnedWins)
        {
            // OneDrive system files can have both PINNED and UNPINNED bits set
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);

            WIN32_FIND_DATA fd = CreateMockFileData(L"desktop.ini", 
                FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE |
                FILE_ATTRIBUTE_PINNED | FILE_ATTRIBUTE_UNPINNED);



            // PINNED should take priority for display
            ECloudStatus status = probe.WrapGetCloudStatus(fd);
            Assert::AreEqual(static_cast<int>(ECloudStatus::CS_PINNED), static_cast<int>(status));
        }




        TEST_METHOD(AttributeFilter_BothAlwaysAndLocallyAvailable_MatchesBothFilters)
        {
            // Files with both bits set match both /A:V and /A:L (bitwise match)
            WIN32_FIND_DATA fd = CreateMockFileData(L"desktop.ini", 
                FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_PINNED | FILE_ATTRIBUTE_UNPINNED);

            CCommandLine clV;
            const wchar_t* argV = L"/A:V";
            wchar_t* argvV[] = { const_cast<wchar_t*>(argV) };
            clV.Parse(1, argvV);

            CCommandLine clL;
            const wchar_t* argL = L"/A:L";
            wchar_t* argvL[] = { const_cast<wchar_t*>(argL) };
            clL.Parse(1, argvL);



            // Should match /A:V (PINNED bit is set)
            bool matchesV = (fd.dwFileAttributes & clV.m_dwAttributesRequired) != 0;
            Assert::IsTrue(matchesV);

            // Should also match /A:L (UNPINNED bit is also set)
            bool matchesL = (fd.dwFileAttributes & clL.m_dwAttributesRequired) != 0;
            Assert::IsTrue(matchesL);
        }

    };
}
