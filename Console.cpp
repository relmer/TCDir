#include "StdAfx.h"
#include "Console.h"

#include "Config.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::CConsole
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CConsole::CConsole(void)
{
    m_hStdOut = GetStdHandle (STD_OUTPUT_HANDLE);   

    ZeroMemory (&m_consoleScreenBufferInfoEx, sizeof (m_consoleScreenBufferInfoEx));
    m_consoleScreenBufferInfoEx.cbSize = sizeof (m_consoleScreenBufferInfoEx);

    GetConsoleScreenBufferInfoEx (m_hStdOut, &m_consoleScreenBufferInfoEx);

    m_coord = m_consoleScreenBufferInfoEx.dwCursorPosition;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::~CConsole
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CConsole::~CConsole(void)
{
    //
    // Update the console cursor position with our final internal cursor position
    //

    SetConsoleCursorPosition (m_hStdOut, m_coord);
}





