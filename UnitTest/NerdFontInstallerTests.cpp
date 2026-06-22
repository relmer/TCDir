#include "pch.h"
#include "EhmTestHelper.h"
#include "Mocks/TestConsole.h"
#include "../TCDirCore/NerdFontSystemAccess.h"
#include "../TCDirCore/WindowsTerminalSettings.h"
#include "../TCDirCore/NerdFontInstaller.h"




using namespace Microsoft::VisualStudio::CppUnitTestFramework;




////////////////////////////////////////////////////////////////////////////////
//
//  CNfSystemAccessMock
//
//  In-memory implementation of INerdFontSystemAccess for unit tests.
//  Stores registry and file state in maps.  No Win32 calls are made.
//
////////////////////////////////////////////////////////////////////////////////

class CNfSystemAccessMock : public INerdFontSystemAccess
{
public:

    //
    // In-memory registry (normalized subkey path → value name → value)
    //

    struct RegValues
    {
        unordered_map<wstring, wstring>  m_strings;
        unordered_map<wstring, DWORD>    m_dwords;
    };

    unordered_map<wstring, RegValues>  m_registry;

    //
    // Helpers for test setup and assertion
    //

    void SetupRegString (LPCWSTR pszSubKey, LPCWSTR pszValue, LPCWSTR pszStr)
    {
        m_registry[Norm (pszSubKey)].m_strings[pszValue] = pszStr;
    }

    bool RegStringExists (LPCWSTR pszSubKey, LPCWSTR pszValue) const
    {
        auto it = m_registry.find (Norm (pszSubKey));
        if (it == m_registry.end())
        {
            return false;
        }
        return it->second.m_strings.count (pszValue) > 0;
    }

    optional<wstring> GetRegString (LPCWSTR pszSubKey, LPCWSTR pszValue) const
    {
        auto itKey = m_registry.find (Norm (pszSubKey));
        if (itKey == m_registry.end())
        {
            return {};
        }
        auto itVal = itKey->second.m_strings.find (pszValue);
        if (itVal == itKey->second.m_strings.end())
        {
            return {};
        }
        return itVal->second;
    }

    optional<DWORD> GetRegDword (LPCWSTR pszSubKey, LPCWSTR pszValue) const
    {
        auto itKey = m_registry.find (Norm (pszSubKey));
        if (itKey == m_registry.end())
        {
            return {};
        }
        auto itVal = itKey->second.m_dwords.find (pszValue);
        if (itVal == itKey->second.m_dwords.end())
        {
            return {};
        }
        return itVal->second;
    }


    // --- INerdFontSystemAccess ---

    LONG OpenRegKey (HKEY, LPCWSTR pszSubKey, REGSAM, HKEY & hKeyOut) override
    {
        wstring norm = Norm (pszSubKey);
        if (m_registry.count (norm) == 0)
        {
            return ERROR_FILE_NOT_FOUND;
        }
        hKeyOut = Allocate (norm);
        return ERROR_SUCCESS;
    }

    LONG CreateOrOpenRegKey (HKEY, LPCWSTR pszSubKey, REGSAM, HKEY & hKeyOut) override
    {
        wstring norm = Norm (pszSubKey);
        m_registry[norm];           // default-init entry if absent
        hKeyOut = Allocate (norm);
        return ERROR_SUCCESS;
    }

    LONG QueryRegString (HKEY hKey, LPCWSTR pszValue, wstring & strOut) override
    {
        auto itH = m_handles.find (hKey);
        if (itH == m_handles.end())
        {
            return ERROR_INVALID_HANDLE;
        }
        auto itK = m_registry.find (itH->second);
        if (itK == m_registry.end())
        {
            return ERROR_FILE_NOT_FOUND;
        }
        auto itV = itK->second.m_strings.find (pszValue);
        if (itV == itK->second.m_strings.end())
        {
            return ERROR_FILE_NOT_FOUND;
        }
        strOut = itV->second;
        return ERROR_SUCCESS;
    }

    LONG SetRegString (HKEY hKey, LPCWSTR pszValue, LPCWSTR pszStr) override
    {
        auto itH = m_handles.find (hKey);
        if (itH == m_handles.end())
        {
            return ERROR_INVALID_HANDLE;
        }
        m_registry[itH->second].m_strings[pszValue] = pszStr;
        return ERROR_SUCCESS;
    }

    LONG SetRegDword (HKEY hKey, LPCWSTR pszValue, DWORD dw) override
    {
        auto itH = m_handles.find (hKey);
        if (itH == m_handles.end())
        {
            return ERROR_INVALID_HANDLE;
        }
        m_registry[itH->second].m_dwords[pszValue] = dw;
        return ERROR_SUCCESS;
    }

    void DeleteRegValue (HKEY hKey, LPCWSTR pszValue) override
    {
        auto itH = m_handles.find (hKey);
        if (itH == m_handles.end())
        {
            return;
        }
        auto itK = m_registry.find (itH->second);
        if (itK != m_registry.end())
        {
            itK->second.m_strings.erase (pszValue);
            itK->second.m_dwords.erase  (pszValue);
        }
    }

    void CloseRegKey (HKEY hKey) override
    {
        m_handles.erase (hKey);
    }


private:
    unordered_map<HKEY, wstring>  m_handles;
    intptr_t                      m_nextHandle = 1;

    static wstring Norm (LPCWSTR psz)
    {
        wstring s = psz;
        transform (s.begin(), s.end(), s.begin(), towlower);
        return s;
    }

    HKEY Allocate (const wstring & strNormKey)
    {
        HKEY h = reinterpret_cast<HKEY>(m_nextHandle++);
        m_handles[h] = strNormKey;
        return h;
    }
};




namespace UnitTest
{
    TEST_CLASS(NerdFontInstallerTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }


        // ------------------------------------------------------------------ //
        // Helpers
        // ------------------------------------------------------------------ //

        static constexpr LPCWSTR kpszPwshKey =
            L"Console\\C:_Program Files_PowerShell_7_pwsh.exe";

        static constexpr LPCWSTR kpszWinPsKey =
            L"Console\\%SystemRoot%_System32_WindowsPowerShell_v1.0_powershell.exe";

        static constexpr LPCWSTR kpszCmdKey =
            L"Console\\%SystemRoot%_System32_cmd.exe";

        static constexpr LPCWSTR kpszNfFace = NerdFontConst::kpszConsoleFontFace;

        // Windows Terminal font-face name and the JSON token it appears as, sourced
        // from the single production constant so these tests track any rename.
        static inline const string kWtFaceToken = string ("\"fontFace\": \"") + NerdFontConst::kpszWtFontFaceUtf8 + "\"";
        static inline const string kWtFontJson  = "{ " + kWtFaceToken + " }";
        static inline const string kWtFaceObjToken = string ("\"face\": \"") + NerdFontConst::kpszWtFontFaceUtf8 + "\"";


        // ------------------------------------------------------------------ //
        // Windows Terminal — CWindowsTerminalSettings
        // ------------------------------------------------------------------ //

        TEST_METHOD(Wt_IsConfigured_WhenFontFacePresent_ReturnsTrue)
        {
            string strJson = kWtFontJson;

            Assert::IsTrue (CWindowsTerminalSettings::IsConfigured (strJson));
        }


        TEST_METHOD(Wt_IsConfigured_WhenFontFaceAbsent_ReturnsFalse)
        {
            string strJson = R"({ "fontFace": "Consolas" })";

            Assert::IsFalse (CWindowsTerminalSettings::IsConfigured (strJson));
        }


        TEST_METHOD(Wt_Install_UpdatesExistingFontFace)
        {
            string strJson = R"({ "fontFace": "Consolas" })";

            CWindowsTerminalSettings::ApplyFontFace (strJson, ENerdFontOperation::Install);

            Assert::IsTrue (CWindowsTerminalSettings::IsConfigured (strJson));
        }


        TEST_METHOD(Wt_Install_InjectsFontFaceIntoDefaultsWhenMissing)
        {
            string strJson =
                "{\n"
                "    \"profiles\": {\n"
                "        \"defaults\": {\n"
                "            \"elevate\": true\n"
                "        }\n"
                "    }\n"
                "}";

            CWindowsTerminalSettings::ApplyFontFace (strJson, ENerdFontOperation::Install);

            Assert::IsTrue (CWindowsTerminalSettings::IsConfigured (strJson));
        }


        TEST_METHOD(Wt_Install_DoesNotTouchProfileFontFaceBeforeDefaults)
        {
            string strJson =
                "{\n"
                "    \"profiles\": {\n"
                "        \"list\": [\n"
                "            {\n"
                "                \"name\": \"PowerShell\",\n"
                "                \"fontFace\": \"Cascadia Code\"\n"
                "            }\n"
                "        ],\n"
                "        \"defaults\": {\n"
                "            \"elevate\": true\n"
                "        }\n"
                "    }\n"
                "}";

            CWindowsTerminalSettings::ApplyFontFace (strJson, ENerdFontOperation::Install);

            Assert::IsTrue (CWindowsTerminalSettings::IsConfigured (strJson));
            Assert::IsTrue (strJson.find ("\"fontFace\": \"Cascadia Code\"") != string::npos);
            Assert::IsTrue (strJson.find (kWtFaceObjToken) != string::npos);
        }


        TEST_METHOD(Wt_Uninstall_ClearsFontFace)
        {
            string strJson = kWtFontJson;

            CWindowsTerminalSettings::ApplyFontFace (strJson, ENerdFontOperation::Uninstall);

            //
            // After uninstall the font name token should be gone.
            //
            Assert::IsFalse (CWindowsTerminalSettings::IsConfigured (strJson));
        }


        TEST_METHOD(Wt_Install_PreservesCommentsAndProfileEntries)
        {
            //
            // The whole point of the parser-based splice: editing defaults.fontFace
            // must leave user comments, layout, and profile-specific fontFace
            // entries byte-for-byte intact.
            //
            string strJson =
                "{\n"
                "    // keep me\n"
                "    \"profiles\": {\n"
                "        \"defaults\": { \"fontFace\": \"Consolas\" },\n"
                "        \"list\": [ { \"name\": \"PS\", \"fontFace\": \"Cascadia Code\" } ]\n"
                "    }\n"
                "}";

            Assert::AreEqual (S_OK, CWindowsTerminalSettings::ApplyFontFace (strJson, ENerdFontOperation::Install));

            Assert::IsTrue (CWindowsTerminalSettings::IsConfigured (strJson));
            Assert::IsTrue (strJson.find ("// keep me") != string::npos);
            Assert::IsTrue (strJson.find ("\"fontFace\": \"Cascadia Code\"") != string::npos);
            Assert::IsTrue (strJson.find (kWtFaceObjToken) != string::npos);
        }


        TEST_METHOD(Wt_Install_DisablesLigatures)
        {
            string strJson =
                "{\n"
                "    \"profiles\": {\n"
                "        \"defaults\": {\n"
                "            \"elevate\": true\n"
                "        }\n"
                "    }\n"
                "}";

            Assert::AreEqual (S_OK, CWindowsTerminalSettings::ApplyFontFace (strJson, ENerdFontOperation::Install));

            Assert::IsTrue (CWindowsTerminalSettings::IsConfigured (strJson));
            Assert::IsTrue (strJson.find ("\"calt\": 0") != string::npos);
            Assert::IsTrue (strJson.find ("\"liga\": 0") != string::npos);
        }


        TEST_METHOD(Wt_Install_PreservesExistingFontSettings)
        {
            //
            // A user with a modern font object (custom size) keeps that size; we
            // only replace the face and disable ligatures.
            //
            string strJson =
                "{\n"
                "    \"profiles\": {\n"
                "        \"defaults\": {\n"
                "            \"font\": { \"face\": \"Consolas\", \"size\": 14 }\n"
                "        }\n"
                "    }\n"
                "}";

            Assert::AreEqual (S_OK, CWindowsTerminalSettings::ApplyFontFace (strJson, ENerdFontOperation::Install));

            Assert::IsTrue (CWindowsTerminalSettings::IsConfigured (strJson));
            Assert::IsTrue (strJson.find (kWtFaceObjToken) != string::npos);
            Assert::IsTrue (strJson.find ("\"size\": 14") != string::npos);
            Assert::IsTrue (strJson.find ("\"calt\": 0") != string::npos);
            Assert::IsTrue (strJson.find ("\"liga\": 0") != string::npos);
            Assert::IsTrue (strJson.find ("Consolas") == string::npos);
        }


        TEST_METHOD(Wt_Install_IsIdempotent)
        {
            string strJson =
                "{\n"
                "    \"profiles\": {\n"
                "        \"defaults\": {\n"
                "            \"elevate\": true\n"
                "        }\n"
                "    }\n"
                "}";

            Assert::AreEqual (S_OK, CWindowsTerminalSettings::ApplyFontFace (strJson, ENerdFontOperation::Install));

            string strOnce = strJson;

            Assert::AreEqual (S_OK, CWindowsTerminalSettings::ApplyFontFace (strJson, ENerdFontOperation::Install));

            // A second install must change nothing and keep the JSON valid.
            Assert::AreEqual (strOnce, strJson);
            Assert::IsTrue (CWindowsTerminalSettings::IsConfigured (strJson));
        }


        // ------------------------------------------------------------------ //
        // pwsh — ConfigureConsoleProfileKey / QueryConsoleProfileConfigured
        // ------------------------------------------------------------------ //

        TEST_METHOD(Pwsh_Install_WritesFaceNameToRegistry)
        {
            CNfSystemAccessMock mock;

            HRESULT hr = CNerdFontInstaller::ConfigureConsoleProfileKey (kpszPwshKey, ENerdFontOperation::Install, mock);

            Assert::AreEqual (S_OK, hr);
            auto strFace = mock.GetRegString (kpszPwshKey, L"FaceName");
            Assert::IsTrue  (strFace.has_value());
            Assert::AreEqual (wstring (kpszNfFace), strFace.value());
        }


        TEST_METHOD(Pwsh_Install_WritesFontFamilyAndWeight)
        {
            CNfSystemAccessMock mock;

            HRESULT hr = CNerdFontInstaller::ConfigureConsoleProfileKey (kpszPwshKey, ENerdFontOperation::Install, mock);

            Assert::AreEqual (S_OK, hr);
            Assert::IsTrue (mock.GetRegDword (kpszPwshKey, L"FontFamily").has_value());
            Assert::IsTrue (mock.GetRegDword (kpszPwshKey, L"FontWeight").has_value());
        }


        TEST_METHOD(Pwsh_Uninstall_DeletesFaceNameFromRegistry)
        {
            CNfSystemAccessMock mock;
            mock.SetupRegString (kpszPwshKey, L"FaceName", kpszNfFace);

            HRESULT hr = CNerdFontInstaller::ConfigureConsoleProfileKey (kpszPwshKey, ENerdFontOperation::Uninstall, mock);

            Assert::AreEqual (S_OK, hr);
            Assert::IsFalse (mock.RegStringExists (kpszPwshKey, L"FaceName"));
        }


        TEST_METHOD(Pwsh_Detect_ConfiguredWhenFaceNameMatchesNf)
        {
            CNfSystemAccessMock mock;
            mock.SetupRegString (kpszPwshKey, L"FaceName", kpszNfFace);

            Assert::IsTrue (CNerdFontInstaller::QueryConsoleProfileConfigured (kpszPwshKey, mock));
        }


        TEST_METHOD(Pwsh_Detect_NotConfiguredWhenFaceNameDiffers)
        {
            CNfSystemAccessMock mock;
            mock.SetupRegString (kpszPwshKey, L"FaceName", L"Consolas");

            Assert::IsFalse (CNerdFontInstaller::QueryConsoleProfileConfigured (kpszPwshKey, mock));
        }


        TEST_METHOD(Pwsh_Detect_NotConfiguredWhenKeyAbsent)
        {
            CNfSystemAccessMock mock;

            Assert::IsFalse (CNerdFontInstaller::QueryConsoleProfileConfigured (kpszPwshKey, mock));
        }


        // ------------------------------------------------------------------ //
        // Windows PowerShell
        // ------------------------------------------------------------------ //

        TEST_METHOD(WinPs_Install_WritesFaceNameToRegistry)
        {
            CNfSystemAccessMock mock;

            HRESULT hr = CNerdFontInstaller::ConfigureConsoleProfileKey (kpszWinPsKey, ENerdFontOperation::Install, mock);

            Assert::AreEqual (S_OK, hr);
            auto strFace = mock.GetRegString (kpszWinPsKey, L"FaceName");
            Assert::IsTrue  (strFace.has_value());
            Assert::AreEqual (wstring (kpszNfFace), strFace.value());
        }


        TEST_METHOD(WinPs_Uninstall_DeletesFaceNameFromRegistry)
        {
            CNfSystemAccessMock mock;
            mock.SetupRegString (kpszWinPsKey, L"FaceName", kpszNfFace);

            HRESULT hr = CNerdFontInstaller::ConfigureConsoleProfileKey (kpszWinPsKey, ENerdFontOperation::Uninstall, mock);

            Assert::AreEqual (S_OK, hr);
            Assert::IsFalse (mock.RegStringExists (kpszWinPsKey, L"FaceName"));
        }


        TEST_METHOD(WinPs_Detect_ConfiguredWhenFaceNameMatchesNf)
        {
            CNfSystemAccessMock mock;
            mock.SetupRegString (kpszWinPsKey, L"FaceName", kpszNfFace);

            Assert::IsTrue (CNerdFontInstaller::QueryConsoleProfileConfigured (kpszWinPsKey, mock));
        }


        TEST_METHOD(WinPs_Detect_NotConfiguredWhenKeyAbsent)
        {
            CNfSystemAccessMock mock;

            Assert::IsFalse (CNerdFontInstaller::QueryConsoleProfileConfigured (kpszWinPsKey, mock));
        }


        // ------------------------------------------------------------------ //
        // Command Prompt
        // ------------------------------------------------------------------ //

        TEST_METHOD(Cmd_Install_WritesFaceNameToRegistry)
        {
            CNfSystemAccessMock mock;

            HRESULT hr = CNerdFontInstaller::ConfigureConsoleProfileKey (kpszCmdKey, ENerdFontOperation::Install, mock);

            Assert::AreEqual (S_OK, hr);
            auto strFace = mock.GetRegString (kpszCmdKey, L"FaceName");
            Assert::IsTrue  (strFace.has_value());
            Assert::AreEqual (wstring (kpszNfFace), strFace.value());
        }


        TEST_METHOD(Cmd_Uninstall_DeletesFaceNameFromRegistry)
        {
            CNfSystemAccessMock mock;
            mock.SetupRegString (kpszCmdKey, L"FaceName", kpszNfFace);

            HRESULT hr = CNerdFontInstaller::ConfigureConsoleProfileKey (kpszCmdKey, ENerdFontOperation::Uninstall, mock);

            Assert::AreEqual (S_OK, hr);
            Assert::IsFalse (mock.RegStringExists (kpszCmdKey, L"FaceName"));
        }


        TEST_METHOD(Cmd_Detect_ConfiguredWhenFaceNameMatchesNf)
        {
            CNfSystemAccessMock mock;
            mock.SetupRegString (kpszCmdKey, L"FaceName", kpszNfFace);

            Assert::IsTrue (CNerdFontInstaller::QueryConsoleProfileConfigured (kpszCmdKey, mock));
        }


        TEST_METHOD(Cmd_Detect_NotConfiguredWhenKeyAbsent)
        {
            CNfSystemAccessMock mock;

            Assert::IsFalse (CNerdFontInstaller::QueryConsoleProfileConfigured (kpszCmdKey, mock));
        }
    };
}
