#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/ProfilePathResolver.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(ProfilePathResolverTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }
    };
}
