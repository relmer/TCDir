#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/Completer.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(CompleterTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        static vector<SCompletionCandidate> Complete (const wstring & strWord, vector<wstring> vOther = {})
        {
            SCompletionRequest             request;
            vector<SCompletionCandidate>   candidates;

            request.m_strWord     = strWord;
            request.m_vOtherWords = move (vOther);

            CCompleter::Complete (request, candidates);
            return candidates;
        }

        static vector<wstring> Texts (const vector<SCompletionCandidate> & candidates)
        {
            vector<wstring> texts;

            for (const SCompletionCandidate & candidate : candidates)
            {
                texts.push_back (candidate.m_strCompletion);
            }

            return texts;
        }

        static bool Contains (const vector<wstring> & texts, LPCWSTR pszText)
        {
            return find (texts.begin(), texts.end(), pszText) != texts.end();
        }




        TEST_METHOD(LongPrefix_MatchesByName)
        {
            vector<wstring> texts = Texts (Complete (L"--Tr"));

            Assert::AreEqual (size_t (2), texts.size());
            Assert::AreEqual (L"--Tree",       texts[0].c_str());
            Assert::AreEqual (L"--TreeIndent", texts[1].c_str());
        }




        TEST_METHOD(LongPrefix_IgnoresCase)
        {
            Assert::IsTrue (Contains (Texts (Complete (L"--str")), L"--Streams"));
        }




        TEST_METHOD(DoubleDash_ListsEveryLongSwitchInCatalogCasing)
        {
            vector<wstring> texts = Texts (Complete (L"--"));

            for (const SSwitchDefinition & definition : CSwitchCatalog::All())
            {
                if (definition.m_eForm == ESwitchForm::Long && CSwitchCatalog::IsAvailable (definition))
                {
                    Assert::IsTrue (Contains (texts, (wstring (L"--") + definition.m_pszName).c_str()), definition.m_pszName);
                }
            }

            Assert::IsFalse (Contains (texts, L"-W"));
        }




        TEST_METHOD(SingleDash_ShortSwitchesThenLong)
        {
            vector<wstring> texts = Texts (Complete (L"-"));

            Assert::AreEqual (L"-A", texts[0].c_str());
            Assert::IsTrue   (Contains (texts, L"-W"));
            Assert::IsTrue   (Contains (texts, L"--Tree"));
            Assert::IsTrue   (find (texts.begin(), texts.end(), L"-W") < find (texts.begin(), texts.end(), L"--Tree"));
        }




        TEST_METHOD(Slash_UsesSlashForBothForms)
        {
            vector<wstring> texts = Texts (Complete (L"/"));

            Assert::IsTrue  (Contains (texts, L"/W"));
            Assert::IsTrue  (Contains (texts, L"/Tree"));
            Assert::IsFalse (Contains (texts, L"--Tree"));
            Assert::IsTrue  (Contains (Texts (Complete (L"/Str")), L"/Streams"));
        }




        TEST_METHOD(Tooltip_PlainTextWithMatchingPrefix)
        {
            vector<SCompletionCandidate> slash = Complete (L"/Owne");
            vector<SCompletionCandidate> dash  = Complete (L"--Owne");

            Assert::AreEqual (size_t (1), slash.size());
            Assert::AreEqual (L"Displays the owner of each file and directory. Not allowed with /Tree.",  slash[0].m_strTooltip.c_str());
            Assert::AreEqual (L"Displays the owner of each file and directory. Not allowed with --Tree.", dash[0].m_strTooltip.c_str());
            Assert::AreEqual (L"/Owner", slash[0].m_strListText.c_str());
        }




        TEST_METHOD(ListText_IncludesValueSyntax)
        {
            vector<SCompletionCandidate> candidates = Complete (L"--Dep");

            Assert::AreEqual (L"--Depth=N", candidates[0].m_strListText.c_str());
            Assert::AreEqual (L"--Depth",   candidates[0].m_strCompletion.c_str());
        }




        TEST_METHOD(DebugSwitch_OnlyInDebugBuilds)
        {
#ifdef _DEBUG
            Assert::IsTrue  (Contains (Texts (Complete (L"--Deb")), L"--Debug"));
#else
            Assert::IsFalse (Contains (Texts (Complete (L"--Deb")), L"--Debug"));
#endif
        }




        TEST_METHOD(NonSwitchWords_NoCandidates)
        {
            for (LPCWSTR pszWord : { L"", L"C:\\Pro", L".\\-notes", L"foo", L"*.txt" })
            {
                Assert::IsTrue (Complete (pszWord).empty(), pszWord);
            }

            Assert::IsTrue (Complete (L"--Bogus").empty());
        }




        TEST_METHOD(FormatLine_ThreeTabSeparatedFields)
        {
            SCompletionCandidate candidate = { L"--Tree", L"--Tree", L"a\tb" };

            Assert::AreEqual (L"--Tree\t--Tree\ta b", CCompleter::FormatLine (candidate).c_str());
        }




        TEST_METHOD(IsCompleteRequest_FirstArgumentOnly)
        {
            wchar_t * rgFirst[]  = { const_cast<wchar_t *> (L"tcdir"), const_cast<wchar_t *> (L"--complete"), const_cast<wchar_t *> (L"w:--T") };
            wchar_t * rgSecond[] = { const_cast<wchar_t *> (L"tcdir"), const_cast<wchar_t *> (L"-w"),         const_cast<wchar_t *> (L"--Complete") };

            Assert::IsTrue  (CCompleter::IsCompleteRequest (3, rgFirst));
            Assert::IsFalse (CCompleter::IsCompleteRequest (3, rgSecond));
            Assert::IsFalse (CCompleter::IsCompleteRequest (1, rgFirst));
        }
    };
}
