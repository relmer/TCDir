#pragma once
////////////////////////////////////////////////////////////////////////////////
//
//  CMockArchiveFileAccess
//
//  In-memory IArchiveFileAccess for tests.  Paths map to attributes and to
//  prepared entry lists (or read failures), so no real file is opened.
//
//  An archive stored inside another archive is added with AddArchive under
//  its full path (C:\\Root\\a.zip\\inner.zip).  ExtractEntry returns bytes
//  that encode that path, and Read decodes them to find the entry list.
//
//  Usage:
//      auto accessPtr = make_shared<CMockArchiveFileAccess>();
//      accessPtr->AddDirectory (L"C:\\Root");
//      accessPtr->AddArchive   (L"C:\\Root\\a.zip", { CMockArchiveFileAccess::File (L"x.txt", 10) });
//
////////////////////////////////////////////////////////////////////////////////

#include "../../TCDirCore/ArchiveFileAccess.h"
#include "../../TCDirCore/WildcardMatcher.h"





class CMockArchiveFileAccess : public IArchiveFileAccess
{
public:
    struct SArchive
    {
        HRESULT              m_hr = S_OK;
        SArchiveReadResult   m_result;
    };



    void AddDirectory (const filesystem::path & path)
    {
        m_mapAttributes[Key (path)] = FILE_ATTRIBUTE_DIRECTORY;
    }

    void AddFile (const filesystem::path & path, DWORD dwAttributes = FILE_ATTRIBUTE_ARCHIVE)
    {
        m_mapAttributes[Key (path)] = dwAttributes;
    }

    void AddArchive (const filesystem::path & path, vector<SArchiveEntry> entries, DWORD dwAttributes = FILE_ATTRIBUTE_ARCHIVE)
    {
        SArchive archive;

        archive.m_result.m_vEntries = move (entries);

        AddFile (path, dwAttributes);
        m_mapArchives[Key (path)] = move (archive);
    }

    void AddBrokenArchive (const filesystem::path & path, HRESULT hr)
    {
        SArchive archive;

        archive.m_hr = hr;

        AddFile (path);
        m_mapArchives[Key (path)] = move (archive);
    }

    void AddEncryptedListingArchive (const filesystem::path & path)
    {
        SArchive archive;

        archive.m_result.m_fListingEncrypted = true;

        AddFile (path);
        m_mapArchives[Key (path)] = move (archive);
    }

    static SArchiveEntry File (const wstring & strPath, ULONGLONG ullSize, DWORD dwAttributes = 0)
    {
        SArchiveEntry entry;

        entry.m_strPath      = strPath;
        entry.m_ullSize      = ullSize;
        entry.m_dwAttributes = dwAttributes;
        return entry;
    }

    static SArchiveEntry Folder (const wstring & strPath)
    {
        SArchiveEntry entry;

        entry.m_strPath    = strPath;
        entry.m_fDirectory = true;
        return entry;
    }



    DWORD GetAttributes (const filesystem::path & path) override
    {
        auto it = m_mapAttributes.find (Key (path));

        return (it != m_mapAttributes.end()) ? it->second : INVALID_FILE_ATTRIBUTES;
    }

    HRESULT Read (const SArchiveSource & source, EArchiveFormat format, SArchiveReadResult & result) override
    {
        auto    it = m_mapArchives.find (SourceKey (source));
        HRESULT hr = HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND);

        ++m_cReads;
        m_lastFormat = format;

        if (it != m_mapArchives.end())
        {
            hr     = it->second.m_hr;
            result = it->second.m_result;
        }

        return hr;
    }

    HRESULT ExtractEntry (
        const SArchiveSource             & source,
        wstring_view                       entryPath,
        ULONGLONG                          cbMax,
        shared_ptr<const vector<BYTE>>   & bytesPtr) override
    {
        wstring   key   = Key (filesystem::path (SourceKey (source)) / entryPath);
        auto      it    = m_mapExtractFailures.find (key);
        HRESULT   hr    = S_OK;

        ++m_cExtracts;
        m_cbLastExtractMax = cbMax;

        if (it != m_mapExtractFailures.end())
        {
            hr = it->second;
        }
        else
        {
            const BYTE * pb = reinterpret_cast<const BYTE *> (key.data());

            bytesPtr = make_shared<const vector<BYTE>> (pb, pb + key.size() * sizeof (wchar_t));
        }

        return hr;
    }

    void AddExtractFailure (const filesystem::path & path, HRESULT hr)
    {
        m_mapExtractFailures[Key (path)] = hr;
    }

    bool IsLibArchiveAvailable() override
    {
        return m_fLibArchiveAvailable;
    }



    atomic<int>       m_cReads               = 0;
    atomic<int>       m_cExtracts            = 0;
    ULONGLONG         m_cbLastExtractMax     = 0;
    EArchiveFormat    m_lastFormat           = EArchiveFormat::None;
    bool              m_fLibArchiveAvailable = true;

private:
    static wstring Key (const filesystem::path & path)
    {
        wstring key = CWildcardMatcher::ToUpper (path.lexically_normal().wstring());

        while (key.size() > 1 && key.back() == L'\\')
        {
            key.pop_back();
        }

        return key;
    }

    static wstring SourceKey (const SArchiveSource & source)
    {
        wstring key;

        if (source.m_bytesPtr != nullptr)
        {
            key.assign (reinterpret_cast<const wchar_t *> (source.m_bytesPtr->data()), source.m_bytesPtr->size() / sizeof (wchar_t));
        }
        else
        {
            key = Key (source.m_path);
        }

        return key;
    }

    unordered_map<wstring, DWORD>      m_mapAttributes;
    unordered_map<wstring, SArchive>   m_mapArchives;
    unordered_map<wstring, HRESULT>    m_mapExtractFailures;
};
