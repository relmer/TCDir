#pragma once

#include "ArchiveEntry.h"





struct SLibArchiveDetails;





////////////////////////////////////////////////////////////////////////////////
//
//  SLibArchiveEntryInfo
//
//  Values read from one libarchive entry, before conversion.
//
////////////////////////////////////////////////////////////////////////////////

struct SLibArchiveEntryInfo
{
    wstring      m_strPath;
    unsigned     m_uFileType      = 0;      // archive_entry_filetype (AE_IF* values)
    bool         m_fSizeSet       = false;
    LONGLONG     m_llSize         = 0;
    bool         m_fMtimeSet      = false;
    LONGLONG     m_llMtime        = 0;      // Seconds since the Unix epoch
    long         m_lMtimeNsec     = 0;
    bool         m_fPermSet       = false;
    unsigned     m_uPerm          = 0;
    bool         m_fEncrypted     = false;
};





////////////////////////////////////////////////////////////////////////////////
//
//  CLibArchiveReader
//
//  Reads archive entry lists, and the data of single entries, through
//  archiveint.dll, the libarchive build that ships with Windows 11 23H2 and
//  later.  The DLL has no import library or header, so it is loaded at run
//  time on first use.
//
////////////////////////////////////////////////////////////////////////////////

class CLibArchiveReader
{
public:
    static bool          IsAvailable();
    static HRESULT       Read           (const SArchiveSource & source, SArchiveReadResult & result);
    static HRESULT       ExtractEntry   (const SArchiveSource & source,
                                         wstring_view           entryPath,
                                         ULONGLONG              cbMax,
                                         vector<BYTE>         & bytes);
    static SArchiveEntry ConvertEntry   (const SLibArchiveEntryInfo & info);
    static bool          IsSamePath     (wstring_view path1, wstring_view path2);

private:
    struct SApi;

    static const SApi *  GetApi();
    static void          LoadApi        (SApi & api);
    static void          LoadDetailApi  (HMODULE hModule, SApi & api);
    static void          AddEntryDetails  (const SApi & api, void * pEntry, const SLibArchiveEntryInfo & info, SLibArchiveDetails & lib);
    static void          AddStreamDetails (const SApi & api, void * pArchive, SLibArchiveDetails & lib);
    static void          AddName        (const wchar_t * (__cdecl * pfnName) (void *), void * pEntry, map<wstring, ULONGLONG> & counts);
    static ULONGLONG     CountIf        (int (__cdecl * pfn) (void *), void * pEntry);
    static wstring       Widen          (const char * psz);
    static HRESULT       Open           (const SApi & api, const SArchiveSource & source, void *& pArchive);
    static HRESULT       ReadEntries    (const SApi & api, void * pArchive, SArchiveReadResult & result);
    static HRESULT       ReadEntryData  (const SApi & api, void * pArchive, ULONGLONG cbMax, ULONGLONG cbExpected, vector<BYTE> & bytes);
    static void          GetEntryInfo   (const SApi & api, void * pEntry, SLibArchiveEntryInfo & info);
    static FILETIME      UnixToFileTime (LONGLONG llSeconds, long lNanoseconds);

    //
    // libarchive constants (archive.h, archive_entry.h)
    //

    static constexpr int       s_kiArchiveOk           = 0;
    static constexpr int       s_kiArchiveEof          = 1;
    static constexpr int       s_kiArchiveWarn         = -20;
    static constexpr unsigned  s_kuFileTypeDirectory   = 0040000;
    static constexpr unsigned  s_kuFileTypeSymlink     = 0120000;
    static constexpr unsigned  s_kuPermMask            = 07777;
    static constexpr int       s_kiAclTypeAll          = 0x3F00;  // ACCESS | DEFAULT | the NFS4 types
    static constexpr int       s_kiRawFilter           = -1;      // archive_filter_bytes index for the bytes read from the source
    static constexpr unsigned  s_kuPermOwnerWrite      = 0000200;
    static constexpr size_t    s_kcbReadBlock          = 65536;

    //
    // FILETIME is in 100 ns units from 1601-01-01; the Unix epoch is 1970-01-01
    //

    static constexpr LONGLONG  s_kllUnixEpochTicks     = 116444736000000000LL;
    static constexpr LONGLONG  s_kllTicksPerSecond     = 10000000LL;
    static constexpr long      s_klNanosecondsPerTick  = 100;
};
