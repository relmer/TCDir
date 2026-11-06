#include "pch.h"

#include "ByteSource.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CFileByteSource::Open
//
//  Opens the file for shared, read-only, random access.  Other processes can
//  write to or delete the file while it is open.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CFileByteSource::Open (const filesystem::path & path)
{
    HRESULT         hr       = S_OK;
    LARGE_INTEGER   liSize   = {};
    BOOL            fSuccess = FALSE;



    m_hFile = CreateFileW (path.c_str(),
                           GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr,
                           OPEN_EXISTING,
                           FILE_FLAG_RANDOM_ACCESS,
                           nullptr);
    CWR (m_hFile != INVALID_HANDLE_VALUE);

    fSuccess = GetFileSizeEx (m_hFile, &liSize);
    CWR (fSuccess);

    m_ullSize = static_cast<ULONGLONG> (liSize.QuadPart);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CFileByteSource::ReadAt
//
//  Reads exactly cb bytes at offset.  A short read returns ERROR_HANDLE_EOF.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CFileByteSource::ReadAt (ULONGLONG offset, void * pBuffer, DWORD cb)
{
    HRESULT          hr       = S_OK;
    OVERLAPPED       ov       = {};
    ULARGE_INTEGER   uliPos   = {};
    DWORD            cbRead   = 0;
    BOOL             fSuccess = FALSE;



    CBREx (offset <= m_ullSize && cb <= m_ullSize - offset, HRESULT_FROM_WIN32 (ERROR_HANDLE_EOF));

    uliPos.QuadPart = offset;
    ov.Offset       = uliPos.LowPart;
    ov.OffsetHigh   = uliPos.HighPart;

    fSuccess = ReadFile (m_hFile, pBuffer, cb, &cbRead, &ov);
    CWR (fSuccess);

    CBREx (cbRead == cb, HRESULT_FROM_WIN32 (ERROR_HANDLE_EOF));



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMemoryByteSource::ReadAt
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CMemoryByteSource::ReadAt (ULONGLONG offset, void * pBuffer, DWORD cb)
{
    HRESULT                hr    = S_OK;
    const vector<BYTE>   & bytes = *m_bytesPtr;



    CBREx (offset <= bytes.size() && cb <= bytes.size() - offset, HRESULT_FROM_WIN32 (ERROR_HANDLE_EOF));

    memcpy (pBuffer, bytes.data() + offset, cb);



Error:
    return hr;
}





