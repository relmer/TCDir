#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/CompletionScript.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(CompletionScriptTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        static wstring Joined (const vector<wstring> & lines)
        {
            wstring strText;

            for (const wstring & strLine : lines)
            {
                strText += strLine + L"\n";
            }

            return strText;
        }




        TEST_METHOD(Generate_HelperAndOneRegistrationPerCommand)
        {
            vector<wstring> lines;
            size_t          cRegistrations = 0;



            CCompletionScript::Generate (L"tcdir", { { L"tcdir", L"" }, { L"d", L"" }, { L"dw", L"-w" } }, lines);

            for (const wstring & strLine : lines)
            {
                cRegistrations += CCompletionScript::IsRegistration (strLine) ? 1 : 0;
            }

            Assert::AreEqual (size_t (3), cRegistrations);
            Assert::IsTrue   (lines[0].starts_with (L"function Invoke-TCDirCompletion"));
            Assert::IsTrue   (Joined (lines).find (L"-CommandName 'dw' -ScriptBlock { param ($w, $a, $p) Invoke-TCDirCompletion @('-w') $w $a $p }") != wstring::npos);
            Assert::IsTrue   (Joined (lines).find (L"-CommandName 'd' -ScriptBlock { param ($w, $a, $p) Invoke-TCDirCompletion @() $w $a $p }") != wstring::npos);
        }




        TEST_METHOD(Generate_InvocationQuotedWithWordPrefix)
        {
            vector<wstring> lines;



            CCompletionScript::Generate (L"C:\\Program Files\\it's\\tcdir.exe", { { L"tcdir", L"" } }, lines);

            Assert::IsTrue (Joined (lines).find (L"& 'C:\\Program Files\\it''s\\tcdir.exe' --Complete \"w:$word\" @others 2>$null") != wstring::npos);
        }




        TEST_METHOD(FlagArray_QuotesEachFlag)
        {
            Assert::AreEqual (L"@()",               CCompletionScript::FlagArray (L"").c_str());
            Assert::AreEqual (L"@('-w')",           CCompletionScript::FlagArray (L"-w").c_str());
            Assert::AreEqual (L"@('/a:d', '-s')",   CCompletionScript::FlagArray (L"/a:d  -s").c_str());
        }




        TEST_METHOD(IsRegistration_OnlyTcDirRegistrations)
        {
            Assert::IsTrue  (CCompletionScript::IsRegistration (L"Register-ArgumentCompleter -Native -CommandName 'd' -ScriptBlock { param ($w, $a, $p) Invoke-TCDirCompletion @() $w $a $p }"));
            Assert::IsFalse (CCompletionScript::IsRegistration (L"Register-ArgumentCompleter -Native -CommandName git -ScriptBlock { }"));
        }
    };
}
