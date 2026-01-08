#include "pch.h"
#include "FileComparator.h"





////////////////////////////////////////////////////////////////////////////////
//
//  FileComparator::FileComparator
//
//  
//
////////////////////////////////////////////////////////////////////////////////

FileComparator::FileComparator (shared_ptr<const CCommandLine> cmdLinePtr) :
    m_cmdLinePtr (cmdLinePtr)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  FileComparator::operator()
//
//  Compare two WIN32_FIND_DATA entries for sorting
//
////////////////////////////////////////////////////////////////////////////////

bool FileComparator::operator() (const WIN32_FIND_DATA & lhs, const WIN32_FIND_DATA & rhs) const
{
    bool                             comesBeforeRhs = false;
    bool                             isLhsDirectory = false;
    bool                             isRhsDirectory = false;
    const CCommandLine::ESortOrder * pSortAttribute = NULL;
    const CCommandLine::ESortOrder * pLastAttribute = &m_cmdLinePtr->m_rgSortPreference[ARRAYSIZE(m_cmdLinePtr->m_rgSortPreference)];
    LONGLONG                         cmp            = 0;



    //
    // If only one of the operands is a directory, it should be sorted first
    //

    isLhsDirectory = lhs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    isRhsDirectory = rhs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

    if (isLhsDirectory ^ isRhsDirectory)
    {
        return isLhsDirectory;
    }

    //
    // Compare based on requested sort attribute with fallbacks
    //

    for (pSortAttribute = m_cmdLinePtr->m_rgSortPreference;
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
            {
                filesystem::path lhsPath (lhs.cFileName);
                filesystem::path rhsPath (rhs.cFileName);
                cmp = lstrcmpiW(lhsPath.extension().c_str(), rhsPath.extension().c_str());
                break;
            }

            case CCommandLine::ESortOrder::SO_SIZE:
            {
                ULARGE_INTEGER uliFileSize;
                ULARGE_INTEGER uliRhsFileSize;



                // Use explicit relational comparison to avoid signed overflow from subtraction on 64-bit sizes.
                uliFileSize.HighPart = lhs.nFileSizeHigh;
                uliFileSize.LowPart  = lhs.nFileSizeLow;

                uliRhsFileSize.HighPart = rhs.nFileSizeHigh;
                uliRhsFileSize.LowPart  = rhs.nFileSizeLow;

                if (uliFileSize.QuadPart < uliRhsFileSize.QuadPart)
                {
                    cmp = -1;
                }
                else if (uliFileSize.QuadPart > uliRhsFileSize.QuadPart)
                {
                    cmp = 1;
                }
                else
                {
                    cmp = 0;
                }
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

    if (pSortAttribute == m_cmdLinePtr->m_rgSortPreference &&
        m_cmdLinePtr->m_sortdirection == CCommandLine::ESortDirection::SD_DESCENDING)
    {
        comesBeforeRhs = !comesBeforeRhs;
    }

    return comesBeforeRhs;
}
