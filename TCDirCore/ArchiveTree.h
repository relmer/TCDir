#pragma once

#include "ArchiveEntry.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveTree
//
//  Immutable folder tree built from the flat entry list of one archive.
//  Shared read-only between worker threads after Build returns.
//
////////////////////////////////////////////////////////////////////////////////

class CArchiveTree
{
public:
    struct SNode
    {
        wstring          m_strName;
        bool             m_fDirectory   = false;
        bool             m_fImplied     = false;   // Folder that exists only as part of entry paths
        ULONGLONG        m_ullSize      = 0;
        FILETIME         m_ftLastWrite  = {};
        DWORD            m_dwAttributes = 0;
        size_t           m_iParent      = 0;
        vector<size_t>   m_vChildren;
    };

    using EnumCallback = function<bool (const WIN32_FIND_DATA &)>;

    static HRESULT Build             (const vector<SArchiveEntry> & entries, unique_ptr<CArchiveTree> & tree);

    const SNode &  Root() const      { return m_vNodes[s_kiRoot]; }
    size_t         NodeCount() const { return m_vNodes.size(); }
    const SNode &  Node (size_t iNode) const { return m_vNodes[iNode]; }
    const SNode *  FindFolder        (wstring_view innerPath) const;
    const SNode *  FindFile          (wstring_view innerPath) const;
    void           EnumerateChildren (const SNode & folder, wstring_view spec, const EnumCallback & callback) const;

    static void    ToFindData        (const SNode & node, WIN32_FIND_DATA & wfd);
    static bool    SplitPath         (wstring_view path, vector<wstring> & components);

private:
    using FolderKey = pair<size_t, wstring>;

    struct SFolderKeyHash
    {
        size_t operator() (const FolderKey & key) const
        {
            return hash<wstring>() (key.second) ^ (key.first * s_kHashMultiplier);
        }
    };

    using FolderIndex = unordered_map<FolderKey, size_t, SFolderKeyHash>;

    size_t         FindOrAddFolder   (size_t iParent, const wstring & name, FolderIndex & folderIndex);
    void           AddEntry          (const SArchiveEntry & entry, FolderIndex & folderIndex);
    void           FillImpliedTimes();

    static bool    IsSkippedComponent (const wstring & component);

    static constexpr size_t s_kiRoot          = 0;
    static constexpr size_t s_kHashMultiplier = 0x9E3779B97F4A7C15ull;

    vector<SNode>   m_vNodes;
};
