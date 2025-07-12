#include "pch.h"
#include "FileComparator.h"





////////////////////////////////////////////////////////////////////////////////
//
//  FileComparator::FileComparator
//
//  
//
////////////////////////////////////////////////////////////////////////////////

FileComparator::FileComparator(const CCommandLine* pCmdLine) :
    m_pCmdLine(pCmdLine)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  FileComparator::operator()
//
//  Compare two WIN32_FIND_DATA entries for sorting
//
////////////////////////////////////////////////////////////////////////////////

bool FileComparator::operator()(const WIN32_FIND_DATA& lhs, const WIN32_FIND_DATA& rhs) const
{
    bool                             comesBeforeRhs   = false;
    bool                             isDirectory      = false;
    bool                             isDirectoryRhs   = false;
    const CCommandLine::ESortOrder * pSortAttribute   = NULL;
    const CCommandLine::ESortOrder * pLastAttribute   = &m_pCmdLine->m_rgSortPreference[ARRAYSIZE(m_pCmdLine->m_rgSortPreference)];
    LONGLONG                         cmp              = 0;

    //
    // If only one of the operands is a directory, it should be sorted first
    //

    isDirectory = !!(lhs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    isDirectoryRhs = !!(rhs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

    if (isDirectory ^ isDirectoryRhs)
    {
        return isDirectory;
    }

    //
    // Compare based on requested sort attribute with fallbacks
    //

    for (pSortAttribute = m_pCmdLine->m_rgSortPreference;
        pSortAttribute < pLastAttribute;
        ++pSortAttribute)
    {
        switch (*pSortAttribute)
        {
        case CCommandLine::ESortOrder::SO_DEFAULT:
        case CCommandLine::ESortOrder::SO_NAME:
            cmp = lstrcmpiW(lhs.cFileName, rhs.cFileName);
            break;

        case CCommandLine::ESortOrder::SO_DATE:
            cmp = CompareFileTime(&lhs.ftLastWriteTime, &rhs.ftLastWriteTime);
            break;

        case CCommandLine::ESortOrder::SO_EXTENSION:
            cmp = lstrcmpiW(PathFindExtension(lhs.cFileName), PathFindExtension(rhs.cFileName));
            break;

        case CCommandLine::ESortOrder::SO_SIZE:
        {
            ULARGE_INTEGER uliFileSize;
            ULARGE_INTEGER uliRhsFileSize;

            uliFileSize.HighPart = lhs.nFileSizeHigh;
            uliFileSize.LowPart  = lhs.nFileSizeLow;

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

    if (pSortAttribute == m_pCmdLine->m_rgSortPreference &&
        m_pCmdLine->m_sortdirection == CCommandLine::ESortDirection::SD_DESCENDING)
    {
        comesBeforeRhs = !comesBeforeRhs;
    }

    return comesBeforeRhs;
}
