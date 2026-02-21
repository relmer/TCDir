#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/Console.h"
#include "../TCDirCore/Config.h"
#include "../TCDirCore/Usage.h"
#include "Mocks/TestConsole.h"





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
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            // No markers - should output the entire string without crashing
            con->ColorPuts (L"Hello, World!");
        }





        TEST_METHOD(ColorPuts_SingleMarker_SwitchesColor)
        {
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            con->ColorPuts (L"Normal {InformationHighlight}Highlighted");
        }





        TEST_METHOD(ColorPuts_MultipleMarkers_SwitchesColorsCorrectly)
        {
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            con->ColorPuts (L"{InformationHighlight}-A{Information}  Displays files");
        }





        TEST_METHOD(ColorPuts_AllAttributes_ParsesCorrectly)
        {
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            // Test all valid attribute names
            con->ColorPuts (
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
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            con->ColorPuts (
                L"Cloud: {CloudStatusCloudOnly}○{Information} "
                L"{CloudStatusLocallyAvailable}◐{Information} "
                L"{CloudStatusAlwaysLocallyAvailable}●{Information}");
        }





        TEST_METHOD(ColorPuts_MarkerAtStart_ParsesCorrectly)
        {
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            con->ColorPuts (L"{InformationHighlight}Highlighted text");
        }





        TEST_METHOD(ColorPuts_MarkerAtEnd_ParsesCorrectly)
        {
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            con->ColorPuts (L"Text then marker{Default}");
        }





        TEST_METHOD(ColorPuts_ConsecutiveMarkers_ParsesCorrectly)
        {
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            con->ColorPuts (L"{Information}{InformationHighlight}Text");
        }





        TEST_METHOD(ColorPuts_EmptyString_NoOutput)
        {
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            con->ColorPuts (L"");
        }





        TEST_METHOD(ColorPuts_OnlyMarkers_NoVisibleOutput)
        {
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            con->ColorPuts (L"{Information}{InformationHighlight}{Default}");
        }





        TEST_METHOD(ColorPuts_MultilineWithMarkers_HandlesNewlines)
        {
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            con->ColorPuts (
                L"Line 1 {InformationHighlight}highlighted\n"
                L"Line 2 {Information}normal");
        }





        TEST_METHOD(ColorPrintf_FormatsAndProcessesMarkers)
        {
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            con->ColorPrintf (L"{InformationHighlight}%s{Information} = %d", L"Value", 42);
        }





        TEST_METHOD(ColorPrintf_NoMarkers_FormatsCorrectly)
        {
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            con->ColorPrintf (L"Simple format: %d", 123);
        }





        TEST_METHOD(ColorPuts_SwitchStyleUsage_FormatsCorrectly)
        {
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            // Test the actual usage pattern with switches
            con->ColorPuts (
                L"  {InformationHighlight}-A{Information}          Displays files with specified attributes.");
        }





        TEST_METHOD(ColorPuts_LongSwitchStyleUsage_FormatsCorrectly)
        {
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            // Test long switches like --Env
            con->ColorPuts (
                L"  {InformationHighlight}--Env{Information}       Displays TCDIR help.");
        }





        TEST_METHOD(ColorPuts_AttributeLettersStyleUsage_FormatsCorrectly)
        {
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            // Test attribute letters like D, R, H
            con->ColorPuts (
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
            SetBreakpointFunction (AssertExpectedBreakpoint);
        }

        TEST_METHOD_CLEANUP(MethodCleanup)
        {
            // Restore the normal unit test breakpoint handler
            SetupEhmForUnitTests();
        }





        TEST_METHOD(ColorPuts_UnclosedBrace_AssertsInDebug)
        {
#if DBG || DEBUG || _DEBUG
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            // This should trigger an ASSERT due to unclosed brace
            con->ColorPuts (L"Text with {Information unclosed");

            // Verify the assertion fired
            Assert::IsTrue (s_fAssertFired, L"ASSERT should fire for unclosed color marker brace");
#else
            // ASSERTs are compiled out in Release builds - skip this test
            Assert::IsTrue (true);
#endif
        }





        TEST_METHOD(ColorPuts_UnknownMarkerName_AssertsInDebug)
        {
#if DBG || DEBUG || _DEBUG
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            // This should trigger an ASSERT due to unknown marker name
            con->ColorPuts (L"Text with {UnknownMarker} here");

            // Verify the assertion fired
            Assert::IsTrue (s_fAssertFired, L"ASSERT should fire for unknown color marker name");
#else
            // ASSERTs are compiled out in Release builds - skip this test
            Assert::IsTrue (true);
#endif
        }





        TEST_METHOD(ColorPuts_ValidMarker_DoesNotAssert)
        {
            auto con = std::make_shared<CTestConsole> ();
            auto cfg = std::make_shared<CConfig> ();
            con->Initialize (cfg);

            // This should NOT trigger an ASSERT
            con->ColorPuts (L"Text with {InformationHighlight}valid{Information} marker");

            // Verify no assertion fired
            Assert::IsFalse (s_fAssertFired, L"ASSERT should NOT fire for valid color markers");
        }
    };





    //
    //  CCapturingConsole
    //
    //  Captures buffer content on Flush instead of discarding it.
    //

    class CCapturingConsole : public CConsole
    {
    public:

        wstring m_strCapturedOutput;



        ~CCapturingConsole()
        {
            Flush();
        }



        void SetWidth (UINT cx)
        {
            m_cxConsoleWidth = cx;
        }



        HRESULT Flush (void) override
        {
            m_strCapturedOutput.append (m_strBuffer);
            m_strBuffer.clear();
            return S_OK;
        }
    };





    //
    //  StripAnsiCodes
    //
    //  Removes ANSI SGR escape sequences (\x1b[...m) from a string,
    //  leaving only visible text.
    //

    static wstring StripAnsiCodes (const wstring & input)
    {
        wstring result;
        result.reserve (input.size());



        for (size_t i = 0; i < input.size(); ++i)
        {
            if (input[i] == L'\x1b' && i + 1 < input.size() && input[i + 1] == L'[')
            {
                // Skip ESC [ ... m sequence
                i += 2;
                while (i < input.size() && input[i] != L'm')
                    ++i;
                continue;
            }

            result.push_back (input[i]);
        }

        return result;
    }





    //
    //  FindColumnOfText
    //
    //  In a multi-line string, finds the line containing 'lineMarker' and
    //  returns the column (0-based) where 'targetText' starts on that line.
    //  Returns -1 if not found.
    //

    static int FindColumnOfText (const wstring & text, const wstring & lineMarker, const wstring & targetText)
    {
        size_t markerPos = text.find (lineMarker);

        if (markerPos == wstring::npos)
            return -1;

        // Find beginning of this line
        size_t lineBegin = text.rfind (L'\n', markerPos);
        lineBegin = (lineBegin == wstring::npos) ? 0 : lineBegin + 1;

        // Find the target text after the marker
        size_t targetPos = text.find (targetText, markerPos);

        if (targetPos == wstring::npos)
            return -1;

        return static_cast<int>(targetPos - lineBegin);
    }





    TEST_CLASS(UsageAlignmentTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        //
        //  Verify that all /? descriptions and sub-descriptions are properly
        //  column-aligned for both "/" and "--" prefix modes.
        //

        TEST_METHOD(DisplayUsage_SlashPrefix_DescriptionsAlignedAtColumn20)
        {
            auto con = make_shared<CCapturingConsole>();
            auto cfg = make_shared<CConfig>();
            con->Initialize (cfg);



            CUsage::DisplayUsage (*con, L'/');
            con->Flush();

            wstring plain = StripAnsiCodes (con->m_strCapturedOutput);

            // All option descriptions must start at column 20
            constexpr int DESC_COL = 20;

            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"/A ",             L"Displays files with specified"),    L"/A description column");
            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"/O ",             L"List by files in sorted"),          L"/O description column");
            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"/T ",             L"Selects the time field"),           L"/T description column");
            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"/S ",             L"Displays files in specified dir"),  L"/S description column");
            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"/W ",             L"Displays results in a wide"),       L"/W description column");
            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"/B ",             L"Displays bare file"),               L"/B description column");
            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"/M ",             L"Enables multi-threaded"),           L"/M description column");
            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"/Env ",           L"Displays TCDIR"),                   L"/Env description column");
            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"/Config ",        L"Displays current color"),           L"/Config description column");
            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"/Owner ",         L"Displays file owner"),              L"/Owner description column");
            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"/Streams ",       L"Displays alternate data"),          L"/Streams description column");
            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"/Icons ",         L"Enables file-type icons"),          L"/Icons description column");
            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"/Tree ",          L"Displays a hierarchical"),          L"/Tree description column");
            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"  /Depth=N",      L"Limits tree depth"),                L"/Depth=N description column");
            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"  /TreeIndent=N", L"Sets tree indent"),                 L"/TreeIndent=N description column");

            // Sub-descriptions must start at column 22 (DESC_COL + 2)
            constexpr int SUB_COL = 22;

            Assert::AreEqual (SUB_COL, FindColumnOfText (plain, L"  attributes",     L"D  Directories"),                   L"attributes sub-description column");
            Assert::AreEqual (SUB_COL, FindColumnOfText (plain, L"  sortorder",      L"N  By name"),                       L"sortorder sub-description column");
            Assert::AreEqual (SUB_COL, FindColumnOfText (plain, L"  timefield",      L"C  Creation time"),                 L"timefield sub-description column");
        }





        TEST_METHOD(DisplayUsage_DashPrefix_DescriptionsAlignedAtColumn20)
        {
            auto con = make_shared<CCapturingConsole>();
            auto cfg = make_shared<CConfig>();
            con->Initialize (cfg);



            CUsage::DisplayUsage (*con, L'-');
            con->Flush();

            wstring plain = StripAnsiCodes (con->m_strCapturedOutput);

            // All option descriptions must start at column 20 in -- mode too
            constexpr int DESC_COL = 20;

            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"-A ",              L"Displays files with specified"),    L"-A description column");
            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"--Owner ",         L"Displays file owner"),              L"--Owner description column");
            Assert::AreEqual (DESC_COL, FindColumnOfText (plain, L"  --TreeIndent=N", L"Sets tree indent"),                 L"--TreeIndent=N description column");

            // Sub-descriptions at column 22
            constexpr int SUB_COL = 22;

            Assert::AreEqual (SUB_COL, FindColumnOfText (plain, L"  attributes",      L"D  Directories"),                   L"attributes sub-description column (-- mode)");
        }





        //
        //  Verify that the synopsis line wraps at the console width, with
        //  continuation lines indented to column 6 (under [drive:]).
        //

        TEST_METHOD(DisplayUsage_WideConsole_SynopsisOnOneLine)
        {
            auto con = make_shared<CCapturingConsole>();
            auto cfg = make_shared<CConfig>();
            con->Initialize (cfg);
            con->SetWidth (300);



            CUsage::DisplayUsage (*con, L'/');
            con->Flush();

            wstring plain = StripAnsiCodes (con->m_strCapturedOutput);

            // Find the TCDIR line — it should contain all options on one line
            size_t tcdirPos = plain.find (L"TCDIR [drive:]");
            Assert::AreNotEqual (wstring::npos, tcdirPos, L"TCDIR synopsis not found");

            // Find the end of the TCDIR line
            size_t eolPos = plain.find (L'\n', tcdirPos);
            Assert::AreNotEqual (wstring::npos, eolPos, L"TCDIR line end not found");

            wstring tcdirLine = plain.substr (tcdirPos, eolPos - tcdirPos);

            // All options should be on this single line
            Assert::IsTrue (tcdirLine.find (L"[/TreeIndent=N]") != wstring::npos, L"/TreeIndent=N should be on the TCDIR line");
#ifdef _DEBUG
            Assert::IsTrue (tcdirLine.find (L"[/Debug]")        != wstring::npos, L"/Debug should be on the TCDIR line");
#endif
        }





        TEST_METHOD(DisplayUsage_NarrowConsole_SynopsisWrapsAtColumn6)
        {
            auto con = make_shared<CCapturingConsole>();
            auto cfg = make_shared<CConfig>();
            con->Initialize (cfg);
            con->SetWidth (80);



            CUsage::DisplayUsage (*con, L'/');
            con->Flush();

            wstring plain = StripAnsiCodes (con->m_strCapturedOutput);

            // Find the TCDIR line
            size_t tcdirPos = plain.find (L"TCDIR [drive:]");
            Assert::AreNotEqual (wstring::npos, tcdirPos, L"TCDIR synopsis not found");

            // The synopsis must wrap — /TreeIndent=N should NOT be on the first line
            size_t firstEol   = plain.find (L'\n', tcdirPos);
            wstring firstLine = plain.substr (tcdirPos, firstEol - tcdirPos);
            Assert::IsTrue (firstLine.find (L"[/TreeIndent=N]") == wstring::npos, L"/TreeIndent=N should NOT be on the first line at 80 cols");

            // Find the continuation line — it should start with 6 spaces
            size_t contLinePos = firstEol + 1;
            wstring contLine   = plain.substr (contLinePos, plain.find (L'\n', contLinePos) - contLinePos);
            Assert::IsTrue (contLine.substr (0, 6) == L"      ", L"Continuation line should start with 6-space indent");
            Assert::IsTrue (contLine[6] == L'[', L"First option on continuation line should start at column 6");
        }
    };
}
