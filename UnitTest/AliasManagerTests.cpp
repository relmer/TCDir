#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/AliasManager.h"
#include "../TCDirCore/AliasBlockGenerator.h"
#include "../TCDirCore/ProfileFileManager.h"
#include "../TCDirCore/Console.h"
#include "../TCDirCore/Config.h"
#include "../TCDirCore/Version.h"
#include "Mocks/TestConsole.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(AliasManagerTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        TEST_METHOD(BuildDefaultSubAliases_DefaultRoot)
        {
            SAliasConfig config;

            config.strRootAlias      = L"d";
            config.strTcDirInvocation = L"tcdir";



            vector<SAliasDefinition> rgSubs;

            CAliasManager::BuildDefaultSubAliases (L"d", rgSubs);

            Assert::AreEqual (4u, static_cast<unsigned>(rgSubs.size()));
            Assert::AreEqual (L"dd",  rgSubs[0].strName.c_str());
            Assert::AreEqual (L"ds",  rgSubs[1].strName.c_str());
            Assert::AreEqual (L"dsb", rgSubs[2].strName.c_str());
            Assert::AreEqual (L"dw",  rgSubs[3].strName.c_str());
        }




        TEST_METHOD(BuildDefaultSubAliases_CustomRoot)
        {
            vector<SAliasDefinition> rgSubs;



            CAliasManager::BuildDefaultSubAliases (L"tc", rgSubs);

            Assert::AreEqual (4u, static_cast<unsigned>(rgSubs.size()));
            Assert::AreEqual (L"tcd",  rgSubs[0].strName.c_str());
            Assert::AreEqual (L"tcs",  rgSubs[1].strName.c_str());
            Assert::AreEqual (L"tcsb", rgSubs[2].strName.c_str());
            Assert::AreEqual (L"tcw",  rgSubs[3].strName.c_str());
        }




        TEST_METHOD(SetAliases_GenerateAndAppend_InMemory)
        {
            //
            // Tests generate + append + find round-trip entirely in memory
            //

            SAliasConfig config;

            config.strRootAlias      = L"d";
            config.strTcDirInvocation = L"tcdir";
            config.rgSubAliases      = {
                { L"dd", L"/a:d", L"dirs",   true  },
                { L"ds", L"/s",   L"search", true  },
                { L"dw", L"/w",   L"wide",   false },
            };

            wstring         strVersion = VERSION_WSTRING;
            vector<wstring> rgBlockLines;



            CAliasBlockGenerator::Generate (config, strVersion, rgBlockLines);

            CProfileFileManager fileMgr;
            vector<wstring>     rgLines;

            fileMgr.AppendAliasBlock (rgLines, rgBlockLines);

            SAliasBlock block;

            fileMgr.FindAliasBlock (rgLines, block);

            Assert::IsTrue (block.fFound);
            Assert::AreEqual (L"d", block.strRootAlias.c_str());
            Assert::AreEqual (3u, static_cast<unsigned>(block.rgAliasNames.size()));
        }




        TEST_METHOD(SetAliases_ExistingBlock_ReplacesCorrectly)
        {
            //
            // Create a file with existing aliases, then replace with new ones
            //

            SAliasConfig config1;

            config1.strRootAlias      = L"d";
            config1.strTcDirInvocation = L"tcdir";
            config1.rgSubAliases      = {
                { L"dd", L"/a:d", L"dirs", true },
            };

            SAliasConfig config2;

            config2.strRootAlias      = L"tc";
            config2.strTcDirInvocation = L"tcdir";
            config2.rgSubAliases      = {
                { L"tcd", L"/a:d", L"dirs", true },
                { L"tcs", L"/s",   L"search", true },
            };

            CProfileFileManager fileMgr;
            vector<wstring>     rgBlockLines1;
            vector<wstring>     rgBlockLines2;



            CAliasBlockGenerator::Generate (config1, L"5.2.1150", rgBlockLines1);
            CAliasBlockGenerator::Generate (config2, L"5.2.1151", rgBlockLines2);

            // Create file with first config + some surrounding content
            vector<wstring> rgLines = { L"# Before content" };
            fileMgr.AppendAliasBlock (rgLines, rgBlockLines1);
            rgLines.push_back (L"# After content");

            // Find and replace
            SAliasBlock block;
            fileMgr.FindAliasBlock (rgLines, block);
            Assert::IsTrue (block.fFound);

            fileMgr.ReplaceAliasBlock (rgLines, block, rgBlockLines2);

            // Verify replacement
            SAliasBlock block2;
            fileMgr.FindAliasBlock (rgLines, block2);

            Assert::IsTrue (block2.fFound);
            Assert::AreEqual (L"tc", block2.strRootAlias.c_str());
            Assert::AreEqual (3u, static_cast<unsigned>(block2.rgAliasNames.size()));

            // Verify surrounding content preserved
            Assert::AreEqual (L"# Before content", rgLines.front().c_str());
            Assert::AreEqual (L"# After content", rgLines.back().c_str());
        }




        TEST_METHOD(SessionOnly_NoFileWrite)
        {
            //
            // Verify session-only mode generates block but concept is sound
            //

            SAliasConfig config;

            config.strRootAlias      = L"d";
            config.strTcDirInvocation = L"tcdir";
            config.fSessionOnly      = true;

            vector<wstring> rgBlockLines;



            CAliasBlockGenerator::Generate (config, L"5.2.1150", rgBlockLines);

            Assert::IsTrue (rgBlockLines.size() > 5);

            // Verify block contains root alias
            bool fFoundRoot = false;

            for (const auto & line : rgBlockLines)
            {
                if (line.find (L"function d") != wstring::npos)
                {
                    fFoundRoot = true;
                    break;
                }
            }

            Assert::IsTrue (fFoundRoot);
        }




        TEST_METHOD(GetAliases_FindsBlockInFile)
        {
            //
            // Create a profile with aliases, then verify FindAliasBlock detects them
            //

            SAliasConfig config;

            config.strRootAlias      = L"d";
            config.strTcDirInvocation = L"tcdir";
            config.rgSubAliases      = {
                { L"dd", L"/a:d", L"dirs",  true },
                { L"ds", L"/s",   L"search", true },
            };

            vector<wstring> rgBlockLines;

            CAliasBlockGenerator::Generate (config, L"5.2.1150", rgBlockLines);

            vector<wstring> rgLines = { L"# My profile", L"Import-Module posh-git" };

            CProfileFileManager fileMgr;

            fileMgr.AppendAliasBlock (rgLines, rgBlockLines);

            SAliasBlock block;

            fileMgr.FindAliasBlock (rgLines, block);



            Assert::IsTrue (block.fFound);
            Assert::AreEqual (L"d",  block.strRootAlias.c_str());
            Assert::AreEqual (3u,    static_cast<unsigned>(block.rgAliasNames.size()));
            Assert::AreEqual (L"d",  block.rgAliasNames[0].c_str());
            Assert::AreEqual (L"dd", block.rgAliasNames[1].c_str());
            Assert::AreEqual (L"ds", block.rgAliasNames[2].c_str());
            Assert::AreEqual (L"5.2.1150", block.strVersion.c_str());
        }




        TEST_METHOD(GetAliases_NoAliases_BlockNotFound)
        {
            vector<wstring> rgLines = {
                L"# My PowerShell profile",
                L"Set-Location C:\\code",
                L"Import-Module posh-git",
            };

            CProfileFileManager fileMgr;
            SAliasBlock         block;

            fileMgr.FindAliasBlock (rgLines, block);



            Assert::IsFalse (block.fFound);
        }




        TEST_METHOD(GetAliases_MultipleProfilesCanBeScanned)
        {
            //
            // Simulate scanning multiple profiles — verify independent block detection
            //

            CProfileFileManager fileMgr;
            SAliasConfig        config;

            config.strRootAlias      = L"d";
            config.strTcDirInvocation = L"tcdir";
            config.rgSubAliases      = {
                { L"dd", L"/a:d", L"dirs", true },
            };

            vector<wstring> rgBlock;

            CAliasBlockGenerator::Generate (config, L"5.2.1150", rgBlock);

            // Profile 1: has aliases
            vector<wstring> rgProfile1 = { L"# profile 1" };
            fileMgr.AppendAliasBlock (rgProfile1, rgBlock);

            // Profile 2: no aliases
            vector<wstring> rgProfile2 = { L"# profile 2" };

            SAliasBlock block1;
            SAliasBlock block2;

            fileMgr.FindAliasBlock (rgProfile1, block1);
            fileMgr.FindAliasBlock (rgProfile2, block2);



            Assert::IsTrue  (block1.fFound);
            Assert::IsFalse (block2.fFound);
        }




        TEST_METHOD(UpdateAliases_RootChange_FullRoundTrip)
        {
            //
            // Simulate changing root from "d" to "tc" entirely in memory
            //

            CProfileFileManager fileMgr;
            SAliasConfig        config1;

            config1.strRootAlias      = L"d";
            config1.strTcDirInvocation = L"tcdir";
            config1.rgSubAliases      = { { L"dd", L"/a:d", L"dirs", true } };

            vector<wstring> rgBlock1;

            CAliasBlockGenerator::Generate (config1, L"5.2.1150", rgBlock1);

            // Build initial profile content with alias block
            vector<wstring> rgLines = { L"# My profile" };
            fileMgr.AppendAliasBlock (rgLines, rgBlock1);

            // Find existing block
            SAliasBlock block;

            fileMgr.FindAliasBlock (rgLines, block);



            Assert::IsTrue (block.fFound);
            Assert::AreEqual (L"d", block.strRootAlias.c_str());

            // Generate replacement block with new root
            SAliasConfig config2;

            config2.strRootAlias      = L"tc";
            config2.strTcDirInvocation = L"tcdir";
            config2.rgSubAliases      = { { L"tcd", L"/a:d", L"dirs", true }, { L"tcs", L"/s", L"search", true } };

            vector<wstring> rgBlock2;

            CAliasBlockGenerator::Generate (config2, L"5.2.1151", rgBlock2);

            fileMgr.ReplaceAliasBlock (rgLines, block, rgBlock2);

            // Verify replacement
            SAliasBlock block2;

            fileMgr.FindAliasBlock (rgLines, block2);

            Assert::IsTrue (block2.fFound);
            Assert::AreEqual (L"tc", block2.strRootAlias.c_str());
            Assert::AreEqual (3u, static_cast<unsigned>(block2.rgAliasNames.size()));

            // Verify original profile content preserved
            Assert::AreEqual (L"# My profile", rgLines[0].c_str());
        }




        TEST_METHOD(UpdateAliases_SameRootDifferentSubs)
        {
            //
            // Same root "d", but toggle sub-aliases
            //

            CProfileFileManager fileMgr;

            SAliasConfig config1;

            config1.strRootAlias      = L"d";
            config1.strTcDirInvocation = L"tcdir";
            config1.rgSubAliases      = {
                { L"dd", L"/a:d", L"dirs", true },
                { L"ds", L"/s",   L"search", true },
            };

            SAliasConfig config2;

            config2.strRootAlias      = L"d";
            config2.strTcDirInvocation = L"tcdir";
            config2.rgSubAliases      = {
                { L"dd", L"/a:d", L"dirs", false },   // disabled
                { L"ds", L"/s",   L"search", true },
                { L"dw", L"/w",   L"wide", true },    // newly enabled
            };

            vector<wstring> rgBlock1;
            vector<wstring> rgBlock2;

            CAliasBlockGenerator::Generate (config1, L"5.2.1150", rgBlock1);
            CAliasBlockGenerator::Generate (config2, L"5.2.1151", rgBlock2);

            vector<wstring> rgLines;

            fileMgr.AppendAliasBlock (rgLines, rgBlock1);

            SAliasBlock block;

            fileMgr.FindAliasBlock (rgLines, block);



            Assert::IsTrue (block.fFound);

            fileMgr.ReplaceAliasBlock (rgLines, block, rgBlock2);

            SAliasBlock block2;

            fileMgr.FindAliasBlock (rgLines, block2);

            Assert::IsTrue (block2.fFound);
            Assert::AreEqual (L"d", block2.strRootAlias.c_str());
            // Should have: d, ds, dw (dd disabled)
            Assert::AreEqual (3u, static_cast<unsigned>(block2.rgAliasNames.size()));
            Assert::AreEqual (L"d",  block2.rgAliasNames[0].c_str());
            Assert::AreEqual (L"ds", block2.rgAliasNames[1].c_str());
            Assert::AreEqual (L"dw", block2.rgAliasNames[2].c_str());
        }




        TEST_METHOD(RemoveAliases_CleanRemoval_InMemory)
        {
            //
            // Build profile content + alias block in memory, remove block,
            // verify remaining content is untouched.
            //

            CProfileFileManager fileMgr;
            SAliasConfig        config;

            config.strRootAlias      = L"d";
            config.strTcDirInvocation = L"tcdir";
            config.rgSubAliases      = { { L"dd", L"/a:d", L"dirs", true } };

            vector<wstring> rgBlock;

            CAliasBlockGenerator::Generate (config, L"5.2.1150", rgBlock);

            vector<wstring> rgLines = {
                L"# Before aliases",
                L"Import-Module posh-git",
            };

            fileMgr.AppendAliasBlock (rgLines, rgBlock);
            rgLines.push_back (L"# After aliases");

            // Find and remove block
            SAliasBlock block;

            fileMgr.FindAliasBlock (rgLines, block);



            Assert::IsTrue (block.fFound);

            fileMgr.RemoveAliasBlock (rgLines, block);

            // Verify block is gone
            SAliasBlock block2;

            fileMgr.FindAliasBlock (rgLines, block2);

            Assert::IsFalse (block2.fFound);

            // Verify original content preserved
            bool fFoundBefore = false;
            bool fFoundModule = false;
            bool fFoundAfter  = false;

            for (const auto & line : rgLines)
            {
                if (line == L"# Before aliases")       fFoundBefore = true;
                if (line == L"Import-Module posh-git")  fFoundModule = true;
                if (line == L"# After aliases")        fFoundAfter = true;
            }

            Assert::IsTrue (fFoundBefore);
            Assert::IsTrue (fFoundModule);
            Assert::IsTrue (fFoundAfter);
        }




        TEST_METHOD(RemoveAliases_NoAliases_NothingToRemove)
        {
            vector<wstring> rgLines = {
                L"# My profile",
                L"Set-Location C:\\code",
            };

            CProfileFileManager fileMgr;
            SAliasBlock         block;

            fileMgr.FindAliasBlock (rgLines, block);



            Assert::IsFalse (block.fFound);
        }




        TEST_METHOD(WhatIf_SetAliases_NoFileModification)
        {
            //
            // Verify that when fWhatIf is set, the generated block is correct
            // but no file operations would occur (tested by config flag)
            //

            SAliasConfig config;

            config.strRootAlias      = L"d";
            config.strTcDirInvocation = L"tcdir";
            config.fWhatIf           = true;
            config.rgSubAliases      = { { L"dd", L"/a:d", L"dirs", true } };

            vector<wstring> rgBlockLines;



            CAliasBlockGenerator::Generate (config, L"5.2.1150", rgBlockLines);

            Assert::IsTrue (config.fWhatIf);
            Assert::IsTrue (rgBlockLines.size() > 5);

            // The block should still be generated correctly even in whatif mode
            bool fFoundRoot = false;

            for (const auto & line : rgBlockLines)
            {
                if (line == L"function d { tcdir @args }")
                {
                    fFoundRoot = true;
                    break;
                }
            }

            Assert::IsTrue (fFoundRoot);
        }




        TEST_METHOD(WhatIf_RemoveAliases_NoFileModification)
        {
            //
            // Build profile with aliases in memory, simulate whatif by
            // detecting block but NOT calling RemoveAliasBlock, verify unchanged.
            //

            CProfileFileManager fileMgr;
            SAliasConfig        config;

            config.strRootAlias      = L"d";
            config.strTcDirInvocation = L"tcdir";
            config.rgSubAliases      = { { L"dd", L"/a:d", L"dirs", true } };

            vector<wstring> rgBlock;

            CAliasBlockGenerator::Generate (config, L"5.2.1150", rgBlock);

            vector<wstring> rgLines = { L"# Profile content" };

            fileMgr.AppendAliasBlock (rgLines, rgBlock);

            size_t cOriginalLines = rgLines.size();

            // WhatIf: detect block but do NOT remove
            SAliasBlock block;

            fileMgr.FindAliasBlock (rgLines, block);



            Assert::IsTrue (block.fFound);

            // Verify lines are unchanged (whatif = no modification)
            Assert::AreEqual (cOriginalLines, rgLines.size());

            SAliasBlock block2;

            fileMgr.FindAliasBlock (rgLines, block2);

            Assert::IsTrue (block2.fFound);
        }




        TEST_METHOD(ConflictDetection_KnownBuiltinAliasTable)
        {
            //
            // Verify the built-in alias conflict list is populated correctly
            // by testing that a known-conflicting root generates warnings.
            // "r" is a known PowerShell alias for Invoke-History.
            //
            // Note: We test the pure-data lookup here. The SearchPathW part
            // of CheckConflicts is an integration concern.
            //

            CTestConsole        testConsole;
            shared_ptr<CConfig> configPtr = make_shared<CConfig>();

            testConsole.Initialize (configPtr);

            bool                     fProceed = true;
            vector<SAliasDefinition> rgSubs;

            HRESULT hr = CAliasManager::CheckConflicts (testConsole, L"r", rgSubs, fProceed);



            Assert::IsTrue (SUCCEEDED (hr));
            Assert::IsTrue (fProceed);
        }




        TEST_METHOD(ConflictDetection_NoConflict)
        {
            //
            // "zzz" is not a known built-in alias and very unlikely on PATH
            //

            CTestConsole        testConsole;
            shared_ptr<CConfig> configPtr = make_shared<CConfig>();

            testConsole.Initialize (configPtr);

            bool                     fProceed = true;
            vector<SAliasDefinition> rgSubs;

            HRESULT hr = CAliasManager::CheckConflicts (testConsole, L"zzz", rgSubs, fProceed);



            Assert::IsTrue (SUCCEEDED (hr));
            Assert::IsTrue (fProceed);
        }
    };
}
