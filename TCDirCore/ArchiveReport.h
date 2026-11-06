#pragma once

#include "ArchiveDetails.h"





////////////////////////////////////////////////////////////////////////////////
//
//  SExtensionGroup
//
//  Files that share one extension.  The extension is lowercase and includes
//  the dot; files without an extension use s_kpszNoExtension.
//
////////////////////////////////////////////////////////////////////////////////

struct SExtensionGroup
{
    static constexpr LPCWSTR s_kpszNoExtension = L"(none)";

    wstring     m_strExtension;
    ULONGLONG   m_cFiles  = 0;
    ULONGLONG   m_ullSize = 0;
};





////////////////////////////////////////////////////////////////////////////////
//
//  SArchiveStatistics
//
//  Values computed from an archive's entry list, for every format.
//
////////////////////////////////////////////////////////////////////////////////

struct SArchiveStatistics
{
    ULONGLONG                 m_cFiles            = 0;
    ULONGLONG                 m_cFolders          = 0;   // Explicit and implied folders
    ULONGLONG                 m_cNestedArchives   = 0;
    ULONGLONG                 m_ullUncompressed   = 0;
    wstring                   m_strLargestPath;
    ULONGLONG                 m_ullLargestSize    = 0;
    wstring                   m_strDeepestPath;
    UINT                      m_cDeepestDepth     = 0;   // Path components
    optional<FILETIME>        m_ftOldest;
    optional<FILETIME>        m_ftNewest;
    ULONGLONG                 m_cEncrypted        = 0;
    bool                      m_fListingEncrypted = false;
    vector<SExtensionGroup>   m_vExtensions;               // Largest total size first
};





////////////////////////////////////////////////////////////////////////////////
//
//  EChecksumCoverage
//
//  What the checksums stored in an archive protect.
//
////////////////////////////////////////////////////////////////////////////////

enum class EChecksumCoverage
{
    PerEntry,       // Each entry's data
    Stream,         // The compressed stream as a whole
    HeadersOnly,    // Headers only; entry data has no checksum
    None,           // No checksums
    Unavailable     // The archive can't be read for verification
};





////////////////////////////////////////////////////////////////////////////////
//
//  SVerifyResult
//
////////////////////////////////////////////////////////////////////////////////

struct SDamagedEntry
{
    wstring   m_strPath;
    wstring   m_strReason;
};

struct SVerifyResult
{
    EChecksumCoverage       m_eCoverage = EChecksumCoverage::None;
    ULONGLONG               m_cPassed   = 0;
    ULONGLONG               m_cDamaged  = 0;
    ULONGLONG               m_cSkipped  = 0;
    vector<SDamagedEntry>   m_vDamaged;
    wstring                 m_strError;                  // Set when verification couldn't run
};





////////////////////////////////////////////////////////////////////////////////
//
//  SArchiveReport
//
//  One block of --Info output.
//
////////////////////////////////////////////////////////////////////////////////

struct SArchiveReport
{
    filesystem::path          m_path;                      // Full path, outer archives included
    ULONGLONG                 m_ullSizeOnDisk = 0;         // Entry size inside the parent for a nested archive
    HRESULT                   m_hr            = S_OK;
    wstring                   m_strError;                  // Set when the archive couldn't be read
    SArchiveStatistics        m_statistics;
    SArchiveDetails           m_details;
    optional<SVerifyResult>   m_verify;
};
