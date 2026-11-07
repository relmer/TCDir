#include "pch.h"

#include "ArchiveStatistics.h"

#include "ArchiveFormat.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveStatistics::Compute
//
//  Folder counts, sizes and paths come from the folder tree, so implied
//  folders are counted and paths are sanitized as they are in a listing.
//  Times and encryption come from the entries, because implied folders don't
//  have a stored time.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveStatistics::Compute (const SArchiveReadResult & result, SArchiveStatistics & statistics)
{
    HRESULT                    hr      = S_OK;
    unique_ptr<CArchiveTree>   treePtr;
    ExtensionMap               extensions;



    statistics                     = {};
    statistics.m_fListingEncrypted = result.m_fListingEncrypted;

    hr = CArchiveTree::Build (result.m_vEntries, treePtr);
    CHR (hr);

    WalkTree (*treePtr, statistics, extensions);
    AddTimesAndEncryption (result.m_vEntries, statistics);
    SortExtensions (extensions, statistics.m_vExtensions);



Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveStatistics::AddTimesAndEncryption
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveStatistics::AddTimesAndEncryption (const vector<SArchiveEntry> & entries, SArchiveStatistics & statistics)
{
    for (const SArchiveEntry & entry : entries)
    {
        if (entry.m_fEncrypted)
        {
            ++statistics.m_cEncrypted;
        }

        if (!IsTimeSet (entry.m_ftLastWrite))
        {
            continue;
        }

        if (!statistics.m_ftOldest.has_value() || CompareFileTime (&entry.m_ftLastWrite, &statistics.m_ftOldest.value()) < 0)
        {
            statistics.m_ftOldest = entry.m_ftLastWrite;
        }

        if (!statistics.m_ftNewest.has_value() || CompareFileTime (&entry.m_ftLastWrite, &statistics.m_ftNewest.value()) > 0)
        {
            statistics.m_ftNewest = entry.m_ftLastWrite;
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveStatistics::WalkTree
//
//  Visits every node below the root in listing order.  On a tie, the first
//  node visited is the largest or deepest.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveStatistics::WalkTree (const CArchiveTree & tree, SArchiveStatistics & statistics, ExtensionMap & extensions)
{
    vector<pair<size_t, UINT>>   stack;
    size_t                       iLargest = 0;
    size_t                       iDeepest = 0;
    bool                         fLargest = false;



    for (auto it = tree.Root().m_vChildren.rbegin(); it != tree.Root().m_vChildren.rend(); ++it)
    {
        stack.emplace_back (*it, 1);
    }

    while (!stack.empty())
    {
        auto [iNode, cDepth] = stack.back();
        const CArchiveTree::SNode & node = tree.Node (iNode);

        stack.pop_back();

        if (cDepth > statistics.m_cDeepestDepth)
        {
            statistics.m_cDeepestDepth = cDepth;
            iDeepest                   = iNode;
        }

        if (node.m_fDirectory)
        {
            ++statistics.m_cFolders;

            for (auto it = node.m_vChildren.rbegin(); it != node.m_vChildren.rend(); ++it)
            {
                stack.emplace_back (*it, cDepth + 1);
            }

            continue;
        }

        AddFile (node, statistics, extensions);

        if (!fLargest || node.m_ullSize > statistics.m_ullLargestSize)
        {
            fLargest                    = true;
            iLargest                    = iNode;
            statistics.m_ullLargestSize = node.m_ullSize;
        }
    }

    if (fLargest)
    {
        statistics.m_strLargestPath = PathOf (tree, iLargest);
    }

    if (statistics.m_cDeepestDepth > 0)
    {
        statistics.m_strDeepestPath = PathOf (tree, iDeepest);
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveStatistics::AddFile
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveStatistics::AddFile (const CArchiveTree::SNode & node, SArchiveStatistics & statistics, ExtensionMap & extensions)
{
    wstring           strExtension = ExtensionOf (node.m_strName);
    SExtensionGroup & group        = extensions[strExtension];



    ++statistics.m_cFiles;
    statistics.m_ullUncompressed += node.m_ullSize;

    if (CArchiveFormat::FromFileName (node.m_strName, true) != EArchiveFormat::None)
    {
        ++statistics.m_cNestedArchives;
    }

    group.m_strExtension  = strExtension;
    group.m_cFiles       += 1;
    group.m_ullSize      += node.m_ullSize;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveStatistics::PathOf
//
//  Returns the path of a node below the root, with '\' separators.
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveStatistics::PathOf (const CArchiveTree & tree, size_t iNode)
{
    vector<const wstring *>       components;
    wstring                       strPath;
    const CArchiveTree::SNode   * pNode   = &tree.Node (iNode);



    while (pNode != &tree.Root())
    {
        components.push_back (&pNode->m_strName);
        pNode = &tree.Node (pNode->m_iParent);
    }

    for (auto it = components.rbegin(); it != components.rend(); ++it)
    {
        if (!strPath.empty())
        {
            strPath += L'\\';
        }

        strPath += **it;
    }

    return strPath;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveStatistics::ExtensionOf
//
//  Returns the lowercase extension with its dot, or s_kpszNoExtension.  A
//  name whose only dot is the first character has no extension.
//
////////////////////////////////////////////////////////////////////////////////

wstring CArchiveStatistics::ExtensionOf (const wstring & strName)
{
    size_t    iDot         = strName.rfind (L'.');
    wstring   strExtension = SExtensionGroup::s_kpszNoExtension;



    if (iDot != wstring::npos && iDot > 0 && iDot + 1 < strName.size())
    {
        strExtension = strName.substr (iDot);

        for (wchar_t & ch : strExtension)
        {
            ch = (wchar_t) towlower (ch);
        }
    }

    return strExtension;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveStatistics::SortExtensions
//
//  Largest total size first, then by extension.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveStatistics::SortExtensions (ExtensionMap & extensions, vector<SExtensionGroup> & vExtensions)
{
    vExtensions.clear();
    vExtensions.reserve (extensions.size());

    for (auto & [strExtension, group] : extensions)
    {
        vExtensions.push_back (move (group));
    }

    stable_sort (vExtensions.begin(), vExtensions.end(), [] (const SExtensionGroup & a, const SExtensionGroup & b)
    {
        return a.m_ullSize > b.m_ullSize;
    });
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveStatistics::IsTimeSet
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveStatistics::IsTimeSet (const FILETIME & ft)
{
    return ft.dwLowDateTime != 0 || ft.dwHighDateTime != 0;
}





