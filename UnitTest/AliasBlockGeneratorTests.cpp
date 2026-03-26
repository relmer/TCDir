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
    };
}
