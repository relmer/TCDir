#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/Config.h"
#include "../TCDirCore/Color.h"
#include "../TCDirCore/IconMapping.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;



namespace Microsoft { namespace VisualStudio { namespace CppUnitTestFramework
{
    template<> inline std::wstring ToString<char32_t> (const char32_t & value)
    {
        wchar_t buf[16];
        swprintf_s (buf, L"U+%05X", static_cast<unsigned>(value));
        return buf;
    }
}}}



namespace UnitTest
{
    class CTestEnvironmentProvider : public IEnvironmentProvider
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




    // Probe class to expose protected CConfig methods for testing
    struct ConfigProbe : public CConfig
    {
        ConfigProbe (void)
        {
            SetEnvironmentProvider (&m_environmentProvider);
        }

        void SetEnvVar (LPCWSTR pszName, wstring value)
        {
            m_environmentProvider.Set (pszName, std::move (value));
        }

        void ClearEnvVar (LPCWSTR pszName)
        {
            m_environmentProvider.Clear (pszName);
        }

        using CConfig::ParseColorName;
        using CConfig::ParseColorSpec;
        using CConfig::TrimWhitespace;
        using CConfig::ParseKeyAndValue;
        using CConfig::ApplyUserColorOverrides;
        using CConfig::ProcessColorOverrideEntry;
        using CConfig::ProcessSwitchOverride;
        using CConfig::ProcessFileExtensionOverride;
        using CConfig::ProcessDisplayAttributeOverride;
        using CConfig::ProcessFileAttributeOverride;
        using CConfig::InitializeExtensionToIconMap;
        using CConfig::InitializeWellKnownDirToIconMap;

    private:
        CTestEnvironmentProvider m_environmentProvider;
    };





    TEST_CLASS(ConfigEnvironmentTests)
    {
    public:
        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }





        TEST_METHOD(ParseColorName_AllForegroundColors_ReturnsCorrectValues)
        {
            ConfigProbe config;
            
            // Test EVERY foreground color exhaustively
            auto AssertParseColorName = [&config] (wstring_view colorName, bool isBackground, WORD expected)
            {
                WORD    value = 0;
                HRESULT hr    = config.ParseColorName (colorName, isBackground, value);

                Assert::AreEqual ((HRESULT)S_OK, hr);
                Assert::AreEqual (expected, value);
            };

            AssertParseColorName (L"Black"sv,        false, (WORD)FC_Black);
            AssertParseColorName (L"Blue"sv,         false, (WORD)FC_Blue);
            AssertParseColorName (L"Green"sv,        false, (WORD)FC_Green);
            AssertParseColorName (L"Cyan"sv,         false, (WORD)FC_Cyan);
            AssertParseColorName (L"Red"sv,          false, (WORD)FC_Red);
            AssertParseColorName (L"Magenta"sv,      false, (WORD)FC_Magenta);
            AssertParseColorName (L"Brown"sv,        false, (WORD)FC_Brown);
            AssertParseColorName (L"LightGrey"sv,    false, (WORD)FC_LightGrey);
            AssertParseColorName (L"DarkGrey"sv,     false, (WORD)FC_DarkGrey);
            AssertParseColorName (L"LightBlue"sv,    false, (WORD)FC_LightBlue);
            AssertParseColorName (L"LightGreen"sv,   false, (WORD)FC_LightGreen);
            AssertParseColorName (L"LightCyan"sv,    false, (WORD)FC_LightCyan);
            AssertParseColorName (L"LightRed"sv,     false, (WORD)FC_LightRed);
            AssertParseColorName (L"LightMagenta"sv, false, (WORD)FC_LightMagenta);
            AssertParseColorName (L"Yellow"sv,       false, (WORD)FC_Yellow);
            AssertParseColorName (L"White"sv,        false, (WORD)FC_White);
        }





        TEST_METHOD(ParseColorName_AllBackgroundColors_ReturnsCorrectValues)
        {
            ConfigProbe config;
            
            // Test EVERY background color exhaustively
            auto AssertParseColorName = [&config] (wstring_view colorName, bool isBackground, WORD expected)
            {
                WORD    value = 0;
                HRESULT hr    = config.ParseColorName (colorName, isBackground, value);

                Assert::AreEqual ((HRESULT)S_OK, hr);
                Assert::AreEqual (expected, value);
            };

            AssertParseColorName (L"Black"sv,        true, (WORD)BC_Black);
            AssertParseColorName (L"Blue"sv,         true, (WORD)BC_Blue);
            AssertParseColorName (L"Green"sv,        true, (WORD)BC_Green);
            AssertParseColorName (L"Cyan"sv,         true, (WORD)BC_Cyan);
            AssertParseColorName (L"Red"sv,          true, (WORD)BC_Red);
            AssertParseColorName (L"Magenta"sv,      true, (WORD)BC_Magenta);
            AssertParseColorName (L"Brown"sv,        true, (WORD)BC_Brown);
            AssertParseColorName (L"LightGrey"sv,    true, (WORD)BC_LightGrey);
            AssertParseColorName (L"DarkGrey"sv,     true, (WORD)BC_DarkGrey);
            AssertParseColorName (L"LightBlue"sv,    true, (WORD)BC_LightBlue);
            AssertParseColorName (L"LightGreen"sv,   true, (WORD)BC_LightGreen);
            AssertParseColorName (L"LightCyan"sv,    true, (WORD)BC_LightCyan);
            AssertParseColorName (L"LightRed"sv,     true, (WORD)BC_LightRed);
            AssertParseColorName (L"LightMagenta"sv, true, (WORD)BC_LightMagenta);
            AssertParseColorName (L"Yellow"sv,       true, (WORD)BC_Yellow);
            AssertParseColorName (L"White"sv,        true, (WORD)BC_White);
        }





        TEST_METHOD(ParseColorName_CaseInsensitive_ReturnsCorrectValues)
        {
            ConfigProbe config;
            
            // Test various case combinations (only need one color since case logic is color-agnostic)
            WORD    value = 0;
            HRESULT hr    = S_OK;

            hr = config.ParseColorName (L"YELLOW"sv, false, value);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual ((WORD)FC_Yellow, value);

            hr = config.ParseColorName (L"yellow"sv, false, value);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual ((WORD)FC_Yellow, value);

            hr = config.ParseColorName (L"YeLLoW"sv, false, value);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual ((WORD)FC_Yellow, value);

            hr = config.ParseColorName (L"yELLOw"sv, false, value);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual ((WORD)FC_Yellow, value);
        }





        TEST_METHOD(ParseColorName_InvalidColor_ReturnsZero)
        {
            ConfigProbe config;
            
            WORD    value = 0;
            HRESULT hr    = S_OK;

            hr = config.ParseColorName (L"InvalidColor"sv,  false, value);
            Assert::AreEqual ((HRESULT)E_INVALIDARG, hr);
            Assert::AreEqual ((WORD)0, value);

            hr = config.ParseColorName (L""sv,              false, value);
            Assert::AreEqual ((HRESULT)E_INVALIDARG, hr);
            Assert::AreEqual ((WORD)0, value);

            hr = config.ParseColorName (L"Purple"sv,        false, value);
            Assert::AreEqual ((HRESULT)E_INVALIDARG, hr);
            Assert::AreEqual ((WORD)0, value);

            hr = config.ParseColorName (L"Orange"sv,        false, value);
            Assert::AreEqual ((HRESULT)E_INVALIDARG, hr);
            Assert::AreEqual ((WORD)0, value);

            hr = config.ParseColorName (L"123"sv,           false, value);
            Assert::AreEqual ((HRESULT)E_INVALIDARG, hr);
            Assert::AreEqual ((WORD)0, value);
        }





        TEST_METHOD(ParseColorSpec_ForegroundOnly_NoWhitespace_ReturnsCorrectValue)
        {
            ConfigProbe config;
            
            WORD    result = 0;
            HRESULT hr     = config.ParseColorSpec (L"Yellow"sv, result);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual ((WORD)FC_Yellow, result);
        }





        TEST_METHOD(ParseColorSpec_ForegroundOnly_LeadingWhitespace_ReturnsCorrectValue)
        {
            ConfigProbe config;
            
            WORD    result = 0;
            HRESULT hr     = config.ParseColorSpec (L"  Yellow"sv, result);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual ((WORD)FC_Yellow, result);
            
            hr = config.ParseColorSpec (L"    Yellow"sv, result);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual ((WORD)FC_Yellow, result);
        }





        TEST_METHOD(ParseColorSpec_ForegroundOnly_TrailingWhitespace_ReturnsCorrectValue)
        {
            ConfigProbe config;
            
            WORD    result = 0;
            HRESULT hr     = config.ParseColorSpec (L"Yellow  "sv, result);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual ((WORD)FC_Yellow, result);
            
            hr = config.ParseColorSpec (L"Yellow    "sv, result);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual ((WORD)FC_Yellow, result);
        }





        TEST_METHOD(ParseColorSpec_ForegroundOnly_BothWhitespace_ReturnsCorrectValue)
        {
            ConfigProbe config;
            
            WORD    result = 0;
            HRESULT hr     = config.ParseColorSpec (L"  Yellow  "sv, result);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual ((WORD)FC_Yellow, result);
            
            hr = config.ParseColorSpec (L"    Yellow    "sv, result);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual ((WORD)FC_Yellow, result);
        }





        TEST_METHOD(ParseColorSpec_ForegroundAndBackground_NoWhitespace_ReturnsCorrectValue)
        {
            ConfigProbe config;
            
            WORD    result = 0;
            HRESULT hr     = config.ParseColorSpec (L"Yellow on Blue"sv, result);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            WORD expected = FC_Yellow | BC_Blue;
            Assert::AreEqual (expected, result);
        }





        TEST_METHOD(ParseColorSpec_ForegroundAndBackground_ExtraWhitespace_ReturnsCorrectValue)
        {
            ConfigProbe config;
            
            WORD expected = FC_LightGreen | BC_Red;
            
            WORD    result = 0;
            HRESULT hr     = config.ParseColorSpec (L"  LightGreen  on  Red  "sv, result);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual (expected, result);
                        
            hr = config.ParseColorSpec (L"    LightGreen    on    Red    "sv, result);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual (expected, result);
        }




        
        TEST_METHOD(ParseColorSpec_CaseInsensitiveOn_AllVariations_ReturnsCorrectValue)
        {
            ConfigProbe config;
            
            WORD expected = FC_White | BC_Black;
            
            WORD    result = 0;
            HRESULT hr     = S_OK;

            hr = config.ParseColorSpec (L"White on Black"sv, result);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual (expected, result);

            hr = config.ParseColorSpec (L"White ON Black"sv, result);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual (expected, result);

            hr = config.ParseColorSpec (L"White On Black"sv, result);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual (expected, result);

            hr = config.ParseColorSpec (L"White oN Black"sv, result);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual (expected, result);

            hr = config.ParseColorSpec (L"white ON black"sv, result);
            Assert::AreEqual ((HRESULT)S_OK, hr);
            Assert::AreEqual (expected, result);
        }





        TEST_METHOD(ParseColorSpec_TabNotWhitespace_ReturnsZero)
        {
            ConfigProbe config;
            
            // Tab characters should NOT be treated as whitespace
            WORD    result = 0;
            HRESULT hr     = config.ParseColorSpec (L"\tYellow"sv, result);
            Assert::AreEqual ((HRESULT)E_INVALIDARG, hr);
            Assert::AreEqual ((WORD)0, result);
            
            hr = config.ParseColorSpec (L"Yellow\t"sv, result);
            Assert::AreEqual ((HRESULT)E_INVALIDARG, hr);
            Assert::AreEqual ((WORD)0, result);
            
            hr = config.ParseColorSpec (L"\tYellow\t"sv, result);
            Assert::AreEqual ((HRESULT)E_INVALIDARG, hr);
            Assert::AreEqual ((WORD)0, result);
        }





        TEST_METHOD(TrimWhitespace_NoWhitespace_ReturnsUnchanged)
        {
            ConfigProbe config;
            
            auto result = config.TrimWhitespace(L"test"sv);
            Assert::AreEqual ((size_t)4, result.length());
            Assert::AreEqual (L't', result[0]);
        }





        TEST_METHOD(TrimWhitespace_LeadingWhitespace_RemovesCorrectly)
        {
            ConfigProbe config;
            
            auto result = config.TrimWhitespace(L"  test"sv);
            Assert::AreEqual ((size_t)4, result.length());
            
            result = config.TrimWhitespace(L"    test"sv);
            Assert::AreEqual ((size_t)4, result.length());
        }





        TEST_METHOD(TrimWhitespace_TrailingWhitespace_RemovesCorrectly)
        {
            ConfigProbe config;
            
            auto result = config.TrimWhitespace(L"test  "sv);
            Assert::AreEqual ((size_t)4, result.length());
            
            result = config.TrimWhitespace(L"test    "sv);
            Assert::AreEqual ((size_t)4, result.length());
        }





        TEST_METHOD(TrimWhitespace_BothWhitespace_RemovesCorrectly)
        {
            ConfigProbe config;
            
            auto result = config.TrimWhitespace(L"  test  "sv);
            Assert::AreEqual ((size_t)4, result.length());
            
            result = config.TrimWhitespace(L"    test    "sv);
            Assert::AreEqual ((size_t)4, result.length());
        }





        TEST_METHOD(TrimWhitespace_EmptyString_ReturnsEmpty)
        {
            ConfigProbe config;
            
            Assert::IsTrue (config.TrimWhitespace(L""sv).empty());
        }





        TEST_METHOD(TrimWhitespace_OnlyWhitespace_ReturnsEmpty)
        {
            ConfigProbe config;
            
            Assert::IsTrue (config.TrimWhitespace(L"   "sv).empty());
        }





        TEST_METHOD(TrimWhitespace_TabNotWhitespace_NotTrimmed)
        {
            ConfigProbe config;
            
            // Tab characters should NOT be treated as whitespace
            auto result = config.TrimWhitespace(L"\ttest"sv);
            Assert::AreEqual ((size_t)5, result.length());
            Assert::AreEqual (L'\t', result[0]);
            
            result = config.TrimWhitespace(L"test\t"sv);
            Assert::AreEqual ((size_t)5, result.length());
            Assert::AreEqual (L'\t', result[4]);
            
            result = config.TrimWhitespace(L"\ttest\t"sv);
            Assert::AreEqual ((size_t)6, result.length());
            
            result = config.TrimWhitespace(L"\t\t"sv);
            Assert::AreEqual ((size_t)2, result.length());
        }





        TEST_METHOD(ParseKeyAndValue_ValidEntry_SplitsCorrectly)
        {
            ConfigProbe config;
            wstring_view keyView, valueView;
            
            HRESULT hr = config.ParseKeyAndValue(L".cpp=Yellow"sv, keyView, valueView);
            
            Assert::IsTrue (SUCCEEDED(hr));
            Assert::AreEqual ((size_t)4, keyView.length());
            Assert::AreEqual ((size_t)6, valueView.length());
        }





        TEST_METHOD(ParseKeyAndValue_WithWhitespace_TrimsCorrectly)
        {
            ConfigProbe config;
            wstring_view keyView, valueView;
            
            HRESULT hr = config.ParseKeyAndValue(L"  .h  =  LightGreen  "sv, keyView, valueView);
            
            Assert::IsTrue (SUCCEEDED(hr));
            Assert::AreEqual ((size_t)2, keyView.length());
            Assert::AreEqual ((size_t)10, valueView.length());
        }





        TEST_METHOD(ParseKeyAndValue_NoEquals_ReturnsError)
        {
            ConfigProbe config;
            wstring_view keyView, valueView;
            
            HRESULT hr = config.ParseKeyAndValue(L".cppYellow"sv, keyView, valueView);
            
            Assert::IsFalse (SUCCEEDED(hr));
        }





        TEST_METHOD(ParseKeyAndValue_EmptyKey_ReturnsError)
        {
            ConfigProbe config;
            wstring_view keyView, valueView;
            
            HRESULT hr = config.ParseKeyAndValue(L"=Yellow"sv, keyView, valueView);
            
            Assert::IsFalse (SUCCEEDED(hr));
        }





        TEST_METHOD(ParseKeyAndValue_EmptyValue_ReturnsError)
        {
            ConfigProbe config;
            wstring_view keyView, valueView;
            
            HRESULT hr = config.ParseKeyAndValue(L".cpp="sv, keyView, valueView);
            
            Assert::IsFalse (SUCCEEDED(hr));
        }





        TEST_METHOD(ParseKeyAndValue_OnlyWhitespace_ReturnsError)
        {
            ConfigProbe config;
            wstring_view keyView, valueView;
            
            HRESULT hr = config.ParseKeyAndValue(L"   =   "sv, keyView, valueView);
            
            Assert::IsFalse (SUCCEEDED(hr));
        }





        TEST_METHOD(ApplyUserColorOverrides_NoEnvironmentVariable_DoesNothing)
        {
            ConfigProbe config;
            
            // Ensure no TCDIR variable
            config.ClearEnvVar (TCDIR_ENV_VAR_NAME);
            
            config.Initialize(FC_LightGrey);
            
            // Should succeed without error
            Assert::IsTrue (true);
        }





        TEST_METHOD(ApplyUserColorOverrides_NewExtension_AddsToMap)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            // Test adding NEW extensions not in the default set
            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L".xyz=Yellow;.abc=LightBlue;.test=Red on White");
            
            config.ApplyUserColorOverrides();
            
            // Verify new extensions were added
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".xyz"));
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".abc"));
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".test"));
            
            Assert::AreEqual ((WORD)FC_Yellow, config.m_mapExtensionToTextAttr[L".xyz"]);
            Assert::AreEqual ((WORD)FC_LightBlue, config.m_mapExtensionToTextAttr[L".abc"]);
            Assert::AreEqual ((WORD)(FC_Red | BC_White), config.m_mapExtensionToTextAttr[L".test"]);
        }





        TEST_METHOD(ApplyUserColorOverrides_OverrideExistingExtension_Overrides)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            // .cpp defaults to FC_LightGreen in s_rgTextAttrs
            Assert::AreEqual ((WORD)FC_LightGreen, config.m_mapExtensionToTextAttr[L".cpp"]);
            
            // Override it
            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L".cpp=Red on Yellow");
            config.ApplyUserColorOverrides();
            
            // Verify override took effect
            WORD expected = FC_Red | BC_Yellow;
            Assert::AreEqual (expected, config.m_mapExtensionToTextAttr[L".cpp"]);
        }





        TEST_METHOD(ApplyUserColorOverrides_MultipleSemicolons_ParsesCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L".a=Red;.b=Blue;.c=Green;.d=Yellow");
            config.ApplyUserColorOverrides();
            
            Assert::AreEqual ((WORD)FC_Red, config.m_mapExtensionToTextAttr[L".a"]);
            Assert::AreEqual ((WORD)FC_Blue, config.m_mapExtensionToTextAttr[L".b"]);
            Assert::AreEqual ((WORD)FC_Green, config.m_mapExtensionToTextAttr[L".c"]);
            Assert::AreEqual ((WORD)FC_Yellow, config.m_mapExtensionToTextAttr[L".d"]);
        }





        TEST_METHOD(ApplyUserColorOverrides_InvalidEntriesMixed_IgnoresInvalidKeepsValid)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            // Mix valid and invalid entries
            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L".cpp=Yellow;InvalidEntry;.h=Blue;NoEquals;=NoKey");
            config.ApplyUserColorOverrides();
            
            // Valid entries should be applied
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".cpp"));
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".h"));
            Assert::AreEqual ((WORD)FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::AreEqual ((WORD)FC_Blue, config.m_mapExtensionToTextAttr[L".h"]);
        }





        TEST_METHOD(ApplyUserColorOverrides_InvalidBackgroundColor_IgnoresEntry)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);

            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L".cpp=White on Bluepickles");
            config.ApplyUserColorOverrides();

            // Malformed entry should be ignored entirely - .cpp keeps its default color
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".cpp"));
            Assert::AreEqual ((WORD)FC_LightGreen, config.m_mapExtensionToTextAttr[L".cpp"]);

            // Invalid background should produce a validation issue
            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::IsTrue (result.errors.size() == 1);
        }





        TEST_METHOD(ApplyUserColorOverrides_BlackForegroundOnMagenta_IsAccepted)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);

            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L".cpp=Black on Magenta");
            config.ApplyUserColorOverrides();

            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".cpp"));
            Assert::AreEqual ((WORD)(FC_Black | BC_Magenta), config.m_mapExtensionToTextAttr[L".cpp"]);

            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::IsTrue (result.errors.size() == 0);
        }





        TEST_METHOD(ApplyUserColorOverrides_SameForegroundAndBackground_IgnoresEntry)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);

            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L".cpp=Blue on Blue");
            config.ApplyUserColorOverrides();

            // Same fore/back should be ignored - .cpp keeps its default color
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".cpp"));
            Assert::AreEqual ((WORD)FC_LightGreen, config.m_mapExtensionToTextAttr[L".cpp"]);

            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::AreEqual (size_t(1), result.errors.size());
            Assert::AreEqual (wstring(L"Foreground and background colors are the same"), result.errors[0].message);
        }





        TEST_METHOD(ApplyUserColorOverrides_BlackOnBlack_IgnoresEntry)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);

            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L".txt=Black on Black");
            config.ApplyUserColorOverrides();

            // Black on Black should be ignored - .txt keeps its default color
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".txt"));
            Assert::AreEqual ((WORD)FC_White, config.m_mapExtensionToTextAttr[L".txt"]);

            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::AreEqual (size_t(1), result.errors.size());
        }





        TEST_METHOD(Initialize_DefaultFileAttributeColors_HiddenAndEncryptedPresent)
        {
            ConfigProbe config;

            config.ClearEnvVar (TCDIR_ENV_VAR_NAME);
            config.Initialize(FC_LightGrey);

            Assert::IsTrue (config.m_mapFileAttributesTextAttr.contains(FILE_ATTRIBUTE_HIDDEN));
            Assert::AreEqual ((WORD)FC_DarkGrey, config.m_mapFileAttributesTextAttr[FILE_ATTRIBUTE_HIDDEN].m_wAttr);
            Assert::IsTrue (config.m_mapFileAttributesTextAttr[FILE_ATTRIBUTE_HIDDEN].m_source == CConfig::EAttributeSource::Default);

            Assert::IsTrue (config.m_mapFileAttributesTextAttr.contains(FILE_ATTRIBUTE_ENCRYPTED));
            Assert::AreEqual ((WORD)FC_LightGreen, config.m_mapFileAttributesTextAttr[FILE_ATTRIBUTE_ENCRYPTED].m_wAttr);
            Assert::IsTrue (config.m_mapFileAttributesTextAttr[FILE_ATTRIBUTE_ENCRYPTED].m_source == CConfig::EAttributeSource::Default);
        }





        TEST_METHOD(ApplyUserColorOverrides_FileAttributeOverride_ParsedAndStored)
        {
            ConfigProbe config;

            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L"AtTr:h=Red");
            config.Initialize(FC_LightGrey);

            Assert::IsTrue (config.m_mapFileAttributesTextAttr.contains(FILE_ATTRIBUTE_HIDDEN));
            Assert::AreEqual ((WORD)FC_Red, config.m_mapFileAttributesTextAttr[FILE_ATTRIBUTE_HIDDEN].m_wAttr);
            Assert::IsTrue (config.m_mapFileAttributesTextAttr[FILE_ATTRIBUTE_HIDDEN].m_source == CConfig::EAttributeSource::Environment);

            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::IsTrue (result.errors.size() == 0);
        }





        TEST_METHOD(GetTextAttrForFile_FileAttributeColor_WinsOverDirectoryAndExtension)
        {
            ConfigProbe config;
            WIN32_FIND_DATA wfd = { 0 };
            WORD expected = static_cast<WORD>(FC_White | BC_Blue);

            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L"attr:H=White;.cpp=Red");
            config.Initialize(static_cast<WORD>(FC_LightGrey | BC_Blue));

            wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN;
            wcscpy_s(wfd.cFileName, L"somedir");
            Assert::AreEqual (expected, config.GetTextAttrForFile(wfd));

            wfd.dwFileAttributes = FILE_ATTRIBUTE_HIDDEN;
            wcscpy_s(wfd.cFileName, L"foo.cpp");
            Assert::AreEqual (expected, config.GetTextAttrForFile(wfd));
        }





        TEST_METHOD(GetTextAttrForFile_MultipleAttributeColors_UsesFixedPriorityOrder)
        {
            ConfigProbe config;
            WIN32_FIND_DATA wfd = { 0 };

            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L"attr:s=Green;attr:h=Red");
            config.Initialize(static_cast<WORD>(FC_LightGrey | BC_Blue));

            wfd.dwFileAttributes = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
            wcscpy_s(wfd.cFileName, L"foo.txt");

            // PSHERC0TA precedence: System (S) has higher priority than Hidden (H)
            Assert::AreEqual (static_cast<WORD>(FC_Green | BC_Blue), config.GetTextAttrForFile(wfd));
        }





        TEST_METHOD(ProcessColorOverrideEntry_UppercaseExtension_ConvertedToLowercase)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessColorOverrideEntry(L".CPP=Yellow"sv);
            
            // Should be stored as lowercase
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".cpp"));
            Assert::IsFalse (config.m_mapExtensionToTextAttr.contains(L".CPP"));
            Assert::AreEqual ((WORD)FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
        }





        TEST_METHOD(ProcessColorOverrideEntry_MixedCaseExtension_ConvertedToLowercase)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessColorOverrideEntry(L".CpP=Blue"sv);
            config.ProcessColorOverrideEntry(L".HpP=Red"sv);
            
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".cpp"));
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".hpp"));
        }





        TEST_METHOD(IntegrationTest_ComplexEnvironmentString_AllEntriesProcessed)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            // Complex real-world scenario
            config.SetEnvVar (TCDIR_ENV_VAR_NAME, 
                L".cpp=LightGreen;.h=Yellow on Blue;.txt=White;.log=LightRed on Black;.xml=Cyan");
            
            config.ApplyUserColorOverrides();
            
            // Verify all entries
            Assert::AreEqual ((WORD)FC_LightGreen,            config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::AreEqual ((WORD)(FC_Yellow | BC_Blue),    config.m_mapExtensionToTextAttr[L".h"]);
            Assert::AreEqual ((WORD)FC_White,                 config.m_mapExtensionToTextAttr[L".txt"]);
            Assert::AreEqual ((WORD)(FC_LightRed | BC_Black), config.m_mapExtensionToTextAttr[L".log"]);
            Assert::AreEqual ((WORD)FC_Cyan,                  config.m_mapExtensionToTextAttr[L".xml"]);
        }





        TEST_METHOD(IntegrationTest_EmptyEnvironmentVariable_NoError)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L"");
            config.ApplyUserColorOverrides();
            
            // Should complete without error
            Assert::IsTrue (true);
        }





        TEST_METHOD(IntegrationTest_TrailingSemicolon_HandledCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L".cpp=Yellow;.h=Blue;");
            config.ApplyUserColorOverrides();
            
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".cpp"));
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".h"));
        }





        //
        // ProcessFileExtensionOverride tests
        //

        TEST_METHOD(ProcessFileExtensionOverride_LowercaseExtension_StoresCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessFileExtensionOverride(L".cpp"sv, FC_Yellow);
            
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".cpp"));
            Assert::AreEqual ((WORD)FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
        }





        TEST_METHOD(ProcessFileExtensionOverride_UppercaseExtension_ConvertsToLowercase)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessFileExtensionOverride(L".CPP"sv, FC_Red);
            
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".cpp"));
            Assert::IsFalse (config.m_mapExtensionToTextAttr.contains(L".CPP"));
            Assert::AreEqual ((WORD)FC_Red, config.m_mapExtensionToTextAttr[L".cpp"]);
        }





        TEST_METHOD(ProcessFileExtensionOverride_MixedCaseExtension_ConvertsToLowercase)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessFileExtensionOverride(L".CpP"sv, FC_Blue);
            config.ProcessFileExtensionOverride(L".HpP"sv, FC_Green);
            
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".cpp"));
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains(L".hpp"));
            Assert::AreEqual ((WORD)FC_Blue, config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::AreEqual ((WORD)FC_Green, config.m_mapExtensionToTextAttr[L".hpp"]);
        }





        TEST_METHOD(ProcessFileExtensionOverride_WithBackground_StoresComplete)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            WORD colorAttr = FC_Yellow | BC_Blue;
            config.ProcessFileExtensionOverride(L".txt"sv, colorAttr);
            
            Assert::AreEqual (colorAttr, config.m_mapExtensionToTextAttr[L".txt"]);
        }





        TEST_METHOD(ProcessFileExtensionOverride_MultipleExtensions_AllStored)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessFileExtensionOverride(L".cpp"sv, FC_LightGreen);
            config.ProcessFileExtensionOverride(L".h"sv, FC_LightBlue);
            config.ProcessFileExtensionOverride(L".txt"sv, FC_White);
            
            Assert::AreEqual ((WORD)FC_LightGreen, config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::AreEqual ((WORD)FC_LightBlue, config.m_mapExtensionToTextAttr[L".h"]);
            Assert::AreEqual ((WORD)FC_White, config.m_mapExtensionToTextAttr[L".txt"]);
        }





        //
        // ProcessDisplayAttributeOverride tests
        //

        TEST_METHOD(ProcessDisplayAttributeOverride_DateAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            WORD originalDate = config.m_rgAttributes[CConfig::EAttribute::Date];
            config.ProcessDisplayAttributeOverride(L'D', FC_Yellow, L"D=Yellow");
            
            Assert::AreEqual ((WORD)FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::Date]);
            Assert::AreNotEqual (originalDate, config.m_rgAttributes[CConfig::EAttribute::Date]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_TimeAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'T', FC_Cyan, L"T=Cyan");
            
            Assert::AreEqual ((WORD)FC_Cyan, config.m_rgAttributes[CConfig::EAttribute::Time]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_SizeAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'S', FC_Magenta, L"S=Magenta");
            
            Assert::AreEqual ((WORD)FC_Magenta, config.m_rgAttributes[CConfig::EAttribute::Size]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_DirectoryAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'R', FC_Green, L"R=Green");
            
            Assert::AreEqual ((WORD)FC_Green, config.m_rgAttributes[CConfig::EAttribute::Directory]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_FileAttributePresentAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'A', FC_LightRed, L"A=LightRed");
            
            Assert::AreEqual ((WORD)FC_LightRed, config.m_rgAttributes[CConfig::EAttribute::FileAttributePresent]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_FileAttributeNotPresentAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'-', FC_DarkGrey, L"-=DarkGrey");
            
            Assert::AreEqual ((WORD)FC_DarkGrey, config.m_rgAttributes[CConfig::EAttribute::FileAttributeNotPresent]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_InformationAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'I', FC_White, L"I=White");
            
            Assert::AreEqual ((WORD)FC_White, config.m_rgAttributes[CConfig::EAttribute::Information]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_InformationHighlightAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'H', FC_Yellow, L"H=Yellow");
            
            Assert::AreEqual ((WORD)FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::InformationHighlight]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_ErrorAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'E', FC_Red, L"E=Red");
            
            Assert::AreEqual ((WORD)FC_Red, config.m_rgAttributes[CConfig::EAttribute::Error]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_DefaultAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'F', FC_Blue, L"F=Blue");
            
            Assert::AreEqual ((WORD)FC_Blue, config.m_rgAttributes[CConfig::EAttribute::Default]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_LowercaseChar_WorksCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'd', FC_Yellow, L"d=Yellow");
            config.ProcessDisplayAttributeOverride(L't', FC_Cyan, L"t=Cyan");
            
            Assert::AreEqual ((WORD)FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::Date]);
            Assert::AreEqual ((WORD)FC_Cyan, config.m_rgAttributes[CConfig::EAttribute::Time]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_WithBackground_StoresComplete)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            WORD colorAttr = FC_White | BC_Blue;
            config.ProcessDisplayAttributeOverride(L'D', colorAttr, L"D=White on Blue");
            
            Assert::AreEqual (colorAttr, config.m_rgAttributes[CConfig::EAttribute::Date]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_InvalidChar_DoesNothing)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            WORD originalDate = config.m_rgAttributes[CConfig::EAttribute::Date];
            
            config.ProcessDisplayAttributeOverride(L'X', FC_Yellow, L"X=Yellow");
            config.ProcessDisplayAttributeOverride(L'Z', FC_Red, L"Z=Red");
            config.ProcessDisplayAttributeOverride(L'1', FC_Green, L"1=Green");
            
            // Date should remain unchanged
            Assert::AreEqual (originalDate, config.m_rgAttributes[CConfig::EAttribute::Date]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_AllValidChars_AllWork)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            // Test all valid characters
            config.ProcessDisplayAttributeOverride(L'D', FC_Black, L"D=Black");
            config.ProcessDisplayAttributeOverride(L'T', FC_Blue, L"T=Blue");
            config.ProcessDisplayAttributeOverride(L'A', FC_Green, L"A=Green");
            config.ProcessDisplayAttributeOverride(L'-', FC_Cyan, L"-=Cyan");
            config.ProcessDisplayAttributeOverride(L'S', FC_Red, L"S=Red");
            config.ProcessDisplayAttributeOverride(L'R', FC_Magenta, L"R=Magenta");
            config.ProcessDisplayAttributeOverride(L'I', FC_Brown, L"I=Brown");
            config.ProcessDisplayAttributeOverride(L'H', FC_LightGrey, L"H=LightGrey");
            config.ProcessDisplayAttributeOverride(L'E', FC_DarkGrey, L"E=DarkGrey");
            config.ProcessDisplayAttributeOverride(L'F', FC_LightBlue, L"F=LightBlue");
            
            Assert::AreEqual ((WORD)FC_Black, config.m_rgAttributes[CConfig::EAttribute::Date]);
            Assert::AreEqual ((WORD)FC_Blue, config.m_rgAttributes[CConfig::EAttribute::Time]);
            Assert::AreEqual ((WORD)FC_Green, config.m_rgAttributes[CConfig::EAttribute::FileAttributePresent]);
            Assert::AreEqual ((WORD)FC_Cyan, config.m_rgAttributes[CConfig::EAttribute::FileAttributeNotPresent]);
            Assert::AreEqual ((WORD)FC_Red, config.m_rgAttributes[CConfig::EAttribute::Size]);
            Assert::AreEqual ((WORD)FC_Magenta, config.m_rgAttributes[CConfig::EAttribute::Directory]);
            Assert::AreEqual ((WORD)FC_Brown, config.m_rgAttributes[CConfig::EAttribute::Information]);
            Assert::AreEqual ((WORD)FC_LightGrey, config.m_rgAttributes[CConfig::EAttribute::InformationHighlight]);
            Assert::AreEqual ((WORD)FC_DarkGrey, config.m_rgAttributes[CConfig::EAttribute::Error]);
            Assert::AreEqual ((WORD)FC_LightBlue, config.m_rgAttributes[CConfig::EAttribute::Default]);
        }





        //
        // Integration tests for mixed extensions and display attributes
        //

        TEST_METHOD(IntegrationTest_MixedExtensionsAndAttributes_AllProcessed)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.SetEnvVar (TCDIR_ENV_VAR_NAME, 
                L"D=LightGreen;S=Yellow;.cpp=White on Blue;T=Cyan;.h=Red");
            
            config.ApplyUserColorOverrides();
            
            // Verify display attributes
            Assert::AreEqual ((WORD)FC_LightGreen, config.m_rgAttributes[CConfig::EAttribute::Date]);
            Assert::AreEqual ((WORD)FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::Size]);
            Assert::AreEqual ((WORD)FC_Cyan, config.m_rgAttributes[CConfig::EAttribute::Time]);
            
            // Verify file extensions
            Assert::AreEqual ((WORD)(FC_White | BC_Blue), config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::AreEqual ((WORD)FC_Red, config.m_mapExtensionToTextAttr[L".h"]);
        }





        TEST_METHOD(IntegrationTest_AllDisplayAttributes_ComplexScenario)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.SetEnvVar (TCDIR_ENV_VAR_NAME, 
                L"D=Red;T=Brown;A=Cyan;-=DarkGrey;S=Yellow;R=LightBlue on Black;I=White;H=LightCyan;E=LightRed;F=Green");
            
            config.ApplyUserColorOverrides();
            
            Assert::AreEqual ((WORD)FC_Red, config.m_rgAttributes[CConfig::EAttribute::Date]);
            Assert::AreEqual ((WORD)FC_Brown, config.m_rgAttributes[CConfig::EAttribute::Time]);
            Assert::AreEqual ((WORD)FC_Cyan, config.m_rgAttributes[CConfig::EAttribute::FileAttributePresent]);
            Assert::AreEqual ((WORD)FC_DarkGrey, config.m_rgAttributes[CConfig::EAttribute::FileAttributeNotPresent]);
            Assert::AreEqual ((WORD)FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::Size]);
            Assert::AreEqual ((WORD)(FC_LightBlue | BC_Black), config.m_rgAttributes[CConfig::EAttribute::Directory]);
            Assert::AreEqual ((WORD)FC_White, config.m_rgAttributes[CConfig::EAttribute::Information]);
            Assert::AreEqual ((WORD)FC_LightCyan, config.m_rgAttributes[CConfig::EAttribute::InformationHighlight]);
            Assert::AreEqual ((WORD)FC_LightRed, config.m_rgAttributes[CConfig::EAttribute::Error]);
            Assert::AreEqual ((WORD)FC_Green, config.m_rgAttributes[CConfig::EAttribute::Default]);
        }





        TEST_METHOD(IntegrationTest_MixedCaseAttributesAndExtensions_AllProcessed)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.SetEnvVar (TCDIR_ENV_VAR_NAME, 
                L"d=Yellow;.CPP=Red;T=Blue;.H=Green");
            
            config.ApplyUserColorOverrides();
            
            Assert::AreEqual ((WORD)FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::Date]);
            Assert::AreEqual ((WORD)FC_Blue, config.m_rgAttributes[CConfig::EAttribute::Time]);
            Assert::AreEqual ((WORD)FC_Red, config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::AreEqual ((WORD)FC_Green, config.m_mapExtensionToTextAttr[L".h"]);
        }





        TEST_METHOD(IntegrationTest_DisplayAttributesWithInvalidEntries_ValidOnesProcessed)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            WORD originalError = config.m_rgAttributes[CConfig::EAttribute::Error];
            
            config.SetEnvVar (TCDIR_ENV_VAR_NAME, 
                L"D=Yellow;X=Blue;S=Red;InvalidEntry;T=Green");
            
            config.ApplyUserColorOverrides();
            
            // Valid entries should be processed
            Assert::AreEqual ((WORD)FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::Date]);
            Assert::AreEqual ((WORD)FC_Red, config.m_rgAttributes[CConfig::EAttribute::Size]);
            Assert::AreEqual ((WORD)FC_Green, config.m_rgAttributes[CConfig::EAttribute::Time]);
            
            // Error attribute should be unchanged (X is invalid, no E was set)
            Assert::AreEqual (originalError, config.m_rgAttributes[CConfig::EAttribute::Error]);
        }





        //
        // ErrorInfo struct population tests
        //

        TEST_METHOD(ErrorInfo_InvalidEntryFormat_PopulatesCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.SetEnvVar(TCDIR_ENV_VAR_NAME, L"NoEqualsSign");
            config.ApplyUserColorOverrides();
            
            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::AreEqual (size_t(1), result.errors.size());
            
            const auto& error = result.errors[0];
            Assert::AreEqual (wstring(L"Invalid entry format (expected key = value)"), error.message);
            Assert::AreEqual (wstring(L"NoEqualsSign"), error.entry);
            Assert::AreEqual (wstring(L"NoEqualsSign"), error.invalidText);
            Assert::AreEqual (size_t(0), error.invalidTextOffset);
        }





        TEST_METHOD(ErrorInfo_InvalidForegroundColor_PopulatesCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.SetEnvVar(TCDIR_ENV_VAR_NAME, L".cpp=Purplish");
            config.ApplyUserColorOverrides();
            
            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::AreEqual (size_t(1), result.errors.size());
            
            const auto& error = result.errors[0];
            Assert::AreEqual (wstring(L"Invalid foreground color"), error.message);
            Assert::AreEqual (wstring(L".cpp=Purplish"), error.entry);
            Assert::AreEqual (wstring(L"Purplish"), error.invalidText);
            Assert::AreEqual (size_t(5), error.invalidTextOffset);  // ".cpp=" = 5 chars
        }





        TEST_METHOD(ErrorInfo_InvalidBackgroundColor_PopulatesCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.SetEnvVar(TCDIR_ENV_VAR_NAME, L".cpp=White on Purplish");
            config.ApplyUserColorOverrides();
            
            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::AreEqual (size_t(1), result.errors.size());
            
            const auto& error = result.errors[0];
            Assert::AreEqual (wstring(L"Invalid background color"), error.message);
            Assert::AreEqual (wstring(L".cpp=White on Purplish"), error.entry);
            Assert::AreEqual (wstring(L"Purplish"), error.invalidText);
            Assert::AreEqual (size_t(14), error.invalidTextOffset);  // ".cpp=White on " = 14 chars
        }





        TEST_METHOD(ErrorInfo_InvalidKey_PopulatesCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.SetEnvVar(TCDIR_ENV_VAR_NAME, L"InvalidKey=Yellow");
            config.ApplyUserColorOverrides();
            
            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::AreEqual (size_t(1), result.errors.size());
            
            const auto& error = result.errors[0];
            Assert::AreEqual (wstring(L"Invalid key (expected single character, .extension, or attr:x)"), error.message);
            Assert::AreEqual (wstring(L"InvalidKey=Yellow"), error.entry);
            Assert::AreEqual (wstring(L"InvalidKey"), error.invalidText);
            Assert::AreEqual (size_t(0), error.invalidTextOffset);
        }





        TEST_METHOD(ErrorInfo_InvalidDisplayAttributeChar_PopulatesCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.SetEnvVar(TCDIR_ENV_VAR_NAME, L"Q=Yellow");
            config.ApplyUserColorOverrides();
            
            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::AreEqual (size_t(1), result.errors.size());
            
            const auto& error = result.errors[0];
            Assert::AreEqual (wstring(L"Invalid display attribute character (valid: D,T,A,-,S,R,I,H,E,F,O)"), error.message);
            Assert::AreEqual (wstring(L"Q=Yellow"), error.entry);
            Assert::AreEqual (wstring(L"Q"), error.invalidText);
            Assert::AreEqual (size_t(0), error.invalidTextOffset);
        }





        TEST_METHOD(ErrorInfo_InvalidFileAttributeChar_PopulatesCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.SetEnvVar(TCDIR_ENV_VAR_NAME, L"attr:Z=Yellow");
            config.ApplyUserColorOverrides();
            
            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::AreEqual (size_t(1), result.errors.size());
            
            const auto& error = result.errors[0];
            Assert::AreEqual (wstring(L"Invalid file attribute character (expected R, H, S, A, T, E, C, P or 0)"), error.message);
            Assert::AreEqual (wstring(L"attr:Z=Yellow"), error.entry);
            Assert::AreEqual (wstring(L"Z"), error.invalidText);
            Assert::AreEqual (size_t(5), error.invalidTextOffset);  // "attr:" = 5 chars
        }





        TEST_METHOD(ErrorInfo_TildeAlignment_InvalidTextAtOffset)
        {
            // This test verifies that invalidTextOffset + invalidText.length() gives the 
            // correct range within entry for tilde alignment
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.SetEnvVar(TCDIR_ENV_VAR_NAME, L".cpp=White on Badcolor");
            config.ApplyUserColorOverrides();
            
            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::AreEqual (size_t(1), result.errors.size());
            
            const auto& error = result.errors[0];
            
            // Verify the invalid text is exactly at the offset position in entry
            wstring extracted = error.entry.substr(error.invalidTextOffset, error.invalidText.length());
            Assert::AreEqual (error.invalidText, extracted);
        }
    };





    TEST_CLASS(ConfigSwitchOverrideTests)
    {
    public:
        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }





        TEST_METHOD(ProcessSwitchOverride_W_SetsWideListing)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.ProcessSwitchOverride (L"w");

            Assert::IsTrue (config.m_fWideListing.has_value());
            Assert::IsTrue (config.m_fWideListing.value());
        }





        TEST_METHOD(ProcessSwitchOverride_WUppercase_SetsWideListing)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.ProcessSwitchOverride (L"W");

            Assert::IsTrue (config.m_fWideListing.has_value());
            Assert::IsTrue (config.m_fWideListing.value());
        }





        TEST_METHOD(ProcessSwitchOverride_wLowercase_SetsWideListing)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.ProcessSwitchOverride (L"w");

            Assert::IsTrue (config.m_fWideListing.has_value());
            Assert::IsTrue (config.m_fWideListing.value());
        }





        TEST_METHOD(ProcessSwitchOverride_WMinus_DisablesWideListing)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.ProcessSwitchOverride (L"W-");

            Assert::IsTrue (config.m_fWideListing.has_value());
            Assert::IsFalse (config.m_fWideListing.value());
        }





        TEST_METHOD(ProcessSwitchOverride_S_SetsRecurse)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.ProcessSwitchOverride (L"S");

            Assert::IsTrue (config.m_fRecurse.has_value());
            Assert::IsTrue (config.m_fRecurse.value());
        }





        TEST_METHOD(ProcessSwitchOverride_P_SetsPerfTimer)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.ProcessSwitchOverride (L"P");

            Assert::IsTrue (config.m_fPerfTimer.has_value());
            Assert::IsTrue (config.m_fPerfTimer.value());
        }





        TEST_METHOD(ProcessSwitchOverride_M_SetsMultiThreaded)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.ProcessSwitchOverride (L"M");

            Assert::IsTrue (config.m_fMultiThreaded.has_value());
            Assert::IsTrue (config.m_fMultiThreaded.value());
        }





        TEST_METHOD(ProcessSwitchOverride_MMinus_DisablesMultiThreaded)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.ProcessSwitchOverride (L"M-");

            Assert::IsTrue (config.m_fMultiThreaded.has_value());
            Assert::IsFalse (config.m_fMultiThreaded.value());
        }




        TEST_METHOD(ProcessSwitchOverride_Owner_SetsShowOwner)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.ProcessSwitchOverride (L"Owner");

            Assert::IsTrue (config.m_fShowOwner.has_value());
            Assert::IsTrue (config.m_fShowOwner.value());
        }




        TEST_METHOD(ProcessSwitchOverride_owner_lowercase_SetsShowOwner)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.ProcessSwitchOverride (L"owner");

            Assert::IsTrue (config.m_fShowOwner.has_value());
            Assert::IsTrue (config.m_fShowOwner.value());
        }




        TEST_METHOD(ProcessSwitchOverride_Streams_SetsShowStreams)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.ProcessSwitchOverride (L"Streams");

            Assert::IsTrue (config.m_fShowStreams.has_value());
            Assert::IsTrue (config.m_fShowStreams.value());
        }




        TEST_METHOD(ProcessSwitchOverride_streams_lowercase_SetsShowStreams)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.ProcessSwitchOverride (L"streams");

            Assert::IsTrue (config.m_fShowStreams.has_value());
            Assert::IsTrue (config.m_fShowStreams.value());
        }




        TEST_METHOD(ProcessSwitchOverride_InvalidSwitch_AddsError)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.ProcessSwitchOverride (L"x");

            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::AreEqual (size_t (1), result.errors.size());
            Assert::AreEqual (wstring (L"Invalid switch (expected W, S, P, M, B, Owner, or Streams)"), result.errors[0].message);
        }





        TEST_METHOD(ProcessSwitchOverride_TooLong_AddsError)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.ProcessSwitchOverride (L"abc");

            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::AreEqual (size_t (1), result.errors.size());
            Assert::AreEqual (wstring (L"Invalid switch (expected W, S, P, M, B, Owner, or Streams)"), result.errors[0].message);
        }





        TEST_METHOD(ProcessSwitchOverride_InvalidLongSwitch_AddsError)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.ProcessSwitchOverride (L"invalid");

            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::AreEqual (size_t (1), result.errors.size());
            Assert::AreEqual (wstring (L"Invalid switch (expected W, S, P, M, B, Owner, or Streams)"), result.errors[0].message);
        }





        TEST_METHOD(ProcessColorOverrideEntry_SwitchWithSlash_RejectsPrefix)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.ProcessColorOverrideEntry (L"/w");

            // Prefixed switches should be rejected
            Assert::IsFalse (config.m_fWideListing.has_value());
            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::AreEqual (size_t (1), result.errors.size());
            Assert::AreEqual (wstring (L"Switch prefixes (/, -, --) are not allowed in env var"), result.errors[0].message);
        }





        TEST_METHOD(ProcessColorOverrideEntry_SwitchWithDash_RejectsPrefix)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.ProcessColorOverrideEntry (L"-s");

            // Prefixed switches should be rejected
            Assert::IsFalse (config.m_fRecurse.has_value());
            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::AreEqual (size_t (1), result.errors.size());
            Assert::AreEqual (wstring (L"Switch prefixes (/, -, --) are not allowed in env var"), result.errors[0].message);
        }





        TEST_METHOD(EnvVar_MixedSwitchesAndColors_ParsesAll)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L"W;S;D=Yellow;.cpp=LightGreen");
            config.ApplyUserColorOverrides();

            // Check switches
            Assert::IsTrue (config.m_fWideListing.has_value());
            Assert::IsTrue (config.m_fWideListing.value());
            Assert::IsTrue (config.m_fRecurse.has_value());
            Assert::IsTrue (config.m_fRecurse.value());

            // Check colors
            Assert::AreEqual ((WORD) FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::Date]);
            Assert::AreEqual ((WORD) FC_LightGreen, config.m_mapExtensionToTextAttr[L".cpp"]);
        }





        TEST_METHOD(EnvVar_SwitchWithDisable_ParsesCorrectly)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L"M-;W");
            config.ApplyUserColorOverrides();

            Assert::IsTrue (config.m_fMultiThreaded.has_value());
            Assert::IsFalse (config.m_fMultiThreaded.value());
            Assert::IsTrue (config.m_fWideListing.has_value());
            Assert::IsTrue (config.m_fWideListing.value());
        }





        TEST_METHOD(EnvVar_DashPrefixSwitch_RejectsPrefix)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L"-w;-p");
            config.ApplyUserColorOverrides();

            // Prefixed switches should not be set
            Assert::IsFalse (config.m_fWideListing.has_value());
            Assert::IsFalse (config.m_fPerfTimer.has_value());

            // Should have errors
            CConfig::ValidationResult result = config.ValidateEnvironmentVariable();
            Assert::AreEqual (size_t (2), result.errors.size());
        }





        TEST_METHOD(DefaultSwitchValues_AreNotSet)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            Assert::IsFalse (config.m_fWideListing.has_value());
            Assert::IsFalse (config.m_fRecurse.has_value());
            Assert::IsFalse (config.m_fPerfTimer.has_value());
            Assert::IsFalse (config.m_fMultiThreaded.has_value());
        }




        TEST_METHOD(EnvVar_PrefixFreeSwitch_ParsesCorrectly)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L"W;S;P");
            config.ApplyUserColorOverrides();

            Assert::IsTrue (config.m_fWideListing.has_value());
            Assert::IsTrue (config.m_fWideListing.value());
            Assert::IsTrue (config.m_fRecurse.has_value());
            Assert::IsTrue (config.m_fRecurse.value());
            Assert::IsTrue (config.m_fPerfTimer.has_value());
            Assert::IsTrue (config.m_fPerfTimer.value());
        }




        TEST_METHOD(EnvVar_PrefixFreeSwitchDisable_ParsesCorrectly)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L"M-;W");
            config.ApplyUserColorOverrides();

            Assert::IsTrue (config.m_fMultiThreaded.has_value());
            Assert::IsFalse (config.m_fMultiThreaded.value());
            Assert::IsTrue (config.m_fWideListing.has_value());
            Assert::IsTrue (config.m_fWideListing.value());
        }




        TEST_METHOD(EnvVar_PrefixFreeOwner_ParsesCorrectly)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L"Owner;Streams");
            config.ApplyUserColorOverrides();

            Assert::IsTrue (config.m_fShowOwner.has_value());
            Assert::IsTrue (config.m_fShowOwner.value());
            Assert::IsTrue (config.m_fShowStreams.has_value());
            Assert::IsTrue (config.m_fShowStreams.value());
        }




        TEST_METHOD(EnvVar_AllPrefixFree_ParsesCorrectly)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L"W;S;Owner;D=Yellow");
            config.ApplyUserColorOverrides();

            Assert::IsTrue (config.m_fWideListing.has_value());
            Assert::IsTrue (config.m_fWideListing.value());
            Assert::IsTrue (config.m_fRecurse.has_value());
            Assert::IsTrue (config.m_fRecurse.value());
            Assert::IsTrue (config.m_fShowOwner.has_value());
            Assert::IsTrue (config.m_fShowOwner.value());
            Assert::AreEqual ((WORD) FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::Date]);
        }




        TEST_METHOD(EnvVar_Icons_SetsIconsTrue)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L"Icons");
            config.ApplyUserColorOverrides();

            Assert::IsTrue (config.m_fIcons.has_value());
            Assert::IsTrue (config.m_fIcons.value());
        }




        TEST_METHOD(EnvVar_IconsDisable_SetsIconsFalse)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L"Icons-");
            config.ApplyUserColorOverrides();

            Assert::IsTrue  (config.m_fIcons.has_value());
            Assert::IsFalse (config.m_fIcons.value());
        }




        TEST_METHOD(EnvVar_IconsWithOtherSwitches_ParsesCorrectly)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L"Icons;W;.cpp=Green");
            config.ApplyUserColorOverrides();

            Assert::IsTrue  (config.m_fIcons.has_value());
            Assert::IsTrue  (config.m_fIcons.value());
            Assert::IsTrue  (config.m_fWideListing.has_value());
            Assert::IsTrue  (config.m_fWideListing.value());
            Assert::AreEqual ((WORD) FC_Green, config.m_mapExtensionToTextAttr[L".cpp"]);
        }




        TEST_METHOD(EnvVar_IconsIsRecognizedSwitch)
        {
            // Verify IsSwitchName recognizes Icons and Icons-
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            // Icons without equals should be treated as a switch, not an error
            config.SetEnvVar (TCDIR_ENV_VAR_NAME, L"Icons");
            config.ApplyUserColorOverrides();

            auto result = config.ValidateEnvironmentVariable();
            Assert::IsFalse (result.hasIssues());
        }
    };





    TEST_CLASS(ConfigIconPrecedenceTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        //
        // Test that extension icon map is populated from defaults
        //

        TEST_METHOD(InitializeExtensionToIconMap_CppExtension_HasCppIcon)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            Assert::IsTrue  (config.m_mapExtensionToIcon.contains (L".cpp"));
            Assert::AreEqual (static_cast<char32_t>(NfIcon::CustomCpp), config.m_mapExtensionToIcon[L".cpp"]);
        }




        TEST_METHOD(InitializeExtensionToIconMap_AllDefaultsLoaded)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            Assert::AreEqual (g_cDefaultExtensionIcons, config.m_mapExtensionToIcon.size());
        }




        //
        // Test that well-known directory icon map is populated from defaults
        //

        TEST_METHOD(InitializeWellKnownDirToIconMap_GitDir_HasGitIcon)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            Assert::IsTrue  (config.m_mapWellKnownDirToIcon.contains (L".git"));
            Assert::AreEqual (static_cast<char32_t>(NfIcon::CustomFolderGit), config.m_mapWellKnownDirToIcon[L".git"]);
        }




        TEST_METHOD(InitializeWellKnownDirToIconMap_AllDefaultsLoaded)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            Assert::AreEqual (g_cDefaultWellKnownDirIcons, config.m_mapWellKnownDirToIcon.size());
        }




        //
        // GetDisplayStyleForFile — precedence tests
        //

        TEST_METHOD(GetDisplayStyle_PlainCppFile_ReturnsCppIcon)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            WIN32_FIND_DATA wfd = { 0 };
            wfd.dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
            wcscpy_s (wfd.cFileName, L"main.cpp");

            auto style = config.GetDisplayStyleForFile (wfd);

            Assert::AreEqual (static_cast<char32_t>(NfIcon::CustomCpp), style.m_iconCodePoint);
            Assert::IsFalse  (style.m_fIconSuppressed);
        }




        TEST_METHOD(GetDisplayStyle_PlainDirectory_ReturnsDirectoryColor)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            WIN32_FIND_DATA wfd = { 0 };
            wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
            wcscpy_s (wfd.cFileName, L"somedir");

            auto style = config.GetDisplayStyleForFile (wfd);

            Assert::AreEqual (static_cast<WORD>(FC_LightBlue), style.m_wTextAttr);
            Assert::AreEqual (static_cast<char32_t>(NfIcon::CustomFolder), style.m_iconCodePoint);
        }




        TEST_METHOD(GetDisplayStyle_WellKnownDir_ReturnsSpecificIcon)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            WIN32_FIND_DATA wfd = { 0 };
            wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
            wcscpy_s (wfd.cFileName, L".git");

            auto style = config.GetDisplayStyleForFile (wfd);

            Assert::AreEqual (static_cast<char32_t>(NfIcon::CustomFolderGit), style.m_iconCodePoint);
        }




        TEST_METHOD(GetDisplayStyle_HiddenCppFile_HiddenColorLocks)
        {
            // Hidden attribute has a color override (DarkGrey) that should win
            // over the extension color, but icon should still come from extension
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            WIN32_FIND_DATA wfd = { 0 };
            wfd.dwFileAttributes = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_ARCHIVE;
            wcscpy_s (wfd.cFileName, L"secret.cpp");

            auto style = config.GetDisplayStyleForFile (wfd);

            // Color is locked by HIDDEN attribute override (DarkGrey)
            Assert::AreEqual (static_cast<WORD>(FC_DarkGrey), style.m_wTextAttr);
            // Icon falls through to extension since no attribute icon override exists
            Assert::AreEqual (static_cast<char32_t>(NfIcon::CustomCpp), style.m_iconCodePoint);
        }




        TEST_METHOD(GetDisplayStyle_UnknownExtension_ReturnsFileDefault)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            WIN32_FIND_DATA wfd = { 0 };
            wfd.dwFileAttributes = FILE_ATTRIBUTE_ARCHIVE;
            wcscpy_s (wfd.cFileName, L"data.xyz");

            auto style = config.GetDisplayStyleForFile (wfd);

            Assert::AreEqual (static_cast<char32_t>(NfIcon::FaFile), style.m_iconCodePoint);
        }




        TEST_METHOD(GetDisplayStyle_HiddenDirectory_HiddenColorWithDirIcon)
        {
            // Hidden dir: attribute color (DarkGrey) wins, but icon comes from
            // directory fallback since no attribute icon is set
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            WIN32_FIND_DATA wfd = { 0 };
            wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN;
            wcscpy_s (wfd.cFileName, L".hidden");

            auto style = config.GetDisplayStyleForFile (wfd);

            Assert::AreEqual (static_cast<WORD>(FC_DarkGrey), style.m_wTextAttr);
            Assert::AreEqual (static_cast<char32_t>(NfIcon::CustomFolder), style.m_iconCodePoint);
        }




        TEST_METHOD(GetDisplayStyle_HiddenGitDir_HiddenColorGitIcon)
        {
            // .git directory that is also hidden:
            // Color locked by hidden attribute, icon from well-known dir
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            WIN32_FIND_DATA wfd = { 0 };
            wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
            wcscpy_s (wfd.cFileName, L".git");

            auto style = config.GetDisplayStyleForFile (wfd);

            // System has higher precedence than Hidden in PSHERC0TA, but
            // no system color override exists by default, so hidden wins
            Assert::AreEqual (static_cast<WORD>(FC_DarkGrey), style.m_wTextAttr);
            // Well-known dir icon
            Assert::AreEqual (static_cast<char32_t>(NfIcon::CustomFolderGit), style.m_iconCodePoint);
        }




        TEST_METHOD(GetDisplayStyle_AttributeIconOverride_LocksIcon)
        {
            // When an attribute has both color and icon overrides, both lock
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            // Manually add an attribute icon override for HIDDEN
            config.m_mapFileAttributeToIcon[FILE_ATTRIBUTE_HIDDEN] = NfIcon::OctFileBinary;

            WIN32_FIND_DATA wfd = { 0 };
            wfd.dwFileAttributes = FILE_ATTRIBUTE_HIDDEN;
            wcscpy_s (wfd.cFileName, L"secret.cpp");

            auto style = config.GetDisplayStyleForFile (wfd);

            Assert::AreEqual (static_cast<WORD>(FC_DarkGrey), style.m_wTextAttr);
            Assert::AreEqual (static_cast<char32_t>(NfIcon::OctFileBinary), style.m_iconCodePoint);
        }




        TEST_METHOD(GetDisplayStyle_SymlinkDir_ReturnsSymlinkIcon)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            WIN32_FIND_DATA wfd = { 0 };
            wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT;
            wfd.dwReserved0      = IO_REPARSE_TAG_SYMLINK;
            wcscpy_s (wfd.cFileName, L"link");

            auto style = config.GetDisplayStyleForFile (wfd);

            Assert::AreEqual (static_cast<char32_t>(NfIcon::CodFileSymlinkDir), style.m_iconCodePoint);
        }




        TEST_METHOD(GetDisplayStyle_JunctionDir_ReturnsJunctionIcon)
        {
            ConfigProbe config;
            config.Initialize (FC_LightGrey);

            WIN32_FIND_DATA wfd = { 0 };
            wfd.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT;
            wfd.dwReserved0      = IO_REPARSE_TAG_MOUNT_POINT;
            wcscpy_s (wfd.cFileName, L"junction");

            auto style = config.GetDisplayStyleForFile (wfd);

            Assert::AreEqual (static_cast<char32_t>(NfIcon::FaExternalLink), style.m_iconCodePoint);
        }
    };
}


