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




        TEST_METHOD(DetectPowerShellVersion_ReturnsValidVersion)
        {
            CProfilePathResolver resolver;
            EPowerShellVersion   eVersion = EPowerShellVersion::Unknown;
            HRESULT              hr       = resolver.DetectPowerShellVersion (eVersion);



            // The test runner may or may not be PowerShell, so we just verify no crash
            // and that we get a valid enum value
            Assert::IsTrue (SUCCEEDED (hr) || eVersion == EPowerShellVersion::Unknown);
        }




        TEST_METHOD(ResolveProfilePaths_PS7Plus_ReturnsFourPaths)
        {
            CProfilePathResolver             resolver;
            vector<SProfileLocation>         rgLocations;
            HRESULT                          hr = resolver.ResolveProfilePaths (EPowerShellVersion::PS7Plus, rgLocations);



            Assert::IsTrue (SUCCEEDED (hr));
            Assert::AreEqual (4u, static_cast<unsigned>(rgLocations.size()));

            // Verify the directory component contains "PowerShell" (not "WindowsPowerShell")
            for (const auto & loc : rgLocations)
            {
                Assert::IsTrue (loc.strResolvedPath.find (L"\\PowerShell\\") != wstring::npos);
                Assert::IsFalse (loc.strResolvedPath.find (L"\\WindowsPowerShell\\") != wstring::npos);
            }
        }




        TEST_METHOD(ResolveProfilePaths_PS51_ReturnsFourPaths)
        {
            CProfilePathResolver             resolver;
            vector<SProfileLocation>         rgLocations;
            HRESULT                          hr = resolver.ResolveProfilePaths (EPowerShellVersion::PS51, rgLocations);



            Assert::IsTrue (SUCCEEDED (hr));
            Assert::AreEqual (4u, static_cast<unsigned>(rgLocations.size()));

            // Verify the directory component contains "WindowsPowerShell"
            for (const auto & loc : rgLocations)
            {
                Assert::IsTrue (loc.strResolvedPath.find (L"\\WindowsPowerShell\\") != wstring::npos);
            }
        }




        TEST_METHOD(ResolveProfilePaths_ScopesAreCorrect)
        {
            CProfilePathResolver             resolver;
            vector<SProfileLocation>         rgLocations;
            HRESULT                          hr = resolver.ResolveProfilePaths (EPowerShellVersion::PS7Plus, rgLocations);



            Assert::IsTrue (SUCCEEDED (hr));

            Assert::AreEqual (static_cast<int>(EProfileScope::CurrentUserCurrentHost), static_cast<int>(rgLocations[0].eScope));
            Assert::AreEqual (static_cast<int>(EProfileScope::CurrentUserAllHosts),    static_cast<int>(rgLocations[1].eScope));
            Assert::AreEqual (static_cast<int>(EProfileScope::AllUsersCurrentHost),    static_cast<int>(rgLocations[2].eScope));
            Assert::AreEqual (static_cast<int>(EProfileScope::AllUsersAllHosts),       static_cast<int>(rgLocations[3].eScope));
        }




        TEST_METHOD(ResolveProfilePaths_AllUsersRequiresAdmin)
        {
            CProfilePathResolver             resolver;
            vector<SProfileLocation>         rgLocations;
            HRESULT                          hr = resolver.ResolveProfilePaths (EPowerShellVersion::PS7Plus, rgLocations);



            Assert::IsTrue (SUCCEEDED (hr));

            Assert::IsFalse (rgLocations[0].fRequiresAdmin);
            Assert::IsFalse (rgLocations[1].fRequiresAdmin);
            Assert::IsTrue  (rgLocations[2].fRequiresAdmin);
            Assert::IsTrue  (rgLocations[3].fRequiresAdmin);
        }




        TEST_METHOD(ResolveProfilePaths_VariableNamesCorrect)
        {
            CProfilePathResolver             resolver;
            vector<SProfileLocation>         rgLocations;
            HRESULT                          hr = resolver.ResolveProfilePaths (EPowerShellVersion::PS7Plus, rgLocations);



            Assert::IsTrue (SUCCEEDED (hr));

            Assert::AreEqual (L"$PROFILE.CurrentUserCurrentHost", rgLocations[0].strVariableName.c_str());
            Assert::AreEqual (L"$PROFILE.CurrentUserAllHosts",    rgLocations[1].strVariableName.c_str());
            Assert::AreEqual (L"$PROFILE.AllUsersCurrentHost",    rgLocations[2].strVariableName.c_str());
            Assert::AreEqual (L"$PROFILE.AllUsersAllHosts",       rgLocations[3].strVariableName.c_str());
        }




        TEST_METHOD(IsRunningAsAdmin_ReturnsResult)
        {
            CProfilePathResolver resolver;
            bool                 fIsAdmin = false;
            HRESULT              hr       = resolver.IsRunningAsAdmin (fIsAdmin);



            Assert::IsTrue (SUCCEEDED (hr));
            // Just verify it returns without error — actual value depends on test runner
        }
    };
}
