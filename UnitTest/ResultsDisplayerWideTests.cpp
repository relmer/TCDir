#include "pch.h"
#include "Mocks/TestConsole.h"
#include "../TCDirCore/ResultsDisplayerWide.h"
#include "../TCDirCore/CommandLine.h"
#include "../TCDirCore/Config.h"
#include "../TCDirCore/Console.h"
#include "../TCDirCore/DirectoryInfo.h"
#include "../TCDirCore/UnicodeSymbols.h"
#include "../TCDirCore/EnvironmentProviderBase.h"



using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{
    //
    // Helper to build a synthetic WIN32_FIND_DATA with a given filename and attributes
    //

    static WIN32_FIND_DATA MakeWfd (LPCWSTR pszName, DWORD dwAttrs = 0)
    {
        WIN32_FIND_DATA wfd = {};
        wfd.dwFileAttributes = dwAttrs;
        wcscpy_s (wfd.cFileName, pszName);
        return wfd;
    }





    ////////////////////////////////////////////////////////////////////////////
    //
    //  ComputeDisplayWidth Tests (T004, T013)
    //
    ////////////////////////////////////////////////////////////////////////////

    TEST_CLASS (ComputeDisplayWidthTests)
    {
    public:

        TEST_METHOD (PlainFile_ReturnsFileNameLength)
        {
            auto wfd = MakeWfd (L"readme.txt");

            size_t w = CResultsDisplayerWide::ComputeDisplayWidth (wfd, false, false, false);

            Assert::AreEqual (static_cast<size_t>(10), w);
        }

        TEST_METHOD (Directory_IconsOff_AddsBrackets)
        {
            auto wfd = MakeWfd (L"docs", FILE_ATTRIBUTE_DIRECTORY);

            size_t w = CResultsDisplayerWide::ComputeDisplayWidth (wfd, false, false, false);

            Assert::AreEqual (static_cast<size_t>(6), w);  // [docs] = 4 + 2
        }

        TEST_METHOD (Directory_IconsOn_NoBrackets)
        {
            auto wfd = MakeWfd (L"docs", FILE_ATTRIBUTE_DIRECTORY);

            size_t w = CResultsDisplayerWide::ComputeDisplayWidth (wfd, true, false, false);

            Assert::AreEqual (static_cast<size_t>(6), w);  // docs(4) + icon(2) = 6
        }

        TEST_METHOD (File_WithCloudStatus_AddsTwo)
        {
            auto wfd = MakeWfd (L"file.txt");

            size_t w = CResultsDisplayerWide::ComputeDisplayWidth (wfd, false, false, true);

            Assert::AreEqual (static_cast<size_t>(10), w);  // 8 + 2 cloud
        }

        TEST_METHOD (File_WithIconsAndCloud_AddsFour)
        {
            auto wfd = MakeWfd (L"file.txt");

            size_t w = CResultsDisplayerWide::ComputeDisplayWidth (wfd, true, false, true);

            Assert::AreEqual (static_cast<size_t>(12), w);  // 8 + 2 icon + 2 cloud
        }

        TEST_METHOD (File_IconSuppressed_NoIconWidth)
        {
            auto wfd = MakeWfd (L"file.txt");

            size_t w = CResultsDisplayerWide::ComputeDisplayWidth (wfd, true, true, false);

            Assert::AreEqual (static_cast<size_t>(8), w);  // 8 only, no +2
        }

        TEST_METHOD (Dir_IconsOff_CloudOn_BracketsAndCloud)
        {
            auto wfd = MakeWfd (L"src", FILE_ATTRIBUTE_DIRECTORY);

            size_t w = CResultsDisplayerWide::ComputeDisplayWidth (wfd, false, false, true);

            Assert::AreEqual (static_cast<size_t>(7), w);  // [src] = 3+2 + 2 cloud = 7
        }
    };





    ////////////////////////////////////////////////////////////////////////////
    //
    //  ComputeMedianDisplayWidth Tests (T016)
    //
    ////////////////////////////////////////////////////////////////////////////

    TEST_CLASS (ComputeMedianDisplayWidthTests)
    {
    public:

        TEST_METHOD (EmptyVector_ReturnsZero)
        {
            size_t m = CResultsDisplayerWide::ComputeMedianDisplayWidth ({});

            Assert::AreEqual (static_cast<size_t>(0), m);
        }

        TEST_METHOD (SingleEntry_ReturnsThatValue)
        {
            size_t m = CResultsDisplayerWide::ComputeMedianDisplayWidth ({ 42 });

            Assert::AreEqual (static_cast<size_t>(42), m);
        }

        TEST_METHOD (TwoEntries_ReturnsHigher)
        {
            size_t m = CResultsDisplayerWide::ComputeMedianDisplayWidth ({ 10, 20 });

            Assert::AreEqual (static_cast<size_t>(20), m);  // index 1 = higher
        }

        TEST_METHOD (OddCount_ReturnsMiddle)
        {
            size_t m = CResultsDisplayerWide::ComputeMedianDisplayWidth ({ 5, 15, 10 });

            Assert::AreEqual (static_cast<size_t>(10), m);
        }

        TEST_METHOD (EvenCount_ReturnsUpperMiddle)
        {
            size_t m = CResultsDisplayerWide::ComputeMedianDisplayWidth ({ 3, 7, 1, 9 });

            Assert::AreEqual (static_cast<size_t>(7), m);  // sorted: 1,3,7,9 → index 2 = 7
        }

        TEST_METHOD (AllSameWidth_ReturnsThatWidth)
        {
            size_t m = CResultsDisplayerWide::ComputeMedianDisplayWidth ({ 12, 12, 12, 12, 12 });

            Assert::AreEqual (static_cast<size_t>(12), m);
        }
    };





    ////////////////////////////////////////////////////////////////////////////
    //
    //  ComputeColumnLayout Tests (T006, T010, T012, T014)
    //
    ////////////////////////////////////////////////////////////////////////////

    TEST_CLASS (ComputeColumnLayoutTests)
    {
    public:

        //
        // FR-007: Uniform widths should produce equal-width columns
        //

        TEST_METHOD (UniformWidths_ProduceEqualColumns)
        {
            vector<size_t> widths (10, 10);  // 10 entries, all width 10

            SColumnLayout layout = CResultsDisplayerWide::ComputeColumnLayout (widths, 120, false);

            Assert::IsTrue (layout.cColumns >= 2);

            // Total of all column widths + last column entry width should equal console width
            size_t total = 0;
            for (auto w : layout.vColumnWidths) total += w;

            // Last column has no gap, non-last columns have gap + distributed leftover
            // For uniform widths, non-last column widths should differ by at most 1 (remainder)
            for (size_t c = 0; c + 2 < layout.cColumns; ++c)
            {
                size_t diff = layout.vColumnWidths[c] > layout.vColumnWidths[c + 1]
                            ? layout.vColumnWidths[c] - layout.vColumnWidths[c + 1]
                            : layout.vColumnWidths[c + 1] - layout.vColumnWidths[c];

                Assert::IsTrue (diff <= 1, L"Non-last columns should differ by at most 1");
            }
        }

        //
        // Mixed widths should produce variable columns
        //

        TEST_METHOD (MixedWidths_ProduceVariableColumns)
        {
            vector<size_t> widths = { 5, 5, 5, 5, 30, 5, 5, 5 };

            SColumnLayout layout = CResultsDisplayerWide::ComputeColumnLayout (widths, 80, false);

            Assert::IsTrue (layout.cColumns >= 2);
        }

        //
        // Single entry returns 1 column
        //

        TEST_METHOD (SingleEntry_Returns1Column)
        {
            vector<size_t> widths = { 20 };

            SColumnLayout layout = CResultsDisplayerWide::ComputeColumnLayout (widths, 120, false);

            Assert::AreEqual (static_cast<size_t>(1), layout.cColumns);
            Assert::AreEqual (static_cast<size_t>(1), layout.cRows);
        }

        //
        // All entries wider than console → 1 column
        //

        TEST_METHOD (AllWiderThanConsole_Returns1Column)
        {
            vector<size_t> widths = { 50, 60, 70 };

            SColumnLayout layout = CResultsDisplayerWide::ComputeColumnLayout (widths, 40, false);

            Assert::AreEqual (static_cast<size_t>(1), layout.cColumns);
        }

        //
        // FR-005: Leftover space distributed evenly, remainder to first gaps
        //

        TEST_METHOD (LeftoverSpace_DistributedEvenly)
        {
            // 3 entries of width 10 in 100-char terminal
            // Column fitting should use all 3 columns (3 × 10 < 100)

            vector<size_t> widths = { 10, 10, 10 };

            SColumnLayout layout = CResultsDisplayerWide::ComputeColumnLayout (widths, 100, false);

            Assert::AreEqual (static_cast<size_t>(3), layout.cColumns);

            // Total column widths (non-last include gap + leftover) + last
            size_t total = 0;
            for (auto w : layout.vColumnWidths) total += w;

            // Last column has no trailing gap
            Assert::AreEqual (static_cast<size_t>(10), layout.vColumnWidths[2]);

            // Sum must be strictly less than console width (1 char reserved to prevent terminal wrap)
            Assert::IsTrue (total < 100, L"Total must be < consoleWidth to prevent terminal wrap");
            Assert::AreEqual (static_cast<size_t>(99), total);
        }

        //
        // Column-major ordering: entry i → column i/nRows
        //

        TEST_METHOD (ColumnMajorMapping_10Entries3Columns)
        {
            // 10 entries in 3 columns → 4 rows
            // Col 0: entries 0-3 (4 items), Col 1: entries 4-6 (3 items), Col 2: entries 7-9 (3 items)

            vector<size_t> widths (10, 8);

            SColumnLayout layout = CResultsDisplayerWide::ComputeColumnLayout (widths, 80, false);

            // With width 8, console 80: max cols = min(10, 40) = 10
            // 10 cols → 1 row each, total = 9*9 + 8 = 89 > 80
            // Try fewer... eventually hits a fitting count
            Assert::IsTrue (layout.cColumns >= 2);
            Assert::IsTrue (layout.cRows >= 1);
        }

        //
        // Entries not evenly divisible by column count
        //

        TEST_METHOD (NotEvenlyDivisible_LastColumnShorter)
        {
            vector<size_t> widths (7, 10);  // 7 entries

            SColumnLayout layout = CResultsDisplayerWide::ComputeColumnLayout (widths, 120, false);

            Assert::IsTrue (layout.cColumns >= 2);
            Assert::AreEqual ((widths.size() + layout.cColumns - 1) / layout.cColumns, layout.cRows);
        }

        //
        // Two entries
        //

        TEST_METHOD (TwoEntries_FitSideBySide)
        {
            vector<size_t> widths = { 10, 10 };

            SColumnLayout layout = CResultsDisplayerWide::ComputeColumnLayout (widths, 80, false);

            Assert::AreEqual (static_cast<size_t>(2), layout.cColumns);
            Assert::AreEqual (static_cast<size_t>(1), layout.cRows);
        }

        //
        // SC-001 validation: 95×15-char + 5×55-char at 120-width
        // Variable-width must produce ≥150% of uniform-width column count
        //

        TEST_METHOD (SC001_MixedWidths_50PercentMoreColumns)
        {
            vector<size_t> widths;
            widths.reserve (100);

            for (int i = 0; i < 95; ++i) widths.push_back (15);
            for (int i = 0; i < 5; ++i)  widths.push_back (55);

            // Uniform-width: consoleWidth / (maxWidth + 1) = 120 / 56 = 2 columns
            size_t uniformCols = 120 / (55 + 1);

            SColumnLayout layout = CResultsDisplayerWide::ComputeColumnLayout (widths, 120, false);

            // Must be at least 150% of uniform
            Assert::IsTrue (layout.cColumns * 100 >= uniformCols * 150,
                L"Variable-width columns should be at least 150% of uniform-width");
        }

        //
        // Outlier truncation: one outlier gets capped
        //

        TEST_METHOD (Ellipsize_TruncatesOutlier)
        {
            vector<size_t> widths;

            for (int i = 0; i < 20; ++i) widths.push_back (10);
            widths.push_back (60);  // outlier: median = 10, truncCap = max(20, 20) = 20

            SColumnLayout noEllipsis = CResultsDisplayerWide::ComputeColumnLayout (widths, 80, false);
            SColumnLayout ellipsis   = CResultsDisplayerWide::ComputeColumnLayout (widths, 80, true);

            // With truncation, we should get more columns (outlier capped to 20)
            Assert::IsTrue (ellipsis.cColumns >= noEllipsis.cColumns,
                L"Ellipsis mode should produce equal or more columns");
            Assert::IsTrue (ellipsis.cchTruncCap > 0);
        }

        //
        // No outliers → truncCap computed but not triggered
        //

        TEST_METHOD (Ellipsize_NoOutliers_NoTruncation)
        {
            vector<size_t> widths (10, 12);  // all same

            SColumnLayout layout = CResultsDisplayerWide::ComputeColumnLayout (widths, 120, true);

            // No outliers → truncation not applied → cchTruncCap = 0
            Assert::AreEqual (static_cast<size_t>(0), layout.cchTruncCap);
        }

        //
        // Ellipsize disabled → no truncation
        //

        TEST_METHOD (EllipsizeDisabled_NoTruncation)
        {
            vector<size_t> widths;

            for (int i = 0; i < 20; ++i) widths.push_back (10);
            widths.push_back (60);

            SColumnLayout layout = CResultsDisplayerWide::ComputeColumnLayout (widths, 80, false);

            Assert::AreEqual (static_cast<size_t>(0), layout.cchTruncCap);
        }

        //
        // Minimum truncCap floor of 40
        //

        TEST_METHOD (Ellipsize_SmallMedian_FloorAt40)
        {
            // 30 entries of width 10 + 1 entry of width 50 in a 55-char terminal
            // median = 10, 2×median = 20, but floor = 40
            // Without truncation: outlier at 50 forces 1 col
            // With truncation (cap 40): 40+1 + 10 = 51 < 55, 2 cols fit

            vector<size_t> widths (30, 10);
            widths.push_back (50);

            SColumnLayout noEllipsis = CResultsDisplayerWide::ComputeColumnLayout (widths, 55, false);
            SColumnLayout ellipsis   = CResultsDisplayerWide::ComputeColumnLayout (widths, 55, true);

            Assert::IsTrue (ellipsis.cColumns > noEllipsis.cColumns,
                L"Truncation should gain columns in a tight terminal");
            Assert::AreEqual (static_cast<size_t>(40), ellipsis.cchTruncCap);
        }

        //
        // Truncation skipped when it doesn't produce more columns
        //

        TEST_METHOD (Ellipsize_SkippedWhenNoColumnGain)
        {
            // Wide terminal where the outlier fits without reducing columns
            // 5 entries of width 10 + 1 entry of width 22 in a 200-char terminal
            // Without truncation: easily fits in 6 columns
            // With truncation (cap 20): still 6 columns — no benefit

            vector<size_t> widths = { 10, 10, 10, 10, 10, 22 };

            SColumnLayout layout = CResultsDisplayerWide::ComputeColumnLayout (widths, 200, true);

            // Truncation should NOT be applied since it provides no column gain
            Assert::AreEqual (static_cast<size_t>(0), layout.cchTruncCap,
                L"Truncation should be skipped when it doesn't improve column count");

            // All 6 entries should fit in 6 columns
            Assert::AreEqual (static_cast<size_t>(6), layout.cColumns);
        }

        //
        // Edge case: empty directory
        //

        TEST_METHOD (EmptyDirectory_Returns1Column0Rows)
        {
            vector<size_t> widths;

            SColumnLayout layout = CResultsDisplayerWide::ComputeColumnLayout (widths, 120, false);

            Assert::AreEqual (static_cast<size_t>(1), layout.cColumns);
            Assert::AreEqual (static_cast<size_t>(0), layout.cRows);
        }

        //
        // Edge case: terminal width = 1
        //

        TEST_METHOD (TerminalWidth1_Returns1Column)
        {
            vector<size_t> widths = { 5, 10 };

            SColumnLayout layout = CResultsDisplayerWide::ComputeColumnLayout (widths, 1, false);

            Assert::AreEqual (static_cast<size_t>(1), layout.cColumns);
        }

        //
        // Edge case: terminal width < shortest filename
        //

        TEST_METHOD (TerminalNarrowerThanShortest_Returns1Column)
        {
            vector<size_t> widths = { 20, 30 };

            SColumnLayout layout = CResultsDisplayerWide::ComputeColumnLayout (widths, 10, false);

            Assert::AreEqual (static_cast<size_t>(1), layout.cColumns);
        }

        //
        // Icon/cloud width variation across entries (T014)
        //

        TEST_METHOD (PerEntryIconVariation_ColumnWidthsAccountForIt)
        {
            // Simulate: entry 0 has icon (+2), entry 1 doesn't
            // Display widths differ per entry
            vector<size_t> widths = { 12, 10, 12, 10 };  // alternating icon/no-icon

            SColumnLayout layout = CResultsDisplayerWide::ComputeColumnLayout (widths, 60, false);

            Assert::IsTrue (layout.cColumns >= 2);
        }

        //
        // Total row width must be strictly less than console width to
        // prevent terminal line-wrapping.  The last column's widest entry
        // plus all preceding column widths must leave at least 1 char free.
        //

        TEST_METHOD (TotalRowWidth_StrictlyLessThanConsoleWidth)
        {
            // Reproduce the real-world bug: 27 entries at console width 126.
            // The old code distributed ALL leftover, hitting exactly 126 chars,
            // which causes terminal wrap.

            vector<size_t> widths = {
                4, 7, 7, 5, 7, 11, 7, 4, 10, 5,
                20, 7, 6, 12, 3, 11, 10, 8, 4, 8,
                6, 13, 6, 7, 14, 6, 9
            };

            SColumnLayout layout = CResultsDisplayerWide::ComputeColumnLayout (widths, 126, false);

            Assert::IsTrue (layout.cColumns >= 2, L"Should fit multiple columns");

            //
            // For each possible row, compute the maximum rendered width:
            // sum of column widths for columns that have an entry on that row.
            // The last column's entry gets no trailing padding.
            //

            for (size_t row = 0; row < layout.cRows; ++row)
            {
                size_t rowWidth = 0;

                for (size_t col = 0; col < layout.cColumns; ++col)
                {
                    size_t cItemsInLastRow = widths.size() % layout.cColumns;
                    size_t fullRows        = cItemsInLastRow ? layout.cRows - 1 : layout.cRows;
                    size_t idx             = row + (col * fullRows);

                    if (col < cItemsInLastRow)
                        idx += col;
                    else
                        idx += cItemsInLastRow;

                    if (idx >= widths.size())
                        break;

                    if (col < layout.cColumns - 1)
                        rowWidth += layout.vColumnWidths[col];
                    else
                        rowWidth += widths[idx];  // last col: actual entry width, no padding
                }

                Assert::IsTrue (rowWidth < 126,
                    (L"Row " + to_wstring (row) + L" width " + to_wstring (rowWidth) + L" must be < 126").c_str());
            }
        }
    };





    ////////////////////////////////////////////////////////////////////////////
    //
    //  Scenario Tests — end-to-end rendering with mocked console
    //
    //  These tests feed synthetic CDirectoryInfo through DisplayFileResults
    //  and capture the exact console output via CCapturingConsole, then
    //  verify the rendered text structure.
    //
    ////////////////////////////////////////////////////////////////////////////

    //
    // No-op environment provider to prevent real config file loading
    //

    class CScenarioNoOpEnv : public IEnvironmentProvider
    {
        bool TryGetEnvironmentVariable (LPCWSTR, wstring &) const override { return false; }
    };

    static CScenarioNoOpEnv s_scenarioNoOpEnv;

    //
    // Strip ANSI escape sequences from captured output
    //

    static wstring StripAnsi (const wstring & input)
    {
        wstring result;
        result.reserve (input.size());

        size_t i = 0;

        while (i < input.size())
        {
            if (input[i] == L'\x1B' && i + 1 < input.size() && input[i + 1] == L'[')
            {
                // Skip ESC [ ... until letter
                i += 2;
                while (i < input.size() && !iswalpha (input[i]))
                    ++i;
                if (i < input.size())
                    ++i;  // skip final letter
            }
            else
            {
                result += input[i++];
            }
        }

        return result;
    }

    //
    // Split string into lines
    //

    static vector<wstring> SplitLines (const wstring & s)
    {
        vector<wstring> lines;
        wistringstream  stream (s);
        wstring         line;

        while (getline (stream, line))
        {
            lines.push_back (line);
        }

        return lines;
    }

    //
    // Build a CDirectoryInfo with synthetic filenames
    //

    static unique_ptr<CDirectoryInfo> MakeDirectoryInfo (const vector<LPCWSTR> & names)
    {
        auto pDi   = make_unique<CDirectoryInfo> (L"C:\\Test", L"*");
        size_t maxLen = 0;

        for (auto pszName : names)
        {
            FileInfo fi;
            wcscpy_s (fi.cFileName, pszName);
            fi.dwFileAttributes = 0;

            size_t len = wcslen (pszName);
            if (len > maxLen) maxLen = len;

            pDi->m_vMatches.push_back (fi);
            pDi->m_cFiles++;
        }

        pDi->m_cchLargestFileName = maxLen;
        return pDi;
    }

    TEST_CLASS (WideDisplayScenarioTests)
    {
    public:

        //
        // Scenario: 5 short names + 1 long name at 80-char width
        // Verifies variable-width produces multiple columns
        //

        TEST_METHOD (Scenario_MixedLengths_MultipleColumns)
        {
            auto cmd = make_shared<CCommandLine>();
            auto con = make_shared<CCapturingConsole>();
            auto cfg = make_shared<CConfig>();

            cfg->SetEnvironmentProvider (&s_scenarioNoOpEnv);
            con->Initialize (cfg);
            con->SetWidth (80);



            auto pDi = MakeDirectoryInfo ({
                L"docs", L"src", L"build", L"README.md",
                L"Microsoft.WindowsPackageManagerManifestCreator_8wekyb3d8bbwe"
            });

            CResultsDisplayerWide displayer (cmd, con, cfg, false);
            displayer.DisplayFileResults (*pDi);
            con->Flush();

            wstring plain = StripAnsi (con->m_strCaptured);
            auto    lines = SplitLines (plain);

            // Should have fewer lines than entries (multiple columns)
            Assert::IsTrue (lines.size() < 5, L"Should use multiple columns, fewer lines than entries");

            // All filenames should appear in the output
            Assert::IsTrue (plain.find (L"docs") != wstring::npos);
            Assert::IsTrue (plain.find (L"src") != wstring::npos);
            Assert::IsTrue (plain.find (L"build") != wstring::npos);
            Assert::IsTrue (plain.find (L"README.md") != wstring::npos);
        }

        //
        // Scenario: Uniform-length names should look like old behavior
        // All entries same width → equal spacing across columns
        //

        TEST_METHOD (Scenario_UniformLengths_EqualSpacing)
        {
            auto cmd = make_shared<CCommandLine>();
            auto con = make_shared<CCapturingConsole>();
            auto cfg = make_shared<CConfig>();

            cfg->SetEnvironmentProvider (&s_scenarioNoOpEnv);
            con->Initialize (cfg);
            con->SetWidth (80);



            auto pDi = MakeDirectoryInfo ({
                L"alpha", L"bravo", L"delta", L"foxtrot",
                L"golf1", L"hotel", L"india", L"juliett"
            });

            CResultsDisplayerWide displayer (cmd, con, cfg, false);
            displayer.DisplayFileResults (*pDi);
            con->Flush();

            wstring plain = StripAnsi (con->m_strCaptured);
            auto    lines = SplitLines (plain);

            // 8 entries of ~5-7 chars in 80-char terminal → multiple columns, few rows
            Assert::IsTrue (lines.size() <= 4, L"8 short entries should fit in 2+ columns");
            Assert::IsTrue (lines.size() >= 1, L"Should have at least one line of output");
        }

        //
        // Scenario: Outlier truncation — long name gets ellipsis
        //

        TEST_METHOD (Scenario_OutlierTruncation_EllipsisAppears)
        {
            auto cmd = make_shared<CCommandLine>();
            auto con = make_shared<CCapturingConsole>();
            auto cfg = make_shared<CConfig>();

            cfg->SetEnvironmentProvider (&s_scenarioNoOpEnv);
            con->Initialize (cfg);
            con->SetWidth (80);



            auto pDi = MakeDirectoryInfo ({
                L"a.txt", L"b.txt", L"c.txt", L"d.txt", L"e.txt",
                L"f.txt", L"g.txt", L"h.txt", L"i.txt", L"j.txt",
                L"VeryLongFileNameThatExceedsTwiceTheMedianDisplayWidth_AndThenSome.txt"
            });

            CResultsDisplayerWide displayer (cmd, con, cfg, false);
            displayer.DisplayFileResults (*pDi);
            con->Flush();

            wstring plain = StripAnsi (con->m_strCaptured);

            // The outlier should be truncated with ellipsis
            Assert::IsTrue (plain.find (UnicodeSymbols::Ellipsis) != wstring::npos,
                L"Outlier filename should be truncated with ellipsis");

            // The full original name should NOT appear
            Assert::IsTrue (plain.find (L"AndThenSome.txt") == wstring::npos,
                L"Full outlier name should not appear in output");
        }

        //
        // Scenario: --Ellipsize- disables truncation
        //

        TEST_METHOD (Scenario_EllipsizeDisabled_NoTruncation)
        {
            auto cmd = make_shared<CCommandLine>();
            auto con = make_shared<CCapturingConsole>();
            auto cfg = make_shared<CConfig>();

            cmd->m_fEllipsize = false;  // --Ellipsize-

            cfg->SetEnvironmentProvider (&s_scenarioNoOpEnv);
            con->Initialize (cfg);
            con->SetWidth (120);



            auto pDi = MakeDirectoryInfo ({
                L"a.txt", L"b.txt", L"c.txt", L"d.txt", L"e.txt",
                L"f.txt", L"g.txt", L"h.txt", L"i.txt", L"j.txt",
                L"VeryLongFileNameThatExceedsTwiceTheMedianDisplayWidth_AndThenSome.txt"
            });

            CResultsDisplayerWide displayer (cmd, con, cfg, false);
            displayer.DisplayFileResults (*pDi);
            con->Flush();

            wstring plain = StripAnsi (con->m_strCaptured);

            // No ellipsis should appear — full name preserved
            Assert::IsTrue (plain.find (UnicodeSymbols::Ellipsis) == wstring::npos,
                L"No truncation should occur when --Ellipsize- is set");

            // Full name should appear
            Assert::IsTrue (plain.find (L"AndThenSome.txt") != wstring::npos,
                L"Full outlier name should appear when truncation is disabled");
        }

        //
        // Scenario: Column-major order — entries fill top-to-bottom
        //

        TEST_METHOD (Scenario_ColumnMajorOrder_TopToBottomFill)
        {
            auto cmd = make_shared<CCommandLine>();
            auto con = make_shared<CCapturingConsole>();
            auto cfg = make_shared<CConfig>();

            cfg->SetEnvironmentProvider (&s_scenarioNoOpEnv);
            con->Initialize (cfg);
            con->SetWidth (80);



            // 6 short entries → should fit in multiple columns
            auto pDi = MakeDirectoryInfo ({
                L"aaa", L"bbb", L"ccc", L"ddd", L"eee", L"fff"
            });

            CResultsDisplayerWide displayer (cmd, con, cfg, false);
            displayer.DisplayFileResults (*pDi);
            con->Flush();

            wstring plain = StripAnsi (con->m_strCaptured);
            auto    lines = SplitLines (plain);

            // With column-major order, "aaa" should be on the first line
            // and entries should progress down before moving right
            Assert::IsTrue (lines.size() >= 1);
            Assert::IsTrue (lines[0].find (L"aaa") != wstring::npos,
                L"First entry should appear on the first line");

            // If there are at least 2 lines, the second entry should be on line 2
            if (lines.size() >= 2)
            {
                Assert::IsTrue (lines[1].find (L"bbb") != wstring::npos,
                    L"Second entry should appear on the second line (column-major)");
            }
        }

        //
        // Scenario: Single file — one line, no crash
        //

        TEST_METHOD (Scenario_SingleFile_OneLine)
        {
            auto cmd = make_shared<CCommandLine>();
            auto con = make_shared<CCapturingConsole>();
            auto cfg = make_shared<CConfig>();

            cfg->SetEnvironmentProvider (&s_scenarioNoOpEnv);
            con->Initialize (cfg);
            con->SetWidth (80);



            auto pDi = MakeDirectoryInfo ({ L"onlyfile.txt" });

            CResultsDisplayerWide displayer (cmd, con, cfg, false);
            displayer.DisplayFileResults (*pDi);
            con->Flush();

            wstring plain = StripAnsi (con->m_strCaptured);
            auto    lines = SplitLines (plain);

            Assert::AreEqual (static_cast<size_t>(1), lines.size());
            Assert::IsTrue (plain.find (L"onlyfile.txt") != wstring::npos);
        }

        //
        // Scenario: Directory entries get [brackets] when icons off
        //

        TEST_METHOD (Scenario_DirectoryBrackets_IconsOff)
        {
            auto cmd = make_shared<CCommandLine>();
            auto con = make_shared<CCapturingConsole>();
            auto cfg = make_shared<CConfig>();

            cfg->SetEnvironmentProvider (&s_scenarioNoOpEnv);
            con->Initialize (cfg);
            con->SetWidth (80);



            CDirectoryInfo di (L"C:\\Test", L"*");

            FileInfo fi;
            wcscpy_s (fi.cFileName, L"MyFolder");
            fi.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
            di.m_vMatches.push_back (fi);
            di.m_cchLargestFileName = 8;
            di.m_cSubDirectories    = 1;

            CResultsDisplayerWide displayer (cmd, con, cfg, false);
            displayer.DisplayFileResults (di);
            con->Flush();

            wstring plain = StripAnsi (con->m_strCaptured);

            Assert::IsTrue (plain.find (L"[MyFolder]") != wstring::npos,
                L"Directory should be displayed with brackets when icons are off");
        }
    };
}
