#pragma once

#include "FileInfo.h"





class CDirectoryInfo
{
public:
    CDirectoryInfo(void) :
        m_pszPath               (NULL),
        m_pszFileSpec           (NULL),
        m_cchLargestFileName    (0),
        m_cFiles                (0),
        m_cSubDirectories       (0)
    {
        m_uliLargestFileSize.QuadPart   = 0;
        m_uliBytesUsed.QuadPart         = 0;
    }

    FileInfoVector m_vMatches;
    LPCWSTR        m_pszPath;
    LPCWSTR        m_pszFileSpec;
    ULARGE_INTEGER m_uliLargestFileSize;
    size_t         m_cchLargestFileName;
    UINT           m_cFiles;
    UINT           m_cSubDirectories;
    ULARGE_INTEGER m_uliBytesUsed;
};

typedef vector<CFileInfo>        FileInfoVector;
typedef FileInfoVector::iterator FileInfoVectorIter;
