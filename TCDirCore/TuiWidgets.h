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
    Confirmed,    // User pressed Enter to confirm
    Cancelled,    // User pressed Escape to cancel
    Interrupted   // User pressed Ctrl+C / Ctrl+Break to abort
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

    ETuiResult PromptText           (LPCWSTR pszPrompt, const wstring & strDefault, wstring & strResult, int cchMax = 4);
    ETuiResult PromptCheckboxList   (LPCWSTR pszPrompt, vector<pair<wstring, bool>> & rgItems, const vector<bool> & rgLocked = {}, LPCWSTR pszLockedMessage = L"already configured", int * pcRenderedLines = nullptr, int iStatusColumn = 0, LPCWSTR pszLockedStatus = nullptr);
    ETuiResult PromptRadioButtonList(LPCWSTR pszPrompt, const vector<wstring> & rgItems, int & iSelected);
    ETuiResult PromptConfirmation   (LPCWSTR pszPrompt, const vector<wstring> & rgPreview);

    void     ShowSpinner      (LPCWSTR pszMessage);
    void     HideSpinner      (void);
    void     MoveCursorUp     (int cLines);
    void     ClearCurrentLine (void);

private:
    enum class EFocusVisibility
    {
        Shown,    // Interactive frame: focused item shows the indicator and the help line is drawn
        Hidden    // Final repaint: no focus indicator and no help line
    };

    struct SCheckboxRenderConfig
    {
        const vector<pair<wstring, bool>> & rgItems;
        const vector<bool>                & rgLocked;
        LPCWSTR                             pszLockedMessage;
        LPCWSTR                             pszLockedStatus;
        int                                 iStatusColumn;
    };

    HRESULT  ReadKey              (INPUT_RECORD & irec);
    void     HideCursor           (void);
    void     ShowCursor           (void);
    void     MoveToRenderStart    (int cRenderedLines);

    int      CountRenderedLines     (const vector<wstring> & rgLabels, const vector<bool> & rgLocked, LPCWSTR pszLockedMessage) const;
    void     RenderFocusPrefix      (EFocusVisibility focus);
    void     RenderItemLabel        (const wstring & strLabel, LPCWSTR pszStatus, int iStatusColumn);
    void     RenderCheckboxItems    (const SCheckboxRenderConfig & cfg, int iFocus, EFocusVisibility focus);
    void     RenderRadioButtonItems (const vector<wstring> & rgItems, int iFocus, EFocusVisibility focus, int iSelectedDot);

    CConsole     & m_console;
    HANDLE         m_hStdIn          = INVALID_HANDLE_VALUE;
    DWORD          m_dwOriginalMode  = 0;
    bool           m_fInitialized    = false;
    bool           m_fCursorHidden   = false;
    bool           m_fUseNerdFocusIndicator = false;
    bool           m_fInterrupted    = false;
    atomic<bool>   m_fSpinnerActive  = false;
    thread         m_spinnerThread;
};
