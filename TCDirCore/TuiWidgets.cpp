#include "pch.h"
#include "TuiWidgets.h"

#include "AnsiCodes.h"
#include "Config.h"
#include "NerdFontDetector.h"
#include "UnicodeSymbols.h"





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::CTuiWidgets
//
////////////////////////////////////////////////////////////////////////////////

CTuiWidgets::CTuiWidgets (CConsole & console)
    : m_console (console)
{
    HRESULT         hr      = S_OK;
    const CConfig * pConfig = m_console.m_configPtr.get();



    CBRAEx (pConfig != nullptr, E_INVALIDARG);

    if (pConfig->m_fIcons.has_value())
    {
        m_fUseNerdFocusIndicator = pConfig->m_fIcons.value();
    }
    else
    {
        CNerdFontDetector detector;
        EDetectionResult  result = EDetectionResult::NotDetected;
        HANDLE            hOut   = GetStdHandle (STD_OUTPUT_HANDLE);



        hr = detector.Detect (hOut, *pConfig->m_pEnvironmentProvider, result);
        CHR (hr);

        m_fUseNerdFocusIndicator = (result == EDetectionResult::Detected);
    }


Error:
    return;
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
    HRESULT hr       = S_OK;
    BOOL    fSuccess = FALSE;



    CBRAEx (!m_fInitialized, E_UNEXPECTED);

    m_fInterrupted = false;

    m_hStdIn = GetStdHandle (STD_INPUT_HANDLE);
    CBRAEx (m_hStdIn != INVALID_HANDLE_VALUE, HRESULT_FROM_WIN32 (GetLastError()));

    //
    // Save original console mode
    //
    fSuccess = GetConsoleMode (m_hStdIn, &m_dwOriginalMode);
    CBRAEx (fSuccess, HRESULT_FROM_WIN32 (GetLastError()));

    //
    // Set raw input mode: disable line input, echo, and processed input
    //


    fSuccess = SetConsoleMode (m_hStdIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT);
    CBRAEx (fSuccess, HRESULT_FROM_WIN32 (GetLastError()));

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
        m_console.Printf (CConfig::Information, L"%s", format (AnsiCodes::CURSOR_UP_FORMAT, cLines).c_str());
    }
}


void CTuiWidgets::ClearCurrentLine (void)
{
    m_console.Printf (CConfig::Information, AnsiCodes::ERASE_LINE);
}




////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::MoveToRenderStart
//
//  Moves the cursor up by the number of previously-rendered lines and back to
//  column 0, positioning at the start of the item area for in-place repaint.
//
////////////////////////////////////////////////////////////////////////////////

void CTuiWidgets::MoveToRenderStart (int cRenderedLines)
{
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

            if ((irec.Event.KeyEvent.wVirtualKeyCode == L'C' && (irec.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)))
                || irec.Event.KeyEvent.wVirtualKeyCode == VK_CANCEL
                || irec.Event.KeyEvent.uChar.UnicodeChar == 0x0003)
            {
                m_fInterrupted = true;
                CHR (ERROR_CANCELLED);
            }

            break;
        }
    }

Error:
    return hr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::PromptText
//
//  Displays a prompt with a default value. User can type 1-cchMax alphanumeric
//  characters. Enter confirms, Escape cancels.
//
////////////////////////////////////////////////////////////////////////////////

ETuiResult CTuiWidgets::PromptText (LPCWSTR pszPrompt, const wstring & strDefault, wstring & strResult, int cchMax)
{
    HRESULT hr = S_OK;



    strResult = strDefault;
    ShowCursor();

    m_console.ColorPrintf (L"{Information}  %s [{InformationHighlight}%s{Information}]: ", pszPrompt, strDefault.c_str());
    m_console.Flush();

    //
    // Display guidance line below the prompt, then move cursor back up
    //

    m_console.Printf (CConfig::Information, L"\n  (Enter=confirm, Esc=cancel)");
    MoveCursorUp (1);

    int cchPromptLine = 2 + static_cast<int>(wcslen (pszPrompt)) + 2 + static_cast<int>(strDefault.size()) + 3;
    m_console.Printf (CConfig::Information, L"%s", format (AnsiCodes::CURSOR_TO_COLUMN_FORMAT, cchPromptLine + 1).c_str());
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
            m_console.Printf (CConfig::Information, L"\n%s", AnsiCodes::ERASE_ENTIRE_LINE);
            m_console.Flush();
            HideCursor();

            return ETuiResult::Cancelled;
        }

        if (vk == VK_RETURN)
        {
            if (!strInput.empty())
            {
                strResult = strInput;
            }

            m_console.Printf (CConfig::Information, L"\n%s", AnsiCodes::ERASE_ENTIRE_LINE);
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
    return m_fInterrupted ? ETuiResult::Interrupted : ETuiResult::Cancelled;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::CountRenderedLines
//
//  Returns the number of physical terminal lines the item list occupies:
//  one line per item, plus one for each embedded newline, plus an optional
//  locked-warning line per locked item, plus the trailing help/blank line.
//
////////////////////////////////////////////////////////////////////////////////

int CTuiWidgets::CountRenderedLines (const vector<wstring> & rgLabels, const vector<bool> & rgLocked, LPCWSTR pszLockedMessage) const
{
    int  cLines             = 0;
    bool fShowLockedMessage = (pszLockedMessage != nullptr && pszLockedMessage[0] != L'\0');

    for (size_t i = 0; i < rgLabels.size(); ++i)
    {
        cLines += 1;  // At least one line per item

        for (wchar_t ch : rgLabels[i])
        {
            if (ch == L'\n')
            {
                ++cLines;
            }
        }

        if (fShowLockedMessage && i < rgLocked.size() && rgLocked[i])
        {
            ++cLines;
        }
    }

    return cLines + 1;  // + blank line (help line has no \n, cursor stays on it)
}




////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::RenderFocusPrefix
//
//  Emits the 4-column left gutter for a list item: a highlighted focus
//  indicator when focused, or blank padding otherwise.
//
////////////////////////////////////////////////////////////////////////////////

void CTuiWidgets::RenderFocusPrefix (EFocusVisibility focus)
{
    if (focus == EFocusVisibility::Shown)
    {
        if (m_fUseNerdFocusIndicator)
        {
            m_console.ColorPrintf (L"  {InformationHighlight}%c{Information} ", UnicodeSymbols::FocusIndicator);
        }
        else
        {
            m_console.ColorPrintf (L"  {InformationHighlight}>{Information} ");
        }
    }
    else
    {
        m_console.ColorPrintf (L"    ");
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::RenderItemLabel
//
//  Renders an item's (possibly multi-line) label, clearing each continuation
//  line.  When pszStatus is non-null, the final line is followed by a green
//  status string at iStatusColumn (used for locked checkbox items).
//
////////////////////////////////////////////////////////////////////////////////

void CTuiWidgets::RenderItemLabel (const wstring & strLabel, LPCWSTR pszStatus, int iStatusColumn)
{
    size_t pos = 0;

    while (pos < strLabel.size())
    {
        size_t nl = strLabel.find (L'\n', pos);

        if (nl == wstring::npos)
        {
            if (pszStatus != nullptr)
            {
                m_console.ColorPrintf (L"{Information}%s", strLabel.substr (pos).c_str());
                if (iStatusColumn > 0)
                {
                    m_console.Printf (CConfig::Information, L"%s", format (AnsiCodes::CURSOR_TO_COLUMN_FORMAT, iStatusColumn).c_str());
                }
                else
                {
                    m_console.ColorPrintf (L"  ");
                }
                m_console.Printf (FOREGROUND_GREEN | FOREGROUND_INTENSITY, L"%s", pszStatus);
                m_console.ColorPrintf (L"\n");
            }
            else
            {
                m_console.ColorPrintf (L"{Information}%s\n", strLabel.substr (pos).c_str());
            }
            break;
        }

        m_console.ColorPrintf (L"{Information}%s\n", strLabel.substr (pos, nl - pos).c_str());
        pos = nl + 1;
        ClearCurrentLine();
    }

    if (!strLabel.empty() && strLabel.back() == L'\n')
    {
        m_console.ColorPrintf (L"\n");
        ClearCurrentLine();
    }
}




////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::RenderCheckboxItems
//
//  Renders the full checkbox list (focus gutter, [ ]/[x] glyph, label, and
//  optional locked-warning line per item) followed by the help line.
//
////////////////////////////////////////////////////////////////////////////////

void CTuiWidgets::RenderCheckboxItems (const SCheckboxRenderConfig & cfg, int iFocus, EFocusVisibility focus)
{
    int  cItems             = static_cast<int>(cfg.rgItems.size());
    bool fShowLockedMessage = (cfg.pszLockedMessage != nullptr && cfg.pszLockedMessage[0] != L'\0');

    for (int i = 0; i < cItems; ++i)
    {
        ClearCurrentLine();

        RenderFocusPrefix (focus == EFocusVisibility::Shown && i == iFocus ? EFocusVisibility::Shown : EFocusVisibility::Hidden);

        if (cfg.rgItems[i].second)
        {
            if (i < static_cast<int>(cfg.rgLocked.size()) && cfg.rgLocked[i])
            {
                m_console.ColorPrintf (L"{Information}[");
                m_console.Printf (FOREGROUND_GREEN | FOREGROUND_INTENSITY, L"%c", UnicodeSymbols::CheckMark);
                m_console.ColorPrintf (L"{Information}] ");
            }
            else
            {
                m_console.ColorPrintf (L"{Information}[{InformationHighlight}%c{Information}] ", UnicodeSymbols::CheckMark);
            }
        }
        else if (i < static_cast<int>(cfg.rgLocked.size()) && cfg.rgLocked[i])
        {
            m_console.ColorPrintf (L"{Information}[{Error}x{Information}] ");
        }
        else
        {
            m_console.ColorPrintf (L"{Information}[ ] ");
        }

        //
        // Render item text — for multi-line items, clear each continuation line
        //

        bool fLockedStatus = (i < static_cast<int>(cfg.rgLocked.size()) && cfg.rgLocked[i] && cfg.pszLockedStatus != nullptr && cfg.pszLockedStatus[0] != L'\0');
        RenderItemLabel (cfg.rgItems[i].first, fLockedStatus ? cfg.pszLockedStatus : nullptr, cfg.iStatusColumn);

        //
        // Locked items can optionally get a warning line below
        //

        if (fShowLockedMessage && i < static_cast<int>(cfg.rgLocked.size()) && cfg.rgLocked[i])
        {
            ClearCurrentLine();
            m_console.ColorPrintf (L"{Error}        ^ %s\n", cfg.pszLockedMessage);
        }
    }

    ClearCurrentLine();
    m_console.ColorPrintf (L"\n");
    ClearCurrentLine();

    if (focus == EFocusVisibility::Shown)
    {
        m_console.ColorPrintf (L"{Information}  (Space=toggle, Enter=confirm, Esc=cancel)");
    }

    m_console.Flush();
}




////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::RenderRadioButtonItems
//
//  Renders the full radio-button list (focus gutter, ( )/(●) glyph, label)
//  followed by the help line.  iSelectedDot forces a filled dot on the
//  confirmed selection during the final repaint.
//
////////////////////////////////////////////////////////////////////////////////

void CTuiWidgets::RenderRadioButtonItems (const vector<wstring> & rgItems, int iFocus, EFocusVisibility focus, int iSelectedDot)
{
    int cItems = static_cast<int>(rgItems.size());

    for (int i = 0; i < cItems; ++i)
    {
        ClearCurrentLine();

        bool fShowDot = (focus == EFocusVisibility::Shown && i == iFocus) || (i == iSelectedDot);

        RenderFocusPrefix (focus == EFocusVisibility::Shown && i == iFocus ? EFocusVisibility::Shown : EFocusVisibility::Hidden);

        if (fShowDot)
        {
            m_console.ColorPrintf (L"{Information}({InformationHighlight}%c{Information}) ", UnicodeSymbols::RadioSelected);
        }
        else
        {
            m_console.ColorPrintf (L"{Information}( ) ");
        }

        RenderItemLabel (rgItems[i], nullptr, 0);
    }

    ClearCurrentLine();
    m_console.ColorPrintf (L"\n");
    ClearCurrentLine();

    if (focus == EFocusVisibility::Shown)
    {
        m_console.ColorPrintf (L"{Information}  (Enter=select, Esc=cancel)");
    }

    m_console.Flush();
}




////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::PromptCheckboxList
//
//  Renders a list of items with [●]/[ ] checkboxes.
//  Arrow keys navigate, Space toggles, Enter confirms, Escape cancels.
//
////////////////////////////////////////////////////////////////////////////////

ETuiResult CTuiWidgets::PromptCheckboxList (LPCWSTR                       pszPrompt,
                                            vector<pair<wstring, bool>> & rgItems,
                                            const vector<bool>          & rgLocked,
                                            LPCWSTR                       pszLockedMessage,
                                            int                         * pcRenderedLines,
                                            int                           iStatusColumn,
                                            LPCWSTR                       pszLockedStatus)
{
    HRESULT               hr       = S_OK;
    int                   iFocus   = 0;
    int                   cItems   = static_cast<int>(rgItems.size());
    vector<wstring>       rgLabels;
    SCheckboxRenderConfig cfg      { rgItems, rgLocked, pszLockedMessage, pszLockedStatus, iStatusColumn };



    rgLabels.reserve (rgItems.size());
    for (const auto & item : rgItems)
    {
        rgLabels.push_back (item.first);
    }

    int cRenderedLines = CountRenderedLines (rgLabels, rgLocked, pszLockedMessage);

    if (pcRenderedLines != nullptr)
    {
        *pcRenderedLines = cRenderedLines;
    }

    //
    // Print prompt, then render items below it
    //

    m_console.Printf (CConfig::Information, L"  %s\n", pszPrompt);
    RenderCheckboxItems (cfg, iFocus, EFocusVisibility::Shown);

    for (;;)
    {
        INPUT_RECORD irec = {};

        hr = ReadKey (irec);

        if (FAILED (hr))
        {
            break;
        }

        WORD vk = irec.Event.KeyEvent.wVirtualKeyCode;

        switch (vk)
        {
            case VK_ESCAPE:
                MoveToRenderStart (cRenderedLines);
                RenderCheckboxItems (cfg, iFocus, EFocusVisibility::Hidden);
                ClearCurrentLine();
                m_console.Printf (CConfig::Information, L"\n");
                m_console.Flush();
                return ETuiResult::Cancelled;

            case VK_RETURN:
                MoveToRenderStart (cRenderedLines);
                RenderCheckboxItems (cfg, iFocus, EFocusVisibility::Hidden);
                ClearCurrentLine();
                m_console.Printf (CConfig::Information, L"\n");
                m_console.Flush();
                return ETuiResult::Confirmed;

            case VK_UP:
                iFocus = (iFocus > 0) ? iFocus - 1 : cItems - 1;
                break;

            case VK_DOWN:
                iFocus = (iFocus < cItems - 1) ? iFocus + 1 : 0;
                break;

            case VK_SPACE:
                if (iFocus < static_cast<int>(rgLocked.size()) && rgLocked[iFocus])
                {
                    continue;  // Locked items cannot be toggled
                }

                rgItems[iFocus].second = !rgItems[iFocus].second;
                break;

            default:
                continue;
        }

        MoveToRenderStart (cRenderedLines);
        RenderCheckboxItems (cfg, iFocus, EFocusVisibility::Shown);
    }

    return m_fInterrupted ? ETuiResult::Interrupted : ETuiResult::Cancelled;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::PromptRadioButtonList
//
//  Renders a list with (●)/( ) radio buttons.
//  Arrow keys navigate, Enter selects, Escape cancels.
//
////////////////////////////////////////////////////////////////////////////////

ETuiResult CTuiWidgets::PromptRadioButtonList (LPCWSTR pszPrompt, const vector<wstring> & rgItems, int & iSelected)
{
    HRESULT hr     = S_OK;
    int     iFocus = iSelected;
    int     cItems = static_cast<int>(rgItems.size());



    int cRenderedLines = CountRenderedLines (rgItems, {}, nullptr);

    //
    // Print prompt, then render items below it
    //

    m_console.ColorPrintf (L"{Information}  %s\n", pszPrompt);
    RenderRadioButtonItems (rgItems, iFocus, EFocusVisibility::Shown, -1);

    for (;;)
    {
        INPUT_RECORD irec = {};

        hr = ReadKey (irec);

        if (FAILED (hr))
        {
            break;
        }

        WORD vk = irec.Event.KeyEvent.wVirtualKeyCode;

        switch (vk)
        {
            case VK_ESCAPE:
                MoveToRenderStart (cRenderedLines);
                RenderRadioButtonItems (rgItems, iFocus, EFocusVisibility::Hidden, -1);
                ClearCurrentLine();
                m_console.Printf (CConfig::Information, L"\n");
                m_console.Flush();
                return ETuiResult::Cancelled;

            case VK_RETURN:
                iSelected = iFocus;
                MoveToRenderStart (cRenderedLines);
                RenderRadioButtonItems (rgItems, iFocus, EFocusVisibility::Hidden, iFocus);
                ClearCurrentLine();
                m_console.Printf (CConfig::Information, L"\n");
                m_console.Flush();
                return ETuiResult::Confirmed;

            case VK_UP:
                iFocus = (iFocus > 0) ? iFocus - 1 : cItems - 1;
                break;

            case VK_DOWN:
                iFocus = (iFocus < cItems - 1) ? iFocus + 1 : 0;
                break;

            default:
                continue;
        }

        MoveToRenderStart (cRenderedLines);
        RenderRadioButtonItems (rgItems, iFocus, EFocusVisibility::Shown, -1);
    }

    return m_fInterrupted ? ETuiResult::Interrupted : ETuiResult::Cancelled;
}





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets::PromptConfirmation
//
//  Displays preview text and a Y/N confirmation prompt.
//
////////////////////////////////////////////////////////////////////////////////

ETuiResult CTuiWidgets::PromptConfirmation (LPCWSTR pszPrompt, const vector<wstring> & rgPreview)
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
            m_console.Printf (CConfig::InformationHighlight, L"n");
            m_console.Printf (CConfig::Information, L"\n");
            m_console.Flush();
            HideCursor();
            return ETuiResult::Cancelled;
        }

        if (vk == VK_RETURN || ch == L'y' || ch == L'Y')
        {
            m_console.Printf (CConfig::InformationHighlight, L"y");
            m_console.Printf (CConfig::Information, L"\n");
            m_console.Flush();
            HideCursor();
            return ETuiResult::Confirmed;
        }
    }

    HideCursor();
    return m_fInterrupted ? ETuiResult::Interrupted : ETuiResult::Cancelled;
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
