# Data Model: Archive Browsing

## EArchiveFormat

`enum class EArchiveFormat { None, Zip, Iso, LibArchive }`

Selected by `CArchiveFormat::FromFileName (name, fLibArchiveAvailable)`. Matching is case-insensitive on the longest recognized suffix (`.tar.gz` before `.gz`). The suffix list is defined once as a `constexpr` table in `ArchiveFormat.cpp` (research R4).

## SArchiveEntry

One record produced by a reader, before folder structure is built.

| Field | Type | Rules |
|-------|------|-------|
| `m_strPath` | `wstring` | Path inside the archive, as stored. Separators `/` or `\`. |
| `m_fDirectory` | `bool` | True for explicit folder entries (trailing separator, directory attribute or Unix `S_IFDIR`). |
| `m_ullSize` | `ULONGLONG` | Uncompressed size; 0 for folders. |
| `m_ftLastWrite` | `FILETIME` | UTC. Zero when the archive has no time for the entry. |
| `m_dwAttributes` | `DWORD` | `FILE_ATTRIBUTE_READONLY`, `HIDDEN`, `SYSTEM`, `ARCHIVE` when stored; `FILE_ATTRIBUTE_DIRECTORY` for folders. |
| `m_fEncrypted` | `bool` | Entry data is encrypted. |

## SArchiveReadResult

| Field | Type | Rules |
|-------|------|-------|
| `m_vEntries` | `vector<SArchiveEntry>` | All entries in archive order. |
| `m_fListingEncrypted` | `bool` | The format encrypts the entry list itself (for example 7z with encrypted headers). `m_vEntries` is empty in that case. |

Readers return `HRESULT`: `S_OK`, `HRESULT_FROM_WIN32 (ERROR_BAD_FORMAT)` for corrupt or unrecognized data, `HRESULT_FROM_WIN32 (ERROR_NOT_SUPPORTED)` for a format variant out of scope (UDF metadata partition with no ISO 9660 fallback, libarchive DLL absent), or the I/O error from the byte source.

## IByteSource

```text
ULONGLONG Size() const
HRESULT   ReadAt (ULONGLONG offset, void * pBuffer, DWORD cb)   // full read or ERROR_HANDLE_EOF
```

Implementations: `CFileByteSource` (`CreateFileW`, `GENERIC_READ`, `FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE`, `FILE_FLAG_RANDOM_ACCESS`), `CMemoryByteSource` (tests).

## CArchiveTree (immutable after Build)

| Member | Purpose |
|--------|---------|
| `struct SNode { wstring m_strName; bool m_fDirectory; ULONGLONG m_ullSize; FILETIME m_ftLastWrite; DWORD m_dwAttributes; vector<size_t> m_vChildren; }` | One file or folder. Index 0 is the root. |
| `static HRESULT Build (const vector<SArchiveEntry> &, unique_ptr<CArchiveTree> &)` | Normalizes separators, drops empty, `.` and `..` components and drive or root prefixes (sanitization), creates implied folders, keeps duplicates. An explicit folder entry that follows an implied one updates its time and attributes. |
| `const SNode * FindFolder (const wstring & innerPath) const` | Case-insensitive component walk. Returns `nullptr` when missing or when a component is a file. |
| `void EnumerateChildren (const SNode &, const wstring & spec, function<void (const WIN32_FIND_DATAW &)>) const` | Applies `CWildcardMatcher`. Synthesizes `WIN32_FIND_DATAW`: name (truncated to `MAX_PATH - 1` with no crash), attributes (`FILE_ATTRIBUTE_DIRECTORY` for folders, `FILE_ATTRIBUTE_NORMAL` when none), size split into high/low, all three times = last-write time, `dwReserved0 = 0`. |

## CArchiveFileSystem

| Member | Purpose |
|--------|---------|
| `CArchiveFileSystem (shared_ptr<IArchiveFileAccess>)` | |
| `bool IsArchiveFile (const WIN32_FIND_DATAW & wfd) const` | Not a directory, not offline/recall-on-access, extension recognized. |
| `void RegisterArchive (const filesystem::path &)` | Adds to the known set (called by `CFileEnumerator` for marked entries). |
| `bool TrySplit (const filesystem::path &, SArchiveLocation &)` | Uses the known set first, then probes components left to right with `IArchiveFileAccess::GetAttributes` (command-line targets). Real directories always win. |
| `HRESULT GetTree (const filesystem::path & archivePath, shared_ptr<const CArchiveTree> &, wstring & strError)` | Parses once; caches the tree or the failure. |
| `bool IsDirectory (path)` / `bool Exists (path)` | Real file system first, then archive lookup. |

`SArchiveLocation { filesystem::path m_archivePath; wstring m_strInnerPath; }`

## IArchiveFileAccess

```text
DWORD   GetAttributes (const filesystem::path &)     // INVALID_FILE_ATTRIBUTES when missing
HRESULT Read (const filesystem::path &, EArchiveFormat, SArchiveReadResult &)
bool    IsLibArchiveAvailable()
```

`CArchiveFileAccess` opens a `CFileByteSource` and dispatches to `CZipReader`, `CIsoReader` (which calls `CUdfReader`) or `CLibArchiveReader`. Tests use `MockArchiveFileAccess` with in-memory images.

## FileInfo / CDirectoryInfo additions

- `FileInfo::m_fInArchive` (`bool`, default `false`): entry came from an archive tree.
- `CDirectoryInfo::m_fInArchive` (`bool`, default `false`): `m_dirPath` is a virtual location inside an archive.
- `CDirectoryInfo::m_strError` (`wstring`): archive parse error text for the main thread to print.

## State transitions

Archive cache entry: `Unknown → Parsing → Ready(tree) | Failed(hr, message)`. `Parsing` is a `shared_future`, so concurrent requests wait for the same parse. There is no transition out of `Ready` or `Failed` for the rest of the run.
