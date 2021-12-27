// TCDir.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "TCDir.h"

#include "CommandLine.h"
#include "ConsoleApi.h"
#include "ConsoleBuffer.h"
#include "DirectoryLister.h"
#include "IConsole.h"
#include "Utils.h"




/*
void TestColors ()
{
    static LPCWSTR s_kpszColors[] =
    {
        L"Black",
        L"Blue",
        L"Green",
        L"Cyan",
        L"Red",
        L"Magenta",
        L"Brown",
        L"Light grey",
        L"Dark grey",
        L"Light blue",
        L"Light green",
        L"Light cyan",
        L"Light red",
        L"Light magenta",
        L"Yellow",
        L"White",
    };

    for (WORD back = 0; back < 16; ++back)
    {
        for (WORD fore = 0; fore < 16; ++fore)
        {
            g_util.SetTextAttr ((back << 4) | fore);
            g_util.ConsolePrintf (s_kpszColors[fore]);
            g_util.ConsolePrintf (L"    ");
        }

        g_util.ConsolePrintf (L"\n");
    }
}
*/




////////////////////////////////////////////////////////////////////////////////
//
//  wmain
//
//  
//
////////////////////////////////////////////////////////////////////////////////

int wmain (int argc, WCHAR * argv[])
{
    HRESULT         hr;      
    CCommandLine   cmdline; 
    IConsole     * pConsole = NULL;

 
#ifdef _DEBUG
    //ReadConsole (GetStdHandle(STD_INPUT_HANDLE), &szBuf, 1, )

    //TestColors (); return 0;
#endif

    //
    // Initialize the console
    //

    pConsole = new CConsoleApi();
    //pConsole = new CConsoleBuffer();
    CBR(pConsole != NULL);

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

    for_each (cmdline.m_listMask.begin(), cmdline.m_listMask.end(), CDirectoryLister (&cmdline));


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
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"Relmerator's Technicolor Directory (C)2004 by Robert Elmer\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"TCDIR [drive:][path][filename] [/A[[:]attributes]] [/O[[:]sortorder]] [/S]\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"  [drive:][path][filename]\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"              Specifies drive, directory, and/or files to list.\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"  /A          Displays files with specified attributes.\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"  attributes   D  Directories                R  Read-only files\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"               H  Hidden files               A  Files ready for archiving\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"               S  System files               T  Temporary files\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"               E  Encrypted files            C  Compressed files\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"               P  Reparse points             0  Sparse files\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"               -  Prefix meaning not\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"  /O          List by files in sorted order.\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"  sortorder    N  By name (alphabetic)       S  By size (smallest first)\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"               E  By extension (alphabetic)  D  By date/time (oldest first)\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"               -  Prefix to reverse order\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"  /S          Displays files in specified directory and all subdirectories.\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"\n");
    g_util.Puts (g_util.m_rgAttributes[CUtils::EAttribute::Default], L"\n");
}






