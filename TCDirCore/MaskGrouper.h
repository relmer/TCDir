#pragma once

#include "pch.h"





typedef pair<filesystem::path, vector<filesystem::path>> MaskGroup;





class CMaskGrouper
{
private:
    struct CaseInsensitiveLess
    {
        bool operator() (const wstring & a, const wstring & b) const
        {
            return _wcsicmp (a.c_str(), b.c_str()) < 0;
        }
    };

public:
    static bool              IsPureMask            (const wstring & mask);
    static vector<MaskGroup> GroupMasksByDirectory (const list<wstring> & masks);

private:
    static void SplitMaskIntoDirAndFileSpec        (const wstring          & mask,
                                                    const filesystem::path & cwd,
                                                    filesystem::path       & dirPath,
                                                    filesystem::path       & fileSpec);

    static void AddMaskToGroups                    (const filesystem::path                      & dirPath,
                                                    const filesystem::path                      & fileSpec,
                                                    vector<MaskGroup>                           & groups,
                                                    map<wstring, size_t, CaseInsensitiveLess>   & dirToIndex);
};
