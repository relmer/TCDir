#include "pch.h"
#include "FileInfo.h"





////////////////////////////////////////////////////////////////////////////////
//
//  Define class statics
//
////////////////////////////////////////////////////////////////////////////////

CCommandLine * CFileInfo::s_pCmdLine = NULL;





////////////////////////////////////////////////////////////////////////////////
//
//  CFileInfo::operator< 
//
//  Define a compoarison operator for use by std::sort
//
////////////////////////////////////////////////////////////////////////////////

CFileInfo::CFileInfo(void)
{
    cAlternateFileName[0]           = L'\0';
    cFileName[0]                    = L'\0';
    dwFileAttributes                = 0;
    dwReserved0                     = 0;
    dwReserved1                     = 0;
    ftCreationTime.dwHighDateTime   = 0;
    ftCreationTime.dwLowDateTime    = 0;
    ftLastAccessTime.dwHighDateTime = 0;
    ftLastAccessTime.dwLowDateTime  = 0;
    ftLastWriteTime.dwHighDateTime  = 0;
    ftLastWriteTime.dwLowDateTime   = 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CFileInfo::operator< 
//
//  Define a compoarison operator for use by std::sort
//
////////////////////////////////////////////////////////////////////////////////

bool CFileInfo::operator< (const CFileInfo& rhs) const
{
    bool                       comesBeforeRhs   = false;
    bool                       isDirectory      = false;
    bool                       isDirectoryRhs   = false;
    CCommandLine::ESortOrder * pSortAttribute   = NULL;
    CCommandLine::ESortOrder * pLastAttribute   = &s_pCmdLine->m_rgSortPreference[ARRAYSIZE(s_pCmdLine->m_rgSortPreference)];
    LONGLONG                   cmp              = 0;


    //
    // If only one of the operands is a directory, it should be sorted first
    //

    isDirectory = !!(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    isDirectoryRhs = !!(rhs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

    if (isDirectory ^ isDirectoryRhs)
    {
        return isDirectory;
    }

    //
    // Compare based on requested sort attribute with fallbacks
    //

    for (pSortAttribute = s_pCmdLine->m_rgSortPreference;
        pSortAttribute < pLastAttribute;
        ++pSortAttribute)
    {
        switch (*pSortAttribute)
        {
        case CCommandLine::ESortOrder::SO_DEFAULT:
        case CCommandLine::ESortOrder::SO_NAME:
            cmp = lstrcmpiW(cFileName, rhs.cFileName);
            break;

        case CCommandLine::ESortOrder::SO_DATE:
            cmp = CompareFileTime(&ftLastWriteTime, &rhs.ftLastWriteTime);
            break;

        case CCommandLine::ESortOrder::SO_EXTENSION:
            cmp = lstrcmpiW(PathFindExtension(cFileName), PathFindExtension(rhs.cFileName));
            break;

        case CCommandLine::ESortOrder::SO_SIZE:
        {
            ULARGE_INTEGER uliFileSize;
            ULARGE_INTEGER uliRhsFileSize;

            uliFileSize.HighPart = nFileSizeHigh;
            uliFileSize.LowPart  = nFileSizeLow;

            uliRhsFileSize.HighPart = rhs.nFileSizeHigh;
            uliRhsFileSize.LowPart  = rhs.nFileSizeLow;

            cmp = uliFileSize.QuadPart - uliRhsFileSize.QuadPart;
            break;
        }
        }

        //
        // If we didn't differ from rhs in this attribute, move on to the next one
        //

        if (cmp == 0)
        {
            continue;
        }

        comesBeforeRhs = cmp < 0;

        //
        // We found an attribute where we're different from the rhs, so break out of the sort loop
        //

        break;
    }

    //
    // Only respect reverse sorting if the comparison was against the requested attribute.  
    // If we had to fall back to another attribute because we did not differ from rhs in the
    // requested attribute, *ignore* reverse sorting for this attribute.
    //

    if (pSortAttribute == s_pCmdLine->m_rgSortPreference &&
        s_pCmdLine->m_sortdirection == CCommandLine::ESortDirection::SD_DESCENDING)
    {
        comesBeforeRhs = !comesBeforeRhs;
    }

    return comesBeforeRhs;
}





