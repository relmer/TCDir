#include "pch.h"

#include "FileEnumerator.h"

#include "ArchiveFileSystem.h"
#include "AutoHandle.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CFileEnumerator::Enumerate
//
//  Calls callback for each entry in dirPath that matches fileSpec, until
//  callback returns false.  Returns the FindFirstFile error when nothing
//  matches.  For an archive that fails to parse, error contains the archive
//  path and the text to print after it.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CFileEnumerator::Enumerate (
    const filesystem::path & dirPath,
    const filesystem::path & fileSpec,
    const Callback         & callback,
    SEnumerationError      & error) const
{
    HRESULT hr = S_OK;



    error = {};

    if (IsArchiveLocation (dirPath))
    {
        hr = EnumerateArchive (dirPath, fileSpec, callback, error);
    }
    else
    {
        hr = EnumerateReal (dirPath, fileSpec, callback);
    }

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CFileEnumerator::IsArchiveLocation
//
////////////////////////////////////////////////////////////////////////////////

bool CFileEnumerator::IsArchiveLocation (const filesystem::path & dirPath) const
{
    SArchiveLocation location;



    return m_archiveFsPtr != nullptr && m_archiveFsPtr->Resolve (dirPath, location);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CFileEnumerator::EnumerateReal
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CFileEnumerator::EnumerateReal (
    const filesystem::path & dirPath,
    const filesystem::path & fileSpec,
    const Callback         & callback) const
{
    HRESULT            hr              = S_OK;
    filesystem::path   pathAndFileSpec = dirPath / fileSpec;
    AutoFindHandle     hFind;
    WIN32_FIND_DATA    wfd             = {};
    bool               fContinue       = true;
    DWORD              dwError         = 0;



    hFind = FindFirstFile (pathAndFileSpec.c_str(), &wfd);
    CWR (hFind != INVALID_HANDLE_VALUE);

    do
    {
        RegisterArchive (dirPath, false, wfd);

        fContinue = callback (wfd);
    }
    while (fContinue && FindNextFile (hFind, &wfd));

    if (fContinue)
    {
        dwError = GetLastError();

        if (dwError != ERROR_NO_MORE_FILES && dwError != ERROR_FILE_NOT_FOUND)
        {
            CHR (HRESULT_FROM_WIN32 (dwError));
        }
    }



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CFileEnumerator::EnumerateArchive
//
//  A folder path that does not exist inside the archive returns
//  ERROR_PATH_NOT_FOUND, the same as for a missing real directory.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CFileEnumerator::EnumerateArchive (
    const filesystem::path & dirPath,
    const filesystem::path & fileSpec,
    const Callback         & callback,
    SEnumerationError      & error) const
{
    HRESULT                          hr       = S_OK;
    SArchiveLocation                 location;
    shared_ptr<const CArchiveTree>   treePtr;
    const CArchiveTree::SNode      * pFolder  = nullptr;
    bool                             fFound   = m_archiveFsPtr->Resolve (dirPath, location);



    CBRAEx (fFound, HRESULT_FROM_WIN32 (ERROR_PATH_NOT_FOUND));

    hr = m_archiveFsPtr->GetTree (location.m_archivePath, treePtr, error.m_strMessage);
    CHRF (hr, error.m_archivePath = location.m_archivePath);

    pFolder = treePtr->FindFolder (location.m_strInnerPath);
    CBREx (pFolder != nullptr, HRESULT_FROM_WIN32 (ERROR_PATH_NOT_FOUND));

    treePtr->EnumerateChildren (*pFolder, fileSpec.native(), [&] (const WIN32_FIND_DATA & wfd)
    {
        RegisterArchive (dirPath, true, wfd);

        return callback (wfd);
    });



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CFileEnumerator::IsArchiveFile
//
//  Returns true when wfd is an archive file that recursion descends into.
//  fInArchive indicates that wfd is an entry of an archive; such an entry
//  qualifies only under CArchiveFileSystem::IsNestedArchiveFile.
//
////////////////////////////////////////////////////////////////////////////////

bool CFileEnumerator::IsArchiveFile (bool fInArchive, const WIN32_FIND_DATA & wfd) const
{
    bool fArchive = false;



    if (m_archiveFsPtr != nullptr)
    {
        fArchive = fInArchive ? m_archiveFsPtr->IsNestedArchiveFile (wfd) : m_archiveFsPtr->IsArchiveFile (wfd);
    }

    return fArchive;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CFileEnumerator::RegisterArchive
//
//  Registers an archive file so that its path resolves to an archive
//  location when recursion descends into it.
//
////////////////////////////////////////////////////////////////////////////////

void CFileEnumerator::RegisterArchive (const filesystem::path & dirPath, bool fInArchive, const WIN32_FIND_DATA & wfd) const
{
    if (IsArchiveFile (fInArchive, wfd))
    {
        m_archiveFsPtr->RegisterArchive (dirPath / wfd.cFileName);
    }
}





