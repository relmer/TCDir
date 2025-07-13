#pragma once

typedef vector<WIN32_FIND_DATA>  FileInfoVector;
typedef FileInfoVector::iterator FileInfoVectorIter;

class CDirectoryInfo
{
public:
    CDirectoryInfo (const filesystem::path & dirPath, const filesystem::path & fileSpec) :
        m_dirPath            (dirPath),
        m_fileSpec           (fileSpec),
        m_cchLargestFileName (0),
        m_cFiles             (0),
        m_cSubDirectories    (0)
    {
        m_uliLargestFileSize.QuadPart = 0;
        m_uliBytesUsed.QuadPart       = 0;
    }

    FileInfoVector           m_vMatches;
    const filesystem::path & m_dirPath;
    const filesystem::path & m_fileSpec;
    ULARGE_INTEGER           m_uliLargestFileSize;
    size_t                   m_cchLargestFileName;
    UINT                     m_cFiles;
    UINT                     m_cSubDirectories;
    ULARGE_INTEGER           m_uliBytesUsed;
};
