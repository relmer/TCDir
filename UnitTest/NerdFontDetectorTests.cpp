#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/NerdFontDetector.h"
#include "../TCDirCore/Config.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{
    class CTestEnvironmentProviderNFD : public IEnvironmentProvider
    {
    public:
        void Set (LPCWSTR pszName, wstring value)
        {
            m_map[pszName] = std::move (value);
        }

        void Clear (LPCWSTR pszName)
        {
            m_map.erase (pszName);
        }

        virtual bool TryGetEnvironmentVariable (LPCWSTR pszName, wstring & value) const override
        {
            auto iter = m_map.find (pszName);
            if (iter == m_map.end())
            {
                value.clear();
                return false;
            }

            value = iter->second;
            return true;
        }

    private:
        unordered_map<wstring, wstring> m_map;
    };



    //
    // Test derivation that overrides GDI-dependent methods
    //

    struct NerdFontDetectorProbe : public CNerdFontDetector
    {
        bool    m_fProbeResult        = false;
        HRESULT m_hrProbe             = S_OK;
        bool    m_fFontInstalled      = false;
        HRESULT m_hrFontInstalled     = S_OK;
        bool    m_fProbeCalled        = false;
        bool    m_fFontInstalledCalled = false;

    protected:
        HRESULT ProbeConsoleFontForGlyph (HANDLE, WCHAR, bool & fHasGlyph) override
        {
            m_fProbeCalled = true;

            if (FAILED (m_hrProbe))
            {
                return m_hrProbe;
            }

            fHasGlyph = m_fProbeResult;
            return S_OK;
        }

        HRESULT IsNerdFontInstalled (bool & fFound) override
        {
            m_fFontInstalledCalled = true;

            if (FAILED (m_hrFontInstalled))
            {
                return m_hrFontInstalled;
            }

            fFound = m_fFontInstalled;
            return S_OK;
        }
    };





    TEST_CLASS(NerdFontDetectorTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        //
        // WezTerm short-circuit
        //

        TEST_METHOD(Detect_WezTerm_ReturnsDetected)
        {
            NerdFontDetectorProbe   detector;
            CTestEnvironmentProviderNFD env;
            EDetectionResult        result = EDetectionResult::NotDetected;

            env.Set (L"TERM_PROGRAM", L"WezTerm");

            HRESULT hr = detector.Detect (nullptr, env, result);

            Assert::AreEqual (S_OK, hr);
            Assert::IsTrue   (result == EDetectionResult::Detected);
            Assert::IsFalse  (detector.m_fProbeCalled);
            Assert::IsFalse  (detector.m_fFontInstalledCalled);
        }




        TEST_METHOD(Detect_WezTermCaseInsensitive_ReturnsDetected)
        {
            NerdFontDetectorProbe   detector;
            CTestEnvironmentProviderNFD env;
            EDetectionResult        result = EDetectionResult::NotDetected;

            env.Set (L"TERM_PROGRAM", L"wezterm");

            HRESULT hr = detector.Detect (nullptr, env, result);

            Assert::AreEqual (S_OK, hr);
            Assert::IsTrue   (result == EDetectionResult::Detected);
        }




        //
        // ConPTY delegation to font enumeration
        //

        TEST_METHOD(Detect_WindowsTerminal_DelegatesToFontEnum)
        {
            NerdFontDetectorProbe   detector;
            CTestEnvironmentProviderNFD env;
            EDetectionResult        result = EDetectionResult::NotDetected;

            env.Set (L"WT_SESSION", L"{some-guid}");
            detector.m_fFontInstalled = true;

            HRESULT hr = detector.Detect (nullptr, env, result);

            Assert::AreEqual (S_OK, hr);
            Assert::IsTrue   (result == EDetectionResult::Detected);
            Assert::IsFalse  (detector.m_fProbeCalled);     // ConPTY skips probe
            Assert::IsTrue   (detector.m_fFontInstalledCalled);
        }




        TEST_METHOD(Detect_VSCodeTerminal_DelegatesToFontEnum)
        {
            NerdFontDetectorProbe   detector;
            CTestEnvironmentProviderNFD env;
            EDetectionResult        result = EDetectionResult::NotDetected;

            env.Set (L"TERM_PROGRAM", L"vscode");
            detector.m_fFontInstalled = false;

            HRESULT hr = detector.Detect (nullptr, env, result);

            Assert::AreEqual (S_OK, hr);
            Assert::IsTrue   (result == EDetectionResult::NotDetected);
        }




        //
        // Classic conhost canary probe
        //

        TEST_METHOD(Detect_ClassicConhost_ProbeSucceeds_GlyphFound)
        {
            NerdFontDetectorProbe   detector;
            CTestEnvironmentProviderNFD env;
            EDetectionResult        result = EDetectionResult::NotDetected;

            // No ConPTY env vars → classic path
            detector.m_fProbeResult = true;

            HRESULT hr = detector.Detect (nullptr, env, result);

            Assert::AreEqual (S_OK, hr);
            Assert::IsTrue   (result == EDetectionResult::Detected);
            Assert::IsTrue   (detector.m_fProbeCalled);
        }




        TEST_METHOD(Detect_ClassicConhost_ProbeSucceeds_GlyphNotFound)
        {
            NerdFontDetectorProbe   detector;
            CTestEnvironmentProviderNFD env;
            EDetectionResult        result = EDetectionResult::Detected;

            detector.m_fProbeResult = false;

            HRESULT hr = detector.Detect (nullptr, env, result);

            Assert::AreEqual (S_OK, hr);
            Assert::IsTrue   (result == EDetectionResult::NotDetected);
        }




        //
        // Probe failure → fall back to font enumeration
        //

        TEST_METHOD(Detect_ProbeFails_FallsBackToFontEnum)
        {
            NerdFontDetectorProbe   detector;
            CTestEnvironmentProviderNFD env;
            EDetectionResult        result = EDetectionResult::NotDetected;

            detector.m_hrProbe        = E_FAIL;
            detector.m_fFontInstalled = true;

            HRESULT hr = detector.Detect (nullptr, env, result);

            Assert::AreEqual (S_OK, hr);
            Assert::IsTrue   (result == EDetectionResult::Detected);
            Assert::IsTrue   (detector.m_fProbeCalled);
            Assert::IsTrue   (detector.m_fFontInstalledCalled);
        }




        TEST_METHOD(Detect_ProbeAndFontEnumFail_ReturnsInconclusive)
        {
            NerdFontDetectorProbe   detector;
            CTestEnvironmentProviderNFD env;
            EDetectionResult        result = EDetectionResult::Detected;

            detector.m_hrProbe        = E_FAIL;
            detector.m_hrFontInstalled = E_FAIL;

            HRESULT hr = detector.Detect (nullptr, env, result);

            Assert::AreEqual (S_OK, hr);
            Assert::IsTrue   (result == EDetectionResult::Inconclusive);
        }




        //
        // Multiple env vars
        //

        TEST_METHOD(Detect_MultipleEnvVars_WezTermWins)
        {
            // WezTerm wins even if WT_SESSION is set
            NerdFontDetectorProbe   detector;
            CTestEnvironmentProviderNFD env;
            EDetectionResult        result = EDetectionResult::NotDetected;

            env.Set (L"WT_SESSION", L"{guid}");
            env.Set (L"TERM_PROGRAM", L"WezTerm");

            HRESULT hr = detector.Detect (nullptr, env, result);

            Assert::AreEqual (S_OK, hr);
            Assert::IsTrue   (result == EDetectionResult::Detected);
            Assert::IsFalse  (detector.m_fProbeCalled);
            Assert::IsFalse  (detector.m_fFontInstalledCalled);
        }




        //
        // Empty env vars
        //

        TEST_METHOD(Detect_EmptyEnvVars_ClassicPath)
        {
            NerdFontDetectorProbe   detector;
            CTestEnvironmentProviderNFD env;
            EDetectionResult        result = EDetectionResult::NotDetected;

            env.Set (L"WT_SESSION",  L"");
            env.Set (L"TERM_PROGRAM", L"");

            // Empty env vars should not trigger ConPTY path
            detector.m_fProbeResult = true;

            HRESULT hr = detector.Detect (nullptr, env, result);

            Assert::AreEqual (S_OK, hr);
            Assert::IsTrue   (result == EDetectionResult::Detected);
            Assert::IsTrue   (detector.m_fProbeCalled);
        }
    };
}
