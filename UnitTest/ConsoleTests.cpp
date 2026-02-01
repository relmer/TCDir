#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/Console.h"
#include "../TCDirCore/Config.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{
    TEST_CLASS(ConsoleColorTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }





        TEST_METHOD(ColorPuts_NoMarkers_OutputsEntireString)
        {
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            // No markers - should output the entire string without crashing
            con->ColorPuts(L"Hello, World!");
        }





        TEST_METHOD(ColorPuts_SingleMarker_SwitchesColor)
        {
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            con->ColorPuts(L"Normal {InformationHighlight}Highlighted");
        }





        TEST_METHOD(ColorPuts_MultipleMarkers_SwitchesColorsCorrectly)
        {
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            con->ColorPuts(L"{InformationHighlight}-A{Information}  Displays files");
        }





        TEST_METHOD(ColorPuts_AllAttributes_ParsesCorrectly)
        {
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            // Test all valid attribute names
            con->ColorPuts(
                L"{Default}D"
                L"{Date}Dt"
                L"{Time}Tm"
                L"{FileAttributePresent}Fa"
                L"{FileAttributeNotPresent}Fn"
                L"{Size}Sz"
                L"{Directory}Dr"
                L"{Information}In"
                L"{InformationHighlight}Hl"
                L"{SeparatorLine}Sl"
                L"{Error}Er"
                L"{Owner}Ow"
                L"{Stream}St"
                L"{CloudStatusCloudOnly}C1"
                L"{CloudStatusLocallyAvailable}C2"
                L"{CloudStatusAlwaysLocallyAvailable}C3"
            );
        }





        TEST_METHOD(ColorPuts_CloudStatusMarkers_ApplyCorrectColors)
        {
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            con->ColorPuts(
                L"Cloud: {CloudStatusCloudOnly}○{Information} "
                L"{CloudStatusLocallyAvailable}◐{Information} "
                L"{CloudStatusAlwaysLocallyAvailable}●{Information}");
        }





        TEST_METHOD(ColorPuts_MarkerAtStart_ParsesCorrectly)
        {
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            con->ColorPuts(L"{InformationHighlight}Highlighted text");
        }





        TEST_METHOD(ColorPuts_MarkerAtEnd_ParsesCorrectly)
        {
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            con->ColorPuts(L"Text then marker{Default}");
        }





        TEST_METHOD(ColorPuts_ConsecutiveMarkers_ParsesCorrectly)
        {
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            con->ColorPuts(L"{Information}{InformationHighlight}Text");
        }





        TEST_METHOD(ColorPuts_EmptyString_NoOutput)
        {
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            con->ColorPuts(L"");
        }





        TEST_METHOD(ColorPuts_OnlyMarkers_NoVisibleOutput)
        {
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            con->ColorPuts(L"{Information}{InformationHighlight}{Default}");
        }





        TEST_METHOD(ColorPuts_MultilineWithMarkers_HandlesNewlines)
        {
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            con->ColorPuts(
                L"Line 1 {InformationHighlight}highlighted\n"
                L"Line 2 {Information}normal");
        }





        TEST_METHOD(ColorPrintf_FormatsAndProcessesMarkers)
        {
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            con->ColorPrintf(L"{InformationHighlight}%s{Information} = %d", L"Value", 42);
        }





        TEST_METHOD(ColorPrintf_NoMarkers_FormatsCorrectly)
        {
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            con->ColorPrintf(L"Simple format: %d", 123);
        }





        TEST_METHOD(ColorPuts_SwitchStyleUsage_FormatsCorrectly)
        {
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            // Test the actual usage pattern with switches
            con->ColorPuts(
                L"  {InformationHighlight}-A{Information}          Displays files with specified attributes.");
        }





        TEST_METHOD(ColorPuts_LongSwitchStyleUsage_FormatsCorrectly)
        {
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            // Test long switches like --Env
            con->ColorPuts(
                L"  {InformationHighlight}--Env{Information}       Displays TCDIR help.");
        }





        TEST_METHOD(ColorPuts_AttributeLettersStyleUsage_FormatsCorrectly)
        {
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            // Test attribute letters like D, R, H
            con->ColorPuts(
                L"  attributes   {InformationHighlight}D{Information}  Directories                "
                L"{InformationHighlight}R{Information}  Read-only files");
        }
    };





    //
    // Helper for testing that ASSERTs fire correctly
    //

    static bool s_fAssertFired = false;

    static void AssertExpectedBreakpoint()
    {
        s_fAssertFired = true;
    }





    TEST_CLASS(ConsoleColorAssertTests)
    {
    public:

        TEST_METHOD_INITIALIZE(MethodInitialize)
        {
            // Reset the flag before each test
            s_fAssertFired = false;

            // Set up our custom breakpoint handler that records the assertion
            SetBreakpointFunction(AssertExpectedBreakpoint);
        }

        TEST_METHOD_CLEANUP(MethodCleanup)
        {
            // Restore the normal unit test breakpoint handler
            SetupEhmForUnitTests();
        }





        TEST_METHOD(ColorPuts_UnclosedBrace_AssertsInDebug)
        {
#if DBG || DEBUG || _DEBUG
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            // This should trigger an ASSERT due to unclosed brace
            con->ColorPuts(L"Text with {Information unclosed");

            // Verify the assertion fired
            Assert::IsTrue(s_fAssertFired, L"ASSERT should fire for unclosed color marker brace");
#else
            // ASSERTs are compiled out in Release builds - skip this test
            Assert::IsTrue(true);
#endif
        }





        TEST_METHOD(ColorPuts_UnknownMarkerName_AssertsInDebug)
        {
#if DBG || DEBUG || _DEBUG
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            // This should trigger an ASSERT due to unknown marker name
            con->ColorPuts(L"Text with {UnknownMarker} here");

            // Verify the assertion fired
            Assert::IsTrue(s_fAssertFired, L"ASSERT should fire for unknown color marker name");
#else
            // ASSERTs are compiled out in Release builds - skip this test
            Assert::IsTrue(true);
#endif
        }





        TEST_METHOD(ColorPuts_ValidMarker_DoesNotAssert)
        {
            auto con = std::make_shared<CConsole>();
            auto cfg = std::make_shared<CConfig>();
            con->Initialize(cfg);

            // This should NOT trigger an ASSERT
            con->ColorPuts(L"Text with {InformationHighlight}valid{Information} marker");

            // Verify no assertion fired
            Assert::IsFalse(s_fAssertFired, L"ASSERT should NOT fire for valid color markers");
        }
    };
}
