#include "pch.h"

#include "ArchiveFileAccess.h"

#include "ByteSource.h"
#include "ZipReader.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileAccess::GetAttributes
//
//  Returns INVALID_FILE_ATTRIBUTES when the path does not exist.
//
////////////////////////////////////////////////////////////////////////////////

DWORD CArchiveFileAccess::GetAttributes (const filesystem::path & path)
{
    return GetFileAttributesW (path.c_str());
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileAccess::Read
//
//  Reads the entry list of one archive with the reader for its format.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveFileAccess::Read (const filesystem::path & path, EArchiveFormat format, SArchiveReadResult & result)
{
    HRESULT           hr     = S_OK;
    CFileByteSource   source;



    result = {};

    switch (format)
    {
        case EArchiveFormat::Zip:
            hr = source.Open (path);
            CHR (hr);

            hr = CZipReader::Read (source, result);
            CHR (hr);
            break;

        default:
            CHR (HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED));
            break;
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileAccess::IsLibArchiveAvailable
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveFileAccess::IsLibArchiveAvailable()
{
    return false;
}





