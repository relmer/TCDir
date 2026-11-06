# Data Model: Archive Info Mode

All types live in `TCDirCore`. Counts and sizes are `ULONGLONG`.

## SArchiveDetails

Format details filled by a reader when the caller requests them
(`SArchiveReadResult::m_detailsPtr` is not null). Exactly one of the
format members applies.

| Field | Type | Notes |
|-------|------|-------|
| `m_format` | `EArchiveFormat` | Reader that filled the record |
| `m_zip` | `SZipDetails` | ZIP only |
| `m_disc` | `SDiscDetails` | ISO and UDF only |
| `m_libArchive` | `SLibArchiveDetails` | libarchive formats only |

## SZipDetails

| Field | Type | Notes |
|-------|------|-------|
| `m_mapMethods` | `map<WORD, SMethodGroup>` | Key: method number (the real method for AES entries) |
| `m_mapEncryption` | `map<EZipEncryption, ULONGLONG>` | Entry count per encryption type |
| `m_fZip64` | `bool` | ZIP64 locator found |
| `m_fUtf8Paths` | `bool` | Any entry has flag bit 11 |
| `m_fMultiDisk` | `bool` | End record disk numbers not zero |
| `m_bCreatorHost`, `m_bCreatorVersion` | `BYTE` | Most common "version made by" |
| `m_ullPrefixSize` | `ULONGLONG` | Bytes before the first local header |
| `m_strComment` | `wstring` | Empty when absent |

`EZipEncryption`: `None`, `ZipCrypto`, `Aes128`, `Aes192`, `Aes256`,
`PkwareStrong`.

## SMethodGroup

| Field | Type |
|-------|------|
| `m_cFiles` | `ULONGLONG` |
| `m_ullCompressed` | `ULONGLONG` |
| `m_ullUncompressed` | `ULONGLONG` |

## SDiscDetails

| Field | Type | Notes |
|-------|------|-------|
| `m_fIso9660`, `m_fJoliet`, `m_fRockRidge` | `bool` | |
| `m_wUdfRevision` | `WORD` | BCD, 0 when no UDF |
| `m_eListedTree` | `EDiscTree` | `Udf`, `Joliet`, `Primary` |
| `m_strVolumeLabel`, `m_strSystemId`, `m_strPublisher`, `m_strPreparer`, `m_strApplication` | `wstring` | Trailing spaces removed. Empty fields are omitted from output |
| `m_ftCreated`, `m_ftModified`, `m_ftExpires` | `optional<FILETIME>` | UTC |
| `m_ulSectorSize` | `ULONG` | |
| `m_ullSectorCount` | `ULONGLONG` | |
| `m_vBootPlatforms` | `vector<BYTE>` | El Torito platform IDs in catalog order. Empty means not bootable |

## SLibArchiveDetails

| Field | Type | Notes |
|-------|------|-------|
| `m_strFormatName` | `wstring` | `archive_format_name` |
| `m_vFilters` | `vector<wstring>` | Outermost first |
| `m_ullStreamCompressed`, `m_ullStreamUncompressed` | `optional<ULONGLONG>` | Set only when a filter is present |
| `m_fHeadersEncrypted` | `bool` | |
| `m_cSymlinks`, `m_cHardLinks` | `ULONGLONG` | |
| `m_mapOwners`, `m_mapGroups` | `map<wstring, ULONGLONG>` | Entry count per name |
| `m_mapPermissions` | `map<unsigned, ULONGLONG>` | Entry count per permission set |
| `m_cBirthTimes`, `m_cAccessTimes` | `ULONGLONG` | Entries with the time set |
| `m_cSparse`, `m_cXattrs`, `m_cAcls` | `ULONGLONG` | Entries with at least one |

## SArchiveStatistics

Computed by `CArchiveStatistics::Compute (const SArchiveReadResult &)`. Pure
function over the entry list, so tests use synthetic entries.

| Field | Type | Notes |
|-------|------|-------|
| `m_cFiles`, `m_cFolders`, `m_cNestedArchives` | `ULONGLONG` | Folders include implied folders. Nested archives are files whose suffix `CArchiveFormat` recognizes |
| `m_ullUncompressed` | `ULONGLONG` | |
| `m_strLargestPath`, `m_ullLargestSize` | `wstring`, `ULONGLONG` | On a tie, the first entry is used |
| `m_strDeepestPath`, `m_cDeepestDepth` | `wstring`, `UINT` | Depth counts path components |
| `m_ftOldest`, `m_ftNewest` | `optional<FILETIME>` | Entries without a time are left out |
| `m_cEncrypted` | `ULONGLONG` | |
| `m_fListingEncrypted` | `bool` | |
| `m_vExtensions` | `vector<SExtensionGroup>` | Sorted by size, largest first, then by extension |

## SExtensionGroup

| Field | Type | Notes |
|-------|------|-------|
| `m_strExtension` | `wstring` | Lowercase, with the dot. "(none)" for no extension |
| `m_cFiles` | `ULONGLONG` | |
| `m_ullSize` | `ULONGLONG` | |

## SVerifyResult

| Field | Type | Notes |
|-------|------|-------|
| `m_eCoverage` | `EChecksumCoverage` | `PerEntry`, `Stream`, `HeadersOnly`, `None`, `Unavailable` |
| `m_cPassed`, `m_cDamaged`, `m_cSkipped` | `ULONGLONG` | |
| `m_vDamaged` | `vector<SDamagedEntry>` | Path and reason |

## SArchiveReport

One block of output.

| Field | Type | Notes |
|-------|------|-------|
| `m_path` | `filesystem::path` | Full path, outer archives included |
| `m_ullSizeOnDisk` | `ULONGLONG` | Entry size inside the parent for nested archives |
| `m_hr`, `m_strError` | `HRESULT`, `wstring` | Set when the archive couldn't be read. The block then contains only the error |
| `m_statistics` | `SArchiveStatistics` | |
| `m_details` | `SArchiveDetails` | |
| `m_verify` | `optional<SVerifyResult>` | Set with `--Verify` |

## Relationships

```text
CArchiveInfoFinder  --yields-->  archive paths
CArchiveInfoBuilder --reads via CArchiveFileSystem / IArchiveFileAccess-->  SArchiveReadResult (+ SArchiveDetails)
                    --computes-->  SArchiveStatistics
                    --optionally runs CArchiveVerifier-->  SVerifyResult
                    --produces-->  SArchiveReport
CArchiveInfoDisplayer --prints-->  SArchiveReport blocks and the summary
```
