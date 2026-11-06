#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/CommandLine.h"
#include "../TCDirCore/SwitchCatalog.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(SwitchCatalogTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        //
        //  Switches that are valid only with another switch, and the switch
        //  that makes them valid.
        //

        static vector<wstring> CompanionsOf (const SSwitchDefinition & definition)
        {
            static const map<wstring, vector<wstring>> s_kmapCompanions =
            {
                { L"Depth",      { L"--Tree" } },
                { L"TreeIndent", { L"--Tree" } },
                { L"WhatIf",     { L"--set-aliases" } },
            };

            auto it = s_kmapCompanions.find (definition.m_pszName);

            return (it != s_kmapCompanions.end()) ? it->second : vector<wstring>();
        }

        static wstring ArgumentFor (const SSwitchDefinition & definition)
        {
            wstring strArg = CSwitchCatalog::PrefixFor (definition, L"-", L"--") + definition.m_pszName;

            if (definition.m_chValueSeparator != 0)
            {
                strArg += definition.m_chValueSeparator;
                strArg += definition.m_values.empty() ? L"2" : definition.m_values[0].m_pszValue;
            }

            return strArg;
        }

        static HRESULT Parse (const vector<wstring> & args)
        {
            CCommandLine        cl;
            vector<wchar_t *>   argv;

            for (const wstring & strArg : args)
            {
                argv.push_back (const_cast<wchar_t *> (strArg.c_str()));
            }

            return cl.Parse (static_cast<int> (argv.size()), argv.data());
        }




        TEST_METHOD(EverySwitch_Parses)
        {
            for (const SSwitchDefinition & definition : CSwitchCatalog::All())
            {
                if (!CSwitchCatalog::IsAvailable (definition))
                {
                    continue;
                }

                vector<wstring> args = CompanionsOf (definition);

                args.push_back (ArgumentFor (definition));

                Assert::IsTrue (SUCCEEDED (Parse (args)), args.back().c_str());
            }
        }




        TEST_METHOD(EveryValue_Parses)
        {
            for (LPCWSTR pszName : { L"A", L"O", L"T" })
            {
                for (const SSwitchValue & value : CSwitchCatalog::ValuesOf (pszName))
                {
                    wstring strArg = wstring (L"-") + pszName + L":" + value.m_pszValue + (value.m_fPrefix ? L"S" : L"");

                    Assert::IsTrue (SUCCEEDED (Parse ({ strArg })), strArg.c_str());
                }
            }
        }




        TEST_METHOD(EveryLongSwitch_IsRecognized)
        {
            for (const SSwitchDefinition & definition : CSwitchCatalog::All())
            {
                if (definition.m_eForm == ESwitchForm::Long)
                {
                    wstring strName = definition.m_pszName;

                    Assert::IsTrue (CCommandLine::IsRecognizedLongSwitch (strName), strName.c_str());
                }
            }
        }




        TEST_METHOD(Find_IgnoresCaseAndForm)
        {
            Assert::IsNotNull (CSwitchCatalog::Find (ESwitchForm::Long,  L"tree"));
            Assert::IsNotNull (CSwitchCatalog::Find (ESwitchForm::Short, L"w"));
            Assert::IsNull    (CSwitchCatalog::Find (ESwitchForm::Short, L"Tree"));
            Assert::IsNull    (CSwitchCatalog::Find (ESwitchForm::Long,  L"bogus"));
        }




        TEST_METHOD(PlainText_RemovesColorMarkers)
        {
            Assert::AreEqual (L"Use --Tree- to disable.", CSwitchCatalog::PlainText (L"Use {InformationHighlight}--Tree-{Information} to disable.").c_str());
            Assert::AreEqual (L"a {not a marker} b",      CSwitchCatalog::PlainText (L"a {not a marker} b").c_str());
        }




        TEST_METHOD(FormatText_FillsPrefixes)
        {
            const SSwitchDefinition * pOwner = CSwitchCatalog::Find (ESwitchForm::Long, L"Owner");

            Assert::IsNotNull (pOwner);
            Assert::IsTrue    (CSwitchCatalog::PlainText (CSwitchCatalog::FormatText (pOwner->m_pszDescription, L"/", L"/")).find (L"/Tree") != wstring::npos);
            Assert::IsTrue    (CSwitchCatalog::PlainText (CSwitchCatalog::FormatText (pOwner->m_pszDescription, L"-", L"--")).find (L"--Tree") != wstring::npos);
        }
    };
}
