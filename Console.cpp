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
    m_wrapMode     (CConsole::EWrapMode::Clip),
    m_cLinesToSkip (0)
{
    m_hStdOut = GetStdHandle (STD_OUTPUT_HANDLE);   

    ZeroMemory (&m_consoleScreenBufferInfoEx, sizeof (m_consoleScreenBufferInfoEx));
    m_consoleScreenBufferInfoEx.cbSize = sizeof (m_consoleScreenBufferInfoEx);

    GetConsoleScreenBufferInfoEx (m_hStdOut, &m_consoleScreenBufferInfoEx);

    m_attrDefault = m_consoleScreenBufferInfoEx.wAttributes;
    m_coord       = m_consoleScreenBufferInfoEx.dwCursorPosition;
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





////////////////////////////////////////////////////////////////////////////////
//
//  CConsoleApi::ReserveLines
//
//  
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CConsole::ReserveLines (int cLinesNeeded)
{
    HRESULT hr                      = S_OK;
    BOOL    fSuccess                = FALSE;
    int     cBufferLinesRemaining   = 0;


    SHORT cBufferLinesTotal           = m_consoleScreenBufferInfoEx.dwSize.Y;
    SHORT cBufferLinesUsed            = m_consoleScreenBufferInfoEx.dwCursorPosition.Y;
    SHORT cBufferLinesFree            = cBufferLinesTotal - cBufferLinesUsed;
    SHORT cBufferLinesWithReservation = cBufferLinesUsed + cLinesNeeded;


    //
    // If there are enough lines left in the current buffer, no need to scroll
    //

    BAIL_OUT_IF (cBufferLinesWithReservation <= cBufferLinesTotal, S_OK);

    //
    // If we need more lines than the total buffer size, scroll the entire buffer
    // and track the number of lines we'll just ignore as the caller writes.
    //

    if (cLinesNeeded > cBufferLinesTotal)
    {
        //
        // Adjust the request to just be for the full size of the buffer
        // and skip any lines prior to that
        //

        m_cLinesToSkip              = cLinesNeeded - cBufferLinesTotal;
        m_coord.Y                   = 0;
        cLinesNeeded                = cBufferLinesTotal;
        cBufferLinesUsed            = 0;
        cBufferLinesFree            = cBufferLinesTotal;
        cBufferLinesWithReservation = cLinesNeeded;

        DbgPrintf (L"Asked to reserve %d lines, but only %d lines exist in buffer.  Will skip next %d lines of output.\n",
            cLinesNeeded,
            m_consoleScreenBufferInfoEx.dwSize.Y,
            m_cLinesToSkip);
    }

    if (cBufferLinesWithReservation > cBufferLinesTotal)
    {
        int cExcessBufferLines = cBufferLinesWithReservation - cBufferLinesTotal;

        ScrollBuffer (cExcessBufferLines);

        cBufferLinesWithReservation = cBufferLinesTotal;
    }

    //
    // If we're going to write past the bottom of the window, reposition the window 
    // to the bottom of the area we need
    //

    if (cBufferLinesWithReservation > m_consoleScreenBufferInfoEx.srWindow.Bottom)
    {
        SHORT      windowHeight = m_consoleScreenBufferInfoEx.srWindow.Bottom - m_consoleScreenBufferInfoEx.srWindow.Top;
        SMALL_RECT srWindow     = m_consoleScreenBufferInfoEx.srWindow;

        // absolute coords
        srWindow.Bottom = (SHORT) m_coord.Y + (SHORT) cLinesNeeded;
        srWindow.Top    = srWindow.Bottom - windowHeight;

        fSuccess = SetConsoleWindowInfo (m_hStdOut, TRUE, &srWindow);
        CWRA (fSuccess);

        m_consoleScreenBufferInfoEx.srWindow = srWindow;
    }
 


Error:
    return hr;
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





