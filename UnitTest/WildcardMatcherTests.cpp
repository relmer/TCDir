#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/WildcardMatcher.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(WildcardMatcherTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        TEST_METHOD(Matches_Star_MatchesAnyName)
        {
            Assert::IsTrue (CWildcardMatcher::Matches (L"readme.txt", L"*"));
            Assert::IsTrue (CWildcardMatcher::Matches (L"Makefile",   L"*"));
            Assert::IsTrue (CWildcardMatcher::Matches (L"",           L"*"));
        }




        TEST_METHOD(Matches_StarExtension_MatchesOnlyThatExtension)
        {
            Assert::IsTrue  (CWildcardMatcher::Matches (L"readme.txt", L"*.txt"));
            Assert::IsFalse (CWildcardMatcher::Matches (L"readme.log", L"*.txt"));
            Assert::IsFalse (CWildcardMatcher::Matches (L"readme",     L"*.txt"));
        }




        TEST_METHOD(Matches_Question_MatchesExactlyOneCharacter)
        {
            Assert::IsTrue  (CWildcardMatcher::Matches (L"a1.txt",  L"a?.txt"));
            Assert::IsFalse (CWildcardMatcher::Matches (L"a.txt",   L"a?.txt"));
            Assert::IsFalse (CWildcardMatcher::Matches (L"a12.txt", L"a?.txt"));
        }




        TEST_METHOD(Matches_StarDotStar_MatchesNamesWithoutDot)
        {
            Assert::IsTrue (CWildcardMatcher::Matches (L"Makefile",   L"*.*"));
            Assert::IsTrue (CWildcardMatcher::Matches (L"readme.txt", L"*.*"));
        }




        TEST_METHOD(Matches_IsCaseInsensitive)
        {
            Assert::IsTrue (CWildcardMatcher::Matches (L"README.TXT", L"*.txt"));
            Assert::IsTrue (CWildcardMatcher::Matches (L"readme.txt", L"READ*.TXT"));
            Assert::IsTrue (CWildcardMatcher::Matches (L"ReadMe.Txt", L"readme.txt"));
        }




        TEST_METHOD(Matches_TrailingDot_MatchesNameWithoutExtension)
        {
            Assert::IsTrue  (CWildcardMatcher::Matches (L"readme",     L"readme."));
            Assert::IsFalse (CWildcardMatcher::Matches (L"readme.txt", L"readme."));
        }




        TEST_METHOD(Matches_EmptySpec_MatchesNothing)
        {
            Assert::IsFalse (CWildcardMatcher::Matches (L"readme.txt", L""));
            Assert::IsFalse (CWildcardMatcher::Matches (L"",           L""));
        }




        TEST_METHOD(Matches_Literal_MatchesOnlyThatName)
        {
            Assert::IsTrue  (CWildcardMatcher::Matches (L"readme.txt",  L"readme.txt"));
            Assert::IsFalse (CWildcardMatcher::Matches (L"readme.txt2", L"readme.txt"));
            Assert::IsFalse (CWildcardMatcher::Matches (L"readme.tx",   L"readme.txt"));
        }




        TEST_METHOD(Matches_StarsInMiddle_Backtrack)
        {
            Assert::IsTrue  (CWildcardMatcher::Matches (L"abc",         L"a*b*c"));
            Assert::IsTrue  (CWildcardMatcher::Matches (L"axxbyybzzc",  L"a*b*c"));
            Assert::IsTrue  (CWildcardMatcher::Matches (L"abcbc",       L"a*b*c"));
            Assert::IsFalse (CWildcardMatcher::Matches (L"axxbyy",      L"a*b*c"));
            Assert::IsFalse (CWildcardMatcher::Matches (L"axxcyy",      L"a*b*c"));
        }
    };
}
