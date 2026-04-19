#include "pch.h"

#include "PathEllipsis.h"
#include "UnicodeSymbols.h"





////////////////////////////////////////////////////////////////////////////////
//
//  SplitPathComponents
//
//  Split a path into its directory components and leaf filename.
//  E.g., "C:\Program Files\App\file.exe" → { "C:", "Program Files", "App" } + "file.exe"
//
////////////////////////////////////////////////////////////////////////////////

static void SplitPathComponents (const wstring & path, vector<wstring> & dirs, wstring & leaf)
{
    filesystem::path fsPath (path);

    leaf = fsPath.filename ().wstring ();

    filesystem::path parentPath = fsPath.parent_path ();

    dirs.clear ();

    // Walk up from the parent, collecting directory names
    // filesystem::path iterates root-name, root-dir, then each component
    for (auto && component : parentPath)
    {
        wstring part = component.wstring ();

        // Merge root-name and root-dir (e.g., "C:" + "\\" → "C:\\")
        if (!dirs.empty () && dirs.back ().ends_with (L':'))
        {
            dirs.back () += part;
        }
        else if (!part.empty ())
        {
            dirs.push_back (part);
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  TryBuildTruncated
//
//  Attempt to build a truncated path from the given prefix dirs, ellipsis,
//  and suffix dirs + leaf.  Returns true and populates result if the
//  total length fits within availableWidth.
//
////////////////////////////////////////////////////////////////////////////////

static bool TryBuildTruncated (
    const vector<wstring> & prefixDirs,
    const vector<wstring> & suffixDirs,
    const wstring         & leaf,
    size_t                  availableWidth,
    SEllipsizedPath       & result)
{
    // Build prefix: "dir1\dir2\" (skip trailing \ if dir already ends with one)
    wstring prefix;

    for (const auto & dir : prefixDirs)
    {
        prefix += dir;

        if (!prefix.ends_with (L'\\'))
        {
            prefix += L'\\';
        }
    }

    // Build suffix: "\dir\leaf" or "\leaf" (skip leading \ if prefix already provides one)
    wstring suffix;

    for (const auto & dir : suffixDirs)
    {
        suffix += L'\\';
        suffix += dir;
    }

    suffix += L'\\';
    suffix += leaf;

    // Total: prefix + … + suffix = prefix.len + 1 + suffix.len
    size_t totalLen = prefix.length () + 1 + suffix.length ();

    if (totalLen <= availableWidth)
    {
        result.prefix     = move (prefix);
        result.suffix     = move (suffix);
        result.fTruncated = true;
        return true;
    }

    return false;
}





////////////////////////////////////////////////////////////////////////////////
//
//  EllipsizePath
//
//  Middle-truncate a path to fit within availableWidth characters.
//
////////////////////////////////////////////////////////////////////////////////

SEllipsizedPath EllipsizePath (const wstring & path, size_t availableWidth)
{
    SEllipsizedPath result;



    // If the path already fits, return it unchanged
    if (path.length () <= availableWidth)
    {
        result.prefix     = path;
        result.fTruncated = false;
        return result;
    }

    // Split into directory components and leaf filename
    vector<wstring> dirs;
    wstring         leaf;

    SplitPathComponents (path, dirs, leaf);

    // Paths with fewer than 3 components can't be meaningfully truncated
    // (need at least: root + middle + leaf)
    if (dirs.size () < 2)
    {
        // Fallback: just show the leaf, with trailing ellipsis if truncated
        if (leaf.length () <= availableWidth)
        {
            result.prefix     = leaf;
            result.fTruncated = false;
        }
        else if (availableWidth >= 2)
        {
            result.prefix     = leaf.substr (0, availableWidth - 1);
            result.suffix     = L"";
            result.fTruncated = true;
        }
        else
        {
            result.prefix     = leaf.substr (0, availableWidth);
            result.fTruncated = false;
        }

        return result;
    }

    // Priority 1: First two dirs + … + leaf dir + filename
    if (dirs.size () >= 3)
    {
        vector<wstring> prefixDirs = { dirs[0], dirs[1] };
        vector<wstring> suffixDirs = { dirs.back () };

        if (TryBuildTruncated (prefixDirs, suffixDirs, leaf, availableWidth, result))
        {
            return result;
        }
    }

    // Priority 2: First two dirs + … + filename
    if (dirs.size () >= 2)
    {
        vector<wstring> prefixDirs = { dirs[0], dirs[1] };
        vector<wstring> suffixDirs;

        if (TryBuildTruncated (prefixDirs, suffixDirs, leaf, availableWidth, result))
        {
            return result;
        }
    }

    // Priority 3: First dir + … + filename
    {
        vector<wstring> prefixDirs = { dirs[0] };
        vector<wstring> suffixDirs;

        if (TryBuildTruncated (prefixDirs, suffixDirs, leaf, availableWidth, result))
        {
            return result;
        }
    }

    // Priority 4: Just the leaf filename (with trailing … if truncated)
    if (leaf.length () <= availableWidth)
    {
        result.prefix     = leaf;
        result.fTruncated = false;
    }
    else if (availableWidth >= 2)
    {
        // Truncate leaf with trailing ellipsis: "DesktopStickerEdito…"
        result.prefix     = leaf.substr (0, availableWidth - 1);
        result.suffix     = L"";
        result.fTruncated = true;
    }
    else
    {
        result.prefix     = leaf.substr (0, availableWidth);
        result.fTruncated = false;
    }

    return result;
}
