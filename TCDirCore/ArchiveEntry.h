#pragma once





////////////////////////////////////////////////////////////////////////////////
//
//  EArchiveFormat
//
//  Reader that handles an archive file.  Selected from the file extension.
//
////////////////////////////////////////////////////////////////////////////////

enum class EArchiveFormat
{
    None,
    Zip,
    Iso,
    LibArchive
};





////////////////////////////////////////////////////////////////////////////////
//
//  SArchiveEntry
//
//  One file or folder record read from an archive, before folder structure
//  is built.
//
////////////////////////////////////////////////////////////////////////////////

struct SArchiveEntry
{
    wstring     m_strPath;                    // Path inside the archive; '/' or '\' separators
    bool        m_fDirectory   = false;       // Explicit folder entry
    ULONGLONG   m_ullSize      = 0;           // Uncompressed size (0 for folders)
    FILETIME    m_ftLastWrite  = {};          // UTC; zero when the archive has no time
    DWORD       m_dwAttributes = 0;           // FILE_ATTRIBUTE_* values stored in the archive
    bool        m_fEncrypted   = false;       // Entry data is encrypted
};





////////////////////////////////////////////////////////////////////////////////
//
//  SArchiveReadResult
//
//  Output of an archive reader.
//
////////////////////////////////////////////////////////////////////////////////

struct SArchiveReadResult
{
    vector<SArchiveEntry>   m_vEntries;
    bool                    m_fListingEncrypted = false;   // The entry list itself is encrypted
};





////////////////////////////////////////////////////////////////////////////////
//
//  SArchiveSource
//
//  The bytes of one archive: a real file, or the contents of an archive
//  stored inside another archive, extracted into memory.
//
////////////////////////////////////////////////////////////////////////////////

struct SArchiveSource
{
    filesystem::path                 m_path;        // Real archive file; used when m_bytesPtr is null
    shared_ptr<const vector<BYTE>>   m_bytesPtr;    // Archive extracted from another archive
};
