#include "pch.h"
#include "IatPatch.h"

#include "../../TCDirCore/Ehm.h"





namespace IatPatch
{

    ////////////////////////////////////////////////////////////////////////////////
    //
    //  FindImportEntry
    //
    //  Walks the IAT of the specified module to find the import entry for a
    //  function. Returns a pointer to the function pointer in the IAT, or
    //  nullptr if not found.
    //
    //  Parameters:
    //      hModule     - Module whose IAT to search (e.g., GetModuleHandle (L"UnitTest.dll"))
    //      pszDllName  - Name of the DLL exporting the function (e.g., "kernel32.dll")
    //      pszFuncName - Name of the function (e.g., "FindFirstFileW")
    //
    //  Returns:
    //      Pointer to the IAT entry (void **) or nullptr if not found.
    //
    ////////////////////////////////////////////////////////////////////////////////

    void ** FindImportEntry (HMODULE hModule, LPCSTR pszDllName, LPCSTR pszFuncName)
    {
        HRESULT                   hr          = S_OK;
        void                   ** ppfnResult  = nullptr;
        PIMAGE_DOS_HEADER         pDosHeader  = nullptr;
        PIMAGE_NT_HEADERS         pNtHeaders  = nullptr;
        PIMAGE_DATA_DIRECTORY     pImportDir  = nullptr;
        PIMAGE_IMPORT_DESCRIPTOR  pImportDesc = nullptr;



        CBR (hModule != nullptr);

        // Get the DOS header and verify signature
        pDosHeader = (PIMAGE_DOS_HEADER) hModule;
        CBR (pDosHeader->e_magic == IMAGE_DOS_SIGNATURE);

        // Get the NT headers and verify signature
        pNtHeaders = (PIMAGE_NT_HEADERS) ((BYTE *) hModule + pDosHeader->e_lfanew);
        CBR (pNtHeaders->Signature == IMAGE_NT_SIGNATURE);

        // Get the import directory
        pImportDir = &pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        CBR (pImportDir->VirtualAddress != 0);

        pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR) ((BYTE *) hModule + pImportDir->VirtualAddress);

        // Walk through each imported DLL
        for (; pImportDesc->Name != 0; ++pImportDesc)
        {
            LPCSTR pszCurrentDll = (LPCSTR) ((BYTE *) hModule + pImportDesc->Name);

            if (_stricmp (pszCurrentDll, pszDllName) != 0)
            {
                continue;
            }

            // Found the DLL - now walk its import name table (INT) and 
            // import address table (IAT) in parallel
            PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA) ((BYTE *) hModule + pImportDesc->OriginalFirstThunk);
            PIMAGE_THUNK_DATA pIat   = (PIMAGE_THUNK_DATA) ((BYTE *) hModule + pImportDesc->FirstThunk);

            for (; pThunk->u1.AddressOfData != 0; ++pThunk, ++pIat)
            {
                // Skip ordinal imports
                if (IMAGE_SNAP_BY_ORDINAL (pThunk->u1.Ordinal))
                {
                    continue;
                }

                // Get the import name
                PIMAGE_IMPORT_BY_NAME pImportByName = (PIMAGE_IMPORT_BY_NAME) ((BYTE *) hModule + pThunk->u1.AddressOfData);

                if (strcmp (pImportByName->Name, pszFuncName) == 0)
                {
                    ppfnResult = (void **) &pIat->u1.Function;
                    BAIL_OUT_IF (TRUE, S_OK);
                }
            }
        }

    Error:
        UNREFERENCED_PARAMETER (hr);
        return ppfnResult;
    }





    ////////////////////////////////////////////////////////////////////////////////
    //
    //  PatchImport
    //
    //  Replaces an IAT entry with a new function pointer.
    //  Returns the original function pointer.
    //
    //  Parameters:
    //      ppfnImport - Pointer to IAT entry (from FindImportEntry)
    //      pfnNew     - New function pointer to install
    //
    //  Returns:
    //      Original function pointer, or nullptr on failure.
    //
    ////////////////////////////////////////////////////////////////////////////////

    void * PatchImport (void ** ppfnImport, void * pfnNew)
    {
        HRESULT   hr           = S_OK;
        BOOL      fSuccess     = FALSE;
        void    * pfnOriginal  = nullptr;
        DWORD     dwOldProtect = 0;



        CBR (ppfnImport != nullptr);
        CBR (pfnNew     != nullptr);

        // Make the IAT entry writable
        fSuccess = VirtualProtect (ppfnImport, sizeof (void *), PAGE_READWRITE, &dwOldProtect);
        CBR (fSuccess);

        // Swap the function pointer
        pfnOriginal = *ppfnImport;
        *ppfnImport = pfnNew;

        // Restore original protection
        VirtualProtect (ppfnImport, sizeof (void *), dwOldProtect, &dwOldProtect);

    Error:
        UNREFERENCED_PARAMETER (hr);
        return pfnOriginal;
    }

} // namespace IatPatch

