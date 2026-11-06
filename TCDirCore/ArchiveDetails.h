#pragma once

#include "ArchiveEntry.h"





////////////////////////////////////////////////////////////////////////////////
//
//  SMethodGroup
//
//  Totals for the ZIP entries that use one compression method.
//
////////////////////////////////////////////////////////////////////////////////

struct SMethodGroup
{
    ULONGLONG   m_cFiles          = 0;
    ULONGLONG   m_ullCompressed   = 0;
    ULONGLONG   m_ullUncompressed = 0;
};





////////////////////////////////////////////////////////////////////////////////
//
//  EZipEncryption
//
////////////////////////////////////////////////////////////////////////////////

enum class EZipEncryption
{
    None,
    ZipCrypto,
    Aes128,
    Aes192,
    Aes256,
    PkwareStrong
};





////////////////////////////////////////////////////////////////////////////////
//
//  SZipDetails
//
//  Values that only ZIP archives store.
//
////////////////////////////////////////////////////////////////////////////////

struct SZipDetails
{
    map<WORD, SMethodGroup>               m_mapMethods;                  // Key: method number (the real method for AES entries)
    map<EZipEncryption, ULONGLONG>        m_mapEncryption;               // Entry count per encryption type
    map<WORD, ULONGLONG>                  m_mapCreators;                 // Entry count per "version made by"
    bool                                  m_fZip64          = false;
    bool                                  m_fUtf8Paths      = false;
    bool                                  m_fMultiDisk      = false;
    ULONGLONG                             m_ullPrefixSize   = 0;         // Bytes before the first local header
    wstring                               m_strComment;
};





////////////////////////////////////////////////////////////////////////////////
//
//  EDiscTree
//
//  The file system tree that a disc image listing comes from.
//
////////////////////////////////////////////////////////////////////////////////

enum class EDiscTree
{
    Primary,
    Joliet,
    Udf
};





////////////////////////////////////////////////////////////////////////////////
//
//  SDiscDetails
//
//  Volume values from an ISO 9660 or UDF image.
//
////////////////////////////////////////////////////////////////////////////////

struct SDiscDetails
{
    bool                 m_fIso9660        = false;
    bool                 m_fJoliet         = false;
    bool                 m_fRockRidge      = false;
    WORD                 m_wUdfRevision    = 0;                  // BCD, such as 0x0250; 0 when there is no UDF volume
    EDiscTree            m_eListedTree     = EDiscTree::Primary;
    wstring              m_strVolumeLabel;
    wstring              m_strSystemId;
    wstring              m_strPublisher;
    wstring              m_strPreparer;
    wstring              m_strApplication;
    optional<FILETIME>   m_ftCreated;                            // UTC
    optional<FILETIME>   m_ftModified;
    optional<FILETIME>   m_ftExpires;
    ULONG                m_ulSectorSize    = 0;
    ULONGLONG            m_ullSectorCount  = 0;
    vector<BYTE>         m_vBootPlatforms;                       // El Torito platform IDs in catalog order
};





////////////////////////////////////////////////////////////////////////////////
//
//  SLibArchiveDetails
//
//  Values reported by libarchive for the formats it reads.
//
////////////////////////////////////////////////////////////////////////////////

struct SLibArchiveDetails
{
    wstring                     m_strFormatName;
    vector<wstring>             m_vFilters;                      // Outermost first
    optional<ULONGLONG>         m_ullStreamCompressed;           // Set only when a filter is present
    optional<ULONGLONG>         m_ullStreamUncompressed;
    bool                        m_fHeadersEncrypted = false;
    ULONGLONG                   m_cSymlinks         = 0;
    ULONGLONG                   m_cHardLinks        = 0;
    map<wstring, ULONGLONG>     m_mapOwners;                     // Entry count per user name
    map<wstring, ULONGLONG>     m_mapGroups;                     // Entry count per group name
    map<unsigned, ULONGLONG>    m_mapPermissions;                // Entry count per permission set
    ULONGLONG                   m_cBirthTimes       = 0;
    ULONGLONG                   m_cAccessTimes      = 0;
    ULONGLONG                   m_cSparse           = 0;
    ULONGLONG                   m_cXattrs           = 0;
    ULONGLONG                   m_cAcls             = 0;
};





////////////////////////////////////////////////////////////////////////////////
//
//  SArchiveDetails
//
//  Format details filled by a reader when the caller requests them.  Only
//  the member for m_format applies.
//
////////////////////////////////////////////////////////////////////////////////

struct SArchiveDetails
{
    EArchiveFormat       m_format = EArchiveFormat::None;
    SZipDetails          m_zip;
    SDiscDetails         m_disc;
    SLibArchiveDetails   m_libArchive;
};
