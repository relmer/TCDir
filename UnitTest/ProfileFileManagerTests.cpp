#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/ProfileFileManager.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(ProfileFileManagerTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }
    };
}
