#include "pch.h"

#include "ArchiveFileSystem.h"

#include "ArchiveFormat.h"
#include "Flag.h"
#include "WildcardMatcher.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::CArchiveFileSystem
//
////////////////////////////////////////////////////////////////////////////////

CArchiveFileSystem::CArchiveFileSystem (shared_ptr<IArchiveFileAccess> accessPtr) :
    m_accessPtr (accessPtr)
{
    m_fLibArchiveAvailable = m_accessPtr->IsLibArchiveAvailable();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::IsArchiveFile
//
//  True for a file (not a directory) with a recognized archive extension.
//  Cloud placeholders whose data is not on the disk are excluded, because
//  reading them would start a download.
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveFileSystem::IsArchiveFile (const WIN32_FIND_DATA & wfd) const
{
    return CFlag::IsNotSet (wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)       &&
           (wfd.dwFileAttributes & s_kdwUnavailableAttributes) == 0               &&
           FormatFor (wfd.cFileName) != EArchiveFormat::None;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::IsNestedArchiveFile
//
//  True for an archive file stored inside another archive that can be
//  browsed: archiveint.dll is available to extract it, and it is no larger
//  than s_kcbMaxNestedArchive.  Other archives inside archives list as files.
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveFileSystem::IsNestedArchiveFile (const WIN32_FIND_DATA & wfd) const
{
    ULARGE_INTEGER uliSize = {};



    uliSize.LowPart  = wfd.nFileSizeLow;
    uliSize.HighPart = wfd.nFileSizeHigh;

    return m_fLibArchiveAvailable && IsArchiveFile (wfd) && uliSize.QuadPart <= s_kcbMaxNestedArchive;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::RegisterArchive
//
//  Adds archivePath to the set of archives listed as directories, so that
//  paths under it resolve to archive locations.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveFileSystem::RegisterArchive (const filesystem::path & archivePath)
{
    wstring key = MakeKey (archivePath);



    lock_guard<mutex> lock (m_mutex);

    m_setArchives.insert (move (key));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::IsRegisteredArchive
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveFileSystem::IsRegisteredArchive (const filesystem::path & archivePath) const
{
    wstring key = MakeKey (archivePath);



    lock_guard<mutex> lock (m_mutex);

    return m_setArchives.contains (key);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::Resolve
//
//  Returns true when dirPath is a registered archive or a folder path under
//  one.  Only registered archives are considered, so the call doesn't do any
//  I/O.
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveFileSystem::Resolve (const filesystem::path & dirPath, SArchiveLocation & location) const
{
    filesystem::path           normalized = dirPath.lexically_normal();
    filesystem::path           current    = normalized;
    vector<filesystem::path>   ancestors;
    vector<wstring>            keys;
    wstring                    strFull;
    bool                       fEmpty     = false;
    bool                       fFound     = false;



    {
        lock_guard<mutex> lock (m_mutex);

        fEmpty = m_setArchives.empty();
    }

    //
    // Most directories have no archive above them.  The keys are computed
    // before the lock is taken so that worker threads don't serialize on it.
    //

    while (!fEmpty && current.has_relative_path())
    {
        ancestors.push_back (current);
        keys.push_back (MakeKey (current));
        current = current.parent_path();
    }

    {
        lock_guard<mutex> lock (m_mutex);

        for (size_t i = 0; !fFound && i < keys.size(); ++i)
        {
            if (m_setArchives.contains (keys[i]))
            {
                fFound  = true;
                current = ancestors[i];
            }
        }
    }

    if (fFound)
    {
        strFull = normalized.wstring();

        location.m_archivePath  = current;
        location.m_strInnerPath = strFull.substr (min (current.wstring().size(), strFull.size()));
        location.m_strInnerPath.erase (0, location.m_strInnerPath.find_first_not_of (L"\\/"));

        while (!location.m_strInnerPath.empty() &&
               (location.m_strInnerPath.back() == L'\\' || location.m_strInnerPath.back() == L'/'))
        {
            location.m_strInnerPath.pop_back();
        }
    }

    return fFound;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::TrySplit
//
//  Splits a command-line path at the first component that is a real file
//  with an archive extension, and registers that archive.  A real directory
//  always takes precedence, even when its name has an archive extension.
//  Archives stored inside the archive are then registered the same way, so
//  the location is in the innermost archive on the path.
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveFileSystem::TrySplit (const filesystem::path & path, SArchiveLocation & location)
{
    filesystem::path   normalized = path.lexically_normal();
    filesystem::path   prefix;
    DWORD              dwAttrs    = INVALID_FILE_ATTRIBUTES;
    bool               fSplit     = Resolve (normalized, location);



    for (auto it = normalized.begin(); !fSplit && it != normalized.end(); ++it)
    {
        prefix /= *it;

        if (!prefix.has_relative_path() || it->empty())
        {
            continue;
        }

        dwAttrs = m_accessPtr->GetAttributes (prefix);

        if (dwAttrs == INVALID_FILE_ATTRIBUTES)
        {
            break;
        }

        if (CFlag::IsSet (dwAttrs, FILE_ATTRIBUTE_DIRECTORY))
        {
            continue;
        }

        if (IsTargetArchive (prefix.filename().wstring()))
        {
            RegisterArchive (prefix);
            fSplit = Resolve (normalized, location);
        }

        break;
    }

    while (fSplit && !location.m_strInnerPath.empty() && RegisterNested (location))
    {
        fSplit = Resolve (normalized, location);
    }

    return fSplit;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::RegisterNested
//
//  Walks the inner path of location and registers the first component that
//  is an archive file stored in the archive.  Returns false when there is
//  none, or when the archive fails to parse.
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveFileSystem::RegisterNested (const SArchiveLocation & location)
{
    HRESULT                          hr          = S_OK;
    shared_ptr<const CArchiveTree>   treePtr;
    wstring                          strError;
    vector<wstring>                  components;
    filesystem::path                 innerPrefix;
    const CArchiveTree::SNode      * pFile       = nullptr;
    WIN32_FIND_DATA                  wfd         = {};
    bool                             fRegistered = false;



    hr = GetTree (location.m_archivePath, treePtr, strError);
    CHR (hr);

    CArchiveTree::SplitPath (location.m_strInnerPath, components);

    for (const wstring & component : components)
    {
        innerPrefix /= component;

        if (treePtr->FindFolder (innerPrefix.native()) != nullptr)
        {
            continue;
        }

        pFile = treePtr->FindFile (innerPrefix.native());

        if (pFile != nullptr)
        {
            CArchiveTree::ToFindData (*pFile, wfd);

            if (IsNestedArchiveFile (wfd))
            {
                RegisterArchive (location.m_archivePath / innerPrefix);
                fRegistered = true;
            }
        }

        break;
    }



Error:
    return fRegistered;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::IsDirectory
//
//  True when path is an archive root or a folder inside an archive.  An
//  archive root is a directory even when it fails to parse, so the listing
//  reports the parse error instead of listing the archive as a file.
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveFileSystem::IsDirectory (const filesystem::path & path)
{
    HRESULT                          hr          = S_OK;
    SArchiveLocation                 location;
    shared_ptr<const CArchiveTree>   treePtr;
    wstring                          strError;
    bool                             fSplit      = false;
    bool                             fDirectory  = false;



    fSplit = TrySplit (path, location);
    BAIL_OUT_IF (!fSplit, S_OK);

    if (location.m_strInnerPath.empty())
    {
        fDirectory = true;
    }
    else
    {
        hr = GetTree (location.m_archivePath, treePtr, strError);
        CHR (hr);

        fDirectory = treePtr->FindFolder (location.m_strInnerPath) != nullptr;
    }



Error:
    return fDirectory;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::GetTree
//
//  Returns the parsed tree for an archive, parsing it on first use.  A
//  failure is cached too, so each broken archive is read and reported once.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveFileSystem::GetTree (
    const filesystem::path         & archivePath,
    shared_ptr<const CArchiveTree> & treePtr,
    wstring                        & strError)
{
    HRESULT                   hr       = S_OK;
    shared_ptr<SCacheEntry>   entryPtr = GetCacheEntry (MakeKey (archivePath));



    call_once (entryPtr->m_onceFlag, [&] () { Parse (archivePath, *entryPtr); });

    treePtr  = entryPtr->m_treePtr;
    strError = entryPtr->m_strError;
    hr       = entryPtr->m_hr;

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::ReadDetails
//
//  Reads the entry list of one archive (a real file or an archive stored
//  inside other archives) without caching it.  The caller sets
//  result.m_detailsPtr to collect format details.  An encrypted listing is
//  not an error here: the caller reports it.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveFileSystem::ReadDetails (
    const filesystem::path & archivePath,
    SArchiveReadResult     & result,
    wstring                & strError)
{
    HRESULT            hr       = S_OK;
    wstring            fileName = archivePath.filename().wstring();
    EArchiveFormat     format   = FormatFor (fileName);
    SArchiveSource     source;



    strError.clear();

    hr = PrepareSource (archivePath, format, source);
    CHR (hr);

    hr = m_accessPtr->Read (source, format, result);
    CHR (hr);



Error:
    if (FAILED (hr))
    {
        strError = FormatError (hr, CArchiveFormat::FromFileName (fileName, true), m_fLibArchiveAvailable);
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::Verify
//
//  Reads every entry of one archive and checks the checksums it stores.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveFileSystem::Verify (
    const filesystem::path & archivePath,
    SVerifyResult          & result,
    wstring                & strError)
{
    HRESULT            hr       = S_OK;
    wstring            fileName = archivePath.filename().wstring();
    EArchiveFormat     format   = FormatFor (fileName);
    SArchiveSource     source;



    strError.clear();

    CBRFEx (m_fLibArchiveAvailable, HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED),
            strError = L": verification requires Windows 11 23H2 or later");

    hr = PrepareSource (archivePath, format, source);
    CHR (hr);

    hr = m_accessPtr->Verify (source, format, result);
    CHR (hr);



Error:
    if (FAILED (hr) && strError.empty())
    {
        strError = FormatError (hr, CArchiveFormat::FromFileName (fileName, true), m_fLibArchiveAvailable);
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::PrepareSource
//
//  Registers the archives on the path to archivePath, so that a nested
//  archive resolves through its outer archives, then opens its bytes.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveFileSystem::PrepareSource (
    const filesystem::path & archivePath,
    EArchiveFormat           format,
    SArchiveSource         & source)
{
    HRESULT            hr = S_OK;
    SArchiveLocation   location;



    CBREx (format != EArchiveFormat::None, HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED));

    TrySplit (archivePath, location);

    hr = OpenSource (archivePath, source);
    CHR (hr);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::GetCacheEntry
//
////////////////////////////////////////////////////////////////////////////////

shared_ptr<CArchiveFileSystem::SCacheEntry> CArchiveFileSystem::GetCacheEntry (const wstring & key)
{
    lock_guard<mutex>   lock (m_mutex);
    auto              & entryPtr = m_mapCache[key];



    if (!entryPtr)
    {
        entryPtr = make_shared<SCacheEntry>();
    }

    return entryPtr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::Parse
//
//  Reads one archive into entry.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveFileSystem::Parse (const filesystem::path & archivePath, SCacheEntry & entry)
{
    HRESULT                    hr       = S_OK;
    EArchiveFormat             format   = FormatFor (archivePath.filename().wstring());
    SArchiveSource             source;
    SArchiveReadResult         result;
    unique_ptr<CArchiveTree>   treePtr;



    hr = OpenSource (archivePath, source);
    CHR (hr);

    CBREx (format != EArchiveFormat::None, HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED));

    hr = m_accessPtr->Read (source, format, result);
    CHR (hr);

    CBREx (!result.m_fListingEncrypted, HRESULT_FROM_WIN32 (ERROR_FILE_ENCRYPTED));

    hr = CArchiveTree::Build (result.m_vEntries, treePtr);
    CHR (hr);

    entry.m_treePtr  = move (treePtr);
    entry.m_bytesPtr = source.m_bytesPtr;



Error:
    entry.m_hr = hr;

    if (FAILED (hr))
    {
        entry.m_strError = FormatError (hr,
                                        CArchiveFormat::FromFileName (archivePath.filename().wstring(), true),
                                        m_fLibArchiveAvailable);
    }

    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::OpenSource
//
//  Returns the bytes of archivePath.  A real archive file is checked first
//  so that a cloud placeholder is never opened (opening it would start a
//  download).  An archive stored inside another archive is extracted from
//  that archive into memory.  archiveint.dll does the extraction and cannot
//  read UDF, so a failure to extract from a disc image is reported as
//  ERROR_UNSUPPORTED_TYPE rather than as a fault in the inner archive.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveFileSystem::OpenSource (const filesystem::path & archivePath, SArchiveSource & source)
{
    HRESULT            hr          = S_OK;
    SArchiveLocation   outer;
    SArchiveSource     outerSource;
    filesystem::path   entryPath;
    DWORD              dwAttrs     = INVALID_FILE_ATTRIBUTES;
    bool               fOuterIso   = false;



    source = {};

    if (Resolve (archivePath.parent_path(), outer))
    {
        hr = OpenOuterSource (outer.m_archivePath, outerSource);
        CHR (hr);

        entryPath = filesystem::path (outer.m_strInnerPath) / archivePath.filename();
        fOuterIso = FormatFor (outer.m_archivePath.filename().wstring()) == EArchiveFormat::Iso;

        hr = m_accessPtr->ExtractEntry (outerSource, entryPath.native(), s_kcbMaxNestedArchive, source.m_bytesPtr);

        if (fOuterIso && (hr == HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT) || hr == HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND)))
        {
            hr = HRESULT_FROM_WIN32 (ERROR_UNSUPPORTED_TYPE);
        }

        CHR (hr);
    }
    else
    {
        dwAttrs = m_accessPtr->GetAttributes (archivePath);

        CBREx (dwAttrs != INVALID_FILE_ATTRIBUTES, HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND));
        CBREx ((dwAttrs & s_kdwUnavailableAttributes) == 0, HRESULT_FROM_WIN32 (ERROR_FILE_OFFLINE));

        source.m_path = archivePath;
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::OpenOuterSource
//
//  Returns the source of an archive that contains another archive.  The
//  outer archive has already been parsed (its tree listed the inner one), so
//  its cache entry supplies the extracted bytes when it is itself nested;
//  otherwise it is a real file whose attributes were checked at parse time.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveFileSystem::OpenOuterSource (const filesystem::path & outerPath, SArchiveSource & source)
{
    HRESULT                          hr       = S_OK;
    shared_ptr<const CArchiveTree>   treePtr;
    wstring                          strError;
    shared_ptr<SCacheEntry>          entryPtr;



    source = {};

    hr = GetTree (outerPath, treePtr, strError);
    CHR (hr);

    entryPtr = GetCacheEntry (MakeKey (outerPath));

    if (entryPtr->m_bytesPtr != nullptr)
    {
        source.m_bytesPtr = entryPtr->m_bytesPtr;
    }
    else
    {
        source.m_path = outerPath;
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::FormatError
//
//  Returns the text that follows the archive path in an error line.
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveFileSystem::FormatError (HRESULT hr, EArchiveFormat format, bool fLibArchiveAvailable)
{
    LPWSTR    pszMessage = nullptr;
    DWORD     cchMessage = 0;
    wstring   strError;



    if (hr == HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT))
    {
        strError = L" is not a valid archive";
    }
    else if (hr == HRESULT_FROM_WIN32 (ERROR_FILE_ENCRYPTED))
    {
        strError = L" is encrypted; its contents cannot be listed without the password";
    }
    else if (hr == HRESULT_FROM_WIN32 (ERROR_FILE_OFFLINE))
    {
        strError = L" is not available offline";
    }
    else if (hr == HRESULT_FROM_WIN32 (ERROR_FILE_TOO_LARGE))
    {
        strError = L" is too large to browse inside another archive";
    }
    else if (hr == HRESULT_FROM_WIN32 (ERROR_UNSUPPORTED_TYPE))
    {
        strError = L" cannot be extracted from this disc image";
    }
    else if (hr == HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED) && format == EArchiveFormat::LibArchive && !fLibArchiveAvailable)
    {
        strError = L": browsing this archive format requires Windows 11 23H2 or later";
    }
    else if (hr == HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED))
    {
        strError = L" uses an unsupported format variant";
    }
    else
    {
        cchMessage = FormatMessageW (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                     nullptr,
                                     static_cast<DWORD> (hr),
                                     0,
                                     reinterpret_cast<LPWSTR> (&pszMessage),
                                     0,
                                     nullptr);

        strError = L": ";
        strError += (cchMessage > 0) ? wstring (pszMessage, cchMessage) : std::format (L"HRESULT 0x{:08X}", static_cast<unsigned> (hr));

        while (!strError.empty() && iswspace (strError.back()))
        {
            strError.pop_back();
        }

        LocalFree (pszMessage);
    }

    return strError;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::FormatFor
//
////////////////////////////////////////////////////////////////////////////////

EArchiveFormat CArchiveFileSystem::FormatFor (wstring_view fileName) const
{
    return CArchiveFormat::FromFileName (fileName, m_fLibArchiveAvailable);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::IsTargetArchive
//
//  A file given on the command line counts as an archive even when its
//  reader is unavailable, so that the user gets an explanation instead of
//  a plain file listing.
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveFileSystem::IsTargetArchive (wstring_view fileName) const
{
    return CArchiveFormat::FromFileName (fileName, true) != EArchiveFormat::None;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem::MakeKey
//
//  Case-insensitive lookup key with no trailing separator.
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveFileSystem::MakeKey (const filesystem::path & path)
{
    wstring key = CWildcardMatcher::ToUpper (path.lexically_normal().wstring());



    while (key.size() > 1 && (key.back() == L'\\' || key.back() == L'/'))
    {
        key.pop_back();
    }

    return key;
}





