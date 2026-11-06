#pragma once





class CCommandLine;
class CConsole;
class IArchiveFileAccess;
struct SArchiveReport;





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfo
//
//  Runs --Info: finds the archives that match the targets and prints a block
//  of statistics for each.
//
////////////////////////////////////////////////////////////////////////////////

class CArchiveInfo
{
public:
    static HRESULT Run       (const CCommandLine & cmdline, CConsole & console, shared_ptr<IArchiveFileAccess> accessPtr);

private:
    static bool    IsDamaged (const SArchiveReport & report);
};
