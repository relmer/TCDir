#pragma once

#include "AutoHandle.h"





////////////////////////////////////////////////////////////////////////////////
//
//  REPARSE_DATA_BUFFER
//
//  Manual definition for user-mode code.  This struct is defined in <ntifs.h>
//  (WDK kernel header) but is not available in standard <windows.h>.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef SYMLINK_FLAG_RELATIVE
#define SYMLINK_FLAG_RELATIVE   0x00000001
#endif

#ifndef IO_REPARSE_TAG_APPEXECLINK
#define IO_REPARSE_TAG_APPEXECLINK  0x8000001B
#endif

typedef struct _REPARSE_DATA_BUFFER
{
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;

    union
    {
        struct
        {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            ULONG  Flags;
            WCHAR  PathBuffer[1];
        } SymbolicLinkReparseBuffer;

        struct
        {
            USHORT SubstituteNameOffset;
            USHORT SubstituteNameLength;
            USHORT PrintNameOffset;
            USHORT PrintNameLength;
            WCHAR  PathBuffer[1];
        } MountPointReparseBuffer;

        struct
        {
            UCHAR DataBuffer[1];
        } GenericReparseBuffer;
    };
} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

#define REPARSE_DATA_BUFFER_HEADER_SIZE  FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer)





////////////////////////////////////////////////////////////////////////////////
//
//  Reparse point resolver — public API
//
////////////////////////////////////////////////////////////////////////////////

wstring StripDevicePrefix      (const wstring & path);

wstring ParseJunctionBuffer    (const BYTE * pBuffer, DWORD cbBuffer);
wstring ParseSymlinkBuffer     (const BYTE * pBuffer, DWORD cbBuffer);
wstring ParseAppExecLinkBuffer (const BYTE * pBuffer, DWORD cbBuffer);

wstring ResolveReparseTarget   (const filesystem::path & dirPath, const WIN32_FIND_DATA & wfd);
