#include "pch.h"

#include "LibArchiveReader.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::SApi
//
//  Function pointers resolved from archiveint.dll.  libarchive declares
//  mode values as unsigned short on Windows.
//
////////////////////////////////////////////////////////////////////////////////

struct CLibArchiveReader::SApi
{
    using PfnReadNew          = void *         (__cdecl *) ();
    using PfnArchiveInt       = int            (__cdecl *) (void *);
    using PfnOpenFileName     = int            (__cdecl *) (void *, const wchar_t *, size_t);
    using PfnNextHeader       = int            (__cdecl *) (void *, void **);
    using PfnErrorString      = const char *   (__cdecl *) (void *);
    using PfnEntryPath        = const wchar_t * (__cdecl *) (void *);
    using PfnEntryMode        = unsigned short (__cdecl *) (void *);
    using PfnEntryInt         = int            (__cdecl *) (void *);
    using PfnEntryInt64       = LONGLONG       (__cdecl *) (void *);
    using PfnEntryLong        = long           (__cdecl *) (void *);

    bool              m_fLoaded                = false;
    PfnReadNew        m_pfnReadNew             = nullptr;
    PfnArchiveInt     m_pfnSupportFilterAll    = nullptr;
    PfnArchiveInt     m_pfnSupportFormatAll    = nullptr;
    PfnOpenFileName   m_pfnOpenFileName        = nullptr;
    PfnNextHeader     m_pfnNextHeader          = nullptr;
    PfnArchiveInt     m_pfnDataSkip            = nullptr;
    PfnArchiveInt     m_pfnHasEncryptedEntries = nullptr;
    PfnArchiveInt     m_pfnReadFree            = nullptr;
    PfnErrorString    m_pfnErrorString         = nullptr;
    PfnEntryPath      m_pfnEntryPath           = nullptr;
    PfnEntryMode      m_pfnEntryFileType       = nullptr;
    PfnEntryInt       m_pfnEntrySizeIsSet      = nullptr;
    PfnEntryInt64     m_pfnEntrySize           = nullptr;
    PfnEntryInt       m_pfnEntryMtimeIsSet     = nullptr;
    PfnEntryInt64     m_pfnEntryMtime          = nullptr;
    PfnEntryLong      m_pfnEntryMtimeNsec      = nullptr;
    PfnEntryInt       m_pfnEntryPermIsSet      = nullptr;
    PfnEntryMode      m_pfnEntryPerm           = nullptr;
    PfnEntryInt       m_pfnEntryIsEncrypted    = nullptr;
};





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::IsAvailable
//
////////////////////////////////////////////////////////////////////////////////

bool CLibArchiveReader::IsAvailable()
{
    return GetApi() != nullptr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::GetApi
//
//  Loads the DLL once per process.  Returns nullptr when the DLL or any
//  required export is missing.
//
////////////////////////////////////////////////////////////////////////////////

const CLibArchiveReader::SApi * CLibArchiveReader::GetApi()
{
    static SApi        s_api;
    static once_flag   s_onceFlag;



    call_once (s_onceFlag, [] () { LoadApi (s_api); });

    return s_api.m_fLoaded ? &s_api : nullptr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::LoadApi
//
//  Loads archiveint.dll only from System32, so a DLL with the same name in
//  the current directory or on the PATH is never used.  The module stays
//  loaded for the life of the process.
//
////////////////////////////////////////////////////////////////////////////////

void CLibArchiveReader::LoadApi (SApi & api)
{
    HRESULT   hr      = S_OK;
    HMODULE   hModule = LoadLibraryExW (L"archiveint.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);



    CWR (hModule != nullptr);

    api.m_pfnReadNew             = reinterpret_cast<SApi::PfnReadNew>      (GetProcAddress (hModule, "archive_read_new"));
    api.m_pfnSupportFilterAll    = reinterpret_cast<SApi::PfnArchiveInt>   (GetProcAddress (hModule, "archive_read_support_filter_all"));
    api.m_pfnSupportFormatAll    = reinterpret_cast<SApi::PfnArchiveInt>   (GetProcAddress (hModule, "archive_read_support_format_all"));
    api.m_pfnOpenFileName        = reinterpret_cast<SApi::PfnOpenFileName> (GetProcAddress (hModule, "archive_read_open_filename_w"));
    api.m_pfnNextHeader          = reinterpret_cast<SApi::PfnNextHeader>   (GetProcAddress (hModule, "archive_read_next_header"));
    api.m_pfnDataSkip            = reinterpret_cast<SApi::PfnArchiveInt>   (GetProcAddress (hModule, "archive_read_data_skip"));
    api.m_pfnHasEncryptedEntries = reinterpret_cast<SApi::PfnArchiveInt>   (GetProcAddress (hModule, "archive_read_has_encrypted_entries"));
    api.m_pfnReadFree            = reinterpret_cast<SApi::PfnArchiveInt>   (GetProcAddress (hModule, "archive_read_free"));
    api.m_pfnErrorString         = reinterpret_cast<SApi::PfnErrorString>  (GetProcAddress (hModule, "archive_error_string"));
    api.m_pfnEntryPath           = reinterpret_cast<SApi::PfnEntryPath>    (GetProcAddress (hModule, "archive_entry_pathname_w"));
    api.m_pfnEntryFileType       = reinterpret_cast<SApi::PfnEntryMode>    (GetProcAddress (hModule, "archive_entry_filetype"));
    api.m_pfnEntrySizeIsSet      = reinterpret_cast<SApi::PfnEntryInt>     (GetProcAddress (hModule, "archive_entry_size_is_set"));
    api.m_pfnEntrySize           = reinterpret_cast<SApi::PfnEntryInt64>   (GetProcAddress (hModule, "archive_entry_size"));
    api.m_pfnEntryMtimeIsSet     = reinterpret_cast<SApi::PfnEntryInt>     (GetProcAddress (hModule, "archive_entry_mtime_is_set"));
    api.m_pfnEntryMtime          = reinterpret_cast<SApi::PfnEntryInt64>   (GetProcAddress (hModule, "archive_entry_mtime"));
    api.m_pfnEntryMtimeNsec      = reinterpret_cast<SApi::PfnEntryLong>    (GetProcAddress (hModule, "archive_entry_mtime_nsec"));
    api.m_pfnEntryPermIsSet      = reinterpret_cast<SApi::PfnEntryInt>     (GetProcAddress (hModule, "archive_entry_perm_is_set"));
    api.m_pfnEntryPerm           = reinterpret_cast<SApi::PfnEntryMode>    (GetProcAddress (hModule, "archive_entry_perm"));
    api.m_pfnEntryIsEncrypted    = reinterpret_cast<SApi::PfnEntryInt>     (GetProcAddress (hModule, "archive_entry_is_encrypted"));

    api.m_fLoaded = api.m_pfnReadNew             && api.m_pfnSupportFilterAll && api.m_pfnSupportFormatAll &&
                    api.m_pfnOpenFileName        && api.m_pfnNextHeader       && api.m_pfnDataSkip         &&
                    api.m_pfnHasEncryptedEntries && api.m_pfnReadFree         && api.m_pfnErrorString      &&
                    api.m_pfnEntryPath           && api.m_pfnEntryFileType    && api.m_pfnEntrySizeIsSet   &&
                    api.m_pfnEntrySize           && api.m_pfnEntryMtimeIsSet  && api.m_pfnEntryMtime       &&
                    api.m_pfnEntryMtimeNsec      && api.m_pfnEntryPermIsSet   && api.m_pfnEntryPerm        &&
                    api.m_pfnEntryIsEncrypted;



Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::Read
//
//  Each call creates and frees a separate reader object, so concurrent calls
//  from worker threads are safe.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CLibArchiveReader::Read (const filesystem::path & path, SArchiveReadResult & result)
{
    HRESULT        hr       = S_OK;
    const SApi   * pApi     = GetApi();
    void         * pArchive = nullptr;
    int            iResult  = 0;



    result = {};

    CBREx (pApi != nullptr, HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED));

    pArchive = pApi->m_pfnReadNew();
    CPRA (pArchive);

    pApi->m_pfnSupportFilterAll (pArchive);
    pApi->m_pfnSupportFormatAll (pArchive);

    iResult = pApi->m_pfnOpenFileName (pArchive, path.c_str(), s_kcbReadBlock);
    CBREx (iResult >= s_kiArchiveWarn, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

    hr = ReadEntries (*pApi, pArchive, result);
    CHR (hr);



Error:
    if (pArchive != nullptr)
    {
        pApi->m_pfnReadFree (pArchive);
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::ReadEntries
//
//  Reading a header can fail partway through an archive whose headers are
//  encrypted (for example 7z with encrypted file names); that case is
//  reported as an encrypted listing rather than a corrupt archive.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CLibArchiveReader::ReadEntries (const SApi & api, void * pArchive, SArchiveReadResult & result)
{
    HRESULT                hr         = S_OK;
    void                 * pEntry     = nullptr;
    int                    iResult    = s_kiArchiveOk;
    int                    iEncrypted = 0;
    SLibArchiveEntryInfo   info;



    for (;;)
    {
        iResult = api.m_pfnNextHeader (pArchive, &pEntry);

        if (iResult == s_kiArchiveEof || iResult < s_kiArchiveWarn)
        {
            break;
        }

        GetEntryInfo (api, pEntry, info);
        result.m_vEntries.push_back (ConvertEntry (info));

        api.m_pfnDataSkip (pArchive);
    }

    if (iResult < s_kiArchiveWarn)
    {
        iEncrypted = api.m_pfnHasEncryptedEntries (pArchive);

        CBREx (iEncrypted > 0, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

        result.m_vEntries.clear();
        result.m_fListingEncrypted = true;
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::GetEntryInfo
//
////////////////////////////////////////////////////////////////////////////////

void CLibArchiveReader::GetEntryInfo (const SApi & api, void * pEntry, SLibArchiveEntryInfo & info)
{
    const wchar_t * pszPath = api.m_pfnEntryPath (pEntry);



    info               = {};
    info.m_strPath     = (pszPath != nullptr) ? pszPath : L"";
    info.m_uFileType   = api.m_pfnEntryFileType (pEntry);
    info.m_fSizeSet    = api.m_pfnEntrySizeIsSet (pEntry) != 0;
    info.m_llSize      = api.m_pfnEntrySize (pEntry);
    info.m_fMtimeSet   = api.m_pfnEntryMtimeIsSet (pEntry) != 0;
    info.m_llMtime     = api.m_pfnEntryMtime (pEntry);
    info.m_lMtimeNsec  = api.m_pfnEntryMtimeNsec (pEntry);
    info.m_fPermSet    = api.m_pfnEntryPermIsSet (pEntry) != 0;
    info.m_uPerm       = api.m_pfnEntryPerm (pEntry);
    info.m_fEncrypted  = api.m_pfnEntryIsEncrypted (pEntry) != 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::ConvertEntry
//
//  Converts libarchive values to an archive entry.  A size that is not set,
//  or is negative, becomes 0.  Missing owner write permission maps to
//  read-only, as for Unix-host .zip entries.
//
////////////////////////////////////////////////////////////////////////////////

SArchiveEntry CLibArchiveReader::ConvertEntry (const SLibArchiveEntryInfo & info)
{
    SArchiveEntry entry;



    entry.m_strPath    = info.m_strPath;
    entry.m_fDirectory = info.m_uFileType == s_kuFileTypeDirectory;
    entry.m_ullSize    = (info.m_fSizeSet && info.m_llSize > 0 && !entry.m_fDirectory) ? static_cast<ULONGLONG> (info.m_llSize) : 0;
    entry.m_fEncrypted = info.m_fEncrypted;

    if (info.m_fMtimeSet)
    {
        entry.m_ftLastWrite = UnixToFileTime (info.m_llMtime, info.m_lMtimeNsec);
    }

    if (entry.m_fDirectory)
    {
        entry.m_dwAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    }

    if (info.m_fPermSet && (info.m_uPerm & s_kuPermOwnerWrite) == 0)
    {
        entry.m_dwAttributes |= FILE_ATTRIBUTE_READONLY;
    }

    return entry;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::UnixToFileTime
//
//  Times before 1601 become a zero FILETIME.
//
////////////////////////////////////////////////////////////////////////////////

FILETIME CLibArchiveReader::UnixToFileTime (LONGLONG llSeconds, long lNanoseconds)
{
    LONGLONG         llTicks = llSeconds * s_kllTicksPerSecond + s_kllUnixEpochTicks + lNanoseconds / s_klNanosecondsPerTick;
    ULARGE_INTEGER   uli     = {};
    FILETIME         ft      = {};



    if (llTicks > 0)
    {
        uli.QuadPart      = static_cast<ULONGLONG> (llTicks);
        ft.dwLowDateTime  = uli.LowPart;
        ft.dwHighDateTime = uli.HighPart;
    }

    return ft;
}





