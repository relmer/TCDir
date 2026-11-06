#pragma once

#include "ArchiveEntry.h"





class IByteSource;





////////////////////////////////////////////////////////////////////////////////
//
//  IArchiveFileAccess
//
//  Injectable access to the real files behind archive browsing.  Production
//  code uses CArchiveFileAccess; tests substitute an in-memory mock.
//
////////////////////////////////////////////////////////////////////////////////

class IArchiveFileAccess
{
public:
    virtual ~IArchiveFileAccess() = default;

    virtual DWORD   GetAttributes          (const filesystem::path & path) = 0;
    virtual HRESULT Read                   (const SArchiveSource & source, EArchiveFormat format, SArchiveReadResult & result) = 0;
    virtual HRESULT ExtractEntry           (const SArchiveSource                 & source,
                                            wstring_view                           entryPath,
                                            ULONGLONG                              cbMax,
                                            shared_ptr<const vector<BYTE>>       & bytesPtr) = 0;
    virtual bool    IsLibArchiveAvailable() = 0;
};





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileAccess
//
////////////////////////////////////////////////////////////////////////////////

class CArchiveFileAccess : public IArchiveFileAccess
{
public:
    DWORD   GetAttributes          (const filesystem::path & path) override;
    HRESULT Read                   (const SArchiveSource & source, EArchiveFormat format, SArchiveReadResult & result) override;
    HRESULT ExtractEntry           (const SArchiveSource                 & source,
                                    wstring_view                           entryPath,
                                    ULONGLONG                              cbMax,
                                    shared_ptr<const vector<BYTE>>       & bytesPtr) override;
    bool    IsLibArchiveAvailable() override;

private:
    static HRESULT OpenByteSource  (const SArchiveSource & source, unique_ptr<IByteSource> & byteSourcePtr);
};
