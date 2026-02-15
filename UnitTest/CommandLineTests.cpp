#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/Color.h"
#include "../TCDirCore/CommandLine.h"
#include "../TCDirCore/Config.h"





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



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::AreEqual (static_cast<int>(CCommandLine::ESortOrder::SO_NAME),          static_cast<int>(cl.m_sortorder));
            Assert::AreEqual (static_cast<int>(CCommandLine::ESortDirection::SD_ASCENDING), static_cast<int>(cl.m_sortdirection));
        }





        TEST_METHOD(ParseOrderReverseAndSize)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/o-s";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::AreEqual (static_cast<int>(CCommandLine::ESortOrder::SO_SIZE),           static_cast<int>(cl.m_sortorder));
            Assert::AreEqual (static_cast<int>(CCommandLine::ESortDirection::SD_DESCENDING), static_cast<int>(cl.m_sortdirection));
        }




        TEST_METHOD(ParseOrderColonAndDate)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/o:d";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::AreEqual (static_cast<int>(CCommandLine::ESortOrder::SO_DATE),          static_cast<int>(cl.m_sortorder));
            Assert::AreEqual (static_cast<int>(CCommandLine::ESortDirection::SD_ASCENDING), static_cast<int>(cl.m_sortdirection));
        }




        TEST_METHOD(ParseOrderColonReverseAndDate)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/o:-d";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::AreEqual (static_cast<int>(CCommandLine::ESortOrder::SO_DATE),           static_cast<int>(cl.m_sortorder));
            Assert::AreEqual (static_cast<int>(CCommandLine::ESortDirection::SD_DESCENDING), static_cast<int>(cl.m_sortdirection));
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


            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue ((cl.m_dwAttributesExcluded & FILE_ATTRIBUTE_DIRECTORY) != 0);
            Assert::IsTrue ((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_HIDDEN)    != 0);
            Assert::IsTrue (cl.m_fRecurse);
            Assert::IsTrue (cl.m_fWideListing);
            Assert::IsTrue (cl.m_fPerfTimer);
        }




        TEST_METHOD(ParseAttributeNotContentIndexed)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/a:x";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue ((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_NOT_CONTENT_INDEXED) != 0);
        }




        TEST_METHOD(ParseAttributeCloudOnlyComposite)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/a:o";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            // 'o' is a composite: OFFLINE | RECALL_ON_OPEN | RECALL_ON_DATA_ACCESS
            Assert::IsTrue ((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_OFFLINE)               != 0);
            Assert::IsTrue ((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_RECALL_ON_OPEN)        != 0);
            Assert::IsTrue ((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS) != 0);
        }




        TEST_METHOD(ParseAttributeIntegrityStream)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/a:i";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue ((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_INTEGRITY_STREAM) != 0);
        }




        TEST_METHOD(ParseAttributeNoScrubData)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/a:b";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue ((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_NO_SCRUB_DATA) != 0);
        }




        TEST_METHOD(ParseAttributeAlwaysLocallyAvailable)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/a:v";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue ((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_PINNED) != 0);
        }




        TEST_METHOD(ParseAttributeLocallyAvailable)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/a:l";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue ((cl.m_dwAttributesRequired & FILE_ATTRIBUTE_UNPINNED) != 0);
        }




        TEST_METHOD(ParseAttributeCloudOnlyNegated)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/a:-o";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            // Negated 'o' should exclude all cloud-only attributes
            Assert::IsTrue ((cl.m_dwAttributesExcluded & FILE_ATTRIBUTE_OFFLINE)               != 0);
            Assert::IsTrue ((cl.m_dwAttributesExcluded & FILE_ATTRIBUTE_RECALL_ON_OPEN)        != 0);
            Assert::IsTrue ((cl.m_dwAttributesExcluded & FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS) != 0);
        }




        TEST_METHOD(ParseEnvSwitch)
        {
            CCommandLine    cl;
            const wchar_t * e1      = L"/Env";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(e1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue (cl.m_fEnv);
            Assert::AreEqual (L'/', cl.GetSwitchPrefix());
        }




        TEST_METHOD(ParseEnvSwitchDoubleDash)
        {
            CCommandLine    cl;
            const wchar_t * e1      = L"--env";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(e1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue (cl.m_fEnv);
            Assert::AreEqual (L'-', cl.GetSwitchPrefix());
        }




        TEST_METHOD(ParseEnvSwitchSingleDashFails)
        {
            CCommandLine    cl;
            const wchar_t * e1      = L"-env";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(e1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (FAILED(hr));
        }




        TEST_METHOD(ParseConfigSwitchSlash)
        {
            CCommandLine    cl;
            const wchar_t * c1      = L"/Config";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(c1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue (cl.m_fConfig);
            Assert::AreEqual (L'/', cl.GetSwitchPrefix());
        }




        TEST_METHOD(ParseConfigSwitchDoubleDash)
        {
            CCommandLine    cl;
            const wchar_t * c1      = L"--config";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(c1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue (cl.m_fConfig);
            Assert::AreEqual (L'-', cl.GetSwitchPrefix());
        }




        TEST_METHOD(ParseConfigSwitchSingleDashFails)
        {
            CCommandLine    cl;
            const wchar_t * c1      = L"-config";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(c1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (FAILED(hr));
        }




        TEST_METHOD(ParseHelpSwitchWithDash)
        {
            CCommandLine    cl;
            const wchar_t * h1      = L"-?";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(h1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue (cl.m_fHelp);
            Assert::AreEqual (L'-', cl.GetSwitchPrefix());
        }




        TEST_METHOD(ParseHelpSwitchWithSlash)
        {
            CCommandLine    cl;
            const wchar_t * h1      = L"/?";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(h1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue (cl.m_fHelp);
            Assert::AreEqual (L'/', cl.GetSwitchPrefix());
        }




        TEST_METHOD(EnvSwitchDoesNotSupportDisable)
        {
            CCommandLine    cl;
            const wchar_t * e1      = L"/Env-";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(e1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (FAILED(hr));
        }




#ifdef _DEBUG
        TEST_METHOD(ParseDebugSwitchDoubleDash)
        {
            CCommandLine    cl;
            const wchar_t * d1      = L"--debug";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(d1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue (cl.m_fDebug);
            Assert::AreEqual (L'-', cl.GetSwitchPrefix());
        }




        TEST_METHOD(ParseDebugSwitchSlash)
        {
            CCommandLine    cl;
            const wchar_t * d1      = L"/Debug";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(d1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue (cl.m_fDebug);
            Assert::AreEqual (L'/', cl.GetSwitchPrefix());
        }
#endif




        TEST_METHOD(ParseDebugSwitchSingleDashFails)
        {
            CCommandLine    cl;
            const wchar_t * d1      = L"-debug";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(d1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (FAILED(hr));
        }




        TEST_METHOD(ParseTimeFieldCreation)
        {
            CCommandLine    cl;
            const wchar_t * t1      = L"/T:C";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(t1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::AreEqual (static_cast<int>(CCommandLine::ETimeField::TF_CREATION), static_cast<int>(cl.m_timeField));
        }




        TEST_METHOD(ParseTimeFieldAccess)
        {
            CCommandLine    cl;
            const wchar_t * t1      = L"/T:A";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(t1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::AreEqual (static_cast<int>(CCommandLine::ETimeField::TF_ACCESS), static_cast<int>(cl.m_timeField));
        }




        TEST_METHOD(ParseTimeFieldWritten)
        {
            CCommandLine    cl;
            const wchar_t * t1      = L"/T:W";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(t1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::AreEqual (static_cast<int>(CCommandLine::ETimeField::TF_WRITTEN), static_cast<int>(cl.m_timeField));
        }




        TEST_METHOD(ParseTimeFieldDefaultIsWritten)
        {
            CCommandLine cl;



            Assert::AreEqual (static_cast<int>(CCommandLine::ETimeField::TF_WRITTEN), static_cast<int>(cl.m_timeField));
        }




        TEST_METHOD(ParseTimeFieldNoColon)
        {
            CCommandLine    cl;
            const wchar_t * t1      = L"/TC";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(t1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::AreEqual (static_cast<int>(CCommandLine::ETimeField::TF_CREATION), static_cast<int>(cl.m_timeField));
        }




        TEST_METHOD(ParseTimeFieldWithDash)
        {
            CCommandLine    cl;
            const wchar_t * t1      = L"-T:C";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(t1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::AreEqual (static_cast<int>(CCommandLine::ETimeField::TF_CREATION), static_cast<int>(cl.m_timeField));
        }




        TEST_METHOD(ParseTimeFieldCaseInsensitive)
        {
            CCommandLine    cl;
            const wchar_t * t1      = L"/t:c";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(t1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::AreEqual (static_cast<int>(CCommandLine::ETimeField::TF_CREATION), static_cast<int>(cl.m_timeField));
        }




        TEST_METHOD(ParseTimeFieldInvalidValue)
        {
            CCommandLine    cl;
            const wchar_t * t1      = L"/T:X";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(t1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (FAILED(hr));
        }




        TEST_METHOD(ParseOwnerSwitchDoubleDash)
        {
            CCommandLine    cl;
            const wchar_t * o1      = L"--owner";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(o1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue (cl.m_fShowOwner);
            Assert::AreEqual (L'-', cl.GetSwitchPrefix());
        }




        TEST_METHOD(ParseOwnerSwitchSlash)
        {
            CCommandLine    cl;
            const wchar_t * o1      = L"/Owner";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(o1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue (cl.m_fShowOwner);
            Assert::AreEqual (L'/', cl.GetSwitchPrefix());
        }




        TEST_METHOD(ParseOwnerSwitchSingleDashFails)
        {
            CCommandLine    cl;
            const wchar_t * o1      = L"-owner";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(o1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (FAILED(hr));
        }




        TEST_METHOD(ParseStreamsSwitchDoubleDash)
        {
            CCommandLine    cl;
            const wchar_t * o1      = L"--streams";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(o1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue (cl.m_fShowStreams);
            Assert::AreEqual (L'-', cl.GetSwitchPrefix());
        }




        TEST_METHOD(ParseStreamsSwitchSlash)
        {
            CCommandLine    cl;
            const wchar_t * o1      = L"/Streams";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(o1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue (cl.m_fShowStreams);
            Assert::AreEqual (L'/', cl.GetSwitchPrefix());
        }




        TEST_METHOD(ParseStreamsSwitchSingleDashFails)
        {
            CCommandLine    cl;
            const wchar_t * o1      = L"-streams";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(o1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (FAILED(hr));
        }




        //
        // ApplyConfigDefaults tests - verify env var settings transfer to command line
        //

        TEST_METHOD(ApplyConfigDefaults_Streams_TransfersToCommandLine)
        {
            CConfig      config;
            CCommandLine cl;

            config.Initialize (FC_LightGrey);
            config.m_fShowStreams = true;

            cl.ApplyConfigDefaults (config);

            Assert::IsTrue (cl.m_fShowStreams);
        }




        TEST_METHOD(ApplyConfigDefaults_Owner_TransfersToCommandLine)
        {
            CConfig      config;
            CCommandLine cl;

            config.Initialize (FC_LightGrey);
            config.m_fShowOwner = true;

            cl.ApplyConfigDefaults (config);

            Assert::IsTrue (cl.m_fShowOwner);
        }




        TEST_METHOD(ApplyConfigDefaults_AllSwitches_TransferToCommandLine)
        {
            CConfig      config;
            CCommandLine cl;

            config.Initialize (FC_LightGrey);
            config.m_fWideListing   = true;
            config.m_fBareListing   = true;
            config.m_fRecurse       = true;
            config.m_fPerfTimer     = true;
            config.m_fMultiThreaded = false;  // Explicitly disable
            config.m_fShowOwner     = true;
            config.m_fShowStreams   = true;

            cl.ApplyConfigDefaults (config);

            Assert::IsTrue (cl.m_fWideListing);
            Assert::IsTrue (cl.m_fBareListing);
            Assert::IsTrue (cl.m_fRecurse);
            Assert::IsTrue (cl.m_fPerfTimer);
            Assert::IsFalse (cl.m_fMultiThreaded);
            Assert::IsTrue (cl.m_fShowOwner);
            Assert::IsTrue (cl.m_fShowStreams);
        }




        TEST_METHOD(ApplyConfigDefaults_UnsetValues_DoNotOverrideDefaults)
        {
            CConfig      config;
            CCommandLine cl;

            config.Initialize (FC_LightGrey);
            // Don't set any switch values - they should remain as optional<> without value

            // Set some non-default values on command line first
            cl.m_fWideListing = true;

            cl.ApplyConfigDefaults (config);

            // Command line value should be preserved since config didn't have a value
            Assert::IsTrue (cl.m_fWideListing);
        }



        //
        //  Scenario 28: /Icons → m_fIcons = true
        //

        TEST_METHOD(ParseIconsSwitchSlash)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/Icons";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue (cl.m_fIcons.has_value ());
            Assert::IsTrue (cl.m_fIcons.value ());
        }



        //
        //  Scenario 29: /Icons- → m_fIcons = false
        //

        TEST_METHOD(ParseIconsDisableSwitchSlash)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/Icons-";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue (cl.m_fIcons.has_value ());
            Assert::IsFalse (cl.m_fIcons.value ());
        }



        //
        //  Scenario 30: --Icons → m_fIcons = true (long switch with double dash)
        //

        TEST_METHOD(ParseIconsSwitchDoubleDash)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"--Icons";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue (cl.m_fIcons.has_value ());
            Assert::IsTrue (cl.m_fIcons.value ());
        }



        //
        //  Scenario 31: No icons flag → m_fIcons = nullopt
        //

        TEST_METHOD(ParseNoIconsFlag_DefaultIsNullopt)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/on";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsFalse (cl.m_fIcons.has_value ());
        }



        //
        //  Icons switch is case-insensitive
        //

        TEST_METHOD(ParseIconsSwitchCaseInsensitive)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"/icons";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (SUCCEEDED(hr));
            Assert::IsTrue (cl.m_fIcons.has_value ());
            Assert::IsTrue (cl.m_fIcons.value ());
        }



        //
        //  Single dash -Icons should fail (just like -env)
        //

        TEST_METHOD(ParseIconsSwitchSingleDashFails)
        {
            CCommandLine    cl;
            const wchar_t * a1      = L"-Icons";
            wchar_t       * argv1[] = { const_cast<wchar_t *>(a1) };
            HRESULT         hr      = cl.Parse(1, argv1);



            Assert::IsTrue (FAILED(hr));
        }

    };
}
