#pragma once





template <typename THandle, THandle InvalidValue, auto CloseFunc>
class CAutoHandleT
{
public:
    CAutoHandleT (void)
        : m_h (InvalidValue)
    {
    }

    //
    //  Non-explicit so callers can write natural assignment syntax:
    //    hFile = CreateFileW (...);
    //

    CAutoHandleT (THandle h)
        : m_h (h)
    {
    }

    ~CAutoHandleT (void)
    {
        if (IsValid())
        {
            CloseFunc (m_h);
        }
    }

    //
    //  Move operations — noexcept because they only assign a pointer-sized
    //  value and cannot fail.  Required for use in STL containers.
    //

    CAutoHandleT (CAutoHandleT && other) noexcept
        : m_h (other.m_h)
    {
        other.m_h = InvalidValue;
    }

    CAutoHandleT & operator= (CAutoHandleT && other) noexcept
    {
        if (this != std::addressof (other))
        {
            if (IsValid())
            {
                CloseFunc (m_h);
            }

            m_h       = other.m_h;
            other.m_h = InvalidValue;
        }

        return *this;
    }

    //
    //  Copy is deleted — handles are unique resources with single ownership.
    //

    CAutoHandleT (const CAutoHandleT &)             = delete;
    CAutoHandleT & operator= (const CAutoHandleT &) = delete;

    //
    //  Implicit conversion to the underlying handle so the wrapper can be
    //  passed directly to Win32 APIs without a .Get() accessor.
    //

    operator THandle (void) const
    {
        return m_h;
    }

    //
    //  Yields the address of the internal slot for out-parameter APIs such as
    //  RegOpenKeyEx (..., key.GetRef()).  A named method rather than operator&
    //  so that taking the wrapper's own address stays normal.  Asserts the
    //  slot is empty first so an existing handle is never silently leaked.
    //

    THandle * GetRef (void)
    {
        ASSERT (!IsValid());
        return &m_h;
    }

private:
    bool IsValid (void) const
    {
        return m_h != InvalidValue && m_h != nullptr;
    }

    THandle m_h;
};




using AutoHandle         = CAutoHandleT<HANDLE, INVALID_HANDLE_VALUE, CloseHandle>;
using AutoFindHandle     = CAutoHandleT<HANDLE, INVALID_HANDLE_VALUE, FindClose>;
using AutoInternetHandle = CAutoHandleT<HANDLE, INVALID_HANDLE_VALUE, InternetCloseHandle>;

//
//  HKEY is a distinct pointer type from HANDLE and registry keys are invalid
//  as nullptr (not INVALID_HANDLE_VALUE), so it instantiates the template with
//  its own type and sentinel.  Use key.GetRef() with RegOpenKeyEx /
//  RegCreateKeyEx.
//

using CAutoRegKey = CAutoHandleT<HKEY, nullptr, RegCloseKey>;