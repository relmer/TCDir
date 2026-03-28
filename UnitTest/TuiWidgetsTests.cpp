#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/TuiWidgets.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(TuiWidgetsTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }
    };
}
