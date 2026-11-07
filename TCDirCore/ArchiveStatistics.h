#pragma once

#include "ArchiveReport.h"
#include "ArchiveTree.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveStatistics
//
//  Computes the --Info values that every archive format has, from the entry
//  list a reader returns.
//
////////////////////////////////////////////////////////////////////////////////

class CArchiveStatistics
{
public:
    static HRESULT Compute (const SArchiveReadResult & result, SArchiveStatistics & statistics);

private:
    using ExtensionMap = map<wstring, SExtensionGroup>;

    static void    AddTimesAndEncryption (const vector<SArchiveEntry> & entries, SArchiveStatistics & statistics);
    static void    WalkTree              (const CArchiveTree & tree, SArchiveStatistics & statistics, ExtensionMap & extensions);
    static void    AddFile               (const CArchiveTree::SNode & node, SArchiveStatistics & statistics, ExtensionMap & extensions);
    static wstring PathOf                (const CArchiveTree & tree, size_t iNode);
    static wstring ExtensionOf           (const wstring & strName);
    static void    SortExtensions        (ExtensionMap & extensions, vector<SExtensionGroup> & vExtensions);
    static bool    IsTimeSet             (const FILETIME & ft);
};
