#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/CommandLine.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(CommandLineTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            // Setup EHM to use unit test framework's assertion mechanism
            SetupEhmForUnitTests();
        }





        TEST_METHOD(ParseOrderSwitches)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/on";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::AreEqual(static_cast<int>(CCommandLine::ESortOrder::SO_NAME),          static_cast<int>(cl.m_sortorder));
            Assert::AreEqual(static_cast<int>(CCommandLine::ESortDirection::SD_ASCENDING), static_cast<int>(cl.m_sortdirection));
        }





        TEST_METHOD(ParseOrderReverseAndSize)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/o-s";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::AreEqual(static_cast<int>(CCommandLine::ESortOrder::SO_SIZE),           static_cast<int>(cl.m_sortorder));
            Assert::AreEqual(static_cast<int>(CCommandLine::ESortDirection::SD_DESCENDING), static_cast<int>(cl.m_sortdirection));
        }




        TEST_METHOD(ParseOrderColonAndDate)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/o:d";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::AreEqual(static_cast<int>(CCommandLine::ESortOrder::SO_DATE),          static_cast<int>(cl.m_sortorder));
            Assert::AreEqual(static_cast<int>(CCommandLine::ESortDirection::SD_ASCENDING), static_cast<int>(cl.m_sortdirection));
        }




        TEST_METHOD(ParseOrderColonReverseAndDate)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/o:-d";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::AreEqual(static_cast<int>(CCommandLine::ESortOrder::SO_DATE),           static_cast<int>(cl.m_sortorder));
            Assert::AreEqual(static_cast<int>(CCommandLine::ESortDirection::SD_DESCENDING), static_cast<int>(cl.m_sortdirection));
        }




        TEST_METHOD(ParseAttributesAndFlags)
        {
            CCommandLine    cl;
            const wchar_t * a1     = L"/a-dh";
            const wchar_t * s      = L"/s";
            const wchar_t * w      = L"/w";
            const wchar_t * p      = L"/p";
            wchar_t       * argv[] = { const_cast<wchar_t *>(a1), const_cast<wchar_t *>(s), const_cast<wchar_t *>(w), const_cast<wchar_t *>(p) };
            HRESULT         hr     = cl.Parse(4, argv);


            Assert::IsTrue(SUCCEEDED(hr));
            Assert::IsTrue((cl.m_dwAttributesExcluded & FILE_ATTRIBUTE_DIRECTORY) != 0);
            Assert::IsTrue((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_HIDDEN)    != 0);
            Assert::IsTrue(cl.m_fRecurse);
            Assert::IsTrue(cl.m_fWideListing);
            Assert::IsTrue(cl.m_fPerfTimer);
        }




        TEST_METHOD(ParseAttributeNotContentIndexed)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/a:x";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::IsTrue((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED) != 0);
        }




        TEST_METHOD(ParseAttributeCloudOnlyComposite)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/a:o";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            // 'o' is a composite: OFFLINE | RECALL_ON_OPEN | RECALL_ON_DATA_ACCESS
            Assert::IsTrue((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_OFFLINE)               != 0);
            Assert::IsTrue((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_RECALL_ON_OPEN)        != 0);
            Assert::IsTrue((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS) != 0);
        }




        TEST_METHOD(ParseAttributeIntegrityStream)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/a:i";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::IsTrue((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_INTEGRITY_STREAM) != 0);
        }




        TEST_METHOD(ParseAttributeNoScrubData)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/a:b";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::IsTrue((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_NO_SCRUB_DATA) != 0);
        }




        TEST_METHOD(ParseAttributePinned)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/a:f";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::IsTrue((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_PINNED) != 0);
        }




        TEST_METHOD(ParseAttributeUnpinned)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/a:u";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::IsTrue((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_UNPINNED) != 0);
        }




        TEST_METHOD(ParseAttributeCloudOnlyNegated)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/a:-o";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            // Negated 'o' should exclude all cloud-only attributes
            Assert::IsTrue((cl.m_dwAttributesExcluded & FILE_ATTRIBUTE_OFFLINE)               != 0);
            Assert::IsTrue((cl.m_dwAttributesExcluded & FILE_ATTRIBUTE_RECALL_ON_OPEN)        != 0);
            Assert::IsTrue((cl.m_dwAttributesExcluded & FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS) != 0);
        }




        TEST_METHOD(ParseEnvSwitch)
        {
            CCommandLine    cl;
            const wchar_t * e1      = L"/Env";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(e1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::IsTrue(cl.m_fEnv);
            Assert::AreEqual(L'/', cl.GetSwitchPrefix());
        }




        TEST_METHOD(ParseEnvSwitchDoubleDash)
        {
            CCommandLine    cl;
            const wchar_t * e1      = L"--env";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(e1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::IsTrue(cl.m_fEnv);
            Assert::AreEqual(L'-', cl.GetSwitchPrefix());
        }




        TEST_METHOD(ParseEnvSwitchSingleDashFails)
        {
            CCommandLine    cl;
            const wchar_t * e1      = L"-env";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(e1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(FAILED(hr));
        }




        TEST_METHOD(ParseConfigSwitchSlash)
        {
            CCommandLine    cl;
            const wchar_t * c1      = L"/Config";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(c1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::IsTrue(cl.m_fConfig);
            Assert::AreEqual(L'/', cl.GetSwitchPrefix());
        }




        TEST_METHOD(ParseConfigSwitchDoubleDash)
        {
            CCommandLine    cl;
            const wchar_t * c1      = L"--config";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(c1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::IsTrue(cl.m_fConfig);
            Assert::AreEqual(L'-', cl.GetSwitchPrefix());
        }




        TEST_METHOD(ParseConfigSwitchSingleDashFails)
        {
            CCommandLine    cl;
            const wchar_t * c1      = L"-config";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(c1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(FAILED(hr));
        }




        TEST_METHOD(ParseHelpSwitchWithDash)
        {
            CCommandLine    cl;
            const wchar_t * h1      = L"-?";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(h1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::IsTrue(cl.m_fHelp);
            Assert::AreEqual(L'-', cl.GetSwitchPrefix());
        }




        TEST_METHOD(ParseHelpSwitchWithSlash)
        {
            CCommandLine    cl;
            const wchar_t * h1      = L"/?";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(h1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::IsTrue(cl.m_fHelp);
            Assert::AreEqual(L'/', cl.GetSwitchPrefix());
        }




        TEST_METHOD(EnvSwitchDoesNotSupportDisable)
        {
            CCommandLine    cl;
            const wchar_t * e1      = L"/Env-";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(e1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(FAILED(hr));
        }




        TEST_METHOD(ParseDebugSwitchDoubleDash)
        {
            CCommandLine    cl;
            const wchar_t * d1      = L"--debug";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(d1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::IsTrue(cl.m_fDebug);
            Assert::AreEqual(L'-', cl.GetSwitchPrefix());
        }




        TEST_METHOD(ParseDebugSwitchSlash)
        {
            CCommandLine    cl;
            const wchar_t * d1      = L"/Debug";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(d1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::IsTrue(cl.m_fDebug);
            Assert::AreEqual(L'/', cl.GetSwitchPrefix());
        }




        TEST_METHOD(ParseDebugSwitchSingleDashFails)
        {
            CCommandLine    cl;
            const wchar_t * d1      = L"-debug";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(d1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(FAILED(hr));
        }

    };
}
