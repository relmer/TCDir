#include "pch.h"

#include "ArchiveFileAccess.h"

#include "ArchiveDetails.h"
#include "ArchiveReport.h"
#include "ByteSource.h"
#include "IsoReader.h"
#include "LibArchiveReader.h"
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
//  Reads the entry list of one archive with the reader for its format.  When
//  result.m_detailsPtr is set, the reader also fills the format details.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveFileAccess::Read (const SArchiveSource & source, EArchiveFormat format, SArchiveReadResult & result)
{
    HRESULT                       hr            = S_OK;
    unique_ptr<IByteSource>       byteSourcePtr;
    shared_ptr<SArchiveDetails>   detailsPtr    = result.m_detailsPtr;



    result              = {};
    result.m_detailsPtr = detailsPtr;

    if (detailsPtr != nullptr)
    {
        *detailsPtr          = {};
        detailsPtr->m_format = format;
    }

    switch (format)
    {
        case EArchiveFormat::Zip:
            hr = OpenByteSource (source, byteSourcePtr);
            CHR (hr);

            hr = CZipReader::Read (*byteSourcePtr, result);
            CHR (hr);
            break;

        case EArchiveFormat::Iso:
            hr = OpenByteSource (source, byteSourcePtr);
            CHR (hr);

            hr = CIsoReader::Read (*byteSourcePtr, result);
            CHR (hr);
            break;

        case EArchiveFormat::LibArchive:
            hr = CLibArchiveReader::Read (source, result);
            CHR (hr);
            break;

        default:
            BAIL_OUT_IF (TRUE, HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED));
            break;
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileAccess::ExtractEntry
//
//  Reads the data of one file entry of an archive into memory through
//  archiveint.dll.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveFileAccess::ExtractEntry (
    const SArchiveSource             & source,
    wstring_view                       entryPath,
    ULONGLONG                          cbMax,
    shared_ptr<const vector<BYTE>>   & bytesPtr)
{
    HRESULT        hr    = S_OK;
    vector<BYTE>   bytes;



    bytesPtr.reset();

    hr = CLibArchiveReader::ExtractEntry (source, entryPath, cbMax, bytes);
    CHR (hr);

    bytesPtr = make_shared<const vector<BYTE>> (move (bytes));



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileAccess::Verify
//
//  Reads every entry of an archive and checks the checksums it stores.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveFileAccess::Verify (const SArchiveSource & source, EArchiveFormat format, SVerifyResult & result)
{
    UNREFERENCED_PARAMETER (source);
    UNREFERENCED_PARAMETER (format);

    result = {};
    return E_NOTIMPL;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileAccess::OpenByteSource
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveFileAccess::OpenByteSource (const SArchiveSource & source, unique_ptr<IByteSource> & byteSourcePtr)
{
    HRESULT                       hr      = S_OK;
    unique_ptr<CFileByteSource>   filePtr;



    if (source.m_bytesPtr != nullptr)
    {
        byteSourcePtr = make_unique<CMemoryByteSource> (source.m_bytesPtr);
    }
    else
    {
        filePtr = make_unique<CFileByteSource>();

        hr = filePtr->Open (source.m_path);
        CHR (hr);

        byteSourcePtr = move (filePtr);
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
    return CLibArchiveReader::IsAvailable();
}





