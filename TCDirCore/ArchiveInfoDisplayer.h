#pragma once

#include "ArchiveReport.h"
#include "Config.h"





class CConsole;





////////////////////////////////////////////////////////////////////////////////
//
//  SInfoLine
//
//  One row of an --Info block.  Rows after the first row of a group have an
//  empty label.
//
////////////////////////////////////////////////////////////////////////////////

struct SInfoLine
{
    wstring               m_strLabel;
    wstring               m_strValue;
    CConfig::EAttribute   m_attrValue = CConfig::InformationHighlight;
};





////////////////////////////////////////////////////////////////////////////////
//
//  SInfoTotals
//
//  Running totals for the summary that follows several blocks.
//
////////////////////////////////////////////////////////////////////////////////

struct SInfoTotals
{
    ULONGLONG   m_cArchives       = 0;
    ULONGLONG   m_ullSizeOnDisk   = 0;
    ULONGLONG   m_ullUncompressed = 0;
};





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoDisplayer
//
//  Prints --Info blocks.  The line builders return text, so tests check the
//  output without a console.
//
////////////////////////////////////////////////////////////////////////////////

class CArchiveInfoDisplayer
{
public:
    explicit CArchiveInfoDisplayer (CConsole & console);

    void    Display               (const SArchiveReport & report);
    void    DisplaySummary();

    static void    BuildLines     (const SArchiveReport & report, vector<SInfoLine> & lines);
    static wstring FormatSummary  (const SInfoTotals & totals);
    static wstring FormatCount    (ULONGLONG cItems, LPCWSTR pszSingular, LPCWSTR pszPlural);
    static wstring FormatBytes    (ULONGLONG cb);
    static wstring FormatRatio    (ULONGLONG ullStored, ULONGLONG ullUncompressed);
    static wstring FormatDateTime (const FILETIME & ft);

    const SInfoTotals & Totals() const { return m_totals; }

private:
    static void    AddFormatLines     (const SArchiveReport & report, vector<SInfoLine> & lines);
    static void    AddContentLines    (const SArchiveReport & report, vector<SInfoLine> & lines);
    static void    AddEncryptionLines (const SArchiveReport & report, vector<SInfoLine> & lines);
    static void    AddExtensionLines  (const SArchiveStatistics & statistics, vector<SInfoLine> & lines);
    static void    AddRows            (LPCWSTR pszLabel, const vector<wstring> & rows, vector<SInfoLine> & lines);
    static wstring FormatName         (const SArchiveDetails & details);
    static wstring FormatDateRange    (const SArchiveStatistics & statistics);
    static size_t  LabelWidth         (const vector<SInfoLine> & lines);

    static constexpr size_t    s_kcchLabelGap        = 3;
    static constexpr size_t    s_kcMaxExtensionRows  = 20;
    static constexpr ULONGLONG s_kullPercent         = 100;
    static constexpr size_t    s_kcchDateTime        = 16;     // "MM/dd/yyyy" or "hh:mm tt" plus room for long AM/PM text

    CConsole      & m_console;
    SInfoTotals     m_totals;
};
