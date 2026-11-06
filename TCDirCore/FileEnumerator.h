#pragma once





class CArchiveFileSystem;





////////////////////////////////////////////////////////////////////////////////
//
//  SEnumerationError
//
//  An archive that could not be read, and the text to print after its path.
//
////////////////////////////////////////////////////////////////////////////////

struct SEnumerationError
{
    filesystem::path   m_archivePath;
    wstring            m_strMessage;
};





////////////////////////////////////////////////////////////////////////////////
//
//  CFileEnumerator
//
//  Enumerates one directory, either through FindFirstFile or, for a
//  location inside an archive, from the archive tree.  When archive
//  browsing is on, archive files found on disk are registered so that
//  recursion can descend into them.  Their attributes are not modified,
//  so they list as files.
//
////////////////////////////////////////////////////////////////////////////////

class CFileEnumerator
{
public:
    using Callback = function<bool (const WIN32_FIND_DATA &)>;

    void    SetArchiveFileSystem (shared_ptr<CArchiveFileSystem> archiveFsPtr) { m_archiveFsPtr = archiveFsPtr; }
    CArchiveFileSystem * ArchiveFileSystem() const                            { return m_archiveFsPtr.get(); }
    shared_ptr<CArchiveFileSystem> ArchiveFileSystemPtr() const               { return m_archiveFsPtr; }

    HRESULT Enumerate            (const filesystem::path & dirPath,
                                  const filesystem::path & fileSpec,
                                  const Callback         & callback,
                                  SEnumerationError      & error) const;
    bool    IsArchiveLocation    (const filesystem::path & dirPath) const;
    bool    IsArchiveFile        (bool fInArchive, const WIN32_FIND_DATA & wfd) const;

private:
    HRESULT EnumerateReal        (const filesystem::path & dirPath,
                                  const filesystem::path & fileSpec,
                                  const Callback         & callback) const;
    HRESULT EnumerateArchive     (const filesystem::path & dirPath,
                                  const filesystem::path & fileSpec,
                                  const Callback         & callback,
                                  SEnumerationError      & error) const;
    void    RegisterArchive      (const filesystem::path & dirPath, const WIN32_FIND_DATA & wfd) const;

    shared_ptr<CArchiveFileSystem> m_archiveFsPtr;
};
