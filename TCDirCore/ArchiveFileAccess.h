#pragma once

#include "ArchiveEntry.h"





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
    virtual HRESULT Read                   (const filesystem::path & path, EArchiveFormat format, SArchiveReadResult & result) = 0;
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
    HRESULT Read                   (const filesystem::path & path, EArchiveFormat format, SArchiveReadResult & result) override;
    bool    IsLibArchiveAvailable() override;
};
