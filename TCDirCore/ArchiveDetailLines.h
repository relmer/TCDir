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

    static constexpr int    s_kcHostShift       = 8;
    static constexpr BYTE   s_kbVersionMask     = 0xFF;
    static constexpr int    s_kcVersionDivisor  = 10;
};
