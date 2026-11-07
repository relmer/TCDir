#pragma once

#include "ByteSource.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CDiscImage
//
//  Sector access and field decoding shared by the ISO 9660 and UDF readers.
//
////////////////////////////////////////////////////////////////////////////////

class CDiscImage
{
public:
    static constexpr int        s_kBitsPerByte    = 8;
    static constexpr DWORD      s_kcbSector       = 2048;
    static constexpr DWORD      s_kcbMaxRead      = 64 * 1024 * 1024;
    static constexpr ULONGLONG  s_kTicksPerMinute = 60ULL * 10000000ULL;

    static USHORT    ReadU16       (const BYTE * p);
    static ULONG     ReadU32       (const BYTE * p);
    static ULONGLONG ReadU64       (const BYTE * p);

    static HRESULT   ReadBytes     (IByteSource & source, ULONGLONG ullSector, ULONGLONG cb, vector<BYTE> & vBuffer);
    static HRESULT   ToUtcFileTime (const SYSTEMTIME & st, int iOffsetMinutes, FILETIME & ft);
};
