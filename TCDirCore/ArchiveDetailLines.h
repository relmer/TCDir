#pragma once

#include "ArchiveInfoDisplayer.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveDetailLines
//
//  Builds the --Info rows that only one format family has: ZIP, disc
//  images, or the formats read through libarchive.
//
////////////////////////////////////////////////////////////////////////////////

class CArchiveDetailLines
{
public:
    static wstring FormatName       (const SArchiveDetails & details);
    static bool    AddEncryption    (const SArchiveReport & report, vector<SInfoLine> & lines);
    static void    AddDetails       (const SArchiveReport & report, vector<SInfoLine> & lines);

    static wstring MethodName       (WORD wMethod);
    static wstring HostName         (BYTE bHost);
    static wstring EncryptionName   (EZipEncryption encryption);

private:
    static wstring ZipName          (const SZipDetails & zip);
    static void    AddZipDetails    (const SZipDetails & zip, vector<SInfoLine> & lines);
    static void    AddZipMethods    (const SZipDetails & zip, vector<SInfoLine> & lines);
    static void    AddCommentLines  (const wstring & strComment, vector<SInfoLine> & lines);
    static void    AddDiscDetails   (const SDiscDetails & disc, vector<SInfoLine> & lines);
    static wstring FileSystemsOf    (const SDiscDetails & disc);
    static wstring BootPlatformsOf  (const SDiscDetails & disc);
    static void    AddLibArchiveDetails (const SLibArchiveDetails & lib, vector<SInfoLine> & lines);
    static wstring LibArchiveName       (const SLibArchiveDetails & lib);
    static void    AddCountPart         (ULONGLONG cItems, LPCWSTR pszSingular, LPCWSTR pszPlural, vector<wstring> & parts);
    static void    AddJoinedLine        (LPCWSTR pszLabel, vector<wstring> & parts, vector<SInfoLine> & lines);
    static void    AddNameCounts        (LPCWSTR pszLabel, const map<wstring, ULONGLONG> & counts, vector<SInfoLine> & lines);
    static void    AddPermissionCounts  (const map<unsigned, ULONGLONG> & counts, vector<SInfoLine> & lines);

    static constexpr int    s_kcHostShift       = 8;
    static constexpr BYTE   s_kbVersionMask     = 0xFF;
    static constexpr int    s_kcVersionDivisor  = 10;
};
