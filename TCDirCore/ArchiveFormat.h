#pragma once

#include "ArchiveEntry.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveFormat
//
//  Maps a file name to the archive reader that handles it.
//
////////////////////////////////////////////////////////////////////////////////

class CArchiveFormat
{
public:
    static EArchiveFormat FromFileName (wstring_view fileName, bool fLibArchiveAvailable);

private:
    struct SSuffix
    {
        wstring_view     m_suffix;
        EArchiveFormat   m_format;
    };

    static bool EndsWithNoCase (wstring_view text, wstring_view suffix);

    static const SSuffix s_krgSuffixes[];
};
