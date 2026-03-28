#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/ProfilePathResolver.h"





using namespace Microsoft::VisualStudio::CppUnitTestFramework;





namespace UnitTest
{
    TEST_CLASS(ProfilePathResolverTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests();
        }




        TEST_METHOD(MapImageName_Pwsh_ReturnsPowerShell)
        {
            Assert::AreEqual (static_cast<int>(EPowerShellVersion::PowerShell),
                              static_cast<int>(CProfilePathResolver::MapImageNameToVersion (L"pwsh.exe")));
        }




        TEST_METHOD(MapImageName_PwshUpperCase_ReturnsPowerShell)
        {
            Assert::AreEqual (static_cast<int>(EPowerShellVersion::PowerShell),
                              static_cast<int>(CProfilePathResolver::MapImageNameToVersion (L"PWSH.EXE")));
        }




        TEST_METHOD(MapImageName_Powershell_ReturnsWindowsPowerShell)
        {
            Assert::AreEqual (static_cast<int>(EPowerShellVersion::WindowsPowerShell),
                              static_cast<int>(CProfilePathResolver::MapImageNameToVersion (L"powershell.exe")));
        }




        TEST_METHOD(MapImageName_Cmd_ReturnsUnknown)
        {
            Assert::AreEqual (static_cast<int>(EPowerShellVersion::Unknown),
                              static_cast<int>(CProfilePathResolver::MapImageNameToVersion (L"cmd.exe")));
        }




        TEST_METHOD(MapImageName_Explorer_ReturnsUnknown)
        {
            Assert::AreEqual (static_cast<int>(EPowerShellVersion::Unknown),
                              static_cast<int>(CProfilePathResolver::MapImageNameToVersion (L"explorer.exe")));
        }




        TEST_METHOD(BuildProfileLocations_PowerShell_ReturnsFourPaths)
        {
            vector<SProfileLocation> rgLocations;

            CProfilePathResolver::BuildProfileLocations (L"C:\\Users\\test\\Documents", L"C:\\Program Files\\PowerShell\\7",
                                                          EPowerShellVersion::PowerShell, rgLocations);



            Assert::AreEqual (4u, static_cast<unsigned>(rgLocations.size()));

            for (const auto & loc : rgLocations)
            {
                Assert::IsTrue (loc.strResolvedPath.find (L"\\PowerShell\\") != wstring::npos);
                Assert::IsFalse (loc.strResolvedPath.find (L"\\WindowsPowerShell\\") != wstring::npos);
            }
        }




        TEST_METHOD(BuildProfileLocations_WindowsPowerShell_UsesCorrectDir)
        {
            vector<SProfileLocation> rgLocations;

            CProfilePathResolver::BuildProfileLocations (L"C:\\Users\\test\\Documents", L"C:\\Windows\\System32\\WindowsPowerShell\\v1.0",
                                                          EPowerShellVersion::WindowsPowerShell, rgLocations);



            Assert::AreEqual (4u, static_cast<unsigned>(rgLocations.size()));

            // CurrentUser paths use Documents\WindowsPowerShell\ subdirectory
            Assert::IsTrue (rgLocations[0].strResolvedPath.find (L"\\WindowsPowerShell\\") != wstring::npos);
            Assert::IsTrue (rgLocations[1].strResolvedPath.find (L"\\WindowsPowerShell\\") != wstring::npos);
        }




        TEST_METHOD(BuildProfileLocations_ScopesAreCorrect)
        {
            vector<SProfileLocation> rgLocations;

            CProfilePathResolver::BuildProfileLocations (L"C:\\Users\\test\\Documents", L"C:\\Program Files\\PowerShell\\7",
                                                          EPowerShellVersion::PowerShell, rgLocations);



            Assert::AreEqual (static_cast<int>(EProfileScope::CurrentUserCurrentHost), static_cast<int>(rgLocations[0].eScope));
            Assert::AreEqual (static_cast<int>(EProfileScope::CurrentUserAllHosts),    static_cast<int>(rgLocations[1].eScope));
            Assert::AreEqual (static_cast<int>(EProfileScope::AllUsersCurrentHost),    static_cast<int>(rgLocations[2].eScope));
            Assert::AreEqual (static_cast<int>(EProfileScope::AllUsersAllHosts),       static_cast<int>(rgLocations[3].eScope));
        }




        TEST_METHOD(BuildProfileLocations_AllUsersRequiresAdmin)
        {
            vector<SProfileLocation> rgLocations;

            CProfilePathResolver::BuildProfileLocations (L"C:\\Users\\test\\Documents", L"C:\\Program Files\\PowerShell\\7",
                                                          EPowerShellVersion::PowerShell, rgLocations);



            Assert::IsFalse (rgLocations[0].fRequiresAdmin);
            Assert::IsFalse (rgLocations[1].fRequiresAdmin);
            Assert::IsTrue  (rgLocations[2].fRequiresAdmin);
            Assert::IsTrue  (rgLocations[3].fRequiresAdmin);
        }




        TEST_METHOD(BuildProfileLocations_VariableNamesCorrect)
        {
            vector<SProfileLocation> rgLocations;

            CProfilePathResolver::BuildProfileLocations (L"C:\\Users\\test\\Documents", L"C:\\Program Files\\PowerShell\\7",
                                                          EPowerShellVersion::PowerShell, rgLocations);



            Assert::AreEqual (L"$PROFILE.CurrentUserCurrentHost", rgLocations[0].strVariableName.c_str());
            Assert::AreEqual (L"$PROFILE.CurrentUserAllHosts",    rgLocations[1].strVariableName.c_str());
            Assert::AreEqual (L"$PROFILE.AllUsersCurrentHost",    rgLocations[2].strVariableName.c_str());
            Assert::AreEqual (L"$PROFILE.AllUsersAllHosts",       rgLocations[3].strVariableName.c_str());
        }




        TEST_METHOD(BuildProfileLocations_FullPathsCorrect_PowerShell)
        {
            vector<SProfileLocation> rgLocations;

            CProfilePathResolver::BuildProfileLocations (L"C:\\Users\\test\\Documents", L"C:\\Program Files\\PowerShell\\7",
                                                          EPowerShellVersion::PowerShell, rgLocations);



            Assert::AreEqual (L"C:\\Users\\test\\Documents\\PowerShell\\Microsoft.PowerShell_profile.ps1", rgLocations[0].strResolvedPath.c_str());
            Assert::AreEqual (L"C:\\Users\\test\\Documents\\PowerShell\\profile.ps1",                      rgLocations[1].strResolvedPath.c_str());
            Assert::AreEqual (L"C:\\Program Files\\PowerShell\\7\\Microsoft.PowerShell_profile.ps1",         rgLocations[2].strResolvedPath.c_str());
            Assert::AreEqual (L"C:\\Program Files\\PowerShell\\7\\profile.ps1",                              rgLocations[3].strResolvedPath.c_str());
        }




        TEST_METHOD(BuildProfileLocations_FullPathsCorrect_WindowsPowerShell)
        {
            vector<SProfileLocation> rgLocations;

            CProfilePathResolver::BuildProfileLocations (L"C:\\Users\\test\\Documents", L"C:\\Windows\\System32\\WindowsPowerShell\\v1.0",
                                                          EPowerShellVersion::WindowsPowerShell, rgLocations);



            Assert::AreEqual (L"C:\\Users\\test\\Documents\\WindowsPowerShell\\Microsoft.PowerShell_profile.ps1", rgLocations[0].strResolvedPath.c_str());
            Assert::AreEqual (L"C:\\Users\\test\\Documents\\WindowsPowerShell\\profile.ps1",                      rgLocations[1].strResolvedPath.c_str());
            Assert::AreEqual (L"C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\Microsoft.PowerShell_profile.ps1", rgLocations[2].strResolvedPath.c_str());
            Assert::AreEqual (L"C:\\Windows\\System32\\WindowsPowerShell\\v1.0\\profile.ps1",                      rgLocations[3].strResolvedPath.c_str());
        }




        TEST_METHOD(BuildProfileLocations_FExistsDefaultsFalse)
        {
            vector<SProfileLocation> rgLocations;

            CProfilePathResolver::BuildProfileLocations (L"C:\\Fake\\Documents", L"C:\\\\Fake\\\\PSHome",
                                                          EPowerShellVersion::PowerShell, rgLocations);



            for (const auto & loc : rgLocations)
            {
                Assert::IsFalse (loc.fExists);
            }
        }




        TEST_METHOD(BuildProfileLocations_OneDriveRedirectedPath)
        {
            //
            // Verify path construction works with OneDrive-redirected Documents
            //

            vector<SProfileLocation> rgLocations;

            CProfilePathResolver::BuildProfileLocations (L"C:\\Users\\test\\OneDrive\\Documents", L"C:\\Program Files\\PowerShell\\7",
                                                          EPowerShellVersion::PowerShell, rgLocations);



            Assert::AreEqual (L"C:\\Users\\test\\OneDrive\\Documents\\PowerShell\\profile.ps1",
                              rgLocations[1].strResolvedPath.c_str());
        }
    };
}
