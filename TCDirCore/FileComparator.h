#pragma once

#include "CommandLine.h"





class FileComparator
{
public:
    FileComparator (shared_ptr<const CCommandLine> cmdLinePtr);
    
    bool operator()(const WIN32_FIND_DATA& lhs, const WIN32_FIND_DATA& rhs) const;

private:
    LONGLONG CompareName      (const WIN32_FIND_DATA & lhs, const WIN32_FIND_DATA & rhs) const;
    LONGLONG CompareDate      (const WIN32_FIND_DATA & lhs, const WIN32_FIND_DATA & rhs) const;
    LONGLONG CompareExtension (const WIN32_FIND_DATA & lhs, const WIN32_FIND_DATA & rhs) const;
    LONGLONG CompareSize      (const WIN32_FIND_DATA & lhs, const WIN32_FIND_DATA & rhs) const;

    shared_ptr<const CCommandLine> m_cmdLinePtr;
};
