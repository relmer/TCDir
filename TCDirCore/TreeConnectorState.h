#pragma once

#include "UnicodeSymbols.h"





////////////////////////////////////////////////////////////////////////////////
//
//  STreeConnectorState
//
//  Lightweight struct tracking the tree drawing state as the main thread
//  recurses through the directory tree.  Passed by reference through the
//  display call chain.
//
//  Each depth level records whether the ancestor at that level has more
//  siblings coming ("has sibling").  This determines whether a vertical
//  continuation line (│) or empty space is drawn at that column.
//
////////////////////////////////////////////////////////////////////////////////

struct STreeConnectorState
{
    //
    // Constructor
    //

    explicit STreeConnectorState (int cTreeIndent = 4) :
        m_cTreeIndent (cTreeIndent)
    {
    }



    //
    // GetPrefix
    //
    // Generates the full tree prefix string for the current entry.
    // Iterates m_vAncestorHasSibling to build continuation lines,
    // then appends the connector (├── or └──) based on fIsLastEntry.
    //
    // At depth 0 (root), returns an empty string (no connectors).
    //

    wstring GetPrefix (bool fIsLastEntry) const
    {
        wstring prefix;

        if (m_vAncestorHasSibling.empty())
        {
            return prefix;
        }

        //
        // Build continuation columns for all ancestor levels except the last
        //

        for (size_t i = 0; i < m_vAncestorHasSibling.size() - 1; ++i)
        {
            if (m_vAncestorHasSibling[i])
            {
                // Ancestor at this level has more siblings: draw │ + spaces
                prefix += UnicodeSymbols::TreeVertical;
                prefix += wstring (m_cTreeIndent - 1, L' ');
            }
            else
            {
                // Ancestor at this level was last: draw spaces only
                prefix += wstring (m_cTreeIndent, L' ');
            }
        }

        //
        // Append the connector for this entry
        //

        int cHorizontalDashes = max (0, m_cTreeIndent - 2);

        if (fIsLastEntry)
        {
            prefix += UnicodeSymbols::TreeCorner;
        }
        else
        {
            prefix += UnicodeSymbols::TreeTee;
        }

        prefix += wstring (cHorizontalDashes, UnicodeSymbols::TreeHorizontal);
        prefix += L' ';

        return prefix;
    }



    //
    // GetStreamContinuation
    //
    // Generates the prefix for a stream line: same ancestor continuation
    // columns, but replaces the entry connector with a vertical
    // continuation (│ + padding).
    //

    wstring GetStreamContinuation () const
    {
        wstring prefix;

        if (m_vAncestorHasSibling.empty())
        {
            return prefix;
        }

        //
        // Build continuation columns for all ancestor levels except the last
        //

        for (size_t i = 0; i < m_vAncestorHasSibling.size() - 1; ++i)
        {
            if (m_vAncestorHasSibling[i])
            {
                prefix += UnicodeSymbols::TreeVertical;
                prefix += wstring (m_cTreeIndent - 1, L' ');
            }
            else
            {
                prefix += wstring (m_cTreeIndent, L' ');
            }
        }

        //
        // Stream continuation: always vertical line + spaces for current level
        //

        prefix += UnicodeSymbols::TreeVertical;
        prefix += wstring (m_cTreeIndent - 1, L' ');

        return prefix;
    }



    //
    // Push — enter a new depth level (entering a subdirectory's children)
    //

    void Push (bool fHasSibling)
    {
        m_vAncestorHasSibling.push_back (fHasSibling);
    }



    //
    // Pop — leave a depth level (leaving a subdirectory's children)
    //

    void Pop ()
    {
        if (!m_vAncestorHasSibling.empty())
        {
            m_vAncestorHasSibling.pop_back();
        }
    }



    //
    // Depth — current nesting depth
    //

    size_t Depth () const
    {
        return m_vAncestorHasSibling.size();
    }



    //
    // Members
    //

    vector<bool> m_vAncestorHasSibling;     // One entry per nesting depth
    int          m_cTreeIndent = 4;         // Characters per indent level
};
