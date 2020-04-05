// TCDir.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "TCDir.h"
#include "CommandLine.h"
#include "Directory.h"
#include "Utils.h"





////////////////////////////////////////////////////////////////////////////////
//
//  wmain
//
//  
//
////////////////////////////////////////////////////////////////////////////////

int wmain (int argc, WCHAR* argv[])
{
    HRESULT      hr;      
    CCommandLine cmdline; 



    //
    // Process the commandline
    //

    hr = cmdline.Parse (argc - 1, argv + 1);
    if (FAILED (hr))
    {
        Usage();
    }
    CHR (hr);



    //
    // Use default mask if no mask is given
    //

    if (cmdline.m_listMask.empty())
    {
        cmdline.m_listMask.push_back (g_kszDefaultMask);
    }


    
    //
    // For each mask, show a directory listing
    //

    for_each (cmdline.m_listMask.begin(), cmdline.m_listMask.end(), CDirectory (&cmdline));


Error:    
    
	return 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Usage
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void Usage (void)
{
    g_util.ConsolePrintf (TEXT ("\n"));
    g_util.ConsolePrintf (TEXT ("Relmerator's Technicolor Directory (C)2004 by Robert Elmer\n"));
    g_util.ConsolePrintf (TEXT ("\n"));
    g_util.ConsolePrintf (TEXT ("TCDIR [drive:][path][filename] [/A[[:]attributes]] [/O[[:]sortorder]] [/S]\n"));
    g_util.ConsolePrintf (TEXT ("\n"));
    g_util.ConsolePrintf (TEXT ("  [drive:][path][filename]\n"));
    g_util.ConsolePrintf (TEXT ("              Specifies drive, directory, and/or files to list.\n"));
    g_util.ConsolePrintf (TEXT ("\n"));
    g_util.ConsolePrintf (TEXT ("  /A          Displays files with specified attributes.\n"));
    g_util.ConsolePrintf (TEXT ("  attributes   D  Directories                R  Read-only files\n"));
    g_util.ConsolePrintf (TEXT ("               H  Hidden files               A  Files ready for archiving\n"));
    g_util.ConsolePrintf (TEXT ("               S  System files               T  Temporary files\n"));
    g_util.ConsolePrintf (TEXT ("               E  Encrypted files            C  Compressed files\n"));
    g_util.ConsolePrintf (TEXT ("               P  Reparse points             0  Sparse files\n"));
    g_util.ConsolePrintf (TEXT ("               -  Prefix meaning not\n"));
    g_util.ConsolePrintf (TEXT ("\n"));
    g_util.ConsolePrintf (TEXT ("  /O          List by files in sorted order.\n"));
    g_util.ConsolePrintf (TEXT ("  sortorder    N  By name (alphabetic)       S  By size (smallest first)\n"));
    g_util.ConsolePrintf (TEXT ("               E  By extension (alphabetic)  D  By date/time (oldest first)\n"));
    g_util.ConsolePrintf (TEXT ("               -  Prefix to reverse order\n"));
    g_util.ConsolePrintf (TEXT ("\n"));
    g_util.ConsolePrintf (TEXT ("  /S          Displays files in specified directory and all subdirectories.\n"));
    g_util.ConsolePrintf (TEXT ("\n"));
    g_util.ConsolePrintf (TEXT ("\n"));
    g_util.ConsolePrintf (TEXT ("\n"));
}






