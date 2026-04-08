#pragma once





template <auto CloseFunc>
class CAutoHandleT
{
public:
    CAutoHandleT (void)
        : m_h (INVALID_HANDLE_VALUE)
    {
    }

    //
    //  Non-explicit so callers can write natural assignment syntax:
    //    hFile = CreateFileW (...);
    //

    CAutoHandleT (HANDLE h)
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
        other.m_h = INVALID_HANDLE_VALUE;
    }

    CAutoHandleT & operator= (CAutoHandleT && other) noexcept
    {
        if (this != &other)
        {
            if (IsValid())
            {
                CloseFunc (m_h);
            }

            m_h       = other.m_h;
            other.m_h = INVALID_HANDLE_VALUE;
        }

        return *this;
    }

    //
    //  Copy is deleted — handles are unique resources with single ownership.
    //

    CAutoHandleT (const CAutoHandleT &)             = delete;
    CAutoHandleT & operator= (const CAutoHandleT &) = delete;

    //
    //  Implicit conversion to HANDLE so the wrapper can be passed directly
    //  to Win32 APIs without a .Get() accessor.
    //

    operator HANDLE (void) const
    {
        return m_h;
    }

private:
    bool IsValid (void) const
    {
        return m_h != INVALID_HANDLE_VALUE && m_h != nullptr;
    }

    HANDLE m_h;
};





using AutoHandle     = CAutoHandleT<CloseHandle>;
using AutoFindHandle = CAutoHandleT<FindClose>;