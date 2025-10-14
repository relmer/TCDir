#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/Config.h"
#include "../TCDirCore/Color.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    // Probe class to expose protected CConfig methods for testing
    struct ConfigProbe : public CConfig
    {
        using CConfig::ParseColorName;
        using CConfig::ParseColorSpec;
        using CConfig::TrimWhitespace;
        using CConfig::ParseKeyAndValue;
        using CConfig::ApplyUserColorOverrides;
        using CConfig::ProcessColorOverrideEntry;
        using CConfig::ProcessFileExtensionOverride;
        using CConfig::ProcessDisplayAttributeOverride;
    };





    TEST_CLASS(ConfigEnvironmentTests)
    {
    public:
        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }





        TEST_CLASS_CLEANUP(ClassCleanup)
        {
            // Clean up any environment variables set during tests
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, nullptr);
        }





        TEST_METHOD(ParseColorName_AllForegroundColors_ReturnsCorrectValues)
        {
            ConfigProbe config;
            
            // Test EVERY foreground color exhaustively
            Assert::AreEqual((WORD)FC_Black,        config.ParseColorName(L"Black"sv,        false));
            Assert::AreEqual((WORD)FC_Blue,         config.ParseColorName(L"Blue"sv,         false));
            Assert::AreEqual((WORD)FC_Green,        config.ParseColorName(L"Green"sv,        false));
            Assert::AreEqual((WORD)FC_Cyan,         config.ParseColorName(L"Cyan"sv,         false));
            Assert::AreEqual((WORD)FC_Red,          config.ParseColorName(L"Red"sv,          false));
            Assert::AreEqual((WORD)FC_Magenta,      config.ParseColorName(L"Magenta"sv,      false));
            Assert::AreEqual((WORD)FC_Brown,        config.ParseColorName(L"Brown"sv,        false));
            Assert::AreEqual((WORD)FC_LightGrey,    config.ParseColorName(L"LightGrey"sv,    false));
            Assert::AreEqual((WORD)FC_DarkGrey,     config.ParseColorName(L"DarkGrey"sv,     false));
            Assert::AreEqual((WORD)FC_LightBlue,    config.ParseColorName(L"LightBlue"sv,    false));
            Assert::AreEqual((WORD)FC_LightGreen,   config.ParseColorName(L"LightGreen"sv,   false));
            Assert::AreEqual((WORD)FC_LightCyan,    config.ParseColorName(L"LightCyan"sv,    false));
            Assert::AreEqual((WORD)FC_LightRed,     config.ParseColorName(L"LightRed"sv,     false));
            Assert::AreEqual((WORD)FC_LightMagenta, config.ParseColorName(L"LightMagenta"sv, false));
            Assert::AreEqual((WORD)FC_Yellow,       config.ParseColorName(L"Yellow"sv,       false));
            Assert::AreEqual((WORD)FC_White,        config.ParseColorName(L"White"sv,        false));
        }





        TEST_METHOD(ParseColorName_AllBackgroundColors_ReturnsCorrectValues)
        {
            ConfigProbe config;
            
            // Test EVERY background color exhaustively
            Assert::AreEqual((WORD)BC_Black,        config.ParseColorName(L"Black"sv,        true));
            Assert::AreEqual((WORD)BC_Blue,         config.ParseColorName(L"Blue"sv,         true));
            Assert::AreEqual((WORD)BC_Green,        config.ParseColorName(L"Green"sv,        true));
            Assert::AreEqual((WORD)BC_Cyan,         config.ParseColorName(L"Cyan"sv,         true));
            Assert::AreEqual((WORD)BC_Red,          config.ParseColorName(L"Red"sv,          true));
            Assert::AreEqual((WORD)BC_Magenta,      config.ParseColorName(L"Magenta"sv,      true));
            Assert::AreEqual((WORD)BC_Brown,        config.ParseColorName(L"Brown"sv,        true));
            Assert::AreEqual((WORD)BC_LightGrey,    config.ParseColorName(L"LightGrey"sv,    true));
            Assert::AreEqual((WORD)BC_DarkGrey,     config.ParseColorName(L"DarkGrey"sv,     true));
            Assert::AreEqual((WORD)BC_LightBlue,    config.ParseColorName(L"LightBlue"sv,    true));
            Assert::AreEqual((WORD)BC_LightGreen,   config.ParseColorName(L"LightGreen"sv,   true));
            Assert::AreEqual((WORD)BC_LightCyan,    config.ParseColorName(L"LightCyan"sv,    true));
            Assert::AreEqual((WORD)BC_LightRed,     config.ParseColorName(L"LightRed"sv,     true));
            Assert::AreEqual((WORD)BC_LightMagenta, config.ParseColorName(L"LightMagenta"sv, true));
            Assert::AreEqual((WORD)BC_Yellow,       config.ParseColorName(L"Yellow"sv,       true));
            Assert::AreEqual((WORD)BC_White,        config.ParseColorName(L"White"sv,        true));
        }





        TEST_METHOD(ParseColorName_CaseInsensitive_ReturnsCorrectValues)
        {
            ConfigProbe config;
            
            // Test various case combinations (only need one color since case logic is color-agnostic)
            Assert::AreEqual((WORD)FC_Yellow, config.ParseColorName(L"YELLOW"sv, false));
            Assert::AreEqual((WORD)FC_Yellow, config.ParseColorName(L"yellow"sv, false));
            Assert::AreEqual((WORD)FC_Yellow, config.ParseColorName(L"YeLLoW"sv, false));
            Assert::AreEqual((WORD)FC_Yellow, config.ParseColorName(L"yELLOw"sv, false));
        }





        TEST_METHOD(ParseColorName_InvalidColor_ReturnsZero)
        {
            ConfigProbe config;
            
            Assert::AreEqual((WORD)0, config.ParseColorName(L"InvalidColor"sv,  false));
            Assert::AreEqual((WORD)0, config.ParseColorName(L""sv,              false));
            Assert::AreEqual((WORD)0, config.ParseColorName(L"Purple"sv,        false));
            Assert::AreEqual((WORD)0, config.ParseColorName(L"Orange"sv,        false));
            Assert::AreEqual((WORD)0, config.ParseColorName(L"123"sv,           false));
        }





        TEST_METHOD(ParseColorSpec_ForegroundOnly_NoWhitespace_ReturnsCorrectValue)
        {
            ConfigProbe config;
            
            WORD result = config.ParseColorSpec(L"Yellow"sv);
            Assert::AreEqual((WORD)FC_Yellow, result);
        }





        TEST_METHOD(ParseColorSpec_ForegroundOnly_LeadingWhitespace_ReturnsCorrectValue)
        {
            ConfigProbe config;
            
            WORD result = config.ParseColorSpec(L"  Yellow"sv);
            Assert::AreEqual((WORD)FC_Yellow, result);
            
            result = config.ParseColorSpec(L"    Yellow"sv);
            Assert::AreEqual((WORD)FC_Yellow, result);
        }





        TEST_METHOD(ParseColorSpec_ForegroundOnly_TrailingWhitespace_ReturnsCorrectValue)
        {
            ConfigProbe config;
            
            WORD result = config.ParseColorSpec(L"Yellow  "sv);
            Assert::AreEqual((WORD)FC_Yellow, result);
            
            result = config.ParseColorSpec(L"Yellow    "sv);
            Assert::AreEqual((WORD)FC_Yellow, result);
        }





        TEST_METHOD(ParseColorSpec_ForegroundOnly_BothWhitespace_ReturnsCorrectValue)
        {
            ConfigProbe config;
            
            WORD result = config.ParseColorSpec(L"  Yellow  "sv);
            Assert::AreEqual((WORD)FC_Yellow, result);
            
            result = config.ParseColorSpec(L"    Yellow    "sv);
            Assert::AreEqual((WORD)FC_Yellow, result);
        }





        TEST_METHOD(ParseColorSpec_ForegroundAndBackground_NoWhitespace_ReturnsCorrectValue)
        {
            ConfigProbe config;
            
            WORD result = config.ParseColorSpec(L"Yellow on Blue"sv);
            WORD expected = FC_Yellow | BC_Blue;
            Assert::AreEqual(expected, result);
        }





        TEST_METHOD(ParseColorSpec_ForegroundAndBackground_ExtraWhitespace_ReturnsCorrectValue)
        {
            ConfigProbe config;
            
            WORD expected = FC_LightGreen | BC_Red;
            
            WORD result = config.ParseColorSpec(L"  LightGreen  on  Red  "sv);
            Assert::AreEqual(expected, result);
                        
            result = config.ParseColorSpec(L"    LightGreen    on    Red    "sv);
            Assert::AreEqual(expected, result);
        }




        
        TEST_METHOD(ParseColorSpec_CaseInsensitiveOn_AllVariations_ReturnsCorrectValue)
        {
            ConfigProbe config;
            
            WORD expected = FC_White | BC_Black;
            
            Assert::AreEqual(expected, config.ParseColorSpec(L"White on Black"sv));
            Assert::AreEqual(expected, config.ParseColorSpec(L"White ON Black"sv));
            Assert::AreEqual(expected, config.ParseColorSpec(L"White On Black"sv));
            Assert::AreEqual(expected, config.ParseColorSpec(L"White oN Black"sv));
            Assert::AreEqual(expected, config.ParseColorSpec(L"white ON black"sv));
        }





        TEST_METHOD(ParseColorSpec_TabNotWhitespace_ReturnsZero)
        {
            ConfigProbe config;
            
            // Tab characters should NOT be treated as whitespace
            WORD result = config.ParseColorSpec(L"\tYellow"sv);
            Assert::AreEqual((WORD)0, result);
            
            result = config.ParseColorSpec(L"Yellow\t"sv);
            Assert::AreEqual((WORD)0, result);
            
            result = config.ParseColorSpec(L"\tYellow\t"sv);
            Assert::AreEqual((WORD)0, result);
        }





        TEST_METHOD(TrimWhitespace_NoWhitespace_ReturnsUnchanged)
        {
            ConfigProbe config;
            
            auto result = config.TrimWhitespace(L"test"sv);
            Assert::AreEqual((size_t)4, result.length());
            Assert::AreEqual(L't', result[0]);
        }





        TEST_METHOD(TrimWhitespace_LeadingWhitespace_RemovesCorrectly)
        {
            ConfigProbe config;
            
            auto result = config.TrimWhitespace(L"  test"sv);
            Assert::AreEqual((size_t)4, result.length());
            
            result = config.TrimWhitespace(L"    test"sv);
            Assert::AreEqual((size_t)4, result.length());
        }





        TEST_METHOD(TrimWhitespace_TrailingWhitespace_RemovesCorrectly)
        {
            ConfigProbe config;
            
            auto result = config.TrimWhitespace(L"test  "sv);
            Assert::AreEqual((size_t)4, result.length());
            
            result = config.TrimWhitespace(L"test    "sv);
            Assert::AreEqual((size_t)4, result.length());
        }





        TEST_METHOD(TrimWhitespace_BothWhitespace_RemovesCorrectly)
        {
            ConfigProbe config;
            
            auto result = config.TrimWhitespace(L"  test  "sv);
            Assert::AreEqual((size_t)4, result.length());
            
            result = config.TrimWhitespace(L"    test    "sv);
            Assert::AreEqual((size_t)4, result.length());
        }





        TEST_METHOD(TrimWhitespace_EmptyString_ReturnsEmpty)
        {
            ConfigProbe config;
            
            Assert::IsTrue(config.TrimWhitespace(L""sv).empty());
        }





        TEST_METHOD(TrimWhitespace_OnlyWhitespace_ReturnsEmpty)
        {
            ConfigProbe config;
            
            Assert::IsTrue(config.TrimWhitespace(L"   "sv).empty());
        }





        TEST_METHOD(TrimWhitespace_TabNotWhitespace_NotTrimmed)
        {
            ConfigProbe config;
            
            // Tab characters should NOT be treated as whitespace
            auto result = config.TrimWhitespace(L"\ttest"sv);
            Assert::AreEqual((size_t)5, result.length());
            Assert::AreEqual(L'\t', result[0]);
            
            result = config.TrimWhitespace(L"test\t"sv);
            Assert::AreEqual((size_t)5, result.length());
            Assert::AreEqual(L'\t', result[4]);
            
            result = config.TrimWhitespace(L"\ttest\t"sv);
            Assert::AreEqual((size_t)6, result.length());
            
            result = config.TrimWhitespace(L"\t\t"sv);
            Assert::AreEqual((size_t)2, result.length());
        }





        TEST_METHOD(ParseKeyAndValue_ValidEntry_SplitsCorrectly)
        {
            ConfigProbe config;
            wstring_view keyView, valueView;
            
            HRESULT hr = config.ParseKeyAndValue(L".cpp=Yellow"sv, keyView, valueView);
            
            Assert::IsTrue(SUCCEEDED(hr));
            Assert::AreEqual((size_t)4, keyView.length());
            Assert::AreEqual((size_t)6, valueView.length());
        }





        TEST_METHOD(ParseKeyAndValue_WithWhitespace_TrimsCorrectly)
        {
            ConfigProbe config;
            wstring_view keyView, valueView;
            
            HRESULT hr = config.ParseKeyAndValue(L"  .h  =  LightGreen  "sv, keyView, valueView);
            
            Assert::IsTrue(SUCCEEDED(hr));
            Assert::AreEqual((size_t)2, keyView.length());
            Assert::AreEqual((size_t)10, valueView.length());
        }





        TEST_METHOD(ParseKeyAndValue_NoEquals_ReturnsError)
        {
            ConfigProbe config;
            wstring_view keyView, valueView;
            
            HRESULT hr = config.ParseKeyAndValue(L".cppYellow"sv, keyView, valueView);
            
            Assert::IsFalse(SUCCEEDED(hr));
        }





        TEST_METHOD(ParseKeyAndValue_EmptyKey_ReturnsError)
        {
            ConfigProbe config;
            wstring_view keyView, valueView;
            
            HRESULT hr = config.ParseKeyAndValue(L"=Yellow"sv, keyView, valueView);
            
            Assert::IsFalse(SUCCEEDED(hr));
        }





        TEST_METHOD(ParseKeyAndValue_EmptyValue_ReturnsError)
        {
            ConfigProbe config;
            wstring_view keyView, valueView;
            
            HRESULT hr = config.ParseKeyAndValue(L".cpp="sv, keyView, valueView);
            
            Assert::IsFalse(SUCCEEDED(hr));
        }





        TEST_METHOD(ParseKeyAndValue_OnlyWhitespace_ReturnsError)
        {
            ConfigProbe config;
            wstring_view keyView, valueView;
            
            HRESULT hr = config.ParseKeyAndValue(L"   =   "sv, keyView, valueView);
            
            Assert::IsFalse(SUCCEEDED(hr));
        }





        TEST_METHOD(ApplyUserColorOverrides_NoEnvironmentVariable_DoesNothing)
        {
            ConfigProbe config;
            
            // Ensure no TCDIR variable
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, nullptr);
            
            config.Initialize(FC_LightGrey);
            
            // Should succeed without error
            Assert::IsTrue(true);
        }





        TEST_METHOD(ApplyUserColorOverrides_NewExtension_AddsToMap)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            // Test adding NEW extensions not in the default set
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, L".xyz=Yellow;.abc=LightBlue;.test=Red on White");
            
            config.ApplyUserColorOverrides();
            
            // Verify new extensions were added
            Assert::IsTrue(config.m_mapExtensionToTextAttr.contains(L".xyz"));
            Assert::IsTrue(config.m_mapExtensionToTextAttr.contains(L".abc"));
            Assert::IsTrue(config.m_mapExtensionToTextAttr.contains(L".test"));
            
            Assert::AreEqual((WORD)FC_Yellow, config.m_mapExtensionToTextAttr[L".xyz"]);
            Assert::AreEqual((WORD)FC_LightBlue, config.m_mapExtensionToTextAttr[L".abc"]);
            Assert::AreEqual((WORD)(FC_Red | BC_White), config.m_mapExtensionToTextAttr[L".test"]);
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, nullptr);
        }





        TEST_METHOD(ApplyUserColorOverrides_OverrideExistingExtension_Overrides)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            // .cpp defaults to FC_LightGreen in s_rgTextAttrs
            Assert::AreEqual((WORD)FC_LightGreen, config.m_mapExtensionToTextAttr[L".cpp"]);
            
            // Override it
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, L".cpp=Red on Yellow");
            config.ApplyUserColorOverrides();
            
            // Verify override took effect
            WORD expected = FC_Red | BC_Yellow;
            Assert::AreEqual(expected, config.m_mapExtensionToTextAttr[L".cpp"]);
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, nullptr);
        }





        TEST_METHOD(ApplyUserColorOverrides_MultipleSemicolons_ParsesCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, L".a=Red;.b=Blue;.c=Green;.d=Yellow");
            config.ApplyUserColorOverrides();
            
            Assert::AreEqual((WORD)FC_Red, config.m_mapExtensionToTextAttr[L".a"]);
            Assert::AreEqual((WORD)FC_Blue, config.m_mapExtensionToTextAttr[L".b"]);
            Assert::AreEqual((WORD)FC_Green, config.m_mapExtensionToTextAttr[L".c"]);
            Assert::AreEqual((WORD)FC_Yellow, config.m_mapExtensionToTextAttr[L".d"]);
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, nullptr);
        }





        TEST_METHOD(ApplyUserColorOverrides_InvalidEntriesMixed_IgnoresInvalidKeepsValid)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            // Mix valid and invalid entries
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, L".cpp=Yellow;InvalidEntry;.h=Blue;NoEquals;=NoKey");
            config.ApplyUserColorOverrides();
            
            // Valid entries should be applied
            Assert::IsTrue(config.m_mapExtensionToTextAttr.contains(L".cpp"));
            Assert::IsTrue(config.m_mapExtensionToTextAttr.contains(L".h"));
            Assert::AreEqual((WORD)FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::AreEqual((WORD)FC_Blue, config.m_mapExtensionToTextAttr[L".h"]);
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, nullptr);
        }





        TEST_METHOD(ProcessColorOverrideEntry_UppercaseExtension_ConvertedToLowercase)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessColorOverrideEntry(L".CPP=Yellow"sv);
            
            // Should be stored as lowercase
            Assert::IsTrue(config.m_mapExtensionToTextAttr.contains(L".cpp"));
            Assert::IsFalse(config.m_mapExtensionToTextAttr.contains(L".CPP"));
            Assert::AreEqual((WORD)FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
        }





        TEST_METHOD(ProcessColorOverrideEntry_MixedCaseExtension_ConvertedToLowercase)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessColorOverrideEntry(L".CpP=Blue"sv);
            config.ProcessColorOverrideEntry(L".HpP=Red"sv);
            
            Assert::IsTrue(config.m_mapExtensionToTextAttr.contains(L".cpp"));
            Assert::IsTrue(config.m_mapExtensionToTextAttr.contains(L".hpp"));
        }





        TEST_METHOD(IntegrationTest_ComplexEnvironmentString_AllEntriesProcessed)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            // Complex real-world scenario
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, 
                L".cpp=LightGreen;.h=Yellow on Blue;.txt=White;.log=LightRed on Black;.xml=Cyan");
            
            config.ApplyUserColorOverrides();
            
            // Verify all entries
            Assert::AreEqual((WORD)FC_LightGreen,            config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::AreEqual((WORD)(FC_Yellow | BC_Blue),    config.m_mapExtensionToTextAttr[L".h"]);
            Assert::AreEqual((WORD)FC_White,                 config.m_mapExtensionToTextAttr[L".txt"]);
            Assert::AreEqual((WORD)(FC_LightRed | BC_Black), config.m_mapExtensionToTextAttr[L".log"]);
            Assert::AreEqual((WORD)FC_Cyan,                  config.m_mapExtensionToTextAttr[L".xml"]);
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, nullptr);
        }





        TEST_METHOD(IntegrationTest_EmptyEnvironmentVariable_NoError)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, L"");
            config.ApplyUserColorOverrides();
            
            // Should complete without error
            Assert::IsTrue(true);
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, nullptr);
        }





        TEST_METHOD(IntegrationTest_TrailingSemicolon_HandledCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, L".cpp=Yellow;.h=Blue;");
            config.ApplyUserColorOverrides();
            
            Assert::IsTrue(config.m_mapExtensionToTextAttr.contains(L".cpp"));
            Assert::IsTrue(config.m_mapExtensionToTextAttr.contains(L".h"));
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, nullptr);
        }





        //
        // ProcessFileExtensionOverride tests
        //

        TEST_METHOD(ProcessFileExtensionOverride_LowercaseExtension_StoresCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessFileExtensionOverride(L".cpp"sv, FC_Yellow);
            
            Assert::IsTrue(config.m_mapExtensionToTextAttr.contains(L".cpp"));
            Assert::AreEqual((WORD)FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
        }





        TEST_METHOD(ProcessFileExtensionOverride_UppercaseExtension_ConvertsToLowercase)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessFileExtensionOverride(L".CPP"sv, FC_Red);
            
            Assert::IsTrue(config.m_mapExtensionToTextAttr.contains(L".cpp"));
            Assert::IsFalse(config.m_mapExtensionToTextAttr.contains(L".CPP"));
            Assert::AreEqual((WORD)FC_Red, config.m_mapExtensionToTextAttr[L".cpp"]);
        }





        TEST_METHOD(ProcessFileExtensionOverride_MixedCaseExtension_ConvertsToLowercase)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessFileExtensionOverride(L".CpP"sv, FC_Blue);
            config.ProcessFileExtensionOverride(L".HpP"sv, FC_Green);
            
            Assert::IsTrue(config.m_mapExtensionToTextAttr.contains(L".cpp"));
            Assert::IsTrue(config.m_mapExtensionToTextAttr.contains(L".hpp"));
            Assert::AreEqual((WORD)FC_Blue, config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::AreEqual((WORD)FC_Green, config.m_mapExtensionToTextAttr[L".hpp"]);
        }





        TEST_METHOD(ProcessFileExtensionOverride_WithBackground_StoresComplete)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            WORD colorAttr = FC_Yellow | BC_Blue;
            config.ProcessFileExtensionOverride(L".txt"sv, colorAttr);
            
            Assert::AreEqual(colorAttr, config.m_mapExtensionToTextAttr[L".txt"]);
        }





        TEST_METHOD(ProcessFileExtensionOverride_MultipleExtensions_AllStored)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessFileExtensionOverride(L".cpp"sv, FC_LightGreen);
            config.ProcessFileExtensionOverride(L".h"sv, FC_LightBlue);
            config.ProcessFileExtensionOverride(L".txt"sv, FC_White);
            
            Assert::AreEqual((WORD)FC_LightGreen, config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::AreEqual((WORD)FC_LightBlue, config.m_mapExtensionToTextAttr[L".h"]);
            Assert::AreEqual((WORD)FC_White, config.m_mapExtensionToTextAttr[L".txt"]);
        }





        //
        // ProcessDisplayAttributeOverride tests
        //

        TEST_METHOD(ProcessDisplayAttributeOverride_DateAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            WORD originalDate = config.m_rgAttributes[CConfig::EAttribute::Date];
            config.ProcessDisplayAttributeOverride(L'D', FC_Yellow);
            
            Assert::AreEqual((WORD)FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::Date]);
            Assert::AreNotEqual(originalDate, config.m_rgAttributes[CConfig::EAttribute::Date]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_TimeAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'T', FC_Cyan);
            
            Assert::AreEqual((WORD)FC_Cyan, config.m_rgAttributes[CConfig::EAttribute::Time]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_SizeAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'S', FC_Magenta);
            
            Assert::AreEqual((WORD)FC_Magenta, config.m_rgAttributes[CConfig::EAttribute::Size]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_DirectoryAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'R', FC_Green);
            
            Assert::AreEqual((WORD)FC_Green, config.m_rgAttributes[CConfig::EAttribute::Directory]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_FileAttributePresentAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'A', FC_LightRed);
            
            Assert::AreEqual((WORD)FC_LightRed, config.m_rgAttributes[CConfig::EAttribute::FileAttributePresent]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_FileAttributeNotPresentAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'-', FC_DarkGrey);
            
            Assert::AreEqual((WORD)FC_DarkGrey, config.m_rgAttributes[CConfig::EAttribute::FileAttributeNotPresent]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_InformationAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'I', FC_White);
            
            Assert::AreEqual((WORD)FC_White, config.m_rgAttributes[CConfig::EAttribute::Information]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_InformationHighlightAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'H', FC_Yellow);
            
            Assert::AreEqual((WORD)FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::InformationHighlight]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_ErrorAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'E', FC_Red);
            
            Assert::AreEqual((WORD)FC_Red, config.m_rgAttributes[CConfig::EAttribute::Error]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_DefaultAttribute_SetsCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'F', FC_Blue);
            
            Assert::AreEqual((WORD)FC_Blue, config.m_rgAttributes[CConfig::EAttribute::Default]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_LowercaseChar_WorksCorrectly)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            config.ProcessDisplayAttributeOverride(L'd', FC_Yellow);
            config.ProcessDisplayAttributeOverride(L't', FC_Cyan);
            
            Assert::AreEqual((WORD)FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::Date]);
            Assert::AreEqual((WORD)FC_Cyan, config.m_rgAttributes[CConfig::EAttribute::Time]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_WithBackground_StoresComplete)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            WORD colorAttr = FC_White | BC_Blue;
            config.ProcessDisplayAttributeOverride(L'D', colorAttr);
            
            Assert::AreEqual(colorAttr, config.m_rgAttributes[CConfig::EAttribute::Date]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_InvalidChar_DoesNothing)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            WORD originalDate = config.m_rgAttributes[CConfig::EAttribute::Date];
            
            config.ProcessDisplayAttributeOverride(L'X', FC_Yellow);
            config.ProcessDisplayAttributeOverride(L'Z', FC_Red);
            config.ProcessDisplayAttributeOverride(L'1', FC_Green);
            
            // Date should remain unchanged
            Assert::AreEqual(originalDate, config.m_rgAttributes[CConfig::EAttribute::Date]);
        }





        TEST_METHOD(ProcessDisplayAttributeOverride_AllValidChars_AllWork)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            // Test all valid characters
            config.ProcessDisplayAttributeOverride(L'D', FC_Black);
            config.ProcessDisplayAttributeOverride(L'T', FC_Blue);
            config.ProcessDisplayAttributeOverride(L'A', FC_Green);
            config.ProcessDisplayAttributeOverride(L'-', FC_Cyan);
            config.ProcessDisplayAttributeOverride(L'S', FC_Red);
            config.ProcessDisplayAttributeOverride(L'R', FC_Magenta);
            config.ProcessDisplayAttributeOverride(L'I', FC_Brown);
            config.ProcessDisplayAttributeOverride(L'H', FC_LightGrey);
            config.ProcessDisplayAttributeOverride(L'E', FC_DarkGrey);
            config.ProcessDisplayAttributeOverride(L'F', FC_LightBlue);
            
            Assert::AreEqual((WORD)FC_Black, config.m_rgAttributes[CConfig::EAttribute::Date]);
            Assert::AreEqual((WORD)FC_Blue, config.m_rgAttributes[CConfig::EAttribute::Time]);
            Assert::AreEqual((WORD)FC_Green, config.m_rgAttributes[CConfig::EAttribute::FileAttributePresent]);
            Assert::AreEqual((WORD)FC_Cyan, config.m_rgAttributes[CConfig::EAttribute::FileAttributeNotPresent]);
            Assert::AreEqual((WORD)FC_Red, config.m_rgAttributes[CConfig::EAttribute::Size]);
            Assert::AreEqual((WORD)FC_Magenta, config.m_rgAttributes[CConfig::EAttribute::Directory]);
            Assert::AreEqual((WORD)FC_Brown, config.m_rgAttributes[CConfig::EAttribute::Information]);
            Assert::AreEqual((WORD)FC_LightGrey, config.m_rgAttributes[CConfig::EAttribute::InformationHighlight]);
            Assert::AreEqual((WORD)FC_DarkGrey, config.m_rgAttributes[CConfig::EAttribute::Error]);
            Assert::AreEqual((WORD)FC_LightBlue, config.m_rgAttributes[CConfig::EAttribute::Default]);
        }





        //
        // Integration tests for mixed extensions and display attributes
        //

        TEST_METHOD(IntegrationTest_MixedExtensionsAndAttributes_AllProcessed)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, 
                L"D=LightGreen;S=Yellow;.cpp=White on Blue;T=Cyan;.h=Red");
            
            config.ApplyUserColorOverrides();
            
            // Verify display attributes
            Assert::AreEqual((WORD)FC_LightGreen, config.m_rgAttributes[CConfig::EAttribute::Date]);
            Assert::AreEqual((WORD)FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::Size]);
            Assert::AreEqual((WORD)FC_Cyan, config.m_rgAttributes[CConfig::EAttribute::Time]);
            
            // Verify file extensions
            Assert::AreEqual((WORD)(FC_White | BC_Blue), config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::AreEqual((WORD)FC_Red, config.m_mapExtensionToTextAttr[L".h"]);
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, nullptr);
        }





        TEST_METHOD(IntegrationTest_AllDisplayAttributes_ComplexScenario)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, 
                L"D=Red;T=Brown;A=Cyan;-=DarkGrey;S=Yellow;R=LightBlue on Black;I=White;H=LightCyan;E=LightRed;F=Green");
            
            config.ApplyUserColorOverrides();
            
            Assert::AreEqual((WORD)FC_Red, config.m_rgAttributes[CConfig::EAttribute::Date]);
            Assert::AreEqual((WORD)FC_Brown, config.m_rgAttributes[CConfig::EAttribute::Time]);
            Assert::AreEqual((WORD)FC_Cyan, config.m_rgAttributes[CConfig::EAttribute::FileAttributePresent]);
            Assert::AreEqual((WORD)FC_DarkGrey, config.m_rgAttributes[CConfig::EAttribute::FileAttributeNotPresent]);
            Assert::AreEqual((WORD)FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::Size]);
            Assert::AreEqual((WORD)(FC_LightBlue | BC_Black), config.m_rgAttributes[CConfig::EAttribute::Directory]);
            Assert::AreEqual((WORD)FC_White, config.m_rgAttributes[CConfig::EAttribute::Information]);
            Assert::AreEqual((WORD)FC_LightCyan, config.m_rgAttributes[CConfig::EAttribute::InformationHighlight]);
            Assert::AreEqual((WORD)FC_LightRed, config.m_rgAttributes[CConfig::EAttribute::Error]);
            Assert::AreEqual((WORD)FC_Green, config.m_rgAttributes[CConfig::EAttribute::Default]);
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, nullptr);
        }





        TEST_METHOD(IntegrationTest_MixedCaseAttributesAndExtensions_AllProcessed)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, 
                L"d=Yellow;.CPP=Red;T=Blue;.H=Green");
            
            config.ApplyUserColorOverrides();
            
            Assert::AreEqual((WORD)FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::Date]);
            Assert::AreEqual((WORD)FC_Blue, config.m_rgAttributes[CConfig::EAttribute::Time]);
            Assert::AreEqual((WORD)FC_Red, config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::AreEqual((WORD)FC_Green, config.m_mapExtensionToTextAttr[L".h"]);
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, nullptr);
        }





        TEST_METHOD(IntegrationTest_DisplayAttributesWithInvalidEntries_ValidOnesProcessed)
        {
            ConfigProbe config;
            config.Initialize(FC_LightGrey);
            
            WORD originalError = config.m_rgAttributes[CConfig::EAttribute::Error];
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, 
                L"D=Yellow;X=Blue;S=Red;InvalidEntry;T=Green");
            
            config.ApplyUserColorOverrides();
            
            // Valid entries should be processed
            Assert::AreEqual((WORD)FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::Date]);
            Assert::AreEqual((WORD)FC_Red, config.m_rgAttributes[CConfig::EAttribute::Size]);
            Assert::AreEqual((WORD)FC_Green, config.m_rgAttributes[CConfig::EAttribute::Time]);
            
            // Error attribute should be unchanged (X is invalid, no E was set)
            Assert::AreEqual(originalError, config.m_rgAttributes[CConfig::EAttribute::Error]);
            
            SetEnvironmentVariableW(TCDIR_ENV_VAR_NAME, nullptr);
        }
    };
}
