#include "pch.h"
#include "EhmTestHelper.h"
#include "Mocks/TestConsole.h"
#include "../TCDirCore/ResultsDisplayerNormal.h"
#include "../TCDirCore/ResultsDisplayerWide.h"
#include "../TCDirCore/ResultsDisplayerBare.h"
#include "../TCDirCore/Config.h"
#include "../TCDirCore/Color.h"
#include "../TCDirCore/Console.h"
#include "../TCDirCore/CommandLine.h"
#include "../TCDirCore/DirectoryInfo.h"
#include "../TCDirCore/IconMapping.h"
#include "../TCDirCore/SizeFormat.h"



using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace Microsoft { namespace VisualStudio { namespace CppUnitTestFramework
{
    template<> inline std::wstring ToString<char32_t> (const char32_t & value)
    {
        wchar_t buf[16];
        swprintf_s (buf, L"U+%05X", static_cast<unsigned>(value));
        return buf;
    }
}}}

namespace UnitTest
{
    //
    // No-op environment provider to prevent tests from loading the real
    // .tcdirconfig file via CConfig::Initialize → LoadConfigFile.
    //

    class CNoOpEnvironmentProvider : public IEnvironmentProvider
    {
        bool TryGetEnvironmentVariable (LPCWSTR, wstring &) const override { return false; }
    };

    static CNoOpEnvironmentProvider s_noOpEnv;




    //
    // Probe class to access protected members of CResultsDisplayerNormal
    //

    struct NormalDisplayerProbe : public CResultsDisplayerNormal
    {
        NormalDisplayerProbe (std::shared_ptr<CCommandLine> cmd, std::shared_ptr<CConsole> con, std::shared_ptr<CConfig> cfg)
            : CResultsDisplayerNormal (cmd, con, cfg, false) {}

        void DisplayFileResults (const CDirectoryInfo &) override {}

        // Expose protected members for testing
        wstring      WrapFormatNumber   (ULONGLONG n)                                            { return FormatNumberWithSeparators (n);    }
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

        // Set a valid timestamp (2026-01-01 00:00:00 UTC) to avoid EHM assertion in date rendering
        SYSTEMTIME st = { 2026, 1, 0, 1, 0, 0, 0, 0 };
        SystemTimeToFileTime (&st, &fd.ftLastWriteTime);
        fd.ftCreationTime    = fd.ftLastWriteTime;
        fd.ftLastAccessTime  = fd.ftLastWriteTime;

        wcscpy_s (fd.cFileName, pszName);

        return fd;
    }





    ////////////////////////////////////////////////////////////////////////////
    //
    //  WideDisplayerProbe
    //
    //  Expose protected members of CResultsDisplayerWide for testing
    //
    ////////////////////////////////////////////////////////////////////////////

    struct WideDisplayerProbe : public CResultsDisplayerWide
    {
        WideDisplayerProbe (std::shared_ptr<CCommandLine> cmd, std::shared_ptr<CConsole> con, std::shared_ptr<CConfig> cfg, bool fIconsActive = false)
            : CResultsDisplayerWide (cmd, con, cfg, fIconsActive) {}

        wstring_view WrapGetWideFormattedName (const WIN32_FIND_DATA & wfd, LPWSTR pszBuffer, size_t cchBuffer)
        {
            return GetWideFormattedName (wfd, pszBuffer, cchBuffer);
        }
    };





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
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);



            std::wstring s0     = probe.WrapFormatNumber(0ull);
            std::wstring s1234  = probe.WrapFormatNumber(1234ull);
            std::wstring s1m    = probe.WrapFormatNumber(1000000ull);

            Assert::AreEqual (L"0",         s0.c_str());
            Assert::AreEqual (L"1,234",     s1234.c_str());
            Assert::AreEqual (L"1,000,000", s1m.c_str());
        }




        TEST_METHOD(Wide_FormatNumber_Samples)
        {
            struct WideProbe : public CResultsDisplayerWide
            {
                WideProbe(std::shared_ptr<CCommandLine> a, std::shared_ptr<CConsole> b, std::shared_ptr<CConfig> c)
                    : CResultsDisplayerWide(a, b, c, false) {}
                void    DisplayFileResults(const CDirectoryInfo &) override {}
                wstring Wrap(ULONGLONG n) { return FormatNumberWithSeparators(n); }
            };

            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);
            WideProbe wp(cmd, con, cfg);



            std::wstring s9     = wp.Wrap(999ull);
            std::wstring s10k   = wp.Wrap(10000ull);

            Assert::AreEqual (L"999",    s9.c_str());
            Assert::AreEqual (L"10,000", s10k.c_str());
        }




        //
        // GetCloudStatus tests via probe
        //

        TEST_METHOD(GetCloudStatus_None_RegularFile)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);

            WIN32_FIND_DATA fd = CreateMockFileData(L"regular.txt", FILE_ATTRIBUTE_ARCHIVE);



            ECloudStatus status = probe.WrapGetCloudStatus(fd);

            Assert::AreEqual (static_cast<int>(ECloudStatus::CS_NONE), static_cast<int>(status));
        }




        TEST_METHOD(GetCloudStatus_None_CloudAttrsIgnoredOutsideSyncRoot)
        {
            // Cloud attributes should be ignored if file is not in a sync root
            // (e.g., stale attributes from files copied from cloud locations)
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);

            WIN32_FIND_DATA fd = CreateMockFileData(L"copied.txt",
                FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_PINNED);



            // fInSyncRoot = false, so cloud attributes are ignored
            ECloudStatus status = probe.WrapGetCloudStatus(fd, false);

            Assert::AreEqual (static_cast<int>(ECloudStatus::CS_NONE), static_cast<int>(status));
        }




        TEST_METHOD(GetCloudStatus_CloudOnly_Offline)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);

            WIN32_FIND_DATA fd = CreateMockFileData(L"cloud.txt", FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_OFFLINE);



            ECloudStatus status = probe.WrapGetCloudStatus(fd, true);

            Assert::AreEqual (static_cast<int>(ECloudStatus::CS_CLOUD_ONLY), static_cast<int>(status));
        }




        TEST_METHOD(GetCloudStatus_CloudOnly_RecallOnDataAccess)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);

            WIN32_FIND_DATA fd = CreateMockFileData(L"cloud.txt", FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS);



            ECloudStatus status = probe.WrapGetCloudStatus(fd, true);

            Assert::AreEqual (static_cast<int>(ECloudStatus::CS_CLOUD_ONLY), static_cast<int>(status));
        }




        TEST_METHOD(GetCloudStatus_CloudOnly_RecallOnOpen)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);

            WIN32_FIND_DATA fd = CreateMockFileData(L"cloud.txt", FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_RECALL_ON_OPEN);



            ECloudStatus status = probe.WrapGetCloudStatus(fd, true);

            Assert::AreEqual (static_cast<int>(ECloudStatus::CS_CLOUD_ONLY), static_cast<int>(status));
        }




        TEST_METHOD(GetCloudStatus_Local_Unpinned)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);

            WIN32_FIND_DATA fd = CreateMockFileData(L"local.txt", FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_UNPINNED);



            ECloudStatus status = probe.WrapGetCloudStatus(fd, true);

            Assert::AreEqual (static_cast<int>(ECloudStatus::CS_LOCAL), static_cast<int>(status));
        }




        TEST_METHOD(GetCloudStatus_Pinned)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);

            WIN32_FIND_DATA fd = CreateMockFileData(L"pinned.txt", FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_PINNED);



            ECloudStatus status = probe.WrapGetCloudStatus(fd, true);

            Assert::AreEqual (static_cast<int>(ECloudStatus::CS_PINNED), static_cast<int>(status));
        }




        TEST_METHOD(GetCloudStatus_PinnedTakesPriority)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);

            // File has both PINNED and OFFLINE - pinned should win
            WIN32_FIND_DATA fd = CreateMockFileData(L"pinned.txt", 
                FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_PINNED | FILE_ATTRIBUTE_OFFLINE);



            ECloudStatus status = probe.WrapGetCloudStatus(fd, true);

            Assert::AreEqual (static_cast<int>(ECloudStatus::CS_PINNED), static_cast<int>(status));
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



            Assert::IsTrue (SUCCEEDED(hr));
            // File should match: has OFFLINE which is part of the O composite mask
            bool matches = (fd.dwFileAttributes & cl.m_dwAttributesRequired) != 0;
            Assert::IsTrue (matches);
        }




        TEST_METHOD(AttributeFilter_CloudOnly_DoesNotMatchLocal)
        {
            CCommandLine cl;
            const wchar_t* arg = L"/A:O";
            wchar_t* argv[] = { const_cast<wchar_t*>(arg) };
            HRESULT hr = cl.Parse(1, argv);

            WIN32_FIND_DATA fd = CreateMockFileData(L"local.txt", 
                FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_UNPINNED);



            Assert::IsTrue (SUCCEEDED(hr));
            // File should NOT match: has UNPINNED but none of the O composite bits
            bool matches = (fd.dwFileAttributes & cl.m_dwAttributesRequired) != 0;
            Assert::IsFalse (matches);
        }




        TEST_METHOD(AttributeFilter_AlwaysLocallyAvailable_MatchesPinnedFile)
        {
            CCommandLine cl;
            const wchar_t* arg = L"/A:V";
            wchar_t* argv[] = { const_cast<wchar_t*>(arg) };
            HRESULT hr = cl.Parse(1, argv);

            WIN32_FIND_DATA fd = CreateMockFileData(L"pinned.txt", 
                FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_PINNED);



            Assert::IsTrue (SUCCEEDED(hr));
            bool matches = (fd.dwFileAttributes & cl.m_dwAttributesRequired) != 0;
            Assert::IsTrue (matches);
        }




        TEST_METHOD(AttributeFilter_LocallyAvailable_MatchesLocalFile)
        {
            CCommandLine cl;
            const wchar_t* arg = L"/A:L";
            wchar_t* argv[] = { const_cast<wchar_t*>(arg) };
            HRESULT hr = cl.Parse(1, argv);

            WIN32_FIND_DATA fd = CreateMockFileData(L"local.txt", 
                FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_UNPINNED);



            Assert::IsTrue (SUCCEEDED(hr));
            bool matches = (fd.dwFileAttributes & cl.m_dwAttributesRequired) != 0;
            Assert::IsTrue (matches);
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



            Assert::IsTrue (SUCCEEDED(hr));
            
            // Cloud file should be excluded (has OFFLINE)
            bool cloudExcluded = (fdCloud.dwFileAttributes & cl.m_dwAttributesExcluded) != 0;
            Assert::IsTrue (cloudExcluded);

            // Local file should NOT be excluded (no cloud-only attributes)
            bool localExcluded = (fdLocal.dwFileAttributes & cl.m_dwAttributesExcluded) != 0;
            Assert::IsFalse (localExcluded);
        }




        TEST_METHOD(GetCloudStatus_BothPinnedAndUnpinned_PinnedWins)
        {
            // OneDrive system files can have both PINNED and UNPINNED bits set
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);
            NormalDisplayerProbe probe(cmd, con, cfg);

            WIN32_FIND_DATA fd = CreateMockFileData(L"desktop.ini", 
                FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE |
                FILE_ATTRIBUTE_PINNED | FILE_ATTRIBUTE_UNPINNED);



            // PINNED should take priority for display
            ECloudStatus status = probe.WrapGetCloudStatus(fd, true);
            Assert::AreEqual (static_cast<int>(ECloudStatus::CS_PINNED), static_cast<int>(status));
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
            Assert::IsTrue (matchesV);

            // Should also match /A:L (UNPINNED bit is also set)
            bool matchesL = (fd.dwFileAttributes & clL.m_dwAttributesRequired) != 0;
            Assert::IsTrue (matchesL);
        }




        //
        // Wide mode icon tests (bracket suppression when icons active)
        //

        TEST_METHOD(GetWideFormattedName_Directory_ClassicMode_HasBrackets)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);

            WideDisplayerProbe probe (cmd, con, cfg, false /* icons off */);

            WIN32_FIND_DATA wfd = CreateMockFileData (L"src", FILE_ATTRIBUTE_DIRECTORY);

            WCHAR szBuf[MAX_PATH + 3];
            wstring_view result = probe.WrapGetWideFormattedName (wfd, szBuf, ARRAYSIZE(szBuf));

            Assert::AreEqual (wstring(L"[src]"), wstring(result));
        }




        TEST_METHOD(GetWideFormattedName_Directory_IconsActive_NoBrackets)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);

            WideDisplayerProbe probe (cmd, con, cfg, true /* icons on */);

            WIN32_FIND_DATA wfd = CreateMockFileData (L"src", FILE_ATTRIBUTE_DIRECTORY);

            WCHAR szBuf[MAX_PATH + 3];
            wstring_view result = probe.WrapGetWideFormattedName (wfd, szBuf, ARRAYSIZE(szBuf));

            Assert::AreEqual (wstring(L"src"), wstring(result));
        }




        TEST_METHOD(GetWideFormattedName_File_ClassicMode_JustName)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);

            WideDisplayerProbe probe (cmd, con, cfg, false);

            WIN32_FIND_DATA wfd = CreateMockFileData (L"test.cpp", FILE_ATTRIBUTE_ARCHIVE);

            WCHAR szBuf[MAX_PATH + 3];
            wstring_view result = probe.WrapGetWideFormattedName (wfd, szBuf, ARRAYSIZE(szBuf));

            Assert::AreEqual (wstring(L"test.cpp"), wstring(result));
        }




        TEST_METHOD(GetWideFormattedName_File_IconsActive_JustName)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);

            WideDisplayerProbe probe (cmd, con, cfg, true);

            WIN32_FIND_DATA wfd = CreateMockFileData (L"test.cpp", FILE_ATTRIBUTE_ARCHIVE);

            WCHAR szBuf[MAX_PATH + 3];
            wstring_view result = probe.WrapGetWideFormattedName (wfd, szBuf, ARRAYSIZE(szBuf));

            Assert::AreEqual (wstring(L"test.cpp"), wstring(result));
        }




        //
        // Column width adjustment tests (scenario 42)
        //

        TEST_METHOD(GetColumnInfo_IconsActive_WidthIncreasedByTwo)
        {
            //
            // Verify that ComputeDisplayWidth adds +2 for icons
            //

            WIN32_FIND_DATA wfd = {};
            wcscpy_s (wfd.cFileName, L"testfile.txt");

            size_t widthOff = CResultsDisplayerWide::ComputeDisplayWidth (wfd, false, false, false);
            size_t widthOn  = CResultsDisplayerWide::ComputeDisplayWidth (wfd, true,  false, false);

            Assert::AreEqual (widthOff + 2, widthOn, L"Icons should add +2 to display width");
        }




        TEST_METHOD(GetColumnInfo_SyncRootWithIcons_WidthAccountsForBoth)
        {
            //
            // Verify that ComputeDisplayWidth adds +2 for icons and +2 for sync root
            //

            WIN32_FIND_DATA wfd = {};
            wcscpy_s (wfd.cFileName, L"testfile.txt");

            size_t widthPlain    = CResultsDisplayerWide::ComputeDisplayWidth (wfd, false, false, false);
            size_t widthSync     = CResultsDisplayerWide::ComputeDisplayWidth (wfd, false, false, true);
            size_t widthIconSync = CResultsDisplayerWide::ComputeDisplayWidth (wfd, true,  false, true);

            Assert::AreEqual (widthPlain + 2, widthSync, L"Sync root should add +2");
            Assert::AreEqual (widthPlain + 4, widthIconSync, L"Icons + sync should add +4");
        }




        //
        // GetDisplayStyleForFile integration tests (scenarios 33, 35)
        //

        TEST_METHOD(GetDisplayStyle_NormalMode_CppFileReturnsIcon)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);

            WIN32_FIND_DATA wfd = CreateMockFileData (L"main.cpp", FILE_ATTRIBUTE_ARCHIVE);
            auto style = cfg->GetDisplayStyleForFile (wfd);

            // C++ files should have the CPP icon
            Assert::AreEqual (static_cast<char32_t>(NfIcon::MdLanguageCpp), style.m_iconCodePoint,
                             L"C++ file should get CPP icon");
            Assert::IsFalse (style.m_fIconSuppressed, L"Icon should not be suppressed");
        }




        TEST_METHOD(GetDisplayStyle_DirectoryReturnsDirectoryIcon)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);

            WIN32_FIND_DATA wfd = CreateMockFileData (L"mydir", FILE_ATTRIBUTE_DIRECTORY);
            auto style = cfg->GetDisplayStyleForFile (wfd);

            Assert::AreEqual (static_cast<char32_t>(NfIcon::CustomFolder), style.m_iconCodePoint,
                             L"Non-well-known directory should get default folder icon");
        }




        TEST_METHOD(GetDisplayStyle_UnknownExtension_FallsBackToFileIcon)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize(cfg);

            WIN32_FIND_DATA wfd = CreateMockFileData (L"data.xyz123", FILE_ATTRIBUTE_ARCHIVE);
            auto style = cfg->GetDisplayStyleForFile (wfd);

            Assert::AreEqual (static_cast<char32_t>(NfIcon::FaFile), style.m_iconCodePoint,
                             L"Unknown extension should get default file icon");
        }




        TEST_METHOD(CodePointToWideChars_BmpIcon_ProducesValidGlyph)
        {
            // Verify that a typical BMP NF icon (e.g., CSS3 icon 0xE749) can be
            // converted to a wchar_t buffer suitable for console output
            WideCharPair wcp = CodePointToWideChars (NfIcon::DevCss3);

            Assert::AreEqual (1u, wcp.count, L"BMP icon should be single wchar_t");
            Assert::AreNotEqual (static_cast<wchar_t>(0), wcp.chars[0], L"Should have non-null glyph");
        }





        //
        //  FormatAbbreviatedSize tests — Explorer-style 3 significant digits, 7-char fixed width
        //  Number right-justified in 4 chars, space, unit left-justified in 2 chars
        //

        TEST_METHOD(FormatAbbreviatedSize_Zero)
        {
            wstring s = CResultsDisplayerNormal::FormatAbbreviatedSize (0);
            Assert::AreEqual (L"   0 B ", s.c_str());
        }





        TEST_METHOD(FormatAbbreviatedSize_SmallBytes)
        {
            Assert::AreEqual (L"   1 B ", CResultsDisplayerNormal::FormatAbbreviatedSize (1).c_str());
            Assert::AreEqual (L" 426 B ", CResultsDisplayerNormal::FormatAbbreviatedSize (426).c_str());
            Assert::AreEqual (L" 999 B ", CResultsDisplayerNormal::FormatAbbreviatedSize (999).c_str());
        }





        TEST_METHOD(FormatAbbreviatedSize_1000BytesRoundsToKB)
        {
            Assert::AreEqual (L"   1 KB", CResultsDisplayerNormal::FormatAbbreviatedSize (1000).c_str());
            Assert::AreEqual (L"   1 KB", CResultsDisplayerNormal::FormatAbbreviatedSize (1023).c_str());
        }





        TEST_METHOD(FormatAbbreviatedSize_ExactlyOneKB)
        {
            // 1024 bytes = 1.00 KB
            wstring s = CResultsDisplayerNormal::FormatAbbreviatedSize (1024);
            Assert::AreEqual (L"1.00 KB", s.c_str());
        }





        TEST_METHOD(FormatAbbreviatedSize_FractionalKB)
        {
            // 4720 bytes = 4.609375 KB ≈ 4.61 KB
            wstring s = CResultsDisplayerNormal::FormatAbbreviatedSize (4720);
            Assert::AreEqual (L"4.61 KB", s.c_str());
        }





        TEST_METHOD(FormatAbbreviatedSize_TensKB)
        {
            // 17510 bytes = 17.099609375 KB ≈ 17.1 KB
            wstring s = CResultsDisplayerNormal::FormatAbbreviatedSize (17510);
            Assert::AreEqual (L"17.1 KB", s.c_str());
        }





        TEST_METHOD(FormatAbbreviatedSize_HundredsKB)
        {
            // 999424 bytes = 976 KB
            wstring s = CResultsDisplayerNormal::FormatAbbreviatedSize (999424);
            Assert::AreEqual (L" 976 KB", s.c_str());
        }





        TEST_METHOD(FormatAbbreviatedSize_OneMB)
        {
            // 1 MB = 1048576 bytes
            wstring s = CResultsDisplayerNormal::FormatAbbreviatedSize (1048576);
            Assert::AreEqual (L"1.00 MB", s.c_str());
        }





        TEST_METHOD(FormatAbbreviatedSize_TensMB)
        {
            // 17_563_648 bytes = 16.75 MB ≈ 16.8 MB
            wstring s = CResultsDisplayerNormal::FormatAbbreviatedSize (17563648);
            Assert::AreEqual (L"16.8 MB", s.c_str());
        }





        TEST_METHOD(FormatAbbreviatedSize_OneGB)
        {
            // 1 GB = 1073741824 bytes
            wstring s = CResultsDisplayerNormal::FormatAbbreviatedSize (1073741824ULL);
            Assert::AreEqual (L"1.00 GB", s.c_str());
        }





        TEST_METHOD(FormatAbbreviatedSize_FractionalGB)
        {
            // 1_493_172_224 bytes = 1.39075... GB ≈ 1.39 GB
            wstring s = CResultsDisplayerNormal::FormatAbbreviatedSize (1493172224ULL);
            Assert::AreEqual (L"1.39 GB", s.c_str());
        }





        TEST_METHOD(FormatAbbreviatedSize_OneTB)
        {
            // 1 TB = 1099511627776 bytes
            wstring s = CResultsDisplayerNormal::FormatAbbreviatedSize (1099511627776ULL);
            Assert::AreEqual (L"1.00 TB", s.c_str());
        }




        //
        // ComputeAvailableWidthForTarget tests
        //

        TEST_METHOD(AvailableWidth_NormalMode_AutoSize_IconsOn_ShortFilename)
        {
            // Normal mode, Auto size, icons on, 120-wide terminal, "python.exe" (10 chars)
            // Expected: 21 (date) + 9 (attrs) + 9 (Auto size) + 4 (cloud w/icons) + 3 (icon) + 10 (filename) + 3 (arrow) = 59
            // Available = 120 - 59 = 61

            size_t avail = CResultsDisplayerNormal::ComputeAvailableWidthForTarget (
                120, ESizeFormat::Auto, 1, true, false, false, 0, 0, 10);

            Assert::AreEqual (size_t (61), avail);
        }



        TEST_METHOD(AvailableWidth_NormalMode_BytesSize_IconsOff_ShortFilename)
        {
            // Normal mode, Bytes size (max file size len = 1 → max(1,5) = 5 → 2+5 = 7), no icons, 120-wide
            // Expected: 21 + 9 + 7 + 3 (cloud no icons) + 0 (no icon) + 10 + 3 = 53
            // Available = 120 - 53 = 67

            size_t avail = CResultsDisplayerNormal::ComputeAvailableWidthForTarget (
                120, ESizeFormat::Bytes, 1, false, false, false, 0, 0, 10);

            Assert::AreEqual (size_t (67), avail);
        }



        TEST_METHOD(AvailableWidth_NormalMode_AutoSize_IconsOn_LongFilename)
        {
            // 126-wide, icons on, Auto, "MicrosoftWindows.DesktopStickerEditorCentennial.exe" (51 chars)
            // Expected: 21 + 9 + 9 + 4 + 3 + 51 + 3 = 100
            // Available = 126 - 100 = 26

            size_t avail = CResultsDisplayerNormal::ComputeAvailableWidthForTarget (
                126, ESizeFormat::Auto, 1, true, false, false, 0, 0, 51);

            Assert::AreEqual (size_t (26), avail);
        }



        TEST_METHOD(AvailableWidth_TreeMode_WithPrefix)
        {
            // Tree mode, depth 1, indent 4: prefix = "├── " = 4 chars
            // 126-wide, icons on, Auto, "ActionsMcpHost.exe" (18 chars)
            // Expected: 21 + 9 + 9 + 4 + 4 (prefix) + 3 (icon) + 18 + 3 = 71
            // Available = 126 - 71 = 55

            size_t avail = CResultsDisplayerNormal::ComputeAvailableWidthForTarget (
                126, ESizeFormat::Auto, 1, true, false, false, 0, 4, 18);

            Assert::AreEqual (size_t (55), avail);
        }



        TEST_METHOD(AvailableWidth_WithDebugAndOwner)
        {
            // 120-wide, icons on, Auto, debug on, owner on (max 15 chars), "test.exe" (8 chars)
            // Expected: 21 + 9 + 9 + 4 + 14 (debug) + 16 (owner 15+1) + 3 (icon) + 8 + 3 = 87
            // Available = 120 - 87 = 33

            size_t avail = CResultsDisplayerNormal::ComputeAvailableWidthForTarget (
                120, ESizeFormat::Auto, 1, true, true, true, 15, 0, 8);

            Assert::AreEqual (size_t (33), avail);
        }



        TEST_METHOD(AvailableWidth_NarrowTerminal_ReturnsZero)
        {
            // Terminal narrower than metadata — available should be 0, not underflow
            // 50-wide, icons on, "VeryLongFilenameExceedsEverything.exe" (36 chars)
            // Expected: 21 + 9 + 9 + 4 + 3 + 36 + 3 = 85 > 50
            // Available = 0

            size_t avail = CResultsDisplayerNormal::ComputeAvailableWidthForTarget (
                50, ESizeFormat::Auto, 1, true, false, false, 0, 0, 36);

            Assert::AreEqual (size_t (0), avail);
        }



        TEST_METHOD(AvailableWidth_BytesMode_LargeFileSize)
        {
            // Bytes mode, largest file = "1,234,567,890" (13 chars), max(13,5) = 13, size col = 2+13 = 15
            // 120-wide, no icons, "test.txt" (8 chars)
            // Expected: 21 + 9 + 15 + 3 (cloud no icons) + 0 + 8 + 3 = 59
            // Available = 120 - 59 = 61

            size_t avail = CResultsDisplayerNormal::ComputeAvailableWidthForTarget (
                120, ESizeFormat::Bytes, 13, false, false, false, 0, 0, 8);

            Assert::AreEqual (size_t (61), avail);
        }



        TEST_METHOD(AvailableWidth_AttributeCount_Matches_FileAttributeMap)
        {
            // Verify the attribute count used in the calculation matches the actual map size
            // This prevents the bug where we hardcoded 7 instead of 9

            size_t avail126 = CResultsDisplayerNormal::ComputeAvailableWidthForTarget (
                126, ESizeFormat::Auto, 1, true, false, false, 0, 0, 10);

            // 21 + _countof(k_rgFileAttributeMap) + 9 + 4 + 3 + 10 + 3 = 50 + _countof
            // At 9 attrs: 59 → avail 67. At 7 attrs: 57 → avail 69. Difference = 2.
            // We expect 9 attrs (avail = 67 at 126-wide)
            size_t expected = 126 - (21 + 9 + 9 + 4 + 3 + 10 + 3);

            Assert::AreEqual (expected, avail126, L"Attribute count should match k_rgFileAttributeMap");
        }




        //
        // Ellipsize integration tests — verify normal mode rendering with mocked data
        //

        TEST_METHOD(Ellipsize_NormalMode_LongTarget_ContainsEllipsis)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CCapturingConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize (cfg);



            // Build a directory info with one file that has a long reparse target
            CDirectoryInfo di (L"C:\\Test", L"*");

            WIN32_FIND_DATA wfd = CreateMockFileData (L"python.exe", FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_REPARSE_POINT, 0);
            wfd.dwReserved0 = 0x8000001B;  // IO_REPARSE_TAG_APPEXECLINK

            FileInfo fi (wfd);
            fi.m_strReparseTarget = L"C:\\Program Files\\WindowsApps\\Microsoft.DesktopAppInstaller_1.29.30.0_arm64__8wekyb3d8bbwe\\winget.exe";

            di.m_vMatches.push_back (move (fi));

            // Render using real CResultsDisplayerNormal (console defaults to 80-wide)
            // At 80-wide, metadata+filename+arrow ~ 51 chars, available ~ 29.
            // Target is 95 chars. Priority 3: "C:\" + ellipsis + "\winget.exe" = 15 chars — fits.
            CResultsDisplayerNormal displayer (cmd, con, cfg, false);
            displayer.DisplayFileResults (di);
            con->Flush ();

            wstring captured = con->m_strCaptured;
            bool hasEllipsis = captured.find (L'\u2026') != wstring::npos;

            Assert::IsTrue (hasEllipsis,
                           L"Ellipsis character should appear in truncated target path");

            // Verify the leaf filename is preserved
            Assert::IsTrue (captured.find (L"winget.exe") != wstring::npos,
                           L"Leaf filename should be preserved");

            // Verify the full middle segment was elided
            Assert::IsTrue (captured.find (L"Microsoft.DesktopAppInstaller") == wstring::npos,
                           L"Middle path segment should be elided");
        }



        TEST_METHOD(Ellipsize_NormalMode_ShortTarget_NoEllipsis)
        {
            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CCapturingConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize (cfg);



            CDirectoryInfo di (L"C:\\Test", L"*");

            WIN32_FIND_DATA wfd = CreateMockFileData (L"AzureVpn.exe", FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_REPARSE_POINT, 0);
            wfd.dwReserved0 = 0x8000001B;

            FileInfo fi (wfd);
            fi.m_strReparseTarget = L"C:\\Windows\\sysuwp.exe";  // Short enough to fit at 80-width

            di.m_vMatches.push_back (move (fi));

            CResultsDisplayerNormal displayer (cmd, con, cfg, false);
            displayer.DisplayFileResults (di);
            con->Flush ();

            // Short target should NOT be truncated
            Assert::IsTrue (con->m_strCaptured.find (L'\u2026') == wstring::npos,
                           L"Ellipsis should NOT appear for short target");

            // Full target path should be present
            Assert::IsTrue (con->m_strCaptured.find (L"sysuwp.exe") != wstring::npos,
                           L"Full target path should be present");
        }



        TEST_METHOD(Ellipsize_Disabled_LongTarget_NoEllipsis)
        {
            auto cmd = std::make_shared<CCommandLine>();
            cmd->m_fEllipsize = false;  // --Ellipsize-

            auto con = std::make_shared<CCapturingConsole> ();
            auto cfg = std::make_shared<CConfig>();
            cfg->SetEnvironmentProvider (&s_noOpEnv);
            con->Initialize (cfg);



            CDirectoryInfo di (L"C:\\Test", L"*");

            WIN32_FIND_DATA wfd = CreateMockFileData (L"python.exe", FILE_ATTRIBUTE_ARCHIVE | FILE_ATTRIBUTE_REPARSE_POINT, 0);
            wfd.dwReserved0 = 0x8000001B;

            FileInfo fi (wfd);
            fi.m_strReparseTarget = L"C:\\Program Files\\WindowsApps\\Microsoft.DesktopAppInstaller_1.29.30.0_arm64__8wekyb3d8bbwe\\AppInstallerPythonRedirector.exe";

            di.m_vMatches.push_back (move (fi));

            CResultsDisplayerNormal displayer (cmd, con, cfg, false);
            displayer.DisplayFileResults (di);
            con->Flush ();

            // With --Ellipsize-, no truncation should occur
            Assert::IsTrue (con->m_strCaptured.find (L'\u2026') == wstring::npos,
                           L"Ellipsis should NOT appear when --Ellipsize- is set");

            // Full untruncated target path should be present
            Assert::IsTrue (con->m_strCaptured.find (L"Microsoft.DesktopAppInstaller_1.29.30.0_arm64__8wekyb3d8bbwe") != wstring::npos,
                           L"Full path (including middle components) should be present when ellipsize is disabled");
        }

    };
}
