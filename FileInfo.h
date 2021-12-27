#pragma once

#include "CommandLine.h"





class CFileInfo : public WIN32_FIND_DATA
{
public:
    CFileInfo(void);
    bool operator< (const CFileInfo& rhs) const;

    static CCommandLine* s_pCmdLine;
};
