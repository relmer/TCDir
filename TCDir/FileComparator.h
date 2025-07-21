#pragma once

#include "CommandLine.h"

class FileComparator
{
public:
    FileComparator (shared_ptr<const CCommandLine> cmdLinePtr);
    
    bool operator()(const WIN32_FIND_DATA& lhs, const WIN32_FIND_DATA& rhs) const;

private:
    shared_ptr<const CCommandLine> m_cmdLinePtr;
};
