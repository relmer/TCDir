#pragma once

#include "Console.h"





////////////////////////////////////////////////////////////////////////////////
//
//  ETuiResult
//
//  Result of a TUI widget interaction.
//
////////////////////////////////////////////////////////////////////////////////

enum class ETuiResult
{
    Confirmed,   // User pressed Enter to confirm
    Cancelled    // User pressed Escape to cancel
};





////////////////////////////////////////////////////////////////////////////////
//
//  CTuiWidgets
//
//  Interactive TUI components for the alias wizard.
//  Uses raw console input (ReadConsoleInputW) for arrow keys, space, enter, esc.
//
////////////////////////////////////////////////////////////////////////////////

class CTuiWidgets
{
public:
    CTuiWidgets  (CConsole & console);
    ~CTuiWidgets (void);

    HRESULT  Init     (void);
    void     Cleanup  (void);

    //
    // Widgets
    //

    ETuiResult TextInput      (LPCWSTR pszPrompt, const wstring & strDefault, wstring & strResult, int cchMax = 4);
    ETuiResult CheckboxList   (LPCWSTR pszPrompt, vector<pair<wstring, bool>> & rgItems);
    ETuiResult RadioButtonList(LPCWSTR pszPrompt, const vector<wstring> & rgItems, int & iSelected);
    ETuiResult Confirmation   (LPCWSTR pszPrompt, const vector<wstring> & rgPreview);

private:
    HRESULT  ReadKey           (INPUT_RECORD & irec);
    void     HideCursor        (void);
    void     ShowCursor        (void);
    void     MoveCursorUp      (int cLines);
    void     ClearCurrentLine  (void);

    CConsole &  m_console;
    HANDLE      m_hStdIn          = INVALID_HANDLE_VALUE;
    DWORD       m_dwOriginalMode  = 0;
    bool        m_fInitialized    = false;
    bool        m_fCursorHidden   = false;
};
