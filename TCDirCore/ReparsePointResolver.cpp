#include "pch.h"

#include "ReparsePointResolver.h"
#include "Flag.h"





////////////////////////////////////////////////////////////////////////////////
//
//  StripDevicePrefix
//
//  Remove the \??\ device prefix from NT paths.
//
////////////////////////////////////////////////////////////////////////////////

wstring StripDevicePrefix (const wstring & path)
{
    static constexpr wstring_view prefix = L"\\??\\";

    if (path.starts_with (prefix))
    {
        return path.substr (prefix.size ());
    }

    return path;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ParseJunctionBuffer
//
//  Extract the target path from a junction (IO_REPARSE_TAG_MOUNT_POINT)
//  reparse data buffer.  Prefers PrintName; falls back to SubstituteName
//  with \??\ prefix stripped.
//
////////////////////////////////////////////////////////////////////////////////

wstring ParseJunctionBuffer (const BYTE * pBuffer, DWORD cbBuffer)
{
    HRESULT                      hr                 = S_OK;
    const REPARSE_DATA_BUFFER  * pRdb               = nullptr;
    DWORD                        cbPathBase         = FIELD_OFFSET (REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer);
    USHORT                       nameOffset         = 0;
    USHORT                       nameLength         = 0;
    bool                         fUseSubstituteName = false;
    DWORD                        cbStart            = 0;
    DWORD                        cbEnd              = 0;
    wstring                      result;



    BAIL_OUT_IF (cbBuffer < REPARSE_DATA_BUFFER_HEADER_SIZE, S_OK);

    pRdb = reinterpret_cast<const REPARSE_DATA_BUFFER *>(pBuffer);
    BAIL_OUT_IF (pRdb->ReparseTag != IO_REPARSE_TAG_MOUNT_POINT, S_OK);

    // Select PrintName or SubstituteName
    if (pRdb->MountPointReparseBuffer.PrintNameLength > 0)
    {
        nameOffset = pRdb->MountPointReparseBuffer.PrintNameOffset;
        nameLength = pRdb->MountPointReparseBuffer.PrintNameLength;
    }
    else if (pRdb->MountPointReparseBuffer.SubstituteNameLength > 0)
    {
        nameOffset         = pRdb->MountPointReparseBuffer.SubstituteNameOffset;
        nameLength         = pRdb->MountPointReparseBuffer.SubstituteNameLength;
        fUseSubstituteName = true;
    }
    else
    {
        BAIL_OUT_IF (TRUE, S_OK);
    }

    // Extract the selected name
    cbStart = cbPathBase + nameOffset;
    cbEnd   = cbStart + nameLength;

    CBREx (cbEnd <= cbBuffer, HRESULT_FROM_WIN32 (ERROR_INVALID_DATA));

    result = wstring (
        reinterpret_cast<const WCHAR *>(pBuffer + cbStart),
        nameLength / sizeof (WCHAR));

    if (fUseSubstituteName)
    {
        result = StripDevicePrefix (result);
    }

Error:
    return result;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ParseSymlinkBuffer
//
//  Extract the target path from a symlink (IO_REPARSE_TAG_SYMLINK)
//  reparse data buffer.  Prefers PrintName; falls back to SubstituteName
//  with \??\ prefix stripped (for absolute symlinks only).
//
////////////////////////////////////////////////////////////////////////////////

wstring ParseSymlinkBuffer (const BYTE * pBuffer, DWORD cbBuffer)
{
    HRESULT                      hr           = S_OK;
    const REPARSE_DATA_BUFFER  * pRdb         = nullptr;
    DWORD                        cbPathBase   = FIELD_OFFSET (REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer);
    USHORT                       nameOffset   = 0;
    USHORT                       nameLength   = 0;
    bool                         fStripPrefix = false;
    DWORD                        cbStart      = 0;
    DWORD                        cbEnd        = 0;
    wstring                      result;



    BAIL_OUT_IF (cbBuffer < REPARSE_DATA_BUFFER_HEADER_SIZE, S_OK);

    pRdb = reinterpret_cast<const REPARSE_DATA_BUFFER *>(pBuffer);
    BAIL_OUT_IF (pRdb->ReparseTag != IO_REPARSE_TAG_SYMLINK, S_OK);

    // Select PrintName or SubstituteName
    if (pRdb->SymbolicLinkReparseBuffer.PrintNameLength > 0)
    {
        nameOffset = pRdb->SymbolicLinkReparseBuffer.PrintNameOffset;
        nameLength = pRdb->SymbolicLinkReparseBuffer.PrintNameLength;
    }
    else if (pRdb->SymbolicLinkReparseBuffer.SubstituteNameLength > 0)
    {
        nameOffset   = pRdb->SymbolicLinkReparseBuffer.SubstituteNameOffset;
        nameLength   = pRdb->SymbolicLinkReparseBuffer.SubstituteNameLength;
        fStripPrefix = !(pRdb->SymbolicLinkReparseBuffer.Flags & SYMLINK_FLAG_RELATIVE);
    }
    else
    {
        BAIL_OUT_IF (TRUE, S_OK);
    }

    // Extract the selected name
    cbStart = cbPathBase + nameOffset;
    cbEnd   = cbStart    + nameLength;

    CBREx (cbEnd <= cbBuffer, HRESULT_FROM_WIN32 (ERROR_INVALID_DATA));

    result = wstring (
        reinterpret_cast<const WCHAR *>(pBuffer + cbStart),
        nameLength / sizeof (WCHAR));

    if (fStripPrefix)
    {
        result = StripDevicePrefix (result);
    }

Error:
    return result;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ParseAppExecLinkBuffer
//
//  Extract the target executable path from an AppExecLink
//  (IO_REPARSE_TAG_APPEXECLINK) reparse data buffer.
//
//  Buffer layout after the 8-byte header:
//    ULONG  Version  (must be 3)
//    WCHAR  String1  (Package ID, NUL-terminated)
//    WCHAR  String2  (App User Model ID, NUL-terminated)
//    WCHAR  String3  (Target Exe Path, NUL-terminated)
//
////////////////////////////////////////////////////////////////////////////////

wstring ParseAppExecLinkBuffer (const BYTE * pBuffer, DWORD cbBuffer)
{
    HRESULT                      hr           = S_OK;
    const REPARSE_DATA_BUFFER  * pRdb         = nullptr;
    const BYTE                 * pData        = nullptr;
    DWORD                        cbData       = 0;
    DWORD                        cbOffset     = 0;
    ULONG                        version      = 0;
    const WCHAR                * pCurrent     = nullptr;
    DWORD                        cchRemaining = 0;
    int                          stringIndex  = 0;
    wstring                      result;



    BAIL_OUT_IF (cbBuffer < REPARSE_DATA_BUFFER_HEADER_SIZE, S_OK);

    pRdb = reinterpret_cast<const REPARSE_DATA_BUFFER *>(pBuffer);
    BAIL_OUT_IF (pRdb->ReparseTag != IO_REPARSE_TAG_APPEXECLINK, S_OK);

    pData    = pRdb->GenericReparseBuffer.DataBuffer;
    cbData   = pRdb->ReparseDataLength;
    cbOffset = static_cast<DWORD>(pData - pBuffer);

    CBREx (cbOffset + cbData <= cbBuffer, HRESULT_FROM_WIN32 (ERROR_INVALID_DATA));

    // Version check
    CBREx (cbData >= sizeof (ULONG), HRESULT_FROM_WIN32 (ERROR_INVALID_DATA));

    version = *reinterpret_cast<const ULONG *>(pData);

    BAIL_OUT_IF (version != 3, S_OK);

    // Walk three NUL-terminated WCHAR strings
    pCurrent     = reinterpret_cast<const WCHAR *>(pData + sizeof (ULONG));
    cchRemaining = (cbData - sizeof (ULONG)) / sizeof (WCHAR);

    while (stringIndex < 3 && cchRemaining > 0)
    {
        size_t cchLen = wcsnlen (pCurrent, cchRemaining);

        // No NUL terminator found — truncated buffer
        CBREx (cchLen < cchRemaining, HRESULT_FROM_WIN32 (ERROR_INVALID_DATA));

        if (stringIndex == 2)
        {
            result = wstring (pCurrent, cchLen);
            break;
        }

        DWORD cchConsumed = static_cast<DWORD>(cchLen + 1);

        pCurrent     += cchConsumed;
        cchRemaining -= cchConsumed;
        stringIndex++;
    }

Error:
    return result;
}





////////////////////////////////////////////////////////////////////////////////
//
//  ResolveReparseTarget
//
//  Open the reparse point and read its target path.
//  Returns an empty string on any failure (graceful degradation per FR-011).
//
////////////////////////////////////////////////////////////////////////////////

#pragma warning(suppress: 6262)  // 16KB stack buffer is safe: non-recursive, shallow call stack
wstring ResolveReparseTarget (const filesystem::path & dirPath, const WIN32_FIND_DATA & wfd)
{
    HRESULT          hr              = S_OK;
    DWORD            dwTag           = wfd.dwReserved0;
    filesystem::path fullPath;
    AutoHandle       hFile;
    DWORD            dwBytesReturned = 0;
    BOOL             fOk             = FALSE;
    wstring          result;



    BAIL_OUT_IF (!CFlag::IsSet (wfd.dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT), S_OK);

    BAIL_OUT_IF (dwTag != IO_REPARSE_TAG_MOUNT_POINT &&
                 dwTag != IO_REPARSE_TAG_SYMLINK     &&
                 dwTag != IO_REPARSE_TAG_APPEXECLINK, S_OK);

    // Build the full path to the reparse point
    fullPath = dirPath / wfd.cFileName;

    hFile = CreateFileW (
        fullPath.c_str (),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
        nullptr);

    BAIL_OUT_IF (hFile == INVALID_HANDLE_VALUE, S_OK);



    // Read reparse data
    BYTE buffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];

    fOk = DeviceIoControl (
        hFile,
        FSCTL_GET_REPARSE_POINT,
        nullptr, 0,
        buffer, sizeof (buffer),
        &dwBytesReturned,
        nullptr);

    BAIL_OUT_IF (!fOk, S_OK);



    // Dispatch to the correct parser
    switch (dwTag)
    {
        case IO_REPARSE_TAG_MOUNT_POINT:   result = ParseJunctionBuffer    (buffer, dwBytesReturned);  break;
        case IO_REPARSE_TAG_SYMLINK:       result = ParseSymlinkBuffer     (buffer, dwBytesReturned);  break;
        case IO_REPARSE_TAG_APPEXECLINK:   result = ParseAppExecLinkBuffer (buffer, dwBytesReturned);  break;
    }

Error:
    return result;
}
