#pragma once





////////////////////////////////////////////////////////////////////////////////
//
//  SListingTotals
//
//  Aggregates file/directory/stream counts and sizes for directory listings.
//  Used to pass totals between directory enumeration and display functions.
//
////////////////////////////////////////////////////////////////////////////////  

struct SListingTotals
{
    UINT           m_cFiles         = 0;
    UINT           m_cDirectories   = 0;
    ULARGE_INTEGER m_uliFileBytes   = {};
    UINT           m_cStreams       = 0;
    ULARGE_INTEGER m_uliStreamBytes = {};



    //
    // Accumulate totals from another SListingTotals
    //

    void Add (const SListingTotals & other)
    {
        m_cFiles                  += other.m_cFiles;
        m_cDirectories            += other.m_cDirectories;
        m_uliFileBytes.QuadPart   += other.m_uliFileBytes.QuadPart;
        m_cStreams                += other.m_cStreams;
        m_uliStreamBytes.QuadPart += other.m_uliStreamBytes.QuadPart;
    }
};
