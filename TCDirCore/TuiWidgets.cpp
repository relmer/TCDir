#include "pch.h"
#include "TuiWidgets.h"

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
        m_console.Puts (CConfig::Information, L"\x1b[?25l");
        m_console.Flush();
        m_fCursorHidden = true;
    }
}


void CTuiWidgets::ShowCursor (void)
{
    if (m_fCursorHidden)
    {
        m_console.Puts (CConfig::Information, L"\x1b[?25h");
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
        m_console.Printf (CConfig::Information, L"\x1b[%dA", cLines);
    }
}


void CTuiWidgets::ClearCurrentLine (void)
{
    m_console.Puts (CConfig::Information, L"\r\x1b[2K");
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

    m_console.Printf (CConfig::Information, L"  %s", pszPrompt);
    m_console.Printf (CConfig::InformationHighlight, L" [%s]", strDefault.c_str());
    m_console.Puts (CConfig::Information, L": ");
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

            m_console.Puts (CConfig::Information, L"\n");
            m_console.Flush();
            HideCursor();
            return ETuiResult::Confirmed;
        }

        if (vk == VK_BACK)
        {
            if (!strInput.empty())
            {
                strInput.pop_back();
                m_console.Puts (CConfig::Information, L"\b \b");
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
    HRESULT hr     = S_OK;
    int     iFocus = 0;
    int     cItems = static_cast<int>(rgItems.size());



    auto Render = [&] ()
    {
        m_console.Printf (CConfig::Information, L"  %s\n", pszPrompt);

        for (int i = 0; i < cItems; ++i)
        {
            LPCWSTR pszFocus = (i == iFocus) ? L"  \x1b[36m\u276F\x1b[0m " : L"    ";
            LPCWSTR pszBox   = rgItems[i].second ? L"[\x1b[36m\u25CF\x1b[0m]" : L"[ ]";

            m_console.Printf (CConfig::Information, L"%s%s %s\n", pszFocus, pszBox, rgItems[i].first.c_str());
        }

        m_console.Puts (CConfig::Information, L"\n  (Space=toggle, Enter=confirm, Esc=cancel)\n");
        m_console.Flush();
    };

    auto ClearRender = [&] ()
    {
        int cTotalLines = cItems + 2;  // prompt + items + help line

        MoveCursorUp (cTotalLines);

        for (int i = 0; i < cTotalLines; ++i)
        {
            ClearCurrentLine();
            m_console.Puts (CConfig::Information, L"\n");
        }

        MoveCursorUp (cTotalLines);
        m_console.Flush();
    };

    Render();

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
            return ETuiResult::Cancelled;
        }

        if (vk == VK_RETURN)
        {
            return ETuiResult::Confirmed;
        }

        if (vk == VK_UP && iFocus > 0)
        {
            --iFocus;
        }
        else if (vk == VK_DOWN && iFocus < cItems - 1)
        {
            ++iFocus;
        }
        else if (vk == VK_SPACE)
        {
            rgItems[iFocus].second = !rgItems[iFocus].second;
        }
        else
        {
            continue;
        }

        ClearRender();
        Render();
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



    auto Render = [&] ()
    {
        m_console.Printf (CConfig::Information, L"  %s\n", pszPrompt);

        for (int i = 0; i < cItems; ++i)
        {
            LPCWSTR pszFocus  = (i == iFocus) ? L"  \x1b[36m\u276F\x1b[0m " : L"    ";
            LPCWSTR pszRadio  = (i == iFocus) ? L"(\x1b[36m\u25CF\x1b[0m)" : L"( )";

            m_console.Printf (CConfig::Information, L"%s%s %s\n", pszFocus, pszRadio, rgItems[i].c_str());
        }

        m_console.Puts (CConfig::Information, L"\n  (Enter=select, Esc=cancel)\n");
        m_console.Flush();
    };

    auto ClearRender = [&] ()
    {
        int cTotalLines = cItems + 2;

        MoveCursorUp (cTotalLines);

        for (int i = 0; i < cTotalLines; ++i)
        {
            ClearCurrentLine();
            m_console.Puts (CConfig::Information, L"\n");
        }

        MoveCursorUp (cTotalLines);
        m_console.Flush();
    };

    Render();

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
            return ETuiResult::Cancelled;
        }

        if (vk == VK_RETURN)
        {
            iSelected = iFocus;
            return ETuiResult::Confirmed;
        }

        if (vk == VK_UP && iFocus > 0)
        {
            --iFocus;
        }
        else if (vk == VK_DOWN && iFocus < cItems - 1)
        {
            ++iFocus;
        }
        else
        {
            continue;
        }

        ClearRender();
        Render();
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
    // Display preview
    //

    for (const auto & line : rgPreview)
    {
        m_console.Printf (CConfig::Information, L"  %s\n", line.c_str());
    }

    m_console.Printf (CConfig::Information, L"\n  %s ", pszPrompt);
    m_console.Puts (CConfig::InformationHighlight, L"[Y/n]");
    m_console.Puts (CConfig::Information, L": ");
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
            m_console.Puts (CConfig::Information, L"n\n");
            m_console.Flush();
            HideCursor();
            return ETuiResult::Cancelled;
        }

        if (vk == VK_RETURN || ch == L'y' || ch == L'Y')
        {
            m_console.Puts (CConfig::Information, L"y\n");
            m_console.Flush();
            HideCursor();
            return ETuiResult::Confirmed;
        }
    }

    HideCursor();
    return ETuiResult::Cancelled;
}
