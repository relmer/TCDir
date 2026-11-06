#include "pch.h"

#include "LibArchiveReader.h"

#include "ArchiveDetails.h"
#include "ArchiveTree.h"





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
    using PfnOpenMemory       = int            (__cdecl *) (void *, const void *, size_t);
    using PfnReadData         = INT_PTR        (__cdecl *) (void *, void *, size_t);
    using PfnNextHeader       = int            (__cdecl *) (void *, void **);
    using PfnErrorString      = const char *   (__cdecl *) (void *);
    using PfnEntryPath        = const wchar_t * (__cdecl *) (void *);
    using PfnEntryMode        = unsigned short (__cdecl *) (void *);
    using PfnEntryInt         = int            (__cdecl *) (void *);
    using PfnEntryInt64       = LONGLONG       (__cdecl *) (void *);
    using PfnEntryLong        = long           (__cdecl *) (void *);
    using PfnFilterName       = const char *   (__cdecl *) (void *, int);
    using PfnFilterBytes      = LONGLONG       (__cdecl *) (void *, int);
    using PfnEntryAclCount    = int            (__cdecl *) (void *, int);

    bool              m_fLoaded                = false;
    PfnReadNew        m_pfnReadNew             = nullptr;
    PfnArchiveInt     m_pfnSupportFilterAll    = nullptr;
    PfnArchiveInt     m_pfnSupportFormatAll    = nullptr;
    PfnOpenFileName   m_pfnOpenFileName        = nullptr;
    PfnOpenMemory     m_pfnOpenMemory          = nullptr;
    PfnReadData       m_pfnReadData            = nullptr;
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

    //
    // Optional, for --Info details
    //

    PfnErrorString    m_pfnFormatName             = nullptr;
    PfnArchiveInt     m_pfnFilterCount            = nullptr;
    PfnFilterName     m_pfnFilterName             = nullptr;
    PfnFilterBytes    m_pfnFilterBytes            = nullptr;
    PfnEntryPath      m_pfnEntryUname             = nullptr;
    PfnEntryPath      m_pfnEntryGname             = nullptr;
    PfnEntryPath      m_pfnEntryHardlink          = nullptr;
    PfnEntryInt       m_pfnEntrySparseCount       = nullptr;
    PfnEntryInt       m_pfnEntryXattrCount        = nullptr;
    PfnEntryAclCount  m_pfnEntryAclCount          = nullptr;
    PfnEntryInt       m_pfnEntryBirthtimeIsSet    = nullptr;
    PfnEntryInt       m_pfnEntryAtimeIsSet        = nullptr;
    PfnEntryInt       m_pfnEntryMetadataEncrypted = nullptr;
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
    api.m_pfnOpenMemory          = reinterpret_cast<SApi::PfnOpenMemory>   (GetProcAddress (hModule, "archive_read_open_memory"));
    api.m_pfnReadData            = reinterpret_cast<SApi::PfnReadData>     (GetProcAddress (hModule, "archive_read_data"));
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
                    api.m_pfnEntryIsEncrypted    && api.m_pfnOpenMemory       && api.m_pfnReadData;

    LoadDetailApi (hModule, api);



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

HRESULT CLibArchiveReader::Read (const SArchiveSource & source, SArchiveReadResult & result)
{
    HRESULT        hr       = S_OK;
    const SApi   * pApi     = GetApi();
    void         * pArchive = nullptr;



    result.Reset();

    CBREx (pApi != nullptr, HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED));

    hr = Open (*pApi, source, pArchive);
    CHR (hr);

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
//  CLibArchiveReader::ExtractEntry
//
//  Reads the data of the first file entry at entryPath into bytes.  Returns
//  ERROR_FILE_NOT_FOUND when no file entry has that path,
//  ERROR_FILE_ENCRYPTED for an encrypted entry, and ERROR_FILE_TOO_LARGE
//  when the data is larger than cbMax.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CLibArchiveReader::ExtractEntry (
    const SArchiveSource & source,
    wstring_view           entryPath,
    ULONGLONG              cbMax,
    vector<BYTE>         & bytes)
{
    HRESULT                hr       = S_OK;
    const SApi           * pApi     = GetApi();
    void                 * pArchive = nullptr;
    void                 * pEntry   = nullptr;
    int                    iResult  = s_kiArchiveOk;
    bool                   fFound   = false;
    SLibArchiveEntryInfo   info;



    bytes.clear();

    CBREx (pApi != nullptr, HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED));

    hr = Open (*pApi, source, pArchive);
    CHR (hr);

    while (!fFound)
    {
        iResult = pApi->m_pfnNextHeader (pArchive, &pEntry);
        CBREx (iResult != s_kiArchiveEof, HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND));
        CBREx (iResult >= s_kiArchiveWarn, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

        GetEntryInfo (*pApi, pEntry, info);

        fFound = info.m_uFileType != s_kuFileTypeDirectory && IsSamePath (info.m_strPath, entryPath);
    }

    CBREx (!info.m_fEncrypted, HRESULT_FROM_WIN32 (ERROR_FILE_ENCRYPTED));
    CBREx (!info.m_fSizeSet || info.m_llSize < 0 || static_cast<ULONGLONG> (info.m_llSize) <= cbMax,
           HRESULT_FROM_WIN32 (ERROR_FILE_TOO_LARGE));

    hr = ReadEntryData (*pApi, pArchive, cbMax, (info.m_fSizeSet && info.m_llSize > 0) ? static_cast<ULONGLONG> (info.m_llSize) : 0, bytes);
    CHR (hr);



Error:
    if (pArchive != nullptr)
    {
        pApi->m_pfnReadFree (pArchive);
    }

    if (FAILED (hr))
    {
        bytes.clear();
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::IsSamePath
//
//  Compares two paths inside an archive component by component, ignoring
//  case and the separator style.  Components that CArchiveTree drops (empty,
//  "." and "..") are ignored, so a path from the archive tree matches the
//  path stored in the archive.
//
////////////////////////////////////////////////////////////////////////////////

bool CLibArchiveReader::IsSamePath (wstring_view path1, wstring_view path2)
{
    vector<wstring>   components1;
    vector<wstring>   components2;
    bool              fSame       = false;



    CArchiveTree::SplitPath (path1, components1);
    CArchiveTree::SplitPath (path2, components2);

    fSame = !components1.empty() && components1.size() == components2.size();

    for (size_t i = 0; fSame && i < components1.size(); ++i)
    {
        fSame = CompareStringOrdinal (components1[i].c_str(), -1, components2[i].c_str(), -1, TRUE) == CSTR_EQUAL;
    }

    return fSame;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::Open
//
//  Creates a reader for source: the real file, or the bytes in memory.  On
//  failure the reader is freed and pArchive is nullptr.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CLibArchiveReader::Open (const SApi & api, const SArchiveSource & source, void *& pArchive)
{
    HRESULT   hr      = S_OK;
    int       iResult = 0;



    pArchive = api.m_pfnReadNew();
    CPRA (pArchive);

    api.m_pfnSupportFilterAll (pArchive);
    api.m_pfnSupportFormatAll (pArchive);

    if (source.m_bytesPtr != nullptr)
    {
        iResult = api.m_pfnOpenMemory (pArchive, source.m_bytesPtr->data(), source.m_bytesPtr->size());
    }
    else
    {
        iResult = api.m_pfnOpenFileName (pArchive, source.m_path.c_str(), s_kcbReadBlock);
    }

    CBREx (iResult >= s_kiArchiveWarn, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));



Error:
    if (FAILED (hr) && pArchive != nullptr)
    {
        api.m_pfnReadFree (pArchive);
        pArchive = nullptr;
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

        if (result.m_detailsPtr != nullptr)
        {
            AddEntryDetails (api, pEntry, info, result.m_detailsPtr->m_libArchive);
        }

        api.m_pfnDataSkip (pArchive);
    }

    if (result.m_detailsPtr != nullptr)
    {
        AddStreamDetails (api, pArchive, result.m_detailsPtr->m_libArchive);
    }

    if (iResult < s_kiArchiveWarn)
    {
        iEncrypted = api.m_pfnHasEncryptedEntries (pArchive);

        CBREx (iEncrypted > 0, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

        result.m_vEntries.clear();
        result.m_fListingEncrypted = true;

        if (result.m_detailsPtr != nullptr)
        {
            result.m_detailsPtr->m_libArchive.m_fHeadersEncrypted = true;
        }
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::LoadDetailApi
//
//  Exports used only for --Info details.  Each one is optional.  When an
//  export is missing, its pointer is null and its value is unknown.
//
////////////////////////////////////////////////////////////////////////////////

void CLibArchiveReader::LoadDetailApi (HMODULE hModule, SApi & api)
{
    api.m_pfnFormatName          = reinterpret_cast<SApi::PfnErrorString>  (GetProcAddress (hModule, "archive_format_name"));
    api.m_pfnFilterCount         = reinterpret_cast<SApi::PfnArchiveInt>   (GetProcAddress (hModule, "archive_filter_count"));
    api.m_pfnFilterName          = reinterpret_cast<SApi::PfnFilterName>   (GetProcAddress (hModule, "archive_filter_name"));
    api.m_pfnFilterBytes         = reinterpret_cast<SApi::PfnFilterBytes>  (GetProcAddress (hModule, "archive_filter_bytes"));
    api.m_pfnEntryUname          = reinterpret_cast<SApi::PfnEntryPath>    (GetProcAddress (hModule, "archive_entry_uname_w"));
    api.m_pfnEntryGname          = reinterpret_cast<SApi::PfnEntryPath>    (GetProcAddress (hModule, "archive_entry_gname_w"));
    api.m_pfnEntryHardlink       = reinterpret_cast<SApi::PfnEntryPath>    (GetProcAddress (hModule, "archive_entry_hardlink_w"));
    api.m_pfnEntrySparseCount    = reinterpret_cast<SApi::PfnEntryInt>     (GetProcAddress (hModule, "archive_entry_sparse_count"));
    api.m_pfnEntryXattrCount     = reinterpret_cast<SApi::PfnEntryInt>     (GetProcAddress (hModule, "archive_entry_xattr_count"));
    api.m_pfnEntryAclCount       = reinterpret_cast<SApi::PfnEntryAclCount>(GetProcAddress (hModule, "archive_entry_acl_count"));
    api.m_pfnEntryBirthtimeIsSet = reinterpret_cast<SApi::PfnEntryInt>     (GetProcAddress (hModule, "archive_entry_birthtime_is_set"));
    api.m_pfnEntryAtimeIsSet     = reinterpret_cast<SApi::PfnEntryInt>     (GetProcAddress (hModule, "archive_entry_atime_is_set"));
    api.m_pfnEntryMetadataEncrypted = reinterpret_cast<SApi::PfnEntryInt>  (GetProcAddress (hModule, "archive_entry_is_metadata_encrypted"));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::AddEntryDetails
//
//  Adds one entry to the link, ownership, permission and attribute totals.
//
////////////////////////////////////////////////////////////////////////////////

void CLibArchiveReader::AddEntryDetails (const SApi & api, void * pEntry, const SLibArchiveEntryInfo & info, SLibArchiveDetails & lib)
{
    if (info.m_uFileType == s_kuFileTypeSymlink)
    {
        ++lib.m_cSymlinks;
    }

    if (api.m_pfnEntryHardlink != nullptr && api.m_pfnEntryHardlink (pEntry) != nullptr)
    {
        ++lib.m_cHardLinks;
    }

    AddName (api.m_pfnEntryUname, pEntry, lib.m_mapOwners);
    AddName (api.m_pfnEntryGname, pEntry, lib.m_mapGroups);

    if (info.m_fPermSet)
    {
        lib.m_mapPermissions[info.m_uPerm & s_kuPermMask] += 1;
    }

    lib.m_cBirthTimes  += CountIf (api.m_pfnEntryBirthtimeIsSet, pEntry);
    lib.m_cAccessTimes += CountIf (api.m_pfnEntryAtimeIsSet,     pEntry);
    lib.m_cSparse      += CountIf (api.m_pfnEntrySparseCount,    pEntry);
    lib.m_cXattrs      += CountIf (api.m_pfnEntryXattrCount,     pEntry);

    if (api.m_pfnEntryAclCount != nullptr && api.m_pfnEntryAclCount (pEntry, s_kiAclTypeAll) > 0)
    {
        ++lib.m_cAcls;
    }

    if (api.m_pfnEntryMetadataEncrypted != nullptr && api.m_pfnEntryMetadataEncrypted (pEntry) != 0)
    {
        lib.m_fHeadersEncrypted = true;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::AddStreamDetails
//
//  The format name, the filters (outermost first; the last filter is the
//  raw read and is not listed), and the stream byte totals.  The totals are
//  meaningful only when a filter decompressed the whole stream; formats such
//  as ZIP and 7z skip entry data.
//
////////////////////////////////////////////////////////////////////////////////

void CLibArchiveReader::AddStreamDetails (const SApi & api, void * pArchive, SLibArchiveDetails & lib)
{
    int cFilters = (api.m_pfnFilterCount != nullptr) ? api.m_pfnFilterCount (pArchive) : 0;



    if (api.m_pfnFormatName != nullptr)
    {
        lib.m_strFormatName = Widen (api.m_pfnFormatName (pArchive));
    }

    for (int iFilter = 0; api.m_pfnFilterName != nullptr && iFilter + 1 < cFilters; ++iFilter)
    {
        lib.m_vFilters.push_back (Widen (api.m_pfnFilterName (pArchive, iFilter)));
    }

    if (cFilters > 1 && api.m_pfnFilterBytes != nullptr)
    {
        lib.m_ullStreamCompressed   = static_cast<ULONGLONG> (max<LONGLONG> (0, api.m_pfnFilterBytes (pArchive, s_kiRawFilter)));
        lib.m_ullStreamUncompressed = static_cast<ULONGLONG> (max<LONGLONG> (0, api.m_pfnFilterBytes (pArchive, 0)));
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::AddName
//
////////////////////////////////////////////////////////////////////////////////

void CLibArchiveReader::AddName (SApi::PfnEntryPath pfnName, void * pEntry, map<wstring, ULONGLONG> & counts)
{
    const wchar_t * pszName = (pfnName != nullptr) ? pfnName (pEntry) : nullptr;



    if (pszName != nullptr && *pszName != L'\0')
    {
        counts[pszName] += 1;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::CountIf
//
//  1 when the optional function exists and returns a positive value.
//
////////////////////////////////////////////////////////////////////////////////

ULONGLONG CLibArchiveReader::CountIf (SApi::PfnEntryInt pfn, void * pEntry)
{
    return (pfn != nullptr && pfn (pEntry) > 0) ? 1 : 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::Widen
//
//  libarchive format and filter strings are ASCII.
//
////////////////////////////////////////////////////////////////////////////////

wstring CLibArchiveReader::Widen (const char * psz)
{
    wstring str;



    for (; psz != nullptr && *psz != '\0'; ++psz)
    {
        str += static_cast<wchar_t> (static_cast<unsigned char> (*psz));
    }

    return str;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader::ReadEntryData
//
//  Reads the data of the current entry.  cbExpected is the size from the
//  entry header (0 when unknown) and reserves the buffer up front.  An entry
//  whose data cannot be read (for example a compression method the build
//  doesn't support) is reported as ERROR_BAD_FORMAT.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CLibArchiveReader::ReadEntryData (const SApi & api, void * pArchive, ULONGLONG cbMax, ULONGLONG cbExpected, vector<BYTE> & bytes)
{
    HRESULT        hr      = S_OK;
    vector<BYTE>   block   (s_kcbReadBlock);
    INT_PTR        cbRead  = 0;



    bytes.reserve (static_cast<size_t> (min (cbExpected, cbMax)));

    for (;;)
    {
        cbRead = api.m_pfnReadData (pArchive, block.data(), block.size());
        CBREx (cbRead >= 0, HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT));

        if (cbRead == 0)
        {
            break;
        }

        CBREx (bytes.size() + static_cast<size_t> (cbRead) <= cbMax, HRESULT_FROM_WIN32 (ERROR_FILE_TOO_LARGE));

        bytes.insert (bytes.end(), block.begin(), block.begin() + cbRead);
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





