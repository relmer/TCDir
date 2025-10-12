#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/DirectoryLister.h"
#include "../TCDirCore/Console.h"
#include "../TCDirCore/Config.h"
#include "../TCDirCore/CommandLine.h"







using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{
    TEST_CLASS(DirectoryListerTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            // Setup EHM to use unit test framework's assertion mechanism
            SetupEhmForUnitTests();
        }





        TEST_METHOD(IsDotsRecognition)
        {
            struct ListerProbe : public CDirectoryLister
            {
                using CDirectoryLister::CDirectoryLister;
                BOOL Call(LPCWSTR s) { return IsDots(s); }
            };

            auto cmd = std::make_shared<CCommandLine>();
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);
            ListerProbe lp(cmd, con, cfg);



            Assert::IsTrue(lp.Call(L"."));
            Assert::IsTrue(lp.Call(L".."));
            Assert::IsFalse(lp.Call(L".git"));
            Assert::IsFalse(lp.Call(L"file.txt"));
        }

    };
}
