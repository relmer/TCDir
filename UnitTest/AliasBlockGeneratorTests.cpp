#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/AliasBlockGenerator.h"
#include "../TCDirCore/AliasManager.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(AliasBlockGeneratorTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        TEST_METHOD(Generate_DefaultConfig_HasHeaderAndFooterMarkers)
        {
            SAliasConfig config;

            config.strRootAlias      = L"d";
            config.strTcDirInvocation = L"tcdir";
            config.rgSubAliases      = { { L"dd", L"/a:d", L"dirs", true } };

            vector<wstring> rgLines;



            CAliasBlockGenerator::Generate (config, L"5.2.1150", rgLines);

            Assert::IsTrue (rgLines.size() > 4);

            // Check header marker
            bool fFoundHeader = false;
            bool fFoundFooter = false;

            for (const auto & line : rgLines)
            {
                if (line.find (L"TCDir Aliases") != wstring::npos)
                    fFoundHeader = true;
                if (line.find (L"End TCDir Aliases") != wstring::npos)
                    fFoundFooter = true;
            }

            Assert::IsTrue (fFoundHeader);
            Assert::IsTrue (fFoundFooter);
        }




        TEST_METHOD(Generate_VersionStampIncluded)
        {
            SAliasConfig config;

            config.strRootAlias      = L"d";
            config.strTcDirInvocation = L"tcdir";

            vector<wstring> rgLines;



            CAliasBlockGenerator::Generate (config, L"5.2.1150", rgLines);

            bool fFoundVersion = false;

            for (const auto & line : rgLines)
            {
                if (line.find (L"tcdir v5.2.1150") != wstring::npos)
                {
                    fFoundVersion = true;
                    break;
                }
            }

            Assert::IsTrue (fFoundVersion);
        }




        TEST_METHOD(Generate_RootAliasShortName)
        {
            SAliasConfig config;

            config.strRootAlias      = L"d";
            config.strTcDirInvocation = L"tcdir";

            vector<wstring> rgLines;



            CAliasBlockGenerator::Generate (config, L"5.2.1150", rgLines);

            bool fFoundRoot = false;

            for (const auto & line : rgLines)
            {
                if (line.find (L"function d") != wstring::npos && line.find (L"tcdir @args") != wstring::npos)
                {
                    fFoundRoot = true;
                    break;
                }
            }

            Assert::IsTrue (fFoundRoot);
        }




        TEST_METHOD(Generate_RootAliasFullPath)
        {
            SAliasConfig config;

            config.strRootAlias      = L"d";
            config.strTcDirInvocation = L"C:\\Program Files\\TCDir\\tcdir.exe";

            vector<wstring> rgLines;



            CAliasBlockGenerator::Generate (config, L"5.2.1150", rgLines);

            bool fFoundRoot = false;

            for (const auto & line : rgLines)
            {
                if (line.find (L"function d {") != wstring::npos && line.find (L"& \"C:\\Program Files\\TCDir\\tcdir.exe\"") != wstring::npos)
                {
                    fFoundRoot = true;
                    break;
                }
            }

            Assert::IsTrue (fFoundRoot);
        }




        TEST_METHOD(Generate_SubAliasesIncluded)
        {
            SAliasConfig config;

            config.strRootAlias      = L"d";
            config.strTcDirInvocation = L"tcdir";
            config.rgSubAliases      = {
                { L"dd", L"/a:d",  L"dirs",   true  },
                { L"ds", L"/s",    L"recurse", true  },
                { L"dw", L"/w",    L"wide",   false },
            };

            vector<wstring> rgLines;



            CAliasBlockGenerator::Generate (config, L"5.2.1150", rgLines);

            bool fFoundDd  = false;
            bool fFoundDs  = false;
            bool fFoundDw  = false;

            for (const auto & line : rgLines)
            {
                if (line.find (L"function dd") != wstring::npos && line.find (L"/a:d @args") != wstring::npos)  fFoundDd = true;
                if (line.find (L"function ds") != wstring::npos && line.find (L"/s @args") != wstring::npos)    fFoundDs = true;
                if (line.find (L"function dw") != wstring::npos)  fFoundDw = true;
            }

            Assert::IsTrue (fFoundDd, L"dd sub-alias should be present");
            Assert::IsTrue (fFoundDs, L"ds sub-alias should be present");
            Assert::IsFalse (fFoundDw, L"dw sub-alias should NOT be present (disabled)");
        }




        TEST_METHOD(Generate_CustomRootAlias)
        {
            SAliasConfig config;

            config.strRootAlias      = L"tc";
            config.strTcDirInvocation = L"tcdir";
            config.rgSubAliases      = {
                { L"tcd", L"/a:d", L"dirs", true },
            };

            vector<wstring> rgLines;



            CAliasBlockGenerator::Generate (config, L"5.2.1150", rgLines);

            bool fFoundRoot = false;
            bool fFoundSub  = false;

            for (const auto & line : rgLines)
            {
                if (line.find (L"function tc") != wstring::npos && line.find (L"tcdir @args") != wstring::npos)     fFoundRoot = true;
                if (line.find (L"function tcd") != wstring::npos && line.find (L"/a:d @args") != wstring::npos)  fFoundSub  = true;
            }

            Assert::IsTrue (fFoundRoot);
            Assert::IsTrue (fFoundSub);
        }




        TEST_METHOD(Generate_PathWithSpaces_ProperQuoting)
        {
            SAliasConfig config;

            config.strRootAlias      = L"d";
            config.strTcDirInvocation = L"C:\\Program Files\\TCDir\\tcdir.exe";
            config.rgSubAliases      = {
                { L"dd", L"/a:d", L"dirs", true },
            };

            vector<wstring> rgLines;



            CAliasBlockGenerator::Generate (config, L"5.2.1150", rgLines);

            bool fFoundQuotedPath = false;

            for (const auto & line : rgLines)
            {
                if (line.find (L"& \"C:\\Program Files\\TCDir\\tcdir.exe\"") != wstring::npos)
                {
                    fFoundQuotedPath = true;
                    break;
                }
            }

            Assert::IsTrue (fFoundQuotedPath, L"Path with spaces must use call operator and quoting");
        }




        TEST_METHOD(Generate_NoSubAliases_OnlyRoot)
        {
            SAliasConfig config;

            config.strRootAlias      = L"d";
            config.strTcDirInvocation = L"tcdir";
            // No sub-aliases at all

            vector<wstring> rgLines;



            CAliasBlockGenerator::Generate (config, L"5.2.1150", rgLines);

            int cFunctions = 0;

            for (const auto & line : rgLines)
            {
                if (line.starts_with (L"function "))
                {
                    ++cFunctions;
                }
            }

            Assert::AreEqual (1, cFunctions, L"Only root function should be present");
        }




        TEST_METHOD(Generate_DashColonFlags_SingleQuoted)
        {
            SAliasConfig config;

            config.strRootAlias       = L"d";
            config.strTcDirInvocation = L"tcdir";
            config.rgSubAliases       = {
                { L"dd", L"-a:d",  L"dirs",   true },
                { L"ds", L"-s",    L"recurse", true },
                { L"do", L"-o:s",  L"sort",   true },
            };

            vector<wstring> rgLines;



            CAliasBlockGenerator::Generate (config, L"5.2.1150", rgLines);

            bool fFoundDd = false;
            bool fFoundDs = false;
            bool fFoundDo = false;

            for (const auto & line : rgLines)
            {
                if (line.find (L"function dd") != wstring::npos && line.find (L"'-a:d'") != wstring::npos)  fFoundDd = true;
                if (line.find (L"function ds") != wstring::npos && line.find (L"-s @args") != wstring::npos) fFoundDs = true;
                if (line.find (L"function do") != wstring::npos && line.find (L"'-o:s'") != wstring::npos)  fFoundDo = true;
            }

            Assert::IsTrue (fFoundDd, L"-a:d should be single-quoted as '-a:d'");
            Assert::IsTrue (fFoundDs, L"-s should NOT be quoted");
            Assert::IsTrue (fFoundDo, L"-o:s should be single-quoted as '-o:s'");
        }




        TEST_METHOD(Generate_SlashColonFlags_NotQuoted)
        {
            SAliasConfig config;

            config.strRootAlias       = L"d";
            config.strTcDirInvocation = L"tcdir";
            config.rgSubAliases       = {
                { L"dd", L"/a:d", L"dirs", true },
            };

            vector<wstring> rgLines;



            CAliasBlockGenerator::Generate (config, L"5.2.1150", rgLines);

            bool fFoundDd = false;

            for (const auto & line : rgLines)
            {
                if (line.find (L"function dd") != wstring::npos && line.find (L"/a:d @args") != wstring::npos)  fFoundDd = true;
            }

            Assert::IsTrue (fFoundDd, L"/a:d should NOT be quoted — only dash-colon needs quoting");
        }
    };
}
