#include "pch.h"

#include "ArchiveInfoFinder.h"

#include "ArchiveFileSystem.h"
#include "DirectoryLister.h"
#include "Flag.h"
#include "WildcardMatcher.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoFinder::CArchiveInfoFinder
//
////////////////////////////////////////////////////////////////////////////////

CArchiveInfoFinder::CArchiveInfoFinder (
    shared_ptr<CArchiveFileSystem> archiveFsPtr,
    bool                           fRecurse,
    ReportCallback                 reportCallback,
    ErrorCallback                  errorCallback) :
    m_archiveFsPtr   (archiveFsPtr),
    m_fRecurse       (fRecurse),
    m_reportCallback (move (reportCallback)),
    m_errorCallback  (move (errorCallback))
{
    m_enumerator.SetArchiveFileSystem (m_archiveFsPtr);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoFinder::Find
//
//  A target is an archive, a folder (searched as folder\*), or a mask in a
//  folder.  The folder can be inside an archive.  Returns true when at least
//  one archive matched.
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveInfoFinder::Find (const wstring & strTarget)
{
    filesystem::path   targetPath = filesystem::absolute (strTarget.empty() ? wstring (s_kpszAll) : strTarget).lexically_normal();
    wstring            strName    = targetPath.filename().wstring();
    SArchiveLocation   location;



    m_cMatches = 0;

    //
    // Registers the archives on the path, so that a folder inside an archive
    // enumerates from the archive tree.
    //

    m_archiveFsPtr->TrySplit (targetPath, location);

    if (strName.empty())
    {
        SearchFolder (targetPath, s_kpszAll);
    }
    else if (HasWildcards (strName))
    {
        SearchFolder (targetPath.parent_path(), strName);
    }
    else
    {
        FindExactTarget (targetPath);
    }

    return m_cMatches > 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoFinder::FindExactTarget
//
//  A target without wildcards: an archive is reported (and, with -s,
//  searched for nested archives); a folder is searched.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveInfoFinder::FindExactTarget (const filesystem::path & targetPath)
{
    HRESULT                   hr      = S_OK;
    vector<WIN32_FIND_DATA>   entries;
    SEnumerationError         error;



    hr = ListFolder (targetPath.parent_path(), targetPath.filename().wstring(), entries, error);
    CHR (hr);
    BAIL_OUT_IF (entries.empty(), S_OK);

    if (IsFolder (entries.front()))
    {
        SearchFolder (targetPath, s_kpszAll);
    }
    else if (m_archiveFsPtr->IsArchiveFile (entries.front()))
    {
        Report (targetPath, entries.front());
        SearchArchive (targetPath, s_kpszAll, true);
    }



Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoFinder::SearchFolder
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveInfoFinder::SearchFolder (const filesystem::path & dirPath, const wstring & strSpec)
{
    SEnumerationError error;



    if (m_fRecurse)
    {
        SearchFolderTree (dirPath, strSpec, error);
    }
    else
    {
        SearchFolderMatches (dirPath, strSpec);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoFinder::SearchFolderMatches
//
//  Without -s: reports the archives in one folder that match strSpec.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveInfoFinder::SearchFolderMatches (const filesystem::path & dirPath, const wstring & strSpec)
{
    HRESULT                   hr      = S_OK;
    vector<WIN32_FIND_DATA>   entries;
    SEnumerationError         error;



    hr = ListFolder (dirPath, strSpec, entries, error);
    CHR (hr);

    for (const WIN32_FIND_DATA & wfd : entries)
    {
        if (!IsFolder (wfd) && m_archiveFsPtr->IsArchiveFile (wfd))
        {
            Report (dirPath / wfd.cFileName, wfd);
        }
    }



Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoFinder::SearchFolderTree
//
//  With -s: reports the archives in the folder that match strSpec, and
//  searches every archive (matching or not) for nested matches right after
//  it.  Subfolders follow the folder's archives.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveInfoFinder::SearchFolderTree (
    const filesystem::path & dirPath,
    const wstring          & strSpec,
    SEnumerationError      & error)
{
    HRESULT                    hr          = S_OK;
    vector<WIN32_FIND_DATA>    entries;
    vector<filesystem::path>   subfolders;
    SEnumerationError          subfolderError;
    filesystem::path           entryPath;
    bool                       fMatch      = false;
    wstring                    strPrepared = CWildcardMatcher::PrepareSpec (strSpec);



    hr = ListFolder (dirPath, s_kpszAll, entries, error);
    CHR (hr);

    for (const WIN32_FIND_DATA & wfd : entries)
    {
        entryPath = dirPath / wfd.cFileName;

        if (IsFolder (wfd))
        {
            subfolders.push_back (entryPath);
            continue;
        }

        if (!m_archiveFsPtr->IsArchiveFile (wfd))
        {
            continue;
        }

        fMatch = CWildcardMatcher::MatchesPrepared (wfd.cFileName, strPrepared);

        if (fMatch)
        {
            Report (entryPath, wfd);
        }

        SearchArchive (entryPath, strSpec, fMatch);
    }

    for (const filesystem::path & subfolder : subfolders)
    {
        SearchFolderTree (subfolder, strSpec, subfolderError);
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoFinder::SearchArchive
//
//  With -s, searches an archive as a folder.  An archive that can't be
//  browsed (too large, or not readable) is skipped.  When it wasn't
//  reported, its error goes to the error callback, because no block
//  contains it.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveInfoFinder::SearchArchive (const filesystem::path & archivePath, const wstring & strSpec, bool fReported)
{
    HRESULT             hr         = S_OK;
    SEnumerationError   error;
    bool                fBrowsable = m_fRecurse && m_enumerator.IsArchiveLocation (archivePath);



    BAIL_OUT_IF (!fBrowsable, S_OK);

    hr = SearchFolderTree (archivePath, strSpec, error);

    if (FAILED (hr) && !fReported && !error.m_strMessage.empty())
    {
        m_errorCallback (error.m_archivePath, error.m_strMessage);
    }



Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoFinder::ListFolder
//
//  Returns the entries of one folder that match strSpec, without "." and
//  "..", in name order.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveInfoFinder::ListFolder (
    const filesystem::path    & dirPath,
    const wstring             & strSpec,
    vector<WIN32_FIND_DATA>   & entries,
    SEnumerationError         & error)
{
    HRESULT hr = S_OK;



    entries.clear();

    hr = m_enumerator.Enumerate (dirPath, strSpec, [&entries] (const WIN32_FIND_DATA & wfd)
    {
        if (!CDirectoryLister::IsDots (wfd.cFileName))
        {
            entries.push_back (wfd);
        }

        return true;
    }, error);

    sort (entries.begin(), entries.end(), [] (const WIN32_FIND_DATA & a, const WIN32_FIND_DATA & b)
    {
        return _wcsicmp (a.cFileName, b.cFileName) < 0;
    });

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoFinder::Report
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveInfoFinder::Report (const filesystem::path & archivePath, const WIN32_FIND_DATA & wfd)
{
    ++m_cMatches;
    m_reportCallback (archivePath, SizeOf (wfd));
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoFinder::HasWildcards
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveInfoFinder::HasWildcards (const wstring & strName)
{
    return strName.find_first_of (L"*?") != wstring::npos;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoFinder::IsFolder
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveInfoFinder::IsFolder (const WIN32_FIND_DATA & wfd)
{
    return CFlag::IsSet (wfd.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoFinder::SizeOf
//
////////////////////////////////////////////////////////////////////////////////

ULONGLONG CArchiveInfoFinder::SizeOf (const WIN32_FIND_DATA & wfd)
{
    ULARGE_INTEGER uliSize = {};



    uliSize.LowPart  = wfd.nFileSizeLow;
    uliSize.HighPart = wfd.nFileSizeHigh;

    return uliSize.QuadPart;
}





