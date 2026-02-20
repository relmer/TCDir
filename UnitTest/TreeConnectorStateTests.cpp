#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/TreeConnectorState.h"
#include "../TCDirCore/UnicodeSymbols.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(TreeConnectorStateTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }





        TEST_METHOD(DefaultConstructor_Depth0)
        {
            STreeConnectorState state;



            Assert::AreEqual (size_t (0), state.Depth());
            Assert::AreEqual (4, state.m_cTreeIndent);
        }





        TEST_METHOD(CustomIndent_StoredCorrectly)
        {
            STreeConnectorState state (2);



            Assert::AreEqual (2, state.m_cTreeIndent);
            Assert::AreEqual (size_t (0), state.Depth());
        }





        TEST_METHOD(PrefixAtDepth0_EmptyString)
        {
            STreeConnectorState state;

            wstring prefix = state.GetPrefix (false);



            Assert::IsTrue (prefix.empty(), L"Prefix at depth 0 should be empty");
        }





        TEST_METHOD(PrefixAtDepth0_LastEntry_EmptyString)
        {
            STreeConnectorState state;

            wstring prefix = state.GetPrefix (true);



            Assert::IsTrue (prefix.empty(), L"Prefix at depth 0 should be empty even for last entry");
        }





        TEST_METHOD(PrefixAtDepth1_MiddleEntry)
        {
            STreeConnectorState state;

            state.Push (true);  // parent has more siblings

            wstring prefix = state.GetPrefix (false);



            // Should be: ├── (tee + 2 horizontal dashes + space) for indent=4
            wstring expected;
            expected += UnicodeSymbols::TreeTee;
            expected += UnicodeSymbols::TreeHorizontal;
            expected += UnicodeSymbols::TreeHorizontal;
            expected += L' ';

            Assert::AreEqual (expected, prefix);
        }





        TEST_METHOD(PrefixAtDepth1_LastEntry)
        {
            STreeConnectorState state;

            state.Push (true);

            wstring prefix = state.GetPrefix (true);



            // Should be: └── (corner + 2 horizontal dashes + space)
            wstring expected;
            expected += UnicodeSymbols::TreeCorner;
            expected += UnicodeSymbols::TreeHorizontal;
            expected += UnicodeSymbols::TreeHorizontal;
            expected += L' ';

            Assert::AreEqual (expected, prefix);
        }





        TEST_METHOD(PrefixAtDepth2_MiddleEntry_AncestorHasSibling)
        {
            STreeConnectorState state;

            state.Push (true);   // depth 1: ancestor has more siblings (draw │)
            state.Push (true);   // depth 2: current level

            wstring prefix = state.GetPrefix (false);



            // Should be: │   ├── 
            // (vertical + 3 spaces) + (tee + 2 horizontal + space)
            wstring expected;
            expected += UnicodeSymbols::TreeVertical;
            expected += L"   ";
            expected += UnicodeSymbols::TreeTee;
            expected += UnicodeSymbols::TreeHorizontal;
            expected += UnicodeSymbols::TreeHorizontal;
            expected += L' ';

            Assert::AreEqual (expected, prefix);
        }





        TEST_METHOD(PrefixAtDepth2_LastEntry_AncestorHasNoSibling)
        {
            STreeConnectorState state;

            state.Push (false);  // depth 1: ancestor was last (draw spaces)
            state.Push (true);   // depth 2: current level

            wstring prefix = state.GetPrefix (true);



            // Should be: "    └── "
            // (4 spaces) + (corner + 2 horizontal + space)
            wstring expected;
            expected += L"    ";
            expected += UnicodeSymbols::TreeCorner;
            expected += UnicodeSymbols::TreeHorizontal;
            expected += UnicodeSymbols::TreeHorizontal;
            expected += L' ';

            Assert::AreEqual (expected, prefix);
        }





        TEST_METHOD(PrefixAtDepth3_MixedAncestors)
        {
            STreeConnectorState state;

            state.Push (true);   // depth 1: has sibling (│)
            state.Push (false);  // depth 2: no sibling (space)
            state.Push (true);   // depth 3: current level

            wstring prefix = state.GetPrefix (false);



            // Should be: "│   " + "    " + "├── "
            wstring expected;
            expected += UnicodeSymbols::TreeVertical;
            expected += L"   ";
            expected += L"    ";
            expected += UnicodeSymbols::TreeTee;
            expected += UnicodeSymbols::TreeHorizontal;
            expected += UnicodeSymbols::TreeHorizontal;
            expected += L' ';

            Assert::AreEqual (expected, prefix);
        }





        TEST_METHOD(PushPop_DepthTracking)
        {
            STreeConnectorState state;



            Assert::AreEqual (size_t (0), state.Depth());

            state.Push (true);
            Assert::AreEqual (size_t (1), state.Depth());

            state.Push (false);
            Assert::AreEqual (size_t (2), state.Depth());

            state.Pop();
            Assert::AreEqual (size_t (1), state.Depth());

            state.Pop();
            Assert::AreEqual (size_t (0), state.Depth());
        }





        TEST_METHOD(Pop_AtDepth0_NoOp)
        {
            STreeConnectorState state;

            state.Pop();  // Should not crash



            Assert::AreEqual (size_t (0), state.Depth());
        }





        TEST_METHOD(StreamContinuation_Depth0_EmptyString)
        {
            STreeConnectorState state;

            wstring cont = state.GetStreamContinuation();



            Assert::IsTrue (cont.empty());
        }





        TEST_METHOD(StreamContinuation_Depth1)
        {
            STreeConnectorState state;

            state.Push (true);

            wstring cont = state.GetStreamContinuation();



            // Should be: │ + 3 spaces (vertical + indent-1 spaces)
            wstring expected;
            expected += UnicodeSymbols::TreeVertical;
            expected += L"   ";

            Assert::AreEqual (expected, cont);
        }





        TEST_METHOD(StreamContinuation_Depth2_AncestorHasSibling)
        {
            STreeConnectorState state;

            state.Push (true);
            state.Push (true);

            wstring cont = state.GetStreamContinuation();



            // Should be: "│   " + "│   "
            wstring expected;
            expected += UnicodeSymbols::TreeVertical;
            expected += L"   ";
            expected += UnicodeSymbols::TreeVertical;
            expected += L"   ";

            Assert::AreEqual (expected, cont);
        }





        TEST_METHOD(CustomIndent1_ShortPrefix)
        {
            STreeConnectorState state (1);

            state.Push (true);

            wstring prefix = state.GetPrefix (false);



            // indent=1: connector is tee + 0 horizontal dashes + space = tee + space
            // But cHorizontalDashes = max(0, 1-2) = 0, so: tee + space
            // Wait, that's wrong. Let me re-check: cHorizontalDashes = max(0, 1-2) = 0
            // prefix = "├ " (tee + space)
            // Actually prefix gets 0 dashes + space = "├ "
            wstring expected;
            expected += UnicodeSymbols::TreeTee;
            expected += L' ';

            Assert::AreEqual (expected, prefix);
        }





        TEST_METHOD(CustomIndent2_NarrowPrefix)
        {
            STreeConnectorState state (2);

            state.Push (true);   // depth 1: has sibling
            state.Push (true);   // depth 2: current level

            wstring prefix = state.GetPrefix (true);



            // For indent=2, continuation is: │ + 1 space
            // Connector is: └ + 0 horizontal = "└ "
            // Wait: cHorizontalDashes = max(0, 2-2) = 0
            // Actually no: connector is corner + 0 dashes + space
            wstring expected;
            expected += UnicodeSymbols::TreeVertical;
            expected += L" ";
            expected += UnicodeSymbols::TreeCorner;
            expected += L' ';

            Assert::AreEqual (expected, prefix);
        }





        TEST_METHOD(CustomIndent8_WidePrefix)
        {
            STreeConnectorState state (8);

            state.Push (true);

            wstring prefix = state.GetPrefix (false);



            // indent=8: tee + 6 horizontal dashes + space
            wstring expected;
            expected += UnicodeSymbols::TreeTee;
            expected += wstring (6, UnicodeSymbols::TreeHorizontal);
            expected += L' ';

            Assert::AreEqual (expected, prefix);
            Assert::AreEqual (size_t (8), prefix.size());
        }
    };
}
