#include "pch.h"

#include "DiscImage.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CDiscImage::ReadU16
//
//  Decodes a little-endian 16-bit field.
//
////////////////////////////////////////////////////////////////////////////////

USHORT CDiscImage::ReadU16 (const BYTE * p)
{
    return (USHORT) (p[0] | (p[1] << s_kBitsPerByte));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDiscImage::ReadU32
//
//  Decodes a little-endian 32-bit field.
//
////////////////////////////////////////////////////////////////////////////////

ULONG CDiscImage::ReadU32 (const BYTE * p)
{
    return (ULONG) ReadU16 (p) | ((ULONG) ReadU16 (p + sizeof (USHORT)) << (sizeof (USHORT) * s_kBitsPerByte));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDiscImage::ReadU64
//
//  Decodes a little-endian 64-bit field.
//
////////////////////////////////////////////////////////////////////////////////

ULONGLONG CDiscImage::ReadU64 (const BYTE * p)
{
    return (ULONGLONG) ReadU32 (p) | ((ULONGLONG) ReadU32 (p + sizeof (ULONG)) << (sizeof (ULONG) * s_kBitsPerByte));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDiscImage::ReadBytes
//
//  Reads cb bytes starting at a sector.  An image too short for the request
//  is corrupt, so a short read returns ERROR_BAD_FORMAT rather than
//  ERROR_HANDLE_EOF.  A request above s_kcbMaxRead also returns
//  ERROR_BAD_FORMAT, so a corrupt length field cannot trigger a huge
//  allocation.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CDiscImage::ReadBytes (
    IByteSource    & source,
    ULONGLONG        ullSector,
    ULONGLONG        cb,
    vector<BYTE>   & vBuffer)
{
    HRESULT   hr        = S_OK;
    ULONGLONG ullOffset = ullSector * s_kcbSector;



    vBuffer.clear();

    CBREx (cb <= s_kcbMaxRead, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    vBuffer.resize ((size_t) cb);

    BAIL_OUT_IF (cb == 0, S_OK);

    hr = source.ReadAt (ullOffset, vBuffer.data(), (DWORD) cb);
    CBREx (hr != HRESULT_FROM_WIN32 (ERROR_HANDLE_EOF), HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));
    CHR (hr);



Error:
    if (FAILED (hr))
    {
        vBuffer.clear();
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CDiscImage::ToUtcFileTime
//
//  Converts a local time and its offset from UTC, in minutes east of UTC,
//  to a UTC FILETIME.  An invalid date returns a failure and a zero
//  FILETIME.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CDiscImage::ToUtcFileTime (
    const SYSTEMTIME & st,
    int                iOffsetMinutes,
    FILETIME         & ft)
{
    HRESULT          hr       = S_OK;
    BOOL             fSuccess = FALSE;
    ULARGE_INTEGER   uli      = {};
    ULONGLONG        ullShift = 0;



    ft = {};

    fSuccess = SystemTimeToFileTime (&st, &ft);
    CWR (fSuccess);

    uli.LowPart  = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    ullShift     = (ULONGLONG) abs (iOffsetMinutes) * s_kTicksPerMinute;

    if (iOffsetMinutes > 0)
    {
        CBREx (uli.QuadPart >= ullShift, HRESULT_FROM_WIN32 (ERROR_INVALID_DATA));
        uli.QuadPart -= ullShift;
    }
    else
    {
        uli.QuadPart += ullShift;
    }

    ft.dwLowDateTime  = uli.LowPart;
    ft.dwHighDateTime = uli.HighPart;



Error:
    if (FAILED (hr))
    {
        ft = {};
    }

    return hr;
}





