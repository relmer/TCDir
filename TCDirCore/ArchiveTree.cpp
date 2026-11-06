#include "pch.h"

#include "ArchiveTree.h"

#include "WildcardMatcher.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveTree::Build
//
//  Builds the folder tree from the flat entry list.  Folders that appear
//  only as part of an entry path are created as implied folders, and get the
//  newest time of anything inside them.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveTree::Build (const vector<SArchiveEntry> & entries, unique_ptr<CArchiveTree> & tree)
{
    HRESULT       hr          = S_OK;
    FolderIndex   folderIndex;
    SNode         root;



    tree = make_unique<CArchiveTree>();

    root.m_fDirectory   = true;
    root.m_fImplied     = true;
    root.m_dwAttributes = FILE_ATTRIBUTE_DIRECTORY;
    tree->m_vNodes.push_back (move (root));

    for (const SArchiveEntry & entry : entries)
    {
        tree->AddEntry (entry, folderIndex);
    }

    tree->FillImpliedTimes();

    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveTree::AddEntry
//
//  Each duplicate file becomes a separate node; a folder that appears more
//  than once is merged into one node.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveTree::AddEntry (const SArchiveEntry & entry, FolderIndex & folderIndex)
{
    HRESULT           hr         = S_OK;
    vector<wstring>   components;
    size_t            iParent    = s_kiRoot;
    size_t            iNode      = 0;
    bool              fHasPath   = false;
    SNode             file;



    fHasPath = SplitPath (entry.m_strPath, components);
    BAIL_OUT_IF (!fHasPath, S_OK);

    for (size_t i = 0; i + 1 < components.size(); ++i)
    {
        iParent = FindOrAddFolder (iParent, components[i], folderIndex);
    }

    if (entry.m_fDirectory)
    {
        iNode = FindOrAddFolder (iParent, components.back(), folderIndex);

        m_vNodes[iNode].m_fImplied     = false;
        m_vNodes[iNode].m_ftLastWrite  = entry.m_ftLastWrite;
        m_vNodes[iNode].m_dwAttributes = entry.m_dwAttributes | FILE_ATTRIBUTE_DIRECTORY;
    }
    else
    {
        file.m_strName      = components.back();
        file.m_ullSize      = entry.m_ullSize;
        file.m_ftLastWrite  = entry.m_ftLastWrite;
        file.m_dwAttributes = entry.m_dwAttributes & ~FILE_ATTRIBUTE_DIRECTORY;
        file.m_iParent      = iParent;

        m_vNodes.push_back (move (file));
        m_vNodes[iParent].m_vChildren.push_back (m_vNodes.size() - 1);
    }



Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveTree::FindOrAddFolder
//
////////////////////////////////////////////////////////////////////////////////

size_t CArchiveTree::FindOrAddFolder (size_t iParent, const wstring & name, FolderIndex & folderIndex)
{
    FolderKey   key     (iParent, CWildcardMatcher::ToUpper (name));
    auto        it      = folderIndex.find (key);
    size_t      iFolder = 0;
    SNode       folder;



    if (it != folderIndex.end())
    {
        iFolder = it->second;
    }
    else
    {
        folder.m_strName      = name;
        folder.m_fDirectory   = true;
        folder.m_fImplied     = true;
        folder.m_dwAttributes = FILE_ATTRIBUTE_DIRECTORY;
        folder.m_iParent      = iParent;

        m_vNodes.push_back (move (folder));
        iFolder = m_vNodes.size() - 1;

        m_vNodes[iParent].m_vChildren.push_back (iFolder);
        folderIndex.emplace (move (key), iFolder);
    }

    return iFolder;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveTree::FillImpliedTimes
//
//  A child always has a higher index than its parent, so one reverse pass
//  propagates the newest time up through any depth without recursion.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveTree::FillImpliedTimes()
{
    for (size_t i = m_vNodes.size() - 1; i > s_kiRoot; --i)
    {
        SNode & parent = m_vNodes[m_vNodes[i].m_iParent];

        if (parent.m_fImplied && CompareFileTime (&m_vNodes[i].m_ftLastWrite, &parent.m_ftLastWrite) > 0)
        {
            parent.m_ftLastWrite = m_vNodes[i].m_ftLastWrite;
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveTree::FindFolder
//
//  Returns the folder at innerPath (empty for the root), or nullptr when a
//  component is missing or is a file.
//
////////////////////////////////////////////////////////////////////////////////

const CArchiveTree::SNode * CArchiveTree::FindFolder (wstring_view innerPath) const
{
    vector<wstring>   components;
    const SNode     * pFolder    = &Root();
    const SNode     * pMatch     = nullptr;



    SplitPath (innerPath, components);

    for (const wstring & component : components)
    {
        pMatch = nullptr;

        for (size_t iChild : pFolder->m_vChildren)
        {
            const SNode & child = m_vNodes[iChild];

            if (child.m_fDirectory &&
                CompareStringOrdinal (child.m_strName.c_str(), -1, component.c_str(), -1, TRUE) == CSTR_EQUAL)
            {
                pMatch = &child;
                break;
            }
        }

        pFolder = pMatch;

        if (pFolder == nullptr)
        {
            break;
        }
    }

    return pFolder;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveTree::FindFile
//
//  Returns the file at innerPath, or nullptr when a folder component is
//  missing or the last component is not a file.  When a folder has duplicate
//  files, the first one matches.
//
////////////////////////////////////////////////////////////////////////////////

const CArchiveTree::SNode * CArchiveTree::FindFile (wstring_view innerPath) const
{
    vector<wstring>   components;
    wstring           strFolder;
    const SNode     * pFolder    = nullptr;
    const SNode     * pFile      = nullptr;



    if (SplitPath (innerPath, components))
    {
        for (size_t i = 0; i + 1 < components.size(); ++i)
        {
            strFolder += components[i];
            strFolder += L'\\';
        }

        pFolder = FindFolder (strFolder);
    }

    if (pFolder != nullptr)
    {
        for (size_t iChild : pFolder->m_vChildren)
        {
            const SNode & child = m_vNodes[iChild];

            if (!child.m_fDirectory &&
                CompareStringOrdinal (child.m_strName.c_str(), -1, components.back().c_str(), -1, TRUE) == CSTR_EQUAL)
            {
                pFile = &child;
                break;
            }
        }
    }

    return pFile;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveTree::EnumerateChildren
//
//  Calls callback for each child of folder whose name matches spec, in
//  archive order.  Enumeration stops when callback returns false.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveTree::EnumerateChildren (const SNode & folder, wstring_view spec, const EnumCallback & callback) const
{
    WIN32_FIND_DATA wfd = {};



    for (size_t iChild : folder.m_vChildren)
    {
        const SNode & child = m_vNodes[iChild];

        if (!CWildcardMatcher::Matches (child.m_strName, spec))
        {
            continue;
        }

        ToFindData (child, wfd);

        if (!callback (wfd))
        {
            break;
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveTree::ToFindData
//
//  All three times are the last-write time, because archives rarely store
//  creation or access times.  Names longer than cFileName are truncated.
//
////////////////////////////////////////////////////////////////////////////////

void CArchiveTree::ToFindData (const SNode & node, WIN32_FIND_DATA & wfd)
{
    ULARGE_INTEGER uliSize = {};



    wfd = {};

    wcsncpy_s (wfd.cFileName, node.m_strName.c_str(), _TRUNCATE);

    wfd.dwFileAttributes = node.m_dwAttributes;

    if (node.m_fDirectory)
    {
        wfd.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    }
    else if (wfd.dwFileAttributes == 0)
    {
        wfd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    }

    uliSize.QuadPart  = node.m_fDirectory ? 0 : node.m_ullSize;
    wfd.nFileSizeLow  = uliSize.LowPart;
    wfd.nFileSizeHigh = uliSize.HighPart;

    wfd.ftCreationTime   = node.m_ftLastWrite;
    wfd.ftLastAccessTime = node.m_ftLastWrite;
    wfd.ftLastWriteTime  = node.m_ftLastWrite;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveTree::SplitPath
//
//  Splits on '/' and '\'.  Empty, "." and ".." components and drive prefixes
//  are dropped, so no entry can point outside the archive root.  Returns
//  false when nothing is left.
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveTree::SplitPath (wstring_view path, vector<wstring> & components)
{
    size_t    iStart    = 0;
    size_t    iEnd      = 0;
    wstring   component;



    components.clear();

    while (iStart <= path.size())
    {
        iEnd = path.find_first_of (L"/\\", iStart);

        if (iEnd == wstring_view::npos)
        {
            iEnd = path.size();
        }

        component = path.substr (iStart, iEnd - iStart);

        if (!IsSkippedComponent (component))
        {
            components.push_back (move (component));
        }

        iStart = iEnd + 1;
    }

    return !components.empty();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveTree::IsSkippedComponent
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveTree::IsSkippedComponent (const wstring & component)
{
    static constexpr WCHAR s_kchDriveSeparator = L':';



    return component.empty()      ||
           component == L"."      ||
           component == L".."     ||
           component.back() == s_kchDriveSeparator;
}





