#include "pch.h"
#include "TuiWidgets.h"

#include "AnsiCodes.h"
#include "Config.h"
#include "UnicodeSymbols.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::CTuiWidgets
//
////////////////////////////////////////////////////////////////////////////////

CTuiWidgets::CTuiWidgets (CConsole & console)
    : m_console (console)
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::~CTuiWidgets
//
////////////////////////////////////////////////////////////////////////////////

CTuiWidgets::~CTuiWidgets (void)
{
    Cleanup();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::Init
//
//  Saves console mode, sets raw input mode, hides cursor.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CTuiWidgets::Init (void)
{
    HRESULT hr = S_OK;



    if (m_fInitialized)
    {
        goto Error;
    }

    m_hStdIn = GetStdHandle (STD_INPUT_HANDLE);
    CBRAEx (m_hStdIn != INVALID_HANDLE_VALUE, HRESULT_FROM_WIN32 (GetLastError()));

    //
    // Save original console mode
    //

    {
        BOOL fOk = GetConsoleMode (m_hStdIn, &m_dwOriginalMode);
        CBRAEx (fOk, HRESULT_FROM_WIN32 (GetLastError()));
    }

    //
    // Set raw input mode: disable line input, echo, and processed input
    //

    {
        DWORD dwNewMode = ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT;

        BOOL fOk = SetConsoleMode (m_hStdIn, dwNewMode);
        CBRAEx (fOk, HRESULT_FROM_WIN32 (GetLastError()));
    }

    //
    // Flush pending input events
    //

    FlushConsoleInputBuffer (m_hStdIn);

    HideCursor();

    m_fInitialized = true;

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::Cleanup
//
//  Restores console mode and cursor visibility.
//
////////////////////////////////////////////////////////////////////////////////

void CTuiWidgets::Cleanup (void)
{
    HideSpinner();

    if (m_fInitialized)
    {
        ShowCursor();
        SetConsoleMode (m_hStdIn, m_dwOriginalMode);
        m_fInitialized = false;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::HideCursor / ShowCursor
//
////////////////////////////////////////////////////////////////////////////////

void CTuiWidgets::HideCursor (void)
{
    if (!m_fCursorHidden)
    {
        m_console.Printf (CConfig::Information, AnsiCodes::CURSOR_HIDE);
        m_console.Flush();
        m_fCursorHidden = true;
    }
}


void CTuiWidgets::ShowCursor (void)
{
    if (m_fCursorHidden)
    {
        m_console.Printf (CConfig::Information, AnsiCodes::CURSOR_SHOW);
        m_console.Flush();
        m_fCursorHidden = false;
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::MoveCursorUp / ClearCurrentLine
//
////////////////////////////////////////////////////////////////////////////////

void CTuiWidgets::MoveCursorUp (int cLines)
{
    if (cLines > 0)
    {
        m_console.Printf (CConfig::Information, L"" CSI L"%dA", cLines);
    }
}


void CTuiWidgets::ClearCurrentLine (void)
{
    m_console.Printf (CConfig::Information, AnsiCodes::ERASE_LINE);
}


void CTuiWidgets::MoveToRenderStart (int cRenderedLines)
{
    //
    // Move cursor up by the number of lines previously rendered,
    // positioning at the start of the item area for overwrite.
    //

    MoveCursorUp (cRenderedLines);
    m_console.Printf (CConfig::Information, L"\r");
    m_console.Flush();
}





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::ReadKey
//
//  Reads a single key event from the console input buffer.
//
////////////////////////////////////////////////////////////////////////////////

HRESULT CTuiWidgets::ReadKey (INPUT_RECORD & irec)
{
    HRESULT hr = S_OK;



    for (;;)
    {
        DWORD cRead = 0;
        BOOL  fOk   = ReadConsoleInputW (m_hStdIn, &irec, 1, &cRead);

        CBRAEx (fOk, HRESULT_FROM_WIN32 (GetLastError()));

        if (irec.EventType == KEY_EVENT && irec.Event.KeyEvent.bKeyDown)
        {
            //
            // Detect Ctrl+C
            //

            if (irec.Event.KeyEvent.wVirtualKeyCode == L'C' &&
                (irec.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)))
            {
                irec.Event.KeyEvent.wVirtualKeyCode = VK_ESCAPE;
            }

            break;
        }
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::TextInput
//
//  Displays a prompt with a default value. User can type 1-cchMax alphanumeric
//  characters. Enter confirms, Escape cancels.
//
////////////////////////////////////////////////////////////////////////////////

ETuiResult CTuiWidgets::TextInput (LPCWSTR pszPrompt, const wstring & strDefault, wstring & strResult, int cchMax)
{
    HRESULT hr = S_OK;



    strResult = strDefault;
    ShowCursor();

    m_console.ColorPrintf (L"{Information}  %s [{InformationHighlight}%s{Information}]: ", pszPrompt, strDefault.c_str());
    m_console.Flush();

    wstring strInput;

    for (;;)
    {
        INPUT_RECORD irec = {};

        hr = ReadKey (irec);

        if (FAILED (hr))
        {
            break;
        }

        WORD vk = irec.Event.KeyEvent.wVirtualKeyCode;

        if (vk == VK_ESCAPE)
        {
            HideCursor();
            return ETuiResult::Cancelled;
        }

        if (vk == VK_RETURN)
        {
            if (!strInput.empty())
            {
                strResult = strInput;
            }

            m_console.Printf (CConfig::Information, L"\n");
            m_console.Flush();
            HideCursor();
            return ETuiResult::Confirmed;
        }

        if (vk == VK_BACK)
        {
            if (!strInput.empty())
            {
                strInput.pop_back();
                m_console.Printf (CConfig::Information, L"\b \b");
                m_console.Flush();
            }

            continue;
        }

        wchar_t ch = irec.Event.KeyEvent.uChar.UnicodeChar;

        if (ch != 0 && iswalnum (ch) && static_cast<int>(strInput.size()) < cchMax)
        {
            strInput += ch;
            m_console.Putchar (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, ch);
            m_console.Flush();
        }
    }

    HideCursor();
    return ETuiResult::Cancelled;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::CheckboxList
//
//  Renders a list of items with [●]/[ ] checkboxes.
//  Arrow keys navigate, Space toggles, Enter confirms, Escape cancels.
//
////////////////////////////////////////////////////////////////////////////////

ETuiResult CTuiWidgets::CheckboxList (LPCWSTR pszPrompt, vector<pair<wstring, bool>> & rgItems)
{
    HRESULT hr             = S_OK;
    int     iFocus         = 0;
    int     cItems         = static_cast<int>(rgItems.size());
    int     cRenderedLines = cItems + 1;  // items + blank line (help line has no \n, cursor stays on it)



    auto Render = [&] (bool fShowFocus)
    {
        for (int i = 0; i < cItems; ++i)
        {
            ClearCurrentLine();

            if (fShowFocus && i == iFocus)
            {
                m_console.ColorPrintf (L"  {InformationHighlight}%c{Information} ", UnicodeSymbols::FocusIndicator);
            }
            else
            {
                m_console.ColorPrintf (L"    ");
            }

            if (rgItems[i].second)
            {
                m_console.ColorPrintf (L"{Information}[{InformationHighlight}%c{Information}] ", UnicodeSymbols::CheckMark);
            }
            else
            {
                m_console.ColorPrintf (L"{Information}[ ] ");
            }

            m_console.ColorPrintf (L"{Information}%s\n", rgItems[i].first.c_str());
        }

        ClearCurrentLine();
        m_console.ColorPrintf (L"\n");
        ClearCurrentLine();

        if (fShowFocus)
        {
            m_console.ColorPrintf (L"{Information}  (Space=toggle, Enter=confirm, Esc=cancel)");
        }

        m_console.Flush();
    };

    //
    // Print prompt, then render items below it
    //

    m_console.Printf (CConfig::Information, L"  %s\n", pszPrompt);
    Render (true);

    for (;;)
    {
        INPUT_RECORD irec = {};

        hr = ReadKey (irec);

        if (FAILED (hr))
        {
            break;
        }

        WORD vk = irec.Event.KeyEvent.wVirtualKeyCode;

        if (vk == VK_ESCAPE)
        {
            MoveToRenderStart (cRenderedLines);
            Render (false);
            ClearCurrentLine();
            m_console.Printf (CConfig::Information, L"\n");
            m_console.Flush();
            return ETuiResult::Cancelled;
        }

        if (vk == VK_RETURN)
        {
            MoveToRenderStart (cRenderedLines);
            Render (false);
            ClearCurrentLine();
            m_console.Printf (CConfig::Information, L"\n");
            m_console.Flush();
            return ETuiResult::Confirmed;
        }

        if (vk == VK_UP)
        {
            iFocus = (iFocus > 0) ? iFocus - 1 : cItems - 1;
        }
        else if (vk == VK_DOWN)
        {
            iFocus = (iFocus < cItems - 1) ? iFocus + 1 : 0;
        }
        else if (vk == VK_SPACE)
        {
            rgItems[iFocus].second = !rgItems[iFocus].second;
        }
        else
        {
            continue;
        }

        MoveToRenderStart (cRenderedLines);
        Render (true);
    }

    return ETuiResult::Cancelled;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::RadioButtonList
//
//  Renders a list with (●)/( ) radio buttons.
//  Arrow keys navigate, Enter selects, Escape cancels.
//
////////////////////////////////////////////////////////////////////////////////

ETuiResult CTuiWidgets::RadioButtonList (LPCWSTR pszPrompt, const vector<wstring> & rgItems, int & iSelected)
{
    HRESULT hr     = S_OK;
    int     iFocus = iSelected;
    int     cItems = static_cast<int>(rgItems.size());



    //
    // Count total rendered lines: each item may span multiple lines (contains \n)
    //

    auto CountRenderedLines = [&] () -> int
    {
        int cLines = 0;

        for (const auto & item : rgItems)
        {
            cLines += 1;  // At least one line per item

            for (wchar_t ch : item)
            {
                if (ch == L'\n')
                {
                    ++cLines;
                }
            }
        }

        return cLines + 1;  // + blank line (help line has no \n, cursor stays on it)
    };

    int cRenderedLines = CountRenderedLines();

    auto Render = [&] (bool fShowFocus, int iSelectedDot = -1)
    {
        for (int i = 0; i < cItems; ++i)
        {
            ClearCurrentLine();

            bool fShowDot = (fShowFocus && i == iFocus) || (i == iSelectedDot);

            if (fShowFocus && i == iFocus)
            {
                m_console.ColorPrintf (L"  {InformationHighlight}%c{Information} ", UnicodeSymbols::FocusIndicator);
            }
            else
            {
                m_console.ColorPrintf (L"    ");
            }

            if (fShowDot)
            {
                m_console.ColorPrintf (L"{Information}({InformationHighlight}%c{Information}) ", UnicodeSymbols::RadioSelected);
            }
            else
            {
                m_console.ColorPrintf (L"{Information}( ) ");
            }

            //
            // Render item text — for multi-line items, clear each continuation line
            //

            const wstring & item = rgItems[i];
            size_t          pos  = 0;

            while (pos < item.size())
            {
                size_t nl = item.find (L'\n', pos);

                if (nl == wstring::npos)
                {
                    m_console.ColorPrintf (L"{Information}%s\n", item.substr (pos).c_str());
                    break;
                }

                m_console.ColorPrintf (L"{Information}%s\n", item.substr (pos, nl - pos).c_str());
                pos = nl + 1;
                ClearCurrentLine();
            }
        }

        ClearCurrentLine();
        m_console.ColorPrintf (L"\n");
        ClearCurrentLine();

        if (fShowFocus)
        {
            m_console.ColorPrintf (L"{Information}  (Enter=select, Esc=cancel)");
        }

        m_console.Flush();
    };

    //
    // Print prompt, then render items below it
    //

    m_console.ColorPrintf (L"{Information}  %s\n", pszPrompt);
    Render (true);

    for (;;)
    {
        INPUT_RECORD irec = {};

        hr = ReadKey (irec);

        if (FAILED (hr))
        {
            break;
        }

        WORD vk = irec.Event.KeyEvent.wVirtualKeyCode;

        if (vk == VK_ESCAPE)
        {
            MoveToRenderStart (cRenderedLines);
            Render (false);
            ClearCurrentLine();
            m_console.Printf (CConfig::Information, L"\n");
            m_console.Flush();
            return ETuiResult::Cancelled;
        }

        if (vk == VK_RETURN)
        {
            iSelected = iFocus;
            MoveToRenderStart (cRenderedLines);
            Render (false, iFocus);
            ClearCurrentLine();
            m_console.Printf (CConfig::Information, L"\n");
            m_console.Flush();
            return ETuiResult::Confirmed;
        }

        if (vk == VK_UP)
        {
            iFocus = (iFocus > 0) ? iFocus - 1 : cItems - 1;
        }
        else if (vk == VK_DOWN)
        {
            iFocus = (iFocus < cItems - 1) ? iFocus + 1 : 0;
        }
        else
        {
            continue;
        }

        MoveToRenderStart (cRenderedLines);
        Render (true);
    }

    return ETuiResult::Cancelled;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::Confirmation
//
//  Displays preview text and a Y/N confirmation prompt.
//
////////////////////////////////////////////////////////////////////////////////

ETuiResult CTuiWidgets::Confirmation (LPCWSTR pszPrompt, const vector<wstring> & rgPreview)
{
    HRESULT hr = S_OK;



    //
    // Display preview in default color
    //

    for (const auto & line : rgPreview)
    {
        m_console.Printf (CConfig::Default, L"  %s\n", line.c_str());
    }

    m_console.Printf (CConfig::Information, L"\n  %s ", pszPrompt);
    m_console.Printf (CConfig::InformationHighlight, L"[Y/n]");
    m_console.Printf (CConfig::Information, L": ");
    m_console.Flush();

    ShowCursor();

    for (;;)
    {
        INPUT_RECORD irec = {};

        hr = ReadKey (irec);

        if (FAILED (hr))
        {
            break;
        }

        WORD    vk = irec.Event.KeyEvent.wVirtualKeyCode;
        wchar_t ch = irec.Event.KeyEvent.uChar.UnicodeChar;

        if (vk == VK_ESCAPE || ch == L'n' || ch == L'N')
        {
            m_console.Printf (CConfig::Information, L"n\n");
            m_console.Flush();
            HideCursor();
            return ETuiResult::Cancelled;
        }

        if (vk == VK_RETURN || ch == L'y' || ch == L'Y')
        {
            m_console.Printf (CConfig::Information, L"y\n");
            m_console.Flush();
            HideCursor();
            return ETuiResult::Confirmed;
        }
    }

    HideCursor();
    return ETuiResult::Cancelled;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::ShowSpinner
//
//  Displays an animated Braille dot spinner with a message on a background
//  thread. Call HideSpinner() to stop and clear the line.
//
////////////////////////////////////////////////////////////////////////////////

void CTuiWidgets::ShowSpinner (LPCWSTR pszMessage)
{
    m_fSpinnerActive = true;

    m_spinnerThread = thread ([this, msg = wstring (pszMessage)] ()
    {
        HANDLE hOut  = GetStdHandle (STD_OUTPUT_HANDLE);
        int    frame = 0;



        while (m_fSpinnerActive)
        {
            wstring line = format (L"\r  {}{}{}  {}", 
                                   AnsiCodes::FG_CYAN_ON,
                                   UnicodeSymbols::SpinnerFrames[frame % UnicodeSymbols::SpinnerFrameCount],
                                   AnsiCodes::SGR_RESET, msg);
            DWORD   cch  = 0;



            WriteConsoleW (hOut, line.c_str(), static_cast<DWORD>(line.size()), &cch, nullptr);

            ++frame;
            Sleep (80);
        }
    });
}





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::HideSpinner
//
//  Stops the spinner thread and clears the line.
//
////////////////////////////////////////////////////////////////////////////////

void CTuiWidgets::HideSpinner (void)
{
    HRESULT hr       = S_OK;
    HANDLE  hOut     = GetStdHandle (STD_OUTPUT_HANDLE);
    LPCWSTR pszClear = AnsiCodes::ERASE_LINE;
    DWORD   cch      = 0;




    BAIL_OUT_IF (!m_fSpinnerActive, S_OK);

    m_fSpinnerActive = false;

    if (m_spinnerThread.joinable())
    {
        m_spinnerThread.join();
    }    

    //
    // Clear the spinner line
    //

    WriteConsoleW (hOut, pszClear, static_cast<DWORD>(wcslen (pszClear)), &cch, nullptr);


Error:
    return;
}
