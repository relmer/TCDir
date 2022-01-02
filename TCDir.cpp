// TCDir.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "TCDir.h"

#include "CommandLine.h"
#include "Console.h"
#include "ConsoleApi.h"
#include "ConsoleBuffer.h"
#include "DirectoryLister.h"




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
    HRESULT        hr;      
    CCommandLine   cmdline; 
    CConsole     * pConsole = NULL;
    CConfig      * pConfig  = NULL;

 

    //
    // Initialize the console and configuration
    //

    pConsole = new CConsoleApi();
    //pConsole = new CConsoleBuffer();
    CBR (pConsole != NULL);

    hr = pConsole->SetWrapMode(CConsole::EWrapMode::Clip);
    CHR (hr);

    pConfig = new CConfig (pConsole->m_consoleScreenBufferInfoEx.wAttributes);
    CBR (pConfig != NULL);

#ifdef _DEBUG
    //pConsole->Test();
#endif

    //
    // Process the commandline
    //

    hr = cmdline.Parse (argc - 1, argv + 1);
    if (FAILED (hr))
    {
        Usage (pConsole, pConfig);
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

    for_each (cmdline.m_listMask.begin(), cmdline.m_listMask.end(), CDirectoryLister (&cmdline, pConsole, pConfig));


Error:      
    delete pConfig;
    delete pConsole;

	return 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Usage
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void Usage (__in CConsole * pConsole, __in CConfig * pConfig)
{
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"Relmerator's Technicolor Directory (C)2004 by Robert Elmer\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"TCDIR [drive:][path][filename] [/A[[:]attributes]] [/O[[:]sortorder]] [/S]\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"  [drive:][path][filename]\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"              Specifies drive, directory, and/or files to list.\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"  /A          Displays files with specified attributes.\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"  attributes   D  Directories                R  Read-only files\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"               H  Hidden files               A  Files ready for archiving\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"               S  System files               T  Temporary files\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"               E  Encrypted files            C  Compressed files\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"               P  Reparse points             0  Sparse files\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"               -  Prefix meaning not\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"  /O          List by files in sorted order.\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"  sortorder    N  By name (alphabetic)       S  By size (smallest first)\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"               E  By extension (alphabetic)  D  By date/time (oldest first)\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"               -  Prefix to reverse order\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"  /S          Displays files in specified directory and all subdirectories.\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"\n");
    pConsole->Puts (pConfig->m_rgAttributes[CConfig::EAttribute::Default], L"\n");
}






