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
    filesystem::path   normalized = dirPath.lexically_normal();
    filesystem::path   current    = normalized;
    filesystem::path   parent;
    wstring            strFull;
    bool               fFound     = false;



    lock_guard<mutex> lock (m_mutex);

    while (current.has_relative_path())
    {
        if (m_setArchives.contains (MakeKey (current)))
        {
            fFound  = true;
            strFull = normalized.wstring();

            location.m_archivePath  = current;
            location.m_strInnerPath = strFull.substr (min (current.wstring().size(), strFull.size()));
            location.m_strInnerPath.erase (0, location.m_strInnerPath.find_first_not_of (L"\\/"));

            while (!location.m_strInnerPath.empty() &&
                   (location.m_strInnerPath.back() == L'\\' || location.m_strInnerPath.back() == L'/'))
            {
                location.m_strInnerPath.pop_back();
            }

            break;
        }

        parent  = current.parent_path();
        current = parent;
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

    return fSplit;
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
//  Reads one archive into entry.  The attributes are checked first so that a
//  cloud placeholder is never opened (opening it would start a download).
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveFileSystem::Parse (const filesystem::path & archivePath, SCacheEntry & entry)
{
    HRESULT                    hr       = S_OK;
    DWORD                      dwAttrs  = m_accessPtr->GetAttributes (archivePath);
    EArchiveFormat             format   = FormatFor (archivePath.filename().wstring());
    SArchiveReadResult         result;
    unique_ptr<CArchiveTree>   treePtr;



    CBREx (dwAttrs != INVALID_FILE_ATTRIBUTES, HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND));
    CBREx ((dwAttrs & s_kdwUnavailableAttributes) == 0, HRESULT_FROM_WIN32 (ERROR_FILE_OFFLINE));
    CBREx (format != EArchiveFormat::None, HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED));

    hr = m_accessPtr->Read (archivePath, format, result);
    CHR (hr);

    CBREx (!result.m_fListingEncrypted, HRESULT_FROM_WIN32 (ERROR_FILE_ENCRYPTED));

    hr = CArchiveTree::Build (result.m_vEntries, treePtr);
    CHR (hr);

    entry.m_treePtr = move (treePtr);



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





