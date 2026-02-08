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
                cmp = CompareName (lhs, rhs);
                break;

            case CCommandLine::ESortOrder::SO_DATE:
                cmp = CompareDate (lhs, rhs);
                break;

            case CCommandLine::ESortOrder::SO_EXTENSION:
                cmp = CompareExtension (lhs, rhs);
                break;

            case CCommandLine::ESortOrder::SO_SIZE:
                cmp = CompareSize (lhs, rhs);
                break;
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





////////////////////////////////////////////////////////////////////////////////
//
//  FileComparator::CompareName
//
//  Compare two files by name (case-insensitive)
//
////////////////////////////////////////////////////////////////////////////////

LONGLONG FileComparator::CompareName (const WIN32_FIND_DATA & lhs, const WIN32_FIND_DATA & rhs) const
{
    return lstrcmpiW (lhs.cFileName, rhs.cFileName);
}





////////////////////////////////////////////////////////////////////////////////
//
//  FileComparator::CompareDate
//
//  Compare two files by date, using the time field selected via /T: switch
//
////////////////////////////////////////////////////////////////////////////////

LONGLONG FileComparator::CompareDate (const WIN32_FIND_DATA & lhs, const WIN32_FIND_DATA & rhs) const
{
    const FILETIME * pftLhs = nullptr;
    const FILETIME * pftRhs = nullptr;



    switch (m_cmdLinePtr->m_timeField)
    {
        case CCommandLine::ETimeField::TF_CREATION:
            pftLhs = &lhs.ftCreationTime;
            pftRhs = &rhs.ftCreationTime;
            break;

        case CCommandLine::ETimeField::TF_ACCESS:
            pftLhs = &lhs.ftLastAccessTime;
            pftRhs = &rhs.ftLastAccessTime;
            break;

        case CCommandLine::ETimeField::TF_WRITTEN:
        default:
            pftLhs = &lhs.ftLastWriteTime;
            pftRhs = &rhs.ftLastWriteTime;
            break;
    }

    return CompareFileTime (pftLhs, pftRhs);
}





////////////////////////////////////////////////////////////////////////////////
//
//  FileComparator::CompareExtension
//
//  Compare two files by extension (case-insensitive)
//
////////////////////////////////////////////////////////////////////////////////

LONGLONG FileComparator::CompareExtension (const WIN32_FIND_DATA & lhs, const WIN32_FIND_DATA & rhs) const
{
    LPCWSTR pszLhsExt = wcsrchr (lhs.cFileName, L'.');
    LPCWSTR pszRhsExt = wcsrchr (rhs.cFileName, L'.');



    return lstrcmpiW (pszLhsExt ? pszLhsExt : L"", pszRhsExt ? pszRhsExt : L"");    
}





////////////////////////////////////////////////////////////////////////////////
//
//  FileComparator::CompareSize
//
//  Compare two files by size
//
////////////////////////////////////////////////////////////////////////////////

LONGLONG FileComparator::CompareSize (const WIN32_FIND_DATA & lhs, const WIN32_FIND_DATA & rhs) const
{
    ULARGE_INTEGER uliLhsSize;
    ULARGE_INTEGER uliRhsSize;



    uliLhsSize.HighPart = lhs.nFileSizeHigh;
    uliLhsSize.LowPart  = lhs.nFileSizeLow;

    uliRhsSize.HighPart = rhs.nFileSizeHigh;
    uliRhsSize.LowPart  = rhs.nFileSizeLow;

    //
    // Use explicit relational comparison to avoid signed overflow 
    // from subtraction on 64-bit sizes.
    //

    if (uliLhsSize.QuadPart < uliRhsSize.QuadPart)
    {
        return -1;
    }
    else if (uliLhsSize.QuadPart > uliRhsSize.QuadPart)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
