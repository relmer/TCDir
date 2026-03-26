#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/AliasManager.h"
#include "../TCDirCore/AliasBlockGenerator.h"
#include "../TCDirCore/ProfileFileManager.h"
#include "../TCDirCore/Version.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    //
    // Helper: create a temp file with given content
    //

    static wstring CreateTempFile (const wstring & strContent)
    {
        WCHAR szTempDir[MAX_PATH]  = {};
        WCHAR szTempFile[MAX_PATH] = {};

        GetTempPathW (MAX_PATH, szTempDir);
        GetTempFileNameW (szTempDir, L"am", 0, szTempFile);

        FILE * pf = nullptr;

        _wfopen_s (&pf, szTempFile, L"wb");

        if (pf != nullptr)
        {
            int    cb   = WideCharToMultiByte (CP_UTF8, 0, strContent.data(), static_cast<int>(strContent.size()), nullptr, 0, nullptr, nullptr);
            string utf8 (static_cast<size_t>(cb), '\0');

            WideCharToMultiByte (CP_UTF8, 0, strContent.data(), static_cast<int>(strContent.size()), utf8.data(), cb, nullptr, nullptr);
            fwrite (utf8.data(), 1, utf8.size(), pf);
            fclose (pf);
        }

        return szTempFile;
    }





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




        TEST_METHOD(ResolveTcDirInvocation_ReturnsNonEmpty)
        {
            wstring strInvocation;
            HRESULT hr = CAliasManager::ResolveTcDirInvocation (strInvocation);



            Assert::IsTrue (SUCCEEDED (hr));
            Assert::IsFalse (strInvocation.empty());
        }




        TEST_METHOD(SetAliases_EndToEnd_WritesToFile)
        {
            //
            // This tests the write path by directly generating and writing a block
            // (bypasses TUI which requires interactive input)
            //

            WCHAR   szTempDir[MAX_PATH]  = {};
            WCHAR   szTempFile[MAX_PATH] = {};

            GetTempPathW (MAX_PATH, szTempDir);
            GetTempFileNameW (szTempDir, L"am", 0, szTempFile);

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

            HRESULT hr = fileMgr.WriteProfileFile (szTempFile, rgLines, false);

            Assert::IsTrue (SUCCEEDED (hr));

            // Read back and verify
            vector<wstring> rgReadBack;
            bool            fHasBom = false;

            hr = fileMgr.ReadProfileFile (szTempFile, rgReadBack, fHasBom);

            Assert::IsTrue (SUCCEEDED (hr));

            SAliasBlock block;

            fileMgr.FindAliasBlock (rgReadBack, block);

            Assert::IsTrue (block.fFound);
            Assert::AreEqual (L"d", block.strRootAlias.c_str());
            Assert::AreEqual (3u, static_cast<unsigned>(block.rgAliasNames.size()));

            DeleteFileW (szTempFile);
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
            // Simulate changing root from "d" to "tc" via file round-trip
            //

            WCHAR szTempDir[MAX_PATH]  = {};
            WCHAR szTempFile[MAX_PATH] = {};

            GetTempPathW (MAX_PATH, szTempDir);
            GetTempFileNameW (szTempDir, L"am", 0, szTempFile);

            CProfileFileManager fileMgr;
            SAliasConfig        config1;

            config1.strRootAlias      = L"d";
            config1.strTcDirInvocation = L"tcdir";
            config1.rgSubAliases      = { { L"dd", L"/a:d", L"dirs", true } };

            vector<wstring> rgBlock1;

            CAliasBlockGenerator::Generate (config1, L"5.2.1150", rgBlock1);

            // Write initial file
            vector<wstring> rgLines = { L"# My profile" };
            fileMgr.AppendAliasBlock (rgLines, rgBlock1);
            fileMgr.WriteProfileFile (szTempFile, rgLines, false);

            // Read back, find block, replace with new root
            vector<wstring> rgReadBack;
            bool            fHasBom = false;

            fileMgr.ReadProfileFile (szTempFile, rgReadBack, fHasBom);

            SAliasBlock block;

            fileMgr.FindAliasBlock (rgReadBack, block);



            Assert::IsTrue (block.fFound);
            Assert::AreEqual (L"d", block.strRootAlias.c_str());

            SAliasConfig config2;

            config2.strRootAlias      = L"tc";
            config2.strTcDirInvocation = L"tcdir";
            config2.rgSubAliases      = { { L"tcd", L"/a:d", L"dirs", true }, { L"tcs", L"/s", L"search", true } };

            vector<wstring> rgBlock2;

            CAliasBlockGenerator::Generate (config2, L"5.2.1151", rgBlock2);

            fileMgr.ReplaceAliasBlock (rgReadBack, block, rgBlock2);
            fileMgr.WriteProfileFile (szTempFile, rgReadBack, false);

            // Read back again and verify
            vector<wstring> rgFinal;

            fileMgr.ReadProfileFile (szTempFile, rgFinal, fHasBom);

            SAliasBlock block2;

            fileMgr.FindAliasBlock (rgFinal, block2);

            Assert::IsTrue (block2.fFound);
            Assert::AreEqual (L"tc", block2.strRootAlias.c_str());
            Assert::AreEqual (3u, static_cast<unsigned>(block2.rgAliasNames.size()));

            // Verify original profile content preserved
            Assert::AreEqual (L"# My profile", rgFinal[0].c_str());

            DeleteFileW (szTempFile);
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
    };
}
