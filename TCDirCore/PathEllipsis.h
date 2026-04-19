#pragma once





////////////////////////////////////////////////////////////////////////////////
//
//  SEllipsizedPath
//
//  Result of EllipsizePath().  When fTruncated is true, the display is
//  rendered as: prefix + … + suffix (with … in a distinct color).
//  When false, prefix contains the full path and suffix is empty.
//
////////////////////////////////////////////////////////////////////////////////

struct SEllipsizedPath
{
    wstring prefix;       // Path text before the ellipsis (or full path if not truncated)
    wstring suffix;       // Path text after the ellipsis (empty if not truncated)
    bool    fTruncated;   // true if the path was middle-truncated
};





////////////////////////////////////////////////////////////////////////////////
//
//  EllipsizePath
//
//  Middle-truncate a path to fit within availableWidth characters.
//  Returns an SEllipsizedPath with prefix/suffix split for rendering
//  with the ellipsis in a different color.
//
//  Priority order for what to preserve:
//    1. First two dirs + …\ + leaf dir + filename
//    2. First two dirs + …\ + filename
//    3. First dir + …\ + filename
//    4. Just the leaf filename (truncated to availableWidth if needed)
//
////////////////////////////////////////////////////////////////////////////////

SEllipsizedPath EllipsizePath (const wstring & path, size_t availableWidth);
