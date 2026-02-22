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



            // Root ancestor at i=0 is skipped (root entries have no connector).
            // Just the connector for this entry: ├── 
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



            // Root ancestor skipped.  Just the corner connector: └── 
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

            state.Push (true);   // depth 1: root ancestor (skipped in rendering)
            state.Push (true);   // depth 2: current level has more siblings (draw │)

            wstring prefix = state.GetPrefix (false);



            // Skip i=0 (root).  Loop i=1 (true): │ + 3 spaces.  Connector: ├── 
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

            state.Push (false);  // depth 1: root ancestor (skipped)
            state.Push (true);   // depth 2: current level has siblings (draw │)

            wstring prefix = state.GetPrefix (true);



            // Skip i=0 (root).  Loop i=1 (true): │ + 3 spaces.  Connector: └── 
            wstring expected;
            expected += UnicodeSymbols::TreeVertical;
            expected += L"   ";
            expected += UnicodeSymbols::TreeCorner;
            expected += UnicodeSymbols::TreeHorizontal;
            expected += UnicodeSymbols::TreeHorizontal;
            expected += L' ';

            Assert::AreEqual (expected, prefix);
        }





        TEST_METHOD(PrefixAtDepth3_MixedAncestors)
        {
            STreeConnectorState state;

            state.Push (true);   // depth 1: root ancestor (skipped)
            state.Push (false);  // depth 2: no sibling (space)
            state.Push (true);   // depth 3: current level has siblings (│)

            wstring prefix = state.GetPrefix (false);



            // Skip i=0 (root).  Loop i=1 (false): 4 spaces.  i=2 (true): │ + 3 spaces.  Connector: ├── 
            wstring expected;
            expected += L"    ";
            expected += UnicodeSymbols::TreeVertical;
            expected += L"   ";
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



            // Root ancestor skipped.  Just stream vertical: │   
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



            // Skip i=0 (root).  Loop i=1 (true): │ + 3 spaces.  Stream vertical: │ + 3 spaces.
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



            // indent=1, root ancestor skipped.
            // Just the connector: ├ + 0 horizontal + space = "├ "
            wstring expected;
            expected += UnicodeSymbols::TreeTee;
            expected += L' ';

            Assert::AreEqual (expected, prefix);
        }





        TEST_METHOD(CustomIndent2_NarrowPrefix)
        {
            STreeConnectorState state (2);

            state.Push (true);   // depth 1: root ancestor (skipped)
            state.Push (true);   // depth 2: has sibling (│)

            wstring prefix = state.GetPrefix (true);



            // Skip i=0 (root).  Loop i=1 (true): │ + 1 space.
            // Connector: └ + 0 dashes + space
            // Total: "│ └ "
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



            // indent=8, root ancestor skipped.
            // Just the connector: tee + 6 horizontal dashes + space (8 chars)
            wstring expected;
            expected += UnicodeSymbols::TreeTee;
            expected += wstring (6, UnicodeSymbols::TreeHorizontal);
            expected += L' ';

            Assert::AreEqual (expected, prefix);
            Assert::AreEqual (size_t (8), prefix.size());
        }
    };
}
