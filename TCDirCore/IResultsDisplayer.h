#pragma once

#include "DirectoryInfo.h"
#include "DriveInfo.h"
#include "ListingTotals.h"





////////////////////////////////////////////////////////////////////////////////
//
//  IResultsDisplayer
//
//  Interface for displaying directory listing results.
//
////////////////////////////////////////////////////////////////////////////////  

class IResultsDisplayer
{
public:
    enum class EDirectoryLevel
    {
        Initial,
        Subdirectory
    };

    virtual ~IResultsDisplayer                (void) = default;
    virtual void DisplayResults               (const CDriveInfo & driveInfo, const CDirectoryInfo & di, EDirectoryLevel level) = 0;
    virtual void DisplayRecursiveSummary      (const CDirectoryInfo & diInitial, const SListingTotals & totals) = 0;
};

