#include "StdAfx.h"
#include "Console.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::CConsole
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CConsole::CConsole(void) : 
    m_wrapMode                  (CConsole::EWrapMode::Clip),
    m_consoleScreenBufferInfoEx (sizeof (m_consoleScreenBufferInfoEx))
{
    // Initialization of m_consoleScreenBufferInfoEx above assumes cbSize is the first member
    static_assert (offsetof (CONSOLE_SCREEN_BUFFER_INFOEX, cbSize) == 0);
}





////////////////////////////////////////////////////////////////////////////////
//
//  CConsole::~CConsole
//
//  
//
////////////////////////////////////////////////////////////////////////////////

CConsole::~CConsole (void)
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
//  CConsole::Initialize
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConsole::Initialize (void)
{
    HRESULT hr       = S_OK;
    BOOL    fSuccess = FALSE;



    m_hStdOut = GetStdHandle (STD_OUTPUT_HANDLE);

    fSuccess = GetConsoleScreenBufferInfoEx (m_hStdOut, &m_consoleScreenBufferInfoEx);
    CWRA (fSuccess);

    m_attrDefault = m_consoleScreenBufferInfoEx.wAttributes;
    m_coord = m_consoleScreenBufferInfoEx.dwCursorPosition;

Error:
    return hr;
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

void CConsole::Test (void)
{
    //TestCanWriteMoreLinesThanWindowSize();
    //TestCanScrollWindow();

    //exit (0);
}
#endif _DEBUG
