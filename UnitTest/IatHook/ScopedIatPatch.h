#pragma once
////////////////////////////////////////////////////////////////////////////////
//
//  RAII wrapper for IAT patching. Automatically restores original function
//  on destruction.
//
//  Usage:
//      {
//          ScopedIatPatch<decltype(&FindFirstFileW)> patch (
//              L"UnitTest.dll", "kernel32.dll", "FindFirstFileW", &MockFindFirstFileW);
//
//          // patch.Original() returns the real FindFirstFileW for pass-through
//          // ... run tests ...
//      }
//      // Original function restored here
//
////////////////////////////////////////////////////////////////////////////////

#include "IatPatch.h"





template <typename TFunc>
class ScopedIatPatch
{
public:
    ////////////////////////////////////////////////////////////////////////////
    //
    //  ScopedIatPatch
    //
    //  Constructor - finds and patches the IAT entry.
    //
    //  Parameters:
    //      hModule       - Module whose IAT to patch (use GetModuleHandleEx to get it)
    //      pszDllName    - DLL exporting the function (e.g., "kernel32.dll")
    //      pszFuncName   - Function name (e.g., "FindFirstFileW")
    //      pfnHook       - Hook function to install
    //
    ////////////////////////////////////////////////////////////////////////////

    ScopedIatPatch (HMODULE hModule, LPCSTR pszDllName, LPCSTR pszFuncName, TFunc pfnHook) :
        m_ppfnImport (nullptr),
        m_pfnOriginal (nullptr)
    {
        if (!hModule)
        {
            return;
        }

        m_ppfnImport = IatPatch::FindImportEntry (hModule, pszDllName, pszFuncName);
        if (!m_ppfnImport)
        {
            return;
        }

        m_pfnOriginal = reinterpret_cast<TFunc> (
            IatPatch::PatchImport (m_ppfnImport, reinterpret_cast<void*> (pfnHook)));
    }





    ////////////////////////////////////////////////////////////////////////////
    //
    //  ~ScopedIatPatch
    //
    //  Destructor - restores the original function pointer.
    //
    ////////////////////////////////////////////////////////////////////////////

    ~ScopedIatPatch()
    {
        if (m_ppfnImport && m_pfnOriginal)
        {
            IatPatch::PatchImport (m_ppfnImport, reinterpret_cast<void*> (m_pfnOriginal));
        }
    }





    ////////////////////////////////////////////////////////////////////////////
    //
    //  Original
    //
    //  Returns the original function pointer for pass-through calls.
    //
    ////////////////////////////////////////////////////////////////////////////

    TFunc Original() const
    {
        return m_pfnOriginal;
    }





    ////////////////////////////////////////////////////////////////////////////
    //
    //  IsPatched
    //
    //  Returns true if the patch was successfully installed.
    //
    ////////////////////////////////////////////////////////////////////////////

    bool IsPatched() const
    {
        return m_pfnOriginal != nullptr;
    }



    //
    // Non-copyable, non-movable
    //

    ScopedIatPatch (const ScopedIatPatch &) = delete;
    ScopedIatPatch & operator= (const ScopedIatPatch &) = delete;
    ScopedIatPatch (ScopedIatPatch &&) = delete;
    ScopedIatPatch & operator= (ScopedIatPatch &&) = delete;

private:
    void** m_ppfnImport;
    TFunc  m_pfnOriginal;
};

