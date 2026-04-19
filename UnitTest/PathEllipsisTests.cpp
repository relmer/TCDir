#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/PathEllipsis.h"



using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{




    TEST_CLASS(PathEllipsisTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests ();
        }




        // T011: Short path passthrough — AzureVpn.exe target

        TEST_METHOD(EllipsizePath_ShortPath_Passthrough)
        {
            wstring target = L"C:\\Windows\\system32\\SystemUWPLauncher.exe";

            SEllipsizedPath result = EllipsizePath (target, 80);

            Assert::IsFalse (result.fTruncated, L"Short path should not be truncated");
            Assert::AreEqual (target.c_str (), result.prefix.c_str ());
            Assert::IsTrue (result.suffix.empty ());
        }




        // T012: Priority 1 — two dirs + leaf dir + filename
        // notepad.exe → C:\Program Files\WindowsApps\...\Notepad\Notepad.exe

        TEST_METHOD(EllipsizePath_Priority1_TwoDirs_LeafDir_Filename)
        {
            wstring target = L"C:\\Program Files\\WindowsApps\\Microsoft.WindowsNotepad_11.2601.26.0_arm64__8wekyb3d8bbwe\\Notepad\\Notepad.exe";

            // Width large enough for: "C:\Program Files\" + "…" + "\Notepad\Notepad.exe" = 18 + 1 + 20 = 39
            SEllipsizedPath result = EllipsizePath (target, 45);

            Assert::IsTrue (result.fTruncated, L"Should be truncated");
            Assert::AreEqual (L"C:\\Program Files\\", result.prefix.c_str ());
            Assert::AreEqual (L"\\Notepad\\Notepad.exe", result.suffix.c_str ());
        }




        // T013: Priority 2 — two dirs + filename
        // wingetcreate.exe → C:\Program Files\...\WingetCreateCLI.exe

        TEST_METHOD(EllipsizePath_Priority2_TwoDirs_Filename)
        {
            wstring target = L"C:\\Program Files\\WindowsApps\\Microsoft.WindowsPackageManagerManifestCreator_1.12.8.0_x64__8wekyb3d8bbwe\\WingetCreateCLI\\WingetCreateCLI.exe";

            // Width large enough for: "C:\Program Files\" + "…" + "\WingetCreateCLI.exe" = 18 + 1 + 21 = 40
            // But NOT large enough for priority 1 (which would also need "\WingetCreateCLI\" = +18)
            SEllipsizedPath result = EllipsizePath (target, 42);

            Assert::IsTrue (result.fTruncated, L"Should be truncated");
            Assert::AreEqual (L"C:\\Program Files\\", result.prefix.c_str ());
            Assert::AreEqual (L"\\WingetCreateCLI.exe", result.suffix.c_str ());
        }




        // T014: Priority 3 — one dir + filename (crafted data)

        TEST_METHOD(EllipsizePath_Priority3_OneDir_Filename)
        {
            wstring target = L"C:\\LongDirName\\SubDir1\\SubDir2\\target.exe";

            // Priority 2 would be: "C:\LongDirName\" + "…" + "\target.exe" = 15 + 1 + 11 = 27
            // Priority 3 would be: "C:\" + "…" + "\target.exe" = 3 + 1 + 11 = 15
            // Use width that fits priority 3 but NOT priority 2
            SEllipsizedPath result = EllipsizePath (target, 20);

            Assert::IsTrue (result.fTruncated, L"Should be truncated");
            Assert::AreEqual (L"C:\\", result.prefix.c_str ());
            Assert::AreEqual (L"\\target.exe", result.suffix.c_str ());
        }




        // T015: Priority 4 — leaf filename only

        TEST_METHOD(EllipsizePath_Priority4_LeafOnly)
        {
            wstring target = L"C:\\Program Files\\WindowsApps\\Microsoft.WindowsPackageManagerManifestCreator_1.12.8.0_x64__8wekyb3d8bbwe\\WingetCreateCLI\\WingetCreateCLI.exe";

            // Too narrow for even priority 3: "C:\" + "…" + "\WingetCreateCLI.exe" = 3 + 1 + 21 = 25
            // But wide enough for leaf with trailing ellipsis
            SEllipsizedPath result = EllipsizePath (target, 15);

            Assert::IsTrue (result.fTruncated, L"Truncated leaf should be flagged as truncated");
            Assert::AreEqual (size_t (14), result.prefix.length (), L"Prefix should be availableWidth - 1 for trailing ellipsis");
            Assert::IsTrue (result.suffix.empty (), L"Suffix should be empty for trailing ellipsis");
        }




        // T016: Two-component path — never truncated

        TEST_METHOD(EllipsizePath_TwoComponents_NeverTruncated)
        {
            wstring target = L"C:\\file.exe";

            // At width 15, the full path (11 chars) fits
            SEllipsizedPath result = EllipsizePath (target, 15);

            Assert::IsFalse (result.fTruncated);
            Assert::AreEqual (target.c_str (), result.prefix.c_str ());
        }



        TEST_METHOD(EllipsizePath_TwoComponents_NarrowWidth_LeafWithTrailingEllipsis)
        {
            wstring target = L"C:\\file.exe";

            // At width 5, leaf "file.exe" (8 chars) > 5, so trailing ellipsis: 4 chars + …
            SEllipsizedPath result = EllipsizePath (target, 5);

            Assert::IsTrue (result.fTruncated, L"Truncated leaf should be flagged");
            Assert::AreEqual (size_t (4), result.prefix.length ());
        }




        // T017: Relative path — passthrough

        TEST_METHOD(EllipsizePath_RelativePath_Passthrough)
        {
            wstring target = L"..\\..\\shared\\config.yml";

            SEllipsizedPath result = EllipsizePath (target, 80);

            Assert::IsFalse (result.fTruncated);
            Assert::AreEqual (target.c_str (), result.prefix.c_str ());
        }




        // T018: Long filename + long target — most aggressive truncation
        // MicrosoftWindows.DesktopStickerEditorCentennial.exe target

        TEST_METHOD(EllipsizePath_LongFilename_LongTarget_AggressiveTruncation)
        {
            wstring target = L"C:\\Windows\\SystemApps\\MicrosoftWindows.Client.CBS_cw5n1h2txyewy\\DesktopStickerEditorWin32Exe\\DesktopStickerEditorWin32Exe.exe";

            SEllipsizedPath result = EllipsizePath (target, 50);

            Assert::IsTrue (result.fTruncated, L"Should be truncated at width 50");

            // Verify total display length fits
            size_t totalLen = result.prefix.length () + 1 + result.suffix.length ();
            Assert::IsTrue (totalLen <= 50, L"Truncated result should fit within available width");

            // Verify leaf filename is preserved in suffix
            Assert::IsTrue (result.suffix.find (L"DesktopStickerEditorWin32Exe.exe") != wstring::npos,
                           L"Leaf filename should be preserved");
        }




        // T019: GameBarElevatedFT_Alias.exe target at 120-wide

        TEST_METHOD(EllipsizePath_GameBar_At120Wide)
        {
            wstring target = L"C:\\Program Files\\WindowsApps\\Microsoft.XboxGamingOverlay_7.326.4151.0_arm64__8wekyb3d8bbwe\\GameBarElevatedFT.exe";

            // At 120 available width, this 112-char path should fit without truncation
            SEllipsizedPath result = EllipsizePath (target, 120);

            Assert::IsFalse (result.fTruncated, L"Should fit at 120 chars");
            Assert::AreEqual (target.c_str (), result.prefix.c_str ());
        }



        TEST_METHOD(EllipsizePath_GameBar_At60Wide)
        {
            wstring target = L"C:\\Program Files\\WindowsApps\\Microsoft.XboxGamingOverlay_7.326.4151.0_arm64__8wekyb3d8bbwe\\GameBarElevatedFT.exe";

            // At 60 available, should truncate
            SEllipsizedPath result = EllipsizePath (target, 60);

            Assert::IsTrue (result.fTruncated, L"Should be truncated at 60 chars");

            size_t totalLen = result.prefix.length () + 1 + result.suffix.length ();
            Assert::IsTrue (totalLen <= 60, L"Should fit within 60 chars");
        }




        // T019: winget.exe target at 120-wide — should NOT be truncated

        TEST_METHOD(EllipsizePath_Winget_At120Wide_NoTruncation)
        {
            wstring target = L"C:\\Program Files\\WindowsApps\\Microsoft.DesktopAppInstaller_1.29.30.0_arm64__8wekyb3d8bbwe\\winget.exe";

            SEllipsizedPath result = EllipsizePath (target, 120);

            Assert::IsFalse (result.fTruncated, L"winget target should fit at 120");
            Assert::AreEqual (target.c_str (), result.prefix.c_str ());
        }




        // Edge case: empty path

        TEST_METHOD(EllipsizePath_EmptyPath)
        {
            SEllipsizedPath result = EllipsizePath (L"", 80);

            Assert::IsFalse (result.fTruncated);
            Assert::IsTrue (result.prefix.empty ());
        }




        // Edge case: available width is 0

        TEST_METHOD(EllipsizePath_ZeroWidth)
        {
            wstring target = L"C:\\Program Files\\test.exe";

            SEllipsizedPath result = EllipsizePath (target, 0);

            // Should return empty or minimal
            Assert::IsFalse (result.fTruncated);
        }
    };
}
