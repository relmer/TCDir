#include "pch.h"

#include "ArchiveInfo.h"

#include "ArchiveFileSystem.h"
#include "ArchiveInfoBuilder.h"
#include "ArchiveInfoDisplayer.h"
#include "ArchiveInfoFinder.h"
#include "CommandLine.h"
#include "Console.h"
#include "DirectoryLister.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfo::Run
//
//  Prints a block for each archive as it is found.  Returns S_OK when every
//  target matched and every archive was read, and E_FAIL otherwise.  Each
//  failure has been printed by then.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CArchiveInfo::Run (const CCommandLine & cmdline, CConsole & console, shared_ptr<IArchiveFileAccess> accessPtr)
{
    shared_ptr<CArchiveFileSystem>   archiveFsPtr = make_shared<CArchiveFileSystem> (accessPtr);
    CArchiveInfoBuilder              builder        (*archiveFsPtr, cmdline.m_fVerify);
    CArchiveInfoDisplayer            displayer      (console);
    list<wstring>                    targets        = cmdline.m_listMask;
    bool                             fFailed        = false;



    auto report = [&] (const filesystem::path & archivePath, ULONGLONG ullSize)
    {
        SArchiveReport archiveReport;
        HRESULT        hrBuild = builder.Build (archivePath, ullSize, archiveReport);

        fFailed |= FAILED (hrBuild) || IsDamaged (archiveReport);
        displayer.Display (archiveReport);
    };

    auto error = [&] (const filesystem::path & archivePath, const wstring & strMessage)
    {
        CDirectoryLister::PrintArchiveError (console, archivePath, strMessage);
    };

    CArchiveInfoFinder finder (archiveFsPtr, cmdline.m_fRecurse, report, error);



    if (targets.empty())
    {
        targets.push_back (L"*");
    }

    //
    // A blank line separates the output from the command, as in a listing
    //

    console.Printf (CConfig::Information, L"\n");

    for (const wstring & strTarget : targets)
    {
        if (!finder.Find (strTarget))
        {
            console.ColorPrintf (L"{Error}Error:   No archives match {InformationHighlight}%s{Error}\n", strTarget.c_str());
            fFailed = true;
        }
    }

    displayer.DisplaySummary();
    console.Flush();

    return fFailed ? E_FAIL : S_OK;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfo::IsDamaged
//
////////////////////////////////////////////////////////////////////////////////

bool CArchiveInfo::IsDamaged (const SArchiveReport & report)
{
    return report.m_verify.has_value() && report.m_verify->m_cDamaged > 0;
}





