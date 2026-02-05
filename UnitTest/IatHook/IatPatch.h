#pragma once
////////////////////////////////////////////////////////////////////////////////
//
//  Lightweight IAT (Import Address Table) patching utility for testing.
//
//  Usage:
//      void ** ppfnOriginal = IatPatch::FindImportEntry (hModule, "kernel32.dll", "FindFirstFileW");
//      void *  pfnOriginal  = IatPatch::PatchImport (ppfnOriginal, &MyMockFunction);
//      // ... run tests ...
//      IatPatch::PatchImport (ppfnOriginal, pfnOriginal);  // restore
//
////////////////////////////////////////////////////////////////////////////////

namespace IatPatch
{
    void ** FindImportEntry (HMODULE hModule, LPCSTR pszDllName, LPCSTR pszFuncName);
    void *  PatchImport     (void ** ppfnImport, void * pfnNew);
}
