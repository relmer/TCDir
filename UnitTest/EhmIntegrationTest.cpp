#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/Ehm.h"
#include "../TCDirCore/Config.h"
#include "../TCDirCore/Console.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(EhmIntegrationTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            // Setup EHM to use unit test framework's assertion mechanism
            SetupEhmForUnitTests();
        }





        TEST_METHOD(EhmCustomBreakpointIsSetup)
        {
            // This test verifies that the custom EHM breakpoint function has been set up
            // It checks that g_pfnBreakpoint is not null, indicating our custom handler is installed
            
            // Verify that the custom breakpoint function has been set
            Assert::IsNotNull((void*)g_pfnBreakpoint, L"Custom EHM breakpoint function should be set");
        }





        TEST_METHOD(EhmAssertionPassesWhenConditionIsTrue)
        {
            // This test verifies that EHM assertions work normally when the condition is true
            
            // This should not trigger any assertion failure
            ASSERT(TRUE);
            
            // If we get here, the assertion passed as expected
            Assert::IsTrue(true, L"EHM assertion passed when condition was true");
        }





        TEST_METHOD(CConsoleCanInitializeInUnitTestEnvironment)
        {
            // This test validates that CConsole::Initialize works in the unit test environment
            
            auto cfg = std::make_shared<CConfig>();
            auto con = std::make_shared<CConsole>();
            
            HRESULT hr = con->Initialize(cfg);
            Assert::IsTrue(SUCCEEDED(hr), L"CConsole::Initialize should succeed");
        }

    };
}
