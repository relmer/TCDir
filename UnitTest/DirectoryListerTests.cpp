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
            // IsDots is now static, can call directly
            Assert::IsTrue (CDirectoryLister::IsDots(L"."));
            Assert::IsTrue (CDirectoryLister::IsDots(L".."));
            Assert::IsFalse (CDirectoryLister::IsDots(L".git"));
            Assert::IsFalse (CDirectoryLister::IsDots(L"file.txt"));
        }

    };
}


