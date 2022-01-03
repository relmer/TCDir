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

    m_attrDefault = m_consoleScreenBufferInfoEx.wAttributes;
    m_coord        = m_consoleScreenBufferInfoEx.dwCursorPosition;
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
    HRESULT hr       = S_OK;
    BOOL    fSuccess = FALSE;



    //
    // Update the console cursor position with our final internal cursor position
    //

    --m_coord.Y;

    fSuccess = SetConsoleCursorPosition (m_hStdOut, m_coord);
    CWRA (fSuccess);

Error:
    return;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::SetWrapMode
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConsole::SetWrapMode (EWrapMode mode)
{
    m_wrapMode = mode;

    return S_OK;
}





#ifdef _DEBUG
////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::Test
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CConsole::Test(void)
{
    //TestCanWriteMoreLinesThanWindowSize();
    //TestCanScrollWindow();

    //exit (0);
}

#endif _DEBUG





#ifdef _DEBUG
////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::TestCanWriteMoreLinesThanWindowSize
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CConsole::TestCanWriteMoreLinesThanWindowSize(void)
{
    int cWindowHeight = m_consoleScreenBufferInfoEx.srWindow.Bottom - m_consoleScreenBufferInfoEx.srWindow.Top;

    for (int i = 0; i < (cWindowHeight + 10); ++i)
    {
        Printf(m_attrDefault, L"TestCanWriteMoreLinesThanWindowSize:  %d\n", i);
    }
}

#endif _DEBUG





#ifdef _DEBUG
////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::TestCanScrollWindow
//
//  
//
////////////////////////////////////////////////////////////////////////////////

void CConsole::TestCanScrollWindow(void)
{
    int cBufferLines = m_consoleScreenBufferInfoEx.dwSize.Y;

    for (int i = 0; i < (cBufferLines * 2); ++i)
    {
        if (i == cBufferLines)
        {
            ScrollBuffer(cBufferLines);
        }

        Printf(m_attrDefault, L"TestCanWriteMoreLinesThanWindowSize:  %d\n", i);
    }
}

#endif _DEBUG





