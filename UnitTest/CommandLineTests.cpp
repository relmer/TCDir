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




        TEST_METHOD(ParseEnvSwitch)
        {
            CCommandLine    cl;
            const wchar_t * e1      = L"/Env";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(e1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(SUCCEEDED(hr));
            Assert::IsTrue(cl.m_fEnv);
        }




        TEST_METHOD(EnvSwitchDoesNotSupportDisable)
        {
            CCommandLine    cl;
            const wchar_t * e1      = L"/Env-";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(e1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue(FAILED(hr));
        }

    };
}
