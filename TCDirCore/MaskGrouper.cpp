#include "pch.h"
#include "MaskGrouper.h"
#include "Ehm.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CMaskGrouper::IsPureMask
//
//  Returns true if the mask has no directory component (no path separator).
//  Pure masks are grouped together for the current working directory.
//
////////////////////////////////////////////////////////////////////////////////

bool CMaskGrouper::IsPureMask (const wstring & mask)
{
    bool fResult = true;



    //
    // A pure mask has no path separator (backslash or forward slash)
    //

    if (mask.find (L'\\') != wstring::npos || mask.find (L'/') != wstring::npos)
    {
        fResult = false;
    }

    //
    // Also check for drive letter prefix (e.g., "C:file.txt")
    //

    else if (mask.length() >= 2 && mask[1] == L':')
    {
        fResult = false;
    }

    return fResult;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMaskGrouper::SplitMaskIntoDirAndFileSpec
//
//  Takes a mask and splits it into a directory path and file spec.  Pure masks
//  use the provided cwd.  Directory-qualified masks are made absolute and split
//  into their components.  If the mask is a directory (ends with separator or
//  exists as a directory), "*" is used as the file spec.
//
////////////////////////////////////////////////////////////////////////////////

void CMaskGrouper::SplitMaskIntoDirAndFileSpec (
    const wstring          & mask,
    const filesystem::path & cwd,
    filesystem::path       & dirPath,
    filesystem::path       & fileSpec)
{
    if (IsPureMask (mask))
    {
        //
        // Pure mask - use current working directory
        //

        dirPath  = cwd;
        fileSpec = mask;
    }
    else
    {
        //
        // Directory-qualified mask - split into directory and file spec
        //

        filesystem::path maskPath = mask;
        
        //
        // Make it absolute so we can normalize the path
        //

        error_code ec;
        filesystem::path absolutePath = filesystem::absolute (maskPath, ec);
        
        if (ec)
        {
            //
            // Fallback: use mask as-is
            //

            absolutePath = maskPath;
        }

        //
        // Check if the mask is a directory (ends with separator or is existing dir)
        //

        bool fIsDir = false;
        
        if (!mask.empty() && (mask.back() == L'\\' || mask.back() == L'/'))
        {
            fIsDir = true;
        }
        else
        {
            fIsDir = filesystem::is_directory (absolutePath, ec);
        }

        if (fIsDir)
        {
            //
            // Directory only - use "*" as fileSpec
            //

            dirPath  = absolutePath;
            fileSpec = L"*";
        }
        else
        {
            //
            // Has file component
            //

            dirPath  = absolutePath.parent_path();
            fileSpec = absolutePath.filename();
        }
    }

    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMaskGrouper::AddMaskToGroups
//
//  Adds a directory/filespec pair to the groups collection.  If a group for
//  the directory already exists, the filespec is added to it.  Otherwise a
//  new group is created.  Directory paths are normalized with a trailing
//  backslash for case-insensitive comparison.
//
////////////////////////////////////////////////////////////////////////////////

void CMaskGrouper::AddMaskToGroups (
    const filesystem::path                      & dirPath,
    const filesystem::path                      & fileSpec,
    vector<MaskGroup>                           & groups,
    map<wstring, size_t, CaseInsensitiveLess>   & dirToIndex)
{
    //
    // Normalize the directory path for grouping (ensure trailing separator)
    //

    wstring normalizedDir = dirPath.wstring();
    
    if (!normalizedDir.empty() && normalizedDir.back() != L'\\')
    {
        normalizedDir += L'\\';
    }

    //
    // Find or create the group for this directory
    //

    auto it = dirToIndex.find (normalizedDir);
    
    if (it != dirToIndex.end())
    {
        //
        // Add to existing group
        //

        groups[it->second].second.push_back (fileSpec);
    }
    else
    {
        //
        // Create new group
        //

        size_t newIndex = groups.size();
        dirToIndex[normalizedDir] = newIndex;
        groups.emplace_back (dirPath, vector<filesystem::path> { fileSpec });
    }

    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CMaskGrouper::GroupMasksByDirectory
//
//  Takes a list of masks from the command line and groups them by target
//  directory.  Pure masks (no path component) are combined into a single
//  group for the current directory.  Masks with the same directory component
//  are also combined.
//
//  Input:  m_listMask = { L"*.cpp", L"*.h", L"foo\\*.txt", L"bar\\" }
//
//  Output: vector<pair<path, vector<wstring>>> groups
//  [
//    { "C:\\cwd\\",      { "*.cpp", "*.h" } },   // pure masks grouped for CWD
//    { "C:\\cwd\\foo\\", { "*.txt" } },          // directory-qualified, separate
//    { "C:\\cwd\\bar\\", { "*" } }               // directory only, separate
//  ]
//
////////////////////////////////////////////////////////////////////////////////

vector<MaskGroup> CMaskGrouper::GroupMasksByDirectory (const list<wstring> & masks)
{
    vector<MaskGroup>                         groups;
    map<wstring, size_t, CaseInsensitiveLess> dirToIndex;
    filesystem::path                          cwd = filesystem::current_path();



    //
    // If no masks provided, return CWD with "*"
    //

    if (masks.empty())
    {
        groups.emplace_back (cwd, vector<filesystem::path> { L"*" });
    }
    else
    {
        //
        // Process each mask
        //

        for (const wstring & mask : masks)
        {
            filesystem::path dirPath;
            filesystem::path fileSpec;

            SplitMaskIntoDirAndFileSpec (mask, cwd, dirPath, fileSpec);
            AddMaskToGroups (dirPath, fileSpec, groups, dirToIndex);
        }
    }

    return groups;
}
