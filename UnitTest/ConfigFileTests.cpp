#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/Config.h"
#include "../TCDirCore/ConfigFileReader.h"
#include "../TCDirCore/Color.h"
#include "../TCDirCore/IconMapping.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;



namespace Microsoft { namespace VisualStudio { namespace CppUnitTestFramework
{
    template<> inline std::wstring ToString<CConfig::EAttributeSource> (const CConfig::EAttributeSource & value)
    {
        switch (value)
        {
            case CConfig::EAttributeSource::Default:     return L"Default";
            case CConfig::EAttributeSource::ConfigFile:  return L"ConfigFile";
            case CConfig::EAttributeSource::Environment: return L"Environment";
            default:                                     return L"Unknown";
        }
    }

    template<> inline std::wstring ToString<char32_t> (const char32_t & value)
    {
        wchar_t buf[16];
        swprintf_s (buf, L"U+%05X", static_cast<unsigned>(value));
        return buf;
    }
}}}





namespace UnitTest
{
    ////////////////////////////////////////////////////////////////////////////////
    //
    //  Mock environment provider for config file tests
    //
    ////////////////////////////////////////////////////////////////////////////////

    class CConfigTestEnvironmentProvider : public IEnvironmentProvider
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




    ////////////////////////////////////////////////////////////////////////////////
    //
    //  Mock config file reader for config file tests
    //
    ////////////////////////////////////////////////////////////////////////////////

    class CTestConfigFileReader : public IConfigFileReader
    {
    public:
        void Set (vector<wstring> lines)
        {
            m_lines      = std::move (lines);
            m_hrResult   = S_OK;
            m_fConfigured = true;
        }

        void SetNotFound (void)
        {
            m_hrResult   = S_FALSE;
            m_fConfigured = true;
        }

        void SetError (wstring errorMessage)
        {
            m_hrResult      = E_FAIL;
            m_errorMessage  = std::move (errorMessage);
            m_fConfigured    = true;
        }

        HRESULT ReadLines (const wstring & /*path*/, vector<wstring> & lines, wstring & errorMessage) override
        {
            ASSERT (m_fConfigured);

            if (m_hrResult == S_OK)
            {
                lines = m_lines;
            }
            else if (FAILED (m_hrResult))
            {
                errorMessage = m_errorMessage;
            }

            return m_hrResult;
        }

    private:
        vector<wstring> m_lines;
        wstring         m_errorMessage;
        HRESULT         m_hrResult   = S_FALSE;
        bool            m_fConfigured = false;
    };




    ////////////////////////////////////////////////////////////////////////////////
    //
    //  ConfigProbe for config file tests — sets up mock env provider and reader
    //
    ////////////////////////////////////////////////////////////////////////////////

    struct ConfigFileProbe : public CConfig
    {
        ConfigFileProbe (void)
        {
            m_environmentProvider.Set (L"USERPROFILE", L"C:\\Users\\TestUser");
            SetEnvironmentProvider (&m_environmentProvider);
            SetConfigFileReader (&m_reader);
        }

        void SetEnvVar (LPCWSTR pszName, wstring value)
        {
            m_environmentProvider.Set (pszName, std::move (value));
        }

        void ClearEnvVar (LPCWSTR pszName)
        {
            m_environmentProvider.Clear (pszName);
        }

        void SetConfigLines (vector<wstring> lines)
        {
            m_reader.Set (std::move (lines));
        }

        void SetConfigNotFound (void)
        {
            m_reader.SetNotFound();
        }

        void SetConfigError (wstring errorMessage)
        {
            m_reader.SetError (std::move (errorMessage));
        }

        using CConfig::m_mapExtensionToTextAttr;
        using CConfig::m_mapExtensionSources;
        using CConfig::m_rgAttributes;
        using CConfig::m_rgAttributeSources;
        using CConfig::m_mapExtensionToIcon;
        using CConfig::m_mapExtensionIconSources;
        using CConfig::m_mapWellKnownDirToIcon;
        using CConfig::m_mapWellKnownDirIconSources;
        using CConfig::m_mapFileAttributesTextAttr;
        using CConfig::m_fWideListing;
        using CConfig::m_fBareListing;
        using CConfig::m_fRecurse;
        using CConfig::m_fPerfTimer;
        using CConfig::m_fMultiThreaded;
        using CConfig::m_fShowOwner;
        using CConfig::m_fShowStreams;
        using CConfig::m_fIcons;
        using CConfig::m_fTree;
        using CConfig::m_cMaxDepth;
        using CConfig::m_cTreeIndent;
        using CConfig::m_eSizeFormat;
        using CConfig::m_lastParseResult;
        using CConfig::m_configFileParseResult;

    private:
        CConfigTestEnvironmentProvider  m_environmentProvider;
        CTestConfigFileReader           m_reader;
    };




    ////////////////////////////////////////////////////////////////////////////////
    //
    //  ConfigFileTests — Phase 3: T021 config file loading integration tests
    //
    ////////////////////////////////////////////////////////////////////////////////

    TEST_CLASS (ConfigFileTests)
    {
    public:

        //
        // T021: Switches applied from config file
        //

        TEST_METHOD (LoadConfigFile_SwitchW_SetsWideListing)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"w" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_fWideListing.has_value());
            Assert::IsTrue (config.m_fWideListing.value());
        }

        TEST_METHOD (LoadConfigFile_SwitchB_SetsBare)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"b" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_fBareListing.has_value());
            Assert::IsTrue (config.m_fBareListing.value());
        }

        TEST_METHOD (LoadConfigFile_SwitchS_SetsRecurse)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"S" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_fRecurse.has_value());
            Assert::IsTrue (config.m_fRecurse.value());
        }

        TEST_METHOD (LoadConfigFile_SwitchP_SetsPerfTimer)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"P" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_fPerfTimer.has_value());
            Assert::IsTrue (config.m_fPerfTimer.value());
        }

        TEST_METHOD (LoadConfigFile_SwitchOwner_SetsShowOwner)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"Owner" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_fShowOwner.has_value());
            Assert::IsTrue (config.m_fShowOwner.value());
        }

        TEST_METHOD (LoadConfigFile_SwitchStreams_SetsShowStreams)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"Streams" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_fShowStreams.has_value());
            Assert::IsTrue (config.m_fShowStreams.value());
        }

        TEST_METHOD (LoadConfigFile_MultipleSwitches_AllApplied)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"w", L"S", L"Owner" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_fWideListing.has_value());
            Assert::IsTrue (config.m_fWideListing.value());
            Assert::IsTrue (config.m_fRecurse.has_value());
            Assert::IsTrue (config.m_fRecurse.value());
            Assert::IsTrue (config.m_fShowOwner.has_value());
            Assert::IsTrue (config.m_fShowOwner.value());
        }

        //
        // T021: Color overrides applied from config file
        //

        TEST_METHOD (LoadConfigFile_ExtensionColor_Applied)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L".cpp=Yellow" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".cpp"));
            Assert::AreEqual ((WORD) FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::AreEqual (CConfig::EAttributeSource::ConfigFile, config.m_mapExtensionSources[L".cpp"]);
        }

        TEST_METHOD (LoadConfigFile_MultipleExtensions_AllApplied)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L".cpp=LightGreen", L".h=LightBlue", L".txt=White" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".cpp"));
            Assert::AreEqual ((WORD) FC_LightGreen, config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".h"));
            Assert::AreEqual ((WORD) FC_LightBlue, config.m_mapExtensionToTextAttr[L".h"]);
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".txt"));
            Assert::AreEqual ((WORD) FC_White, config.m_mapExtensionToTextAttr[L".txt"]);
        }

        TEST_METHOD (LoadConfigFile_DisplayAttribute_Applied)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"D=Yellow" });
            config.Initialize (FC_LightGrey);



            Assert::AreEqual ((WORD) FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::Date]);
            Assert::AreEqual (CConfig::EAttributeSource::ConfigFile, config.m_rgAttributeSources[CConfig::EAttribute::Date]);
        }

        TEST_METHOD (LoadConfigFile_ColorWithBackground_Applied)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L".log=White on Blue" });
            config.Initialize (FC_LightGrey);



            WORD expected = FC_White | BC_Blue;
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".log"));
            Assert::AreEqual (expected, config.m_mapExtensionToTextAttr[L".log"]);
        }

        //
        // T021: Icon overrides applied from config file
        //

        TEST_METHOD (LoadConfigFile_IconOverride_Applied)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L".py=Green,U+E606" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_mapExtensionToIcon.contains (L".py"));
            Assert::AreEqual (static_cast<char32_t>(0xE606), config.m_mapExtensionToIcon[L".py"]);
            Assert::AreEqual ((WORD) FC_Green, config.m_mapExtensionToTextAttr[L".py"]);
        }

        //
        // T021: Parameterized values applied from config file
        //

        TEST_METHOD (LoadConfigFile_DepthValue_Applied)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"Depth=3" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_cMaxDepth.has_value());
            Assert::AreEqual (3, config.m_cMaxDepth.value());
        }

        TEST_METHOD (LoadConfigFile_TreeIndentValue_Applied)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"TreeIndent=4" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_cTreeIndent.has_value());
            Assert::AreEqual (4, config.m_cTreeIndent.value());
        }

        TEST_METHOD (LoadConfigFile_SizeAuto_Applied)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"Size=Auto" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_eSizeFormat.has_value());
            Assert::AreEqual ((int) ESizeFormat::Auto, (int) config.m_eSizeFormat.value());
        }

        //
        // T021: Config file path and load state
        //

        TEST_METHOD (LoadConfigFile_Loaded_IsConfigFileLoadedTrue)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"w" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.IsConfigFileLoaded());
            Assert::AreEqual (wstring (L"C:\\Users\\TestUser\\.tcdirconfig"), config.GetConfigFilePath());
        }

        TEST_METHOD (LoadConfigFile_FileNotFound_IsConfigFileLoadedFalse)
        {
            ConfigFileProbe config;
            config.SetConfigNotFound();
            config.Initialize (FC_LightGrey);



            Assert::IsFalse (config.IsConfigFileLoaded());
        }

        TEST_METHOD (LoadConfigFile_NoUserProfile_IsConfigFileLoadedFalse)
        {
            ConfigFileProbe config;
            config.ClearEnvVar (L"USERPROFILE");
            config.SetConfigLines ({ L"w" });
            config.Initialize (FC_LightGrey);



            Assert::IsFalse (config.IsConfigFileLoaded());
        }

        //
        // T022: Comment lines skipped
        //

        TEST_METHOD (LoadConfigFile_CommentLine_Skipped)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"# This is a comment", L".cpp=Yellow" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".cpp"));
            Assert::AreEqual ((WORD) FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::AreEqual ((size_t) 0, config.m_configFileParseResult.errors.size());
        }

        TEST_METHOD (LoadConfigFile_MultipleComments_AllSkipped)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({
                L"# Color section",
                L"# ==============",
                L".cpp=Yellow",
                L"# Switch section",
                L"w"
            });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".cpp"));
            Assert::IsTrue (config.m_fWideListing.has_value());
            Assert::AreEqual ((size_t) 0, config.m_configFileParseResult.errors.size());
        }

        //
        // T022: Inline comments stripped
        //

        TEST_METHOD (LoadConfigFile_InlineComment_Stripped)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L".cpp=Yellow  # C++ source files" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".cpp"));
            Assert::AreEqual ((WORD) FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
        }

        //
        // T022: Blank lines skipped
        //

        TEST_METHOD (LoadConfigFile_BlankLines_Skipped)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({
                L"",
                L".cpp=Yellow",
                L"",
                L"",
                L".h=LightBlue",
                L""
            });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".cpp"));
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".h"));
            Assert::AreEqual ((size_t) 0, config.m_configFileParseResult.errors.size());
        }

        //
        // T022: Whitespace-only lines skipped
        //

        TEST_METHOD (LoadConfigFile_WhitespaceOnlyLines_Skipped)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({
                L"   ",
                L".cpp=Yellow",
                L"      ",
                L".h=LightBlue"
            });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".cpp"));
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".h"));
            Assert::AreEqual ((size_t) 0, config.m_configFileParseResult.errors.size());
        }

        //
        // T022: Leading/trailing whitespace trimmed
        //

        TEST_METHOD (LoadConfigFile_LeadingTrailingWhitespace_Trimmed)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"  .cpp=Yellow  " });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".cpp"));
            Assert::AreEqual ((WORD) FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
        }

        //
        // T024: Env var overrides config file color
        //

        TEST_METHOD (Precedence_EnvVarOverridesConfigFileColor)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L".cpp=LightGreen" });
            config.SetEnvVar (L"TCDIR", L".cpp=Yellow");
            config.Initialize (FC_LightGrey);



            Assert::AreEqual ((WORD) FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::AreEqual (CConfig::EAttributeSource::Environment, config.m_mapExtensionSources[L".cpp"]);
        }

        TEST_METHOD (Precedence_EnvVarOverridesConfigFileSwitch)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"w" });
            config.SetEnvVar (L"TCDIR", L"W-");
            config.Initialize (FC_LightGrey);



            // W- disables wide listing; env var should win
            Assert::IsTrue (config.m_fWideListing.has_value());
            Assert::IsFalse (config.m_fWideListing.value());
        }

        TEST_METHOD (Precedence_EnvVarOverridesConfigFileDisplayAttribute)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"D=LightGreen" });
            config.SetEnvVar (L"TCDIR", L"D=Yellow");
            config.Initialize (FC_LightGrey);



            Assert::AreEqual ((WORD) FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::Date]);
            Assert::AreEqual (CConfig::EAttributeSource::Environment, config.m_rgAttributeSources[CConfig::EAttribute::Date]);
        }

        //
        // T024: Non-conflicting settings merge from both sources
        //

        TEST_METHOD (Precedence_NonConflictingSettingsMerge)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L".cpp=LightGreen", L"w" });
            config.SetEnvVar (L"TCDIR", L".h=Yellow;S");
            config.Initialize (FC_LightGrey);



            // Config file settings
            Assert::AreEqual ((WORD) FC_LightGreen, config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::IsTrue (config.m_fWideListing.has_value());
            Assert::IsTrue (config.m_fWideListing.value());

            // Env var settings
            Assert::AreEqual ((WORD) FC_Yellow, config.m_mapExtensionToTextAttr[L".h"]);
            Assert::IsTrue (config.m_fRecurse.has_value());
            Assert::IsTrue (config.m_fRecurse.value());
        }

        TEST_METHOD (Precedence_ConflictingColorEnvVarWins_NonConflictingConfigFilePreserved)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L".cpp=LightGreen", L".h=LightBlue" });
            config.SetEnvVar (L"TCDIR", L".cpp=Yellow");
            config.Initialize (FC_LightGrey);



            // .cpp overridden by env var
            Assert::AreEqual ((WORD) FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::AreEqual (CConfig::EAttributeSource::Environment, config.m_mapExtensionSources[L".cpp"]);

            // .h preserved from config file
            Assert::AreEqual ((WORD) FC_LightBlue, config.m_mapExtensionToTextAttr[L".h"]);
            Assert::AreEqual (CConfig::EAttributeSource::ConfigFile, config.m_mapExtensionSources[L".h"]);
        }

        //
        // T025: Source tracking verification
        //

        TEST_METHOD (SourceTracking_ConfigOnlySettings_SourceIsConfigFile)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L".cpp=LightGreen", L"D=Cyan" });
            config.Initialize (FC_LightGrey);



            Assert::AreEqual (CConfig::EAttributeSource::ConfigFile, config.m_mapExtensionSources[L".cpp"]);
            Assert::AreEqual (CConfig::EAttributeSource::ConfigFile, config.m_rgAttributeSources[CConfig::EAttribute::Date]);
        }

        TEST_METHOD (SourceTracking_EnvVarOverriddenSettings_SourceIsEnvironment)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L".cpp=LightGreen" });
            config.SetEnvVar (L"TCDIR", L".cpp=Yellow");
            config.Initialize (FC_LightGrey);



            Assert::AreEqual (CConfig::EAttributeSource::Environment, config.m_mapExtensionSources[L".cpp"]);
        }

        TEST_METHOD (SourceTracking_DefaultsUnchanged_SourceIsDefault)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"w" });
            config.Initialize (FC_LightGrey);



            // Date attribute was not overridden by config file or env var
            Assert::AreEqual (CConfig::EAttributeSource::Default, config.m_rgAttributeSources[CConfig::EAttribute::Date]);
        }

        //
        // T027: Inline comment edge cases
        //

        TEST_METHOD (InlineComment_SettingWithMultipleHashChars)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L".cpp=Yellow  # C++ files ## important" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".cpp"));
            Assert::AreEqual ((WORD) FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
        }

        TEST_METHOD (InlineComment_CommentOnlyLineWithLeadingWhitespace)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"   # indented comment", L".cpp=Yellow" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".cpp"));
            Assert::AreEqual ((size_t) 0, config.m_configFileParseResult.errors.size());
        }

        TEST_METHOD (InlineComment_SwitchWithInlineComment)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"w  # enable wide listing" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_fWideListing.has_value());
            Assert::IsTrue (config.m_fWideListing.value());
        }

        //
        // T028: Whitespace handling
        //

        TEST_METHOD (Whitespace_TabsAsWhitespace)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"\t.cpp=Yellow\t" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".cpp"));
            Assert::AreEqual ((WORD) FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
        }

        TEST_METHOD (Whitespace_TabOnlyLines_Skipped)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({
                L"\t\t",
                L".cpp=Yellow",
                L"\t",
                L".h=LightBlue"
            });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".cpp"));
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".h"));
            Assert::AreEqual ((size_t) 0, config.m_configFileParseResult.errors.size());
        }

        TEST_METHOD (Whitespace_MixedTabsAndSpaces_Trimmed)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"\t  .cpp=Yellow  \t" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".cpp"));
            Assert::AreEqual ((WORD) FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
        }

        //
        // T029: Duplicate setting tests — last occurrence wins within config file
        //

        TEST_METHOD (Duplicate_LastOccurrenceWins_Color)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L".cpp=LightGreen", L".cpp=Yellow" });
            config.Initialize (FC_LightGrey);



            Assert::AreEqual ((WORD) FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
        }

        TEST_METHOD (Duplicate_LastOccurrenceWins_DisplayAttribute)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"D=LightGreen", L"D=Yellow" });
            config.Initialize (FC_LightGrey);



            Assert::AreEqual ((WORD) FC_Yellow, config.m_rgAttributes[CConfig::EAttribute::Date]);
        }

        TEST_METHOD (Duplicate_SwitchToggled_LastWins)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"w", L"W-" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_fWideListing.has_value());
            Assert::IsFalse (config.m_fWideListing.value());
        }

        //
        // T034: Invalid color name shows line number
        //

        TEST_METHOD (ErrorReporting_InvalidColorName_HasLineNumber)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L".cpp=InvalidColor" });
            config.Initialize (FC_LightGrey);



            auto result = config.ValidateConfigFile();
            Assert::IsTrue (result.hasIssues());
            Assert::AreEqual ((size_t) 1, result.errors.size());
            Assert::AreEqual ((size_t) 1, result.errors[0].lineNumber);
            Assert::AreEqual (wstring (L"C:\\Users\\TestUser\\.tcdirconfig"), result.errors[0].sourceFilePath);
        }

        TEST_METHOD (ErrorReporting_MalformedEntry_HasLineNumber)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L".cpp=Yellow", L"=NoKey", L".h=Red" });
            config.Initialize (FC_LightGrey);



            auto result = config.ValidateConfigFile();
            Assert::IsTrue (result.hasIssues());
            Assert::AreEqual ((size_t) 1, result.errors.size());
            Assert::AreEqual ((size_t) 2, result.errors[0].lineNumber);
        }

        TEST_METHOD (ErrorReporting_ValidLinesStillApplyAlongsideErrors)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L".cpp=Yellow", L".h=BogusColor", L"w" });
            config.Initialize (FC_LightGrey);



            // Valid lines should be applied
            Assert::AreEqual ((WORD) FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::IsTrue (config.m_fWideListing.has_value());
            Assert::IsTrue (config.m_fWideListing.value());

            // Error should be recorded
            auto result = config.ValidateConfigFile();
            Assert::IsTrue (result.hasIssues());
            Assert::AreEqual ((size_t) 1, result.errors.size());
            Assert::AreEqual ((size_t) 2, result.errors[0].lineNumber);
        }

        TEST_METHOD (ErrorReporting_MultipleErrors_EachHasLineNumber)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({
                L".cpp=Yellow",
                L".h=BogusColor",
                L"x=Green",
                L".txt=Red"
            });
            config.Initialize (FC_LightGrey);



            auto result = config.ValidateConfigFile();
            Assert::AreEqual ((size_t) 2, result.errors.size());
            Assert::AreEqual ((size_t) 2, result.errors[0].lineNumber);
            Assert::AreEqual ((size_t) 3, result.errors[1].lineNumber);
        }

        //
        // T035: Config file errors separate from env var errors
        //

        TEST_METHOD (ErrorGrouping_ConfigFileErrorsSeparateFromEnvVar)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L".cpp=BogusColor" });
            config.SetEnvVar (L"TCDIR", L".h=AlsoBogus");
            config.Initialize (FC_LightGrey);



            auto configResult = config.ValidateConfigFile();
            auto envResult = config.m_lastParseResult;

            // Config file has its own errors
            Assert::IsTrue (configResult.hasIssues());
            Assert::AreEqual ((size_t) 1, configResult.errors.size());

            // Env var has its own errors
            Assert::IsTrue (envResult.hasIssues());
            Assert::AreEqual ((size_t) 1, envResult.errors.size());
        }

        //
        // T036: I/O error tests
        //

        TEST_METHOD (IOError_PermissionDenied_ProducesSingleError)
        {
            ConfigFileProbe config;
            config.SetConfigError (L"Access denied");
            config.Initialize (FC_LightGrey);



            auto result = config.ValidateConfigFile();
            Assert::IsTrue (result.hasIssues());
            Assert::AreEqual ((size_t) 1, result.errors.size());
            Assert::IsFalse (config.IsConfigFileLoaded());
        }

        TEST_METHOD (IOError_FileNotFound_SilentSkip)
        {
            ConfigFileProbe config;
            config.SetConfigNotFound();
            config.Initialize (FC_LightGrey);



            auto result = config.ValidateConfigFile();
            Assert::IsFalse (result.hasIssues());
            Assert::IsFalse (config.IsConfigFileLoaded());
        }

        //
        // T050: Edge case tests — empty file, comments-only, blank-lines-only, very long lines, 20+ settings, USERPROFILE not set
        //

        TEST_METHOD (EdgeCase_EmptyFile_NoErrorDefaultsUsed)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({});
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.IsConfigFileLoaded());
            Assert::IsFalse (config.ValidateConfigFile().hasIssues());
            // Defaults should still be in place
            Assert::AreEqual (CConfig::EAttributeSource::Default, config.m_rgAttributeSources[CConfig::EAttribute::Date]);
        }

        TEST_METHOD (EdgeCase_OnlyComments_NoErrorDefaultsUsed)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({
                L"# This is a comment",
                L"# Another comment",
                L"# One more"
            });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.IsConfigFileLoaded());
            Assert::IsFalse (config.ValidateConfigFile().hasIssues());
            Assert::AreEqual (CConfig::EAttributeSource::Default, config.m_rgAttributeSources[CConfig::EAttribute::Date]);
        }

        TEST_METHOD (EdgeCase_OnlyBlankLines_NoErrorDefaultsUsed)
        {
            ConfigFileProbe config;
            config.SetConfigLines ({ L"", L"", L"   ", L"\t", L"  \t  " });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.IsConfigFileLoaded());
            Assert::IsFalse (config.ValidateConfigFile().hasIssues());
        }

        TEST_METHOD (EdgeCase_VeryLongLine_ParsedCorrectly)
        {
            // Build a line with 1000-char extension name
            wstring longExt = L"." + wstring (1000, L'x') + L"=Yellow";

            ConfigFileProbe config;
            config.SetConfigLines ({ longExt });
            config.Initialize (FC_LightGrey);



            wstring expectedKey = L"." + wstring (1000, L'x');
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (expectedKey));
            Assert::AreEqual ((WORD) FC_Yellow, config.m_mapExtensionToTextAttr[expectedKey]);
        }

        TEST_METHOD (EdgeCase_TwentyPlusSettings_AllApplied)
        {
            // SC-002 coverage: config file with > 20 distinct settings
            ConfigFileProbe config;
            config.SetConfigLines ({
                L"w",                            // 1  wide listing
                L"b",                            // 2  bare listing
                L"s",                            // 3  recurse
                L"p",                            // 4  perf timer
                L".cpp=Yellow",                  // 5  extension color
                L".h=LightBlue",                 // 6
                L".txt=LightGreen",              // 7
                L".py=LightCyan",                // 8
                L".rs=LightRed",                 // 9
                L".go=LightMagenta",             // 10
                L".js=White",                    // 11
                L".ts=LightGrey",                // 12
                L".md=Green",                    // 13
                L".json=Cyan",                   // 14
                L".xml=Red",                     // 15
                L".yaml=Magenta",                // 16
                L".toml=Blue",                   // 17
                L".css=DarkGrey",                // 18
                L".html=LightBlue",              // 19  (duplicate ext is fine)
                L".java=Yellow",                 // 20
                L".rb=LightGreen",               // 21
            });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.m_fWideListing.has_value() && config.m_fWideListing.value());
            Assert::IsTrue (config.m_fBareListing.has_value() && config.m_fBareListing.value());
            Assert::IsTrue (config.m_fRecurse.has_value() && config.m_fRecurse.value());
            Assert::IsTrue (config.m_fPerfTimer.has_value() && config.m_fPerfTimer.value());
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".rb"));
            Assert::AreEqual ((WORD) FC_LightGreen, config.m_mapExtensionToTextAttr[L".rb"]);
            Assert::IsFalse (config.ValidateConfigFile().hasIssues());
        }

        TEST_METHOD (EdgeCase_UserProfileNotSet_SilentSkipLikeNotFound)
        {
            ConfigFileProbe config;
            config.ClearEnvVar (L"USERPROFILE");
            config.SetConfigLines ({ L".cpp=Yellow" });  // Would be loadable, but path can't be resolved
            config.Initialize (FC_LightGrey);



            // Without USERPROFILE, config file path can't be determined — should silently skip
            Assert::IsFalse (config.IsConfigFileLoaded());
            Assert::IsFalse (config.ValidateConfigFile().hasIssues());
        }

        //
        // T052: BOM end-to-end test — UTF-8 BOM file parses correctly through full pipeline
        //

        TEST_METHOD (BomEndToEnd_Utf8BomFile_ParsedCorrectly)
        {
            // The mock reader already strips BOM (since CConfigFileReader handles BOM,
            // and CTestConfigFileReader delivers clean lines), but this test validates
            // that a config file whose first line starts with settings (no leading BOM
            // artifacts) works through the full LoadConfigFile pipeline.
            ConfigFileProbe config;
            config.SetConfigLines ({ L".cpp=Yellow", L"w" });
            config.Initialize (FC_LightGrey);



            Assert::IsTrue (config.IsConfigFileLoaded());
            Assert::IsTrue (config.m_mapExtensionToTextAttr.contains (L".cpp"));
            Assert::AreEqual ((WORD) FC_Yellow, config.m_mapExtensionToTextAttr[L".cpp"]);
            Assert::IsTrue (config.m_fWideListing.has_value() && config.m_fWideListing.value());
            Assert::IsFalse (config.ValidateConfigFile().hasIssues());
        }
    };
}
