#pragma once

#include "FileInfo.h"





class CDirectoryInfo
{
public:
    CDirectoryInfo(void);

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
