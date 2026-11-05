#include "pch.h"

#include "ArchiveFormat.h"





//
// Compound suffixes come before the simple suffixes they end with, so the
// first match is the longest one.
//

const CArchiveFormat::SSuffix CArchiveFormat::s_krgSuffixes[] =
{
    { L".zip"sv,      EArchiveFormat::Zip        },
    { L".iso"sv,      EArchiveFormat::Iso        },
    { L".tar.gz"sv,   EArchiveFormat::LibArchive },
    { L".tar.bz2"sv,  EArchiveFormat::LibArchive },
    { L".tar.xz"sv,   EArchiveFormat::LibArchive },
    { L".tar.zst"sv,  EArchiveFormat::LibArchive },
    { L".tar.lz4"sv,  EArchiveFormat::LibArchive },
    { L".tar.lzma"sv, EArchiveFormat::LibArchive },
    { L".7z"sv,       EArchiveFormat::LibArchive },
    { L".rar"sv,      EArchiveFormat::LibArchive },
    { L".tar"sv,      EArchiveFormat::LibArchive },
    { L".tgz"sv,      EArchiveFormat::LibArchive },
    { L".tbz"sv,      EArchiveFormat::LibArchive },
    { L".tbz2"sv,     EArchiveFormat::LibArchive },
    { L".txz"sv,      EArchiveFormat::LibArchive },
    { L".tzst"sv,     EArchiveFormat::LibArchive },
    { L".tlz"sv,      EArchiveFormat::LibArchive },
    { L".cab"sv,      EArchiveFormat::LibArchive },
    { L".cpio"sv,     EArchiveFormat::LibArchive },
    { L".xar"sv,      EArchiveFormat::LibArchive },
    { L".lzh"sv,      EArchiveFormat::LibArchive },
    { L".lha"sv,      EArchiveFormat::LibArchive },
};





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFormat::FromFileName
//
//  Returns the reader for fileName, or EArchiveFormat::None.  Formats read
//  through archiveint.dll are recognized only when that DLL is available, so
//  on older Windows versions those files list as ordinary files.
//
////////////////////////////////////////////////////////////////////////////////

EArchiveFormat CArchiveFormat::FromFileName (wstring_view fileName, bool fLibArchiveAvailable)
{
    EArchiveFormat format = EArchiveFormat::None;



    for (const SSuffix & entry : s_krgSuffixes)
    {
        if (!EndsWithNoCase (fileName, entry.m_suffix))
        {
            continue;
        }

        if (entry.m_format != EArchiveFormat::LibArchive || fLibArchiveAvailable)
        {
            format = entry.m_format;
        }

        break;
    }

    return format;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFormat::EndsWithNoCase
//
//  The suffix must follow at least one character, so a file called ".zip"
//  is not an archive.
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveFormat::EndsWithNoCase (wstring_view text, wstring_view suffix)
{
    bool fEndsWith = false;



    if (text.size() > suffix.size())
    {
        wstring_view tail = text.substr (text.size() - suffix.size());

        fEndsWith = CompareStringOrdinal (tail.data(),
                                          static_cast<int> (tail.size()),
                                          suffix.data(),
                                          static_cast<int> (suffix.size()),
                                          TRUE) == CSTR_EQUAL;
    }

    return fEndsWith;
}





