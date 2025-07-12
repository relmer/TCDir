#pragma once

#include "CommandLine.h"

class FileComparator
{
public:
    FileComparator(const CCommandLine* pCmdLine);
    
    bool operator()(const WIN32_FIND_DATA& lhs, const WIN32_FIND_DATA& rhs) const;

private:
    const CCommandLine* m_pCmdLine;
};
