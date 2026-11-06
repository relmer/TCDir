#include "pch.h"

#include "ArchiveInfo.h"

#include "ArchiveFileSystem.h"
#include "ArchiveInfoBuilder.h"
#include "ArchiveInfoDisplayer.h"
#include "CommandLine.h"
#include "Console.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfo::Run
//
//  Returns S_OK when every target matched and every archive was read, and
//  E_FAIL otherwise.  Each failure has been printed by then.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveInfo::Run (const CCommandLine & cmdline, CConsole & console, shared_ptr<IArchiveFileAccess> accessPtr)
{
    HRESULT                     hr        = S_OK;
    CArchiveFileSystem          archiveFs (accessPtr);
    CArchiveInfoBuilder         builder   (archiveFs, cmdline.m_fVerify);
    CArchiveInfoDisplayer       displayer (console);
    WIN32_FILE_ATTRIBUTE_DATA   data      = {};
    ULARGE_INTEGER              uliSize   = {};
    SArchiveReport              report;
    bool                        fFailed   = false;



    for (const wstring & strTarget : cmdline.m_listMask)
    {
        filesystem::path path = filesystem::absolute (strTarget);

        uliSize = {};

        if (GetFileAttributesExW (path.c_str(), GetFileExInfoStandard, &data))
        {
            uliSize.LowPart  = data.nFileSizeLow;
            uliSize.HighPart = data.nFileSizeHigh;
        }

        hr       = builder.Build (path, uliSize.QuadPart, report);
        fFailed |= FAILED (hr);

        displayer.Display (report);
    }

    displayer.DisplaySummary();
    console.Flush();

    return fFailed ? E_FAIL : S_OK;
}
