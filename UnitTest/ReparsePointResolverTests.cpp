#include "pch.h"
#include "EhmTestHelper.h"
#include "../TCDirCore/ReparsePointResolver.h"



using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTest
{




    ////////////////////////////////////////////////////////////////////////////
    //
    //  Helper to build a synthetic junction (MOUNT_POINT) reparse buffer
    //
    ////////////////////////////////////////////////////////////////////////////

    static vector<BYTE> BuildJunctionBuffer (const wstring & printName, const wstring & substituteName)
    {
        USHORT cbPrint = static_cast<USHORT>(printName.size () * sizeof (WCHAR));
        USHORT cbSub   = static_cast<USHORT>(substituteName.size () * sizeof (WCHAR));

        // MountPointReparseBuffer: SubOff, SubLen, PrintOff, PrintLen, PathBuffer
        USHORT cbPathBuffer    = cbSub + sizeof (WCHAR) + cbPrint + sizeof (WCHAR);
        USHORT cbReparseData   = static_cast<USHORT>(FIELD_OFFSET (REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer) - REPARSE_DATA_BUFFER_HEADER_SIZE + cbPathBuffer);
        DWORD  cbTotal         = REPARSE_DATA_BUFFER_HEADER_SIZE + cbReparseData;

        vector<BYTE> buffer (cbTotal, 0);
        auto * pRdb = reinterpret_cast<REPARSE_DATA_BUFFER *>(buffer.data ());

        pRdb->ReparseTag        = IO_REPARSE_TAG_MOUNT_POINT;
        pRdb->ReparseDataLength = cbReparseData;
        pRdb->Reserved          = 0;

        auto & mp = pRdb->MountPointReparseBuffer;

        mp.SubstituteNameOffset = 0;
        mp.SubstituteNameLength = cbSub;
        mp.PrintNameOffset      = cbSub + sizeof (WCHAR);
        mp.PrintNameLength      = cbPrint;

        memcpy (mp.PathBuffer, substituteName.c_str (), cbSub);
        memcpy (reinterpret_cast<BYTE *>(mp.PathBuffer) + cbSub + sizeof (WCHAR), printName.c_str (), cbPrint);

        return buffer;
    }





    ////////////////////////////////////////////////////////////////////////////
    //
    //  Helper to build a synthetic symlink buffer
    //
    ////////////////////////////////////////////////////////////////////////////

    static vector<BYTE> BuildSymlinkBuffer (const wstring & printName, const wstring & substituteName, ULONG flags)
    {
        USHORT cbPrint = static_cast<USHORT>(printName.size () * sizeof (WCHAR));
        USHORT cbSub   = static_cast<USHORT>(substituteName.size () * sizeof (WCHAR));

        USHORT cbPathBuffer    = cbSub + sizeof (WCHAR) + cbPrint + sizeof (WCHAR);
        USHORT cbReparseData   = static_cast<USHORT>(FIELD_OFFSET (REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) - REPARSE_DATA_BUFFER_HEADER_SIZE + cbPathBuffer);
        DWORD  cbTotal         = REPARSE_DATA_BUFFER_HEADER_SIZE + cbReparseData;

        vector<BYTE> buffer (cbTotal, 0);
        auto * pRdb = reinterpret_cast<REPARSE_DATA_BUFFER *>(buffer.data ());

        pRdb->ReparseTag        = IO_REPARSE_TAG_SYMLINK;
        pRdb->ReparseDataLength = cbReparseData;
        pRdb->Reserved          = 0;

        auto & sl = pRdb->SymbolicLinkReparseBuffer;

        sl.SubstituteNameOffset = 0;
        sl.SubstituteNameLength = cbSub;
        sl.PrintNameOffset      = cbSub + sizeof (WCHAR);
        sl.PrintNameLength      = cbPrint;
        sl.Flags                = flags;

        memcpy (sl.PathBuffer, substituteName.c_str (), cbSub);
        memcpy (reinterpret_cast<BYTE *>(sl.PathBuffer) + cbSub + sizeof (WCHAR), printName.c_str (), cbPrint);

        return buffer;
    }





    ////////////////////////////////////////////////////////////////////////////
    //
    //  Helper to build a synthetic AppExecLink buffer
    //
    ////////////////////////////////////////////////////////////////////////////

    static vector<BYTE> BuildAppExecLinkBuffer (ULONG version, const wstring & packageId, const wstring & appId, const wstring & targetExe)
    {
        // Calculate total size of the three NUL-terminated strings
        DWORD cbStrings = static_cast<DWORD>((packageId.size () + 1 + appId.size () + 1 + targetExe.size () + 1) * sizeof (WCHAR));
        DWORD cbData    = sizeof (ULONG) + cbStrings;

        USHORT cbReparseData = static_cast<USHORT>(cbData);
        DWORD  cbTotal       = REPARSE_DATA_BUFFER_HEADER_SIZE + cbReparseData;

        vector<BYTE> buffer (cbTotal, 0);
        auto * pRdb = reinterpret_cast<REPARSE_DATA_BUFFER *>(buffer.data ());

        pRdb->ReparseTag        = IO_REPARSE_TAG_APPEXECLINK;
        pRdb->ReparseDataLength = cbReparseData;
        pRdb->Reserved          = 0;

        BYTE * pData = pRdb->GenericReparseBuffer.DataBuffer;

        // Version
        *reinterpret_cast<ULONG *>(pData) = version;
        pData += sizeof (ULONG);

        // String 1: Package ID
        memcpy (pData, packageId.c_str (), (packageId.size () + 1) * sizeof (WCHAR));
        pData += (packageId.size () + 1) * sizeof (WCHAR);

        // String 2: App User Model ID
        memcpy (pData, appId.c_str (), (appId.size () + 1) * sizeof (WCHAR));
        pData += (appId.size () + 1) * sizeof (WCHAR);

        // String 3: Target Exe Path
        memcpy (pData, targetExe.c_str (), (targetExe.size () + 1) * sizeof (WCHAR));

        return buffer;
    }





    TEST_CLASS(ReparsePointResolverTests)
    {
    public:

        TEST_CLASS_INITIALIZE(ClassInitialize)
        {
            SetupEhmForUnitTests ();
        }




        // T012: StripDevicePrefix tests

        TEST_METHOD(StripDevicePrefix_RemovesPrefix)
        {
            wstring result = StripDevicePrefix (L"\\??\\C:\\Users\\Dev");

            Assert::AreEqual (L"C:\\Users\\Dev", result.c_str ());
        }

        TEST_METHOD(StripDevicePrefix_UncPath)
        {
            wstring result = StripDevicePrefix (L"\\??\\UNC\\server\\share");

            Assert::AreEqual (L"UNC\\server\\share", result.c_str ());
        }

        TEST_METHOD(StripDevicePrefix_EmptyString)
        {
            wstring result = StripDevicePrefix (L"");

            Assert::AreEqual (L"", result.c_str ());
        }

        TEST_METHOD(StripDevicePrefix_NoPrefix)
        {
            wstring result = StripDevicePrefix (L"C:\\Normal\\Path");

            Assert::AreEqual (L"C:\\Normal\\Path", result.c_str ());
        }




        // T013: ParseJunctionBuffer tests

        TEST_METHOD(ParseJunction_PrintName)
        {
            auto buffer = BuildJunctionBuffer (L"C:\\Target\\Dir", L"\\??\\C:\\Target\\Dir");

            wstring result = ParseJunctionBuffer (buffer.data (), static_cast<DWORD>(buffer.size ()));

            Assert::AreEqual (L"C:\\Target\\Dir", result.c_str ());
        }

        TEST_METHOD(ParseJunction_SubstituteNameWithPrefix)
        {
            // Verify that when PrintName is available, it's used (not SubstituteName)
            auto buffer = BuildJunctionBuffer (L"C:\\Print\\Path", L"\\??\\C:\\Sub\\Path");

            wstring result = ParseJunctionBuffer (buffer.data (), static_cast<DWORD>(buffer.size ()));

            Assert::AreEqual (L"C:\\Print\\Path", result.c_str ());
        }




        // T014: ParseSymlinkBuffer tests

        TEST_METHOD(ParseSymlink_Absolute)
        {
            auto buffer = BuildSymlinkBuffer (L"C:\\Target\\File.txt", L"\\??\\C:\\Target\\File.txt", 0);

            wstring result = ParseSymlinkBuffer (buffer.data (), static_cast<DWORD>(buffer.size ()));

            Assert::AreEqual (L"C:\\Target\\File.txt", result.c_str ());
        }

        TEST_METHOD(ParseSymlink_Relative)
        {
            auto buffer = BuildSymlinkBuffer (L"..\\shared\\config.yml", L"..\\shared\\config.yml", SYMLINK_FLAG_RELATIVE);

            wstring result = ParseSymlinkBuffer (buffer.data (), static_cast<DWORD>(buffer.size ()));

            Assert::AreEqual (L"..\\shared\\config.yml", result.c_str ());
        }




        // T015: ParseAppExecLinkBuffer tests

        TEST_METHOD(ParseAppExecLink_Version3)
        {
            auto buffer = BuildAppExecLinkBuffer (
                3,
                L"Microsoft.DesktopAppInstaller_8wekyb3d8bbwe",
                L"Microsoft.DesktopAppInstaller_8wekyb3d8bbwe!PythonRedirector",
                L"C:\\Program Files\\WindowsApps\\python3.12.exe");

            wstring result = ParseAppExecLinkBuffer (buffer.data (), static_cast<DWORD>(buffer.size ()));

            Assert::AreEqual (L"C:\\Program Files\\WindowsApps\\python3.12.exe", result.c_str ());
        }

        TEST_METHOD(ParseAppExecLink_VersionMismatch)
        {
            auto buffer = BuildAppExecLinkBuffer (
                99,
                L"pkg",
                L"app",
                L"C:\\target.exe");

            wstring result = ParseAppExecLinkBuffer (buffer.data (), static_cast<DWORD>(buffer.size ()));

            Assert::AreEqual (L"", result.c_str ());
        }




        // T016: ParseJunctionBuffer fallback to SubstituteName

        TEST_METHOD(ParseJunction_EmptyPrintName_FallsBackToSubstituteName)
        {
            auto buffer = BuildJunctionBuffer (L"", L"\\??\\C:\\Fallback\\Path");

            wstring result = ParseJunctionBuffer (buffer.data (), static_cast<DWORD>(buffer.size ()));

            Assert::AreEqual (L"C:\\Fallback\\Path", result.c_str ());
        }




        // T017: ParseAppExecLinkBuffer with truncated buffer

        TEST_METHOD(ParseAppExecLink_TruncatedBuffer)
        {
            auto buffer = BuildAppExecLinkBuffer (
                3,
                L"Microsoft.DesktopAppInstaller_8wekyb3d8bbwe",
                L"Microsoft.DesktopAppInstaller_8wekyb3d8bbwe!PythonRedirector",
                L"C:\\Program Files\\WindowsApps\\python3.12.exe");

            // Truncate the buffer to cut off the third string
            DWORD truncatedSize = REPARSE_DATA_BUFFER_HEADER_SIZE + sizeof (ULONG) + 20;

            wstring result = ParseAppExecLinkBuffer (buffer.data (), truncatedSize);

            Assert::AreEqual (L"", result.c_str ());
        }



        TEST_METHOD(ParseJunction_BufferTooSmall)
        {
            BYTE smallBuffer[4] = { 0 };

            wstring result = ParseJunctionBuffer (smallBuffer, sizeof (smallBuffer));

            Assert::AreEqual (L"", result.c_str ());
        }



        TEST_METHOD(ParseSymlink_BufferTooSmall)
        {
            BYTE smallBuffer[4] = { 0 };

            wstring result = ParseSymlinkBuffer (smallBuffer, sizeof (smallBuffer));

            Assert::AreEqual (L"", result.c_str ());
        }



        TEST_METHOD(ParseAppExecLink_BufferTooSmall)
        {
            BYTE smallBuffer[4] = { 0 };

            wstring result = ParseAppExecLinkBuffer (smallBuffer, sizeof (smallBuffer));

            Assert::AreEqual (L"", result.c_str ());
        }



        TEST_METHOD(ParseJunction_WrongTag)
        {
            auto buffer = BuildJunctionBuffer (L"C:\\Target", L"\\??\\C:\\Target");

            // Corrupt the tag
            auto * pRdb = reinterpret_cast<REPARSE_DATA_BUFFER *>(buffer.data ());
            pRdb->ReparseTag = IO_REPARSE_TAG_SYMLINK;

            wstring result = ParseJunctionBuffer (buffer.data (), static_cast<DWORD>(buffer.size ()));

            Assert::AreEqual (L"", result.c_str ());
        }



        TEST_METHOD(ParseSymlink_WrongTag)
        {
            auto buffer = BuildSymlinkBuffer (L"C:\\Target", L"\\??\\C:\\Target", 0);

            // Corrupt the tag
            auto * pRdb = reinterpret_cast<REPARSE_DATA_BUFFER *>(buffer.data ());
            pRdb->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;

            wstring result = ParseSymlinkBuffer (buffer.data (), static_cast<DWORD>(buffer.size ()));

            Assert::AreEqual (L"", result.c_str ());
        }



        TEST_METHOD(ParseSymlink_Absolute_FallbackToSubstituteName)
        {
            auto buffer = BuildSymlinkBuffer (L"", L"\\??\\C:\\Fallback\\Symlink", 0);

            wstring result = ParseSymlinkBuffer (buffer.data (), static_cast<DWORD>(buffer.size ()));

            Assert::AreEqual (L"C:\\Fallback\\Symlink", result.c_str ());
        }



        TEST_METHOD(ParseSymlink_Relative_FallbackToSubstituteName)
        {
            auto buffer = BuildSymlinkBuffer (L"", L"..\\relative\\path", SYMLINK_FLAG_RELATIVE);

            wstring result = ParseSymlinkBuffer (buffer.data (), static_cast<DWORD>(buffer.size ()));

            Assert::AreEqual (L"..\\relative\\path", result.c_str ());
        }
    };
}
