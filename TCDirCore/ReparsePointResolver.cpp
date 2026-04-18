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
    if (cbBuffer < REPARSE_DATA_BUFFER_HEADER_SIZE)
    {
        return {};
    }

    auto * pRdb = reinterpret_cast<const REPARSE_DATA_BUFFER *>(pBuffer);

    if (pRdb->ReparseTag != IO_REPARSE_TAG_MOUNT_POINT)
    {
        return {};
    }

    const auto & mp = pRdb->MountPointReparseBuffer;



    // Try PrintName first (clean user-readable path)
    if (mp.PrintNameLength > 0)
    {
        DWORD cbPrintStart = FIELD_OFFSET (REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer) + mp.PrintNameOffset;
        DWORD cbPrintEnd   = cbPrintStart + mp.PrintNameLength;

        if (cbPrintEnd > cbBuffer)
        {
            return {};
        }

        return wstring (
            reinterpret_cast<const WCHAR *>(pBuffer + cbPrintStart),
            mp.PrintNameLength / sizeof (WCHAR));
    }



    // Fall back to SubstituteName with \??\ stripped
    if (mp.SubstituteNameLength > 0)
    {
        DWORD cbSubStart = FIELD_OFFSET (REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer) + mp.SubstituteNameOffset;
        DWORD cbSubEnd   = cbSubStart + mp.SubstituteNameLength;

        if (cbSubEnd > cbBuffer)
        {
            return {};
        }

        wstring subName (
            reinterpret_cast<const WCHAR *>(pBuffer + cbSubStart),
            mp.SubstituteNameLength / sizeof (WCHAR));

        return StripDevicePrefix (subName);
    }

    return {};
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
    if (cbBuffer < REPARSE_DATA_BUFFER_HEADER_SIZE)
    {
        return {};
    }

    auto * pRdb = reinterpret_cast<const REPARSE_DATA_BUFFER *>(pBuffer);

    if (pRdb->ReparseTag != IO_REPARSE_TAG_SYMLINK)
    {
        return {};
    }

    const auto & sl = pRdb->SymbolicLinkReparseBuffer;



    // Try PrintName first
    if (sl.PrintNameLength > 0)
    {
        DWORD cbPrintStart = FIELD_OFFSET (REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) + sl.PrintNameOffset;
        DWORD cbPrintEnd   = cbPrintStart + sl.PrintNameLength;

        if (cbPrintEnd > cbBuffer)
        {
            return {};
        }

        return wstring (
            reinterpret_cast<const WCHAR *>(pBuffer + cbPrintStart),
            sl.PrintNameLength / sizeof (WCHAR));
    }



    // Fall back to SubstituteName
    if (sl.SubstituteNameLength > 0)
    {
        DWORD cbSubStart = FIELD_OFFSET (REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) + sl.SubstituteNameOffset;
        DWORD cbSubEnd   = cbSubStart + sl.SubstituteNameLength;

        if (cbSubEnd > cbBuffer)
        {
            return {};
        }

        wstring subName (
            reinterpret_cast<const WCHAR *>(pBuffer + cbSubStart),
            sl.SubstituteNameLength / sizeof (WCHAR));

        // Only strip prefix for absolute symlinks
        if (!(sl.Flags & SYMLINK_FLAG_RELATIVE))
        {
            return StripDevicePrefix (subName);
        }

        return subName;
    }

    return {};
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
    if (cbBuffer < REPARSE_DATA_BUFFER_HEADER_SIZE)
    {
        return {};
    }

    auto * pRdb = reinterpret_cast<const REPARSE_DATA_BUFFER *>(pBuffer);

    if (pRdb->ReparseTag != IO_REPARSE_TAG_APPEXECLINK)
    {
        return {};
    }

    const BYTE * pData    = pRdb->GenericReparseBuffer.DataBuffer;
    DWORD        cbData   = pRdb->ReparseDataLength;
    DWORD        cbOffset = static_cast<DWORD>(pData - pBuffer);

    if (cbOffset + cbData > cbBuffer)
    {
        return {};
    }



    // Version check
    if (cbData < sizeof (ULONG))
    {
        return {};
    }

    ULONG version = *reinterpret_cast<const ULONG *>(pData);

    if (version != 3)
    {
        return {};
    }

    // Walk three NUL-terminated WCHAR strings
    const WCHAR * pStrings    = reinterpret_cast<const WCHAR *>(pData + sizeof (ULONG));
    DWORD         cbRemaining = cbData - sizeof (ULONG);
    DWORD         cchRemaining = cbRemaining / sizeof (WCHAR);
    int           stringIndex  = 0;

    const WCHAR * pCurrent = pStrings;

    while (stringIndex < 3 && cchRemaining > 0)
    {
        // Find the NUL terminator
        size_t cchLen = wcsnlen (pCurrent, cchRemaining);

        if (cchLen == cchRemaining)
        {
            // No NUL terminator found — truncated buffer
            return {};
        }

        if (stringIndex == 2)
        {
            // This is the third string — the target exe path
            return wstring (pCurrent, cchLen);
        }

        // Skip past this string and its NUL terminator
        DWORD cchConsumed = static_cast<DWORD>(cchLen + 1);
        pCurrent     += cchConsumed;
        cchRemaining -= cchConsumed;
        stringIndex++;
    }

    return {};
}





////////////////////////////////////////////////////////////////////////////////
//
//  ResolveReparseTarget
//
//  Open the reparse point and read its target path.
//  Returns an empty string on any failure (graceful degradation per FR-011).
//
////////////////////////////////////////////////////////////////////////////////

wstring ResolveReparseTarget (const filesystem::path & dirPath, const WIN32_FIND_DATA & wfd)
{
    if (!CFlag::IsSet (wfd.dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT))
    {
        return {};
    }

    DWORD dwTag = wfd.dwReserved0;

    if (dwTag != IO_REPARSE_TAG_MOUNT_POINT &&
        dwTag != IO_REPARSE_TAG_SYMLINK     &&
        dwTag != IO_REPARSE_TAG_APPEXECLINK)
    {
        return {};
    }



    // Build the full path to the reparse point
    filesystem::path fullPath = dirPath / wfd.cFileName;

    AutoHandle hFile (CreateFileW (
        fullPath.c_str (),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
        nullptr));

    if (hFile == INVALID_HANDLE_VALUE)
    {
        return {};
    }



    // Read reparse data
    BYTE  buffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
    DWORD dwBytesReturned = 0;

    BOOL fOk = DeviceIoControl (
        hFile,
        FSCTL_GET_REPARSE_POINT,
        nullptr, 0,
        buffer, sizeof (buffer),
        &dwBytesReturned,
        nullptr);

    if (!fOk)
    {
        return {};
    }



    // Dispatch to the correct parser
    switch (dwTag)
    {
        case IO_REPARSE_TAG_MOUNT_POINT:   return ParseJunctionBuffer    (buffer, dwBytesReturned);
        case IO_REPARSE_TAG_SYMLINK:       return ParseSymlinkBuffer     (buffer, dwBytesReturned);
        case IO_REPARSE_TAG_APPEXECLINK:   return ParseAppExecLinkBuffer (buffer, dwBytesReturned);
        default:                           return {};
    }
}
