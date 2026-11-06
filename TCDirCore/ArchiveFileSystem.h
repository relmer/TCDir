#pragma once

#include "ArchiveEntry.h"
#include "ArchiveFileAccess.h"
#include "ArchiveTree.h"





////////////////////////////////////////////////////////////////////////////////
//
//  SArchiveLocation
//
//  A path split into an archive and the folder inside it.  The archive is a
//  real file, or an archive stored inside another archive.
//
////////////////////////////////////////////////////////////////////////////////

struct SArchiveLocation
{
    filesystem::path   m_archivePath;
    wstring            m_strInnerPath;    // Empty for the archive root
};





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFileSystem
//
//  Presents archive files as read-only directories.  Each archive is parsed
//  at most once per run, and all threads share the result (tree or error).
//  When archiveint.dll is available, an archive stored inside another
//  archive is browsed too: its data is extracted into memory and parsed.
//
////////////////////////////////////////////////////////////////////////////////

class CArchiveFileSystem
{
public:
    explicit CArchiveFileSystem (shared_ptr<IArchiveFileAccess> accessPtr);

    bool    IsArchiveFile       (const WIN32_FIND_DATA & wfd) const;
    bool    IsNestedArchiveFile (const WIN32_FIND_DATA & wfd) const;
    void    RegisterArchive     (const filesystem::path & archivePath);
    bool    IsRegisteredArchive (const filesystem::path & archivePath) const;
    bool    Resolve             (const filesystem::path & dirPath, SArchiveLocation & location) const;
    bool    TrySplit            (const filesystem::path & path, SArchiveLocation & location);
    bool    IsDirectory         (const filesystem::path & path);
    HRESULT GetTree             (const filesystem::path        & archivePath,
                                 shared_ptr<const CArchiveTree> & treePtr,
                                 wstring                        & strError);

    static wstring FormatError  (HRESULT hr, EArchiveFormat format, bool fLibArchiveAvailable);

private:
    struct SCacheEntry
    {
        once_flag                        m_onceFlag;
        HRESULT                          m_hr = S_OK;
        wstring                          m_strError;
        shared_ptr<const CArchiveTree>   m_treePtr;
    };

    EArchiveFormat         FormatFor       (wstring_view fileName) const;
    bool                   IsTargetArchive (wstring_view fileName) const;
    shared_ptr<SCacheEntry> GetCacheEntry  (const wstring & key);
    void                   Parse           (const filesystem::path & archivePath, SCacheEntry & entry);
    HRESULT                OpenSource      (const filesystem::path & archivePath, SArchiveSource & source);
    bool                   RegisterNested  (const SArchiveLocation & location);

    static wstring         MakeKey         (const filesystem::path & path);

    static constexpr DWORD     s_kdwUnavailableAttributes = FILE_ATTRIBUTE_OFFLINE | FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS;
    static constexpr ULONGLONG s_kcbMaxNestedArchive      = 256ull * 1024 * 1024;   // Largest archive extracted from another archive

    shared_ptr<IArchiveFileAccess>                     m_accessPtr;
    bool                                               m_fLibArchiveAvailable = false;
    mutable mutex                                      m_mutex;
    unordered_set<wstring>                             m_setArchives;
    unordered_map<wstring, shared_ptr<SCacheEntry>>    m_mapCache;
};
