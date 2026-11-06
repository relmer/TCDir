#pragma once

#include "AutoHandle.h"





////////////////////////////////////////////////////////////////////////////////
//
//  IByteSource
//
//  Random-access, read-only byte stream used by the archive readers.
//  Production code uses CFileByteSource; tests use CMemoryByteSource.
//
////////////////////////////////////////////////////////////////////////////////

class IByteSource
{
public:
    virtual ~IByteSource() = default;

    virtual ULONGLONG Size() const = 0;
    virtual HRESULT   ReadAt (ULONGLONG offset, void * pBuffer, DWORD cb) = 0;
};





////////////////////////////////////////////////////////////////////////////////
//
//  CFileByteSource
//
////////////////////////////////////////////////////////////////////////////////

class CFileByteSource : public IByteSource
{
public:
    HRESULT   Open   (const filesystem::path & path);

    ULONGLONG Size() const override { return m_ullSize; }
    HRESULT   ReadAt (ULONGLONG offset, void * pBuffer, DWORD cb) override;

private:
    AutoHandle   m_hFile;
    ULONGLONG    m_ullSize = 0;
};





////////////////////////////////////////////////////////////////////////////////
//
//  CMemoryByteSource
//
////////////////////////////////////////////////////////////////////////////////

class CMemoryByteSource : public IByteSource
{
public:
    explicit CMemoryByteSource (vector<BYTE> bytes)                      : m_bytesPtr (make_shared<const vector<BYTE>> (std::move (bytes))) {}
    explicit CMemoryByteSource (shared_ptr<const vector<BYTE>> bytesPtr) : m_bytesPtr (std::move (bytesPtr)) {}

    ULONGLONG Size() const override { return m_bytesPtr->size(); }
    HRESULT   ReadAt (ULONGLONG offset, void * pBuffer, DWORD cb) override;

private:
    shared_ptr<const vector<BYTE>> m_bytesPtr;
};
