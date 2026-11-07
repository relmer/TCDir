#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/AliasBlockGenerator.h"
#include "../TCDirCore/AliasManager.h"
#include "../TCDirCore/CompletionManager.h"
#include "../TCDirCore/CompletionScript.h"
#include "../TCDirCore/ProfileFileManager.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(CompletionManagerTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        static vector<wstring> ProfileWithAliases()
        {
            SAliasConfig    config;
            vector<wstring> rgLines = { L"Set-Location C:\\code" };
            vector<wstring> rgAliases;
            CProfileFileManager fileMgr;

            config.strRootAlias       = L"d";
            config.strTcDirInvocation = L"tcdir";
            config.rgSubAliases       = { { L"dw", L"-w", L"wide", true }, { L"dd", L"-a:d", L"dirs", true } };

            CAliasBlockGenerator::Generate (config, L"5.7.1", rgAliases);
            fileMgr.AppendAliasBlock (rgLines, rgAliases);
            return rgLines;
        }

        static vector<wstring> Registrations (const vector<wstring> & rgLines)
        {
            vector<wstring> rgRegistrations;

            for (const wstring & strLine : rgLines)
            {
                if (CCompletionScript::IsRegistration (strLine))
                {
                    rgRegistrations.push_back (strLine);
                }
            }

            return rgRegistrations;
        }




        TEST_METHOD(UpdateLines_Add_RegistersTcDirAndAliasesWithFlags)
        {
            vector<wstring> rgLines = ProfileWithAliases();
            vector<wstring> rgRegistrations;



            CCompletionManager::UpdateLines (rgLines, ECompletionUpdate::Add, L"tcdir", L"5.7.1");
            rgRegistrations = Registrations (rgLines);

            Assert::IsTrue   (CCompletionManager::HasCompletionBlock (rgLines));
            Assert::AreEqual (size_t (5), rgRegistrations.size());
            Assert::IsTrue   (rgRegistrations[3].find (L"'dw'") != wstring::npos && rgRegistrations[3].find (L"@('-w')") != wstring::npos);
            Assert::IsTrue   (rgRegistrations[4].find (L"@('-a:d')") != wstring::npos);
            Assert::AreEqual (L"Set-Location C:\\code", rgLines[0].c_str());
        }




        TEST_METHOD(UpdateLines_AddWithoutAliases_RegistersTcDirOnly)
        {
            vector<wstring> rgLines = { L"# my profile" };



            CCompletionManager::UpdateLines (rgLines, ECompletionUpdate::Add, L"tcdir", L"5.7.1");

            Assert::AreEqual (size_t (2), Registrations (rgLines).size());
        }




        TEST_METHOD(UpdateLines_AddTwice_ReplacesBlock)
        {
            vector<wstring> rgLines = ProfileWithAliases();
            size_t          cLines  = 0;



            CCompletionManager::UpdateLines (rgLines, ECompletionUpdate::Add, L"tcdir", L"5.7.1");
            cLines = rgLines.size();
            CCompletionManager::UpdateLines (rgLines, ECompletionUpdate::Add, L"tcdir", L"5.7.1");

            Assert::AreEqual (cLines, rgLines.size());
        }




        TEST_METHOD(UpdateLines_Refresh_OnlyWhenPresent)
        {
            vector<wstring> rgLines = ProfileWithAliases();
            vector<wstring> rgBefore = rgLines;
            CProfileFileManager fileMgr;
            SAliasBlock     aliases;



            CCompletionManager::UpdateLines (rgLines, ECompletionUpdate::Refresh, L"tcdir", L"5.7.1");
            Assert::IsTrue (rgLines == rgBefore);

            CCompletionManager::UpdateLines (rgLines, ECompletionUpdate::Add, L"tcdir", L"5.7.1");
            fileMgr.FindAliasBlock (rgLines, aliases);
            fileMgr.RemoveAliasBlock (rgLines, aliases);
            CCompletionManager::UpdateLines (rgLines, ECompletionUpdate::Refresh, L"tcdir", L"5.7.1");

            Assert::AreEqual (size_t (2), Registrations (rgLines).size());
        }




        TEST_METHOD(UpdateLines_Remove_KeepsAliasBlock)
        {
            vector<wstring> rgLines = ProfileWithAliases();
            vector<wstring> rgBefore = rgLines;



            CCompletionManager::UpdateLines (rgLines, ECompletionUpdate::Add, L"tcdir", L"5.7.1");
            CCompletionManager::UpdateLines (rgLines, ECompletionUpdate::Remove, L"", L"");

            Assert::IsFalse (CCompletionManager::HasCompletionBlock (rgLines));
            Assert::IsTrue  (rgLines == rgBefore);
        }
    };
}
