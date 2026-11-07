#pragma once

#include "ArchiveReport.h"





class CArchiveFileSystem;





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoBuilder
//
//  Builds the --Info report for one archive.
//
////////////////////////////////////////////////////////////////////////////////

class CArchiveInfoBuilder
{
public:
    CArchiveInfoBuilder (CArchiveFileSystem & archiveFs, bool fVerify);

    HRESULT Build (const filesystem::path & archivePath, ULONGLONG ullSizeOnDisk, SArchiveReport & report);

private:
    void    AddVerification (SArchiveReport & report);

    CArchiveFileSystem   & m_archiveFs;
    bool                   m_fVerify;
};
