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
                if (line == L"function d { tcdir @args }")
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
                if (line == L"function dd { d /a:d @args }")  fFoundDd = true;
                if (line == L"function ds { d /s @args }")    fFoundDs = true;
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
                if (line == L"function tc { tcdir @args }")    fFoundRoot = true;
                if (line == L"function tcd { tc /a:d @args }") fFoundSub  = true;
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
    };
}
