#pragma once
////////////////////////////////////////////////////////////////////////////////
//
//  In-memory .zip builder for archive reader tests.
//
//  Local headers contain no entry data.  Central directory offsets, sizes
//  and counts are consistent with the bytes that Build returns.
//
//  Usage:
//      CZipBuilder builder;
//      builder.AddFile ("docs/readme.txt", 1024);
//      builder.AddFolder ("docs/empty/");
//
//      CMemoryByteSource source (builder.Build());
//
////////////////////////////////////////////////////////////////////////////////





////////////////////////////////////////////////////////////////////////////////
//
//  CZipBuilder
//
////////////////////////////////////////////////////////////////////////////////

class CZipBuilder
{
public:
    struct SEntryOptions
    {
        string      m_strName;                          // Raw name bytes as stored in the headers
        bool        m_fUtf8                = false;     // General purpose flag bit 11
        WORD        m_wDosDate             = 0;
        WORD        m_wDosTime             = 0;
        BYTE        m_bHost                = 0;         // High byte of "version made by"
        DWORD       m_dwExternalAttributes = 0;
        ULONGLONG   m_ullSize              = 0;         // Uncompressed size
        bool        m_fEncrypted           = false;     // General purpose flag bit 0
        bool        m_fNtfsTime            = false;     // Emit the NTFS extra field (0x000A)
        ULONGLONG   m_ullNtfsLastWrite     = 0;         // FILETIME value for the NTFS extra field
        string      m_strUnicodePath;                   // UTF-8 Info-ZIP Unicode Path (0x7075); empty for none
        bool        m_fZip64Size           = false;     // Emit 0xFFFFFFFF sizes and the ZIP64 extra field
    };

    CZipBuilder &  AddFile               (const string & name, ULONGLONG ullSize = 0);
    CZipBuilder &  AddFolder             (const string & name);
    CZipBuilder &  AddEntry              (const SEntryOptions & options);
    CZipBuilder &  SetComment            (const string & comment);
    CZipBuilder &  SetZip64EndRecord     (bool fZip64EndRecord);
    CZipBuilder &  SetReportedEntryCount (ULONGLONG cEntries);

    vector<BYTE>   Build() const;

    static WORD    DosDate               (WORD wYear, WORD wMonth, WORD wDay);
    static WORD    DosTime               (WORD wHour, WORD wMinute, WORD wSecond);

private:
    static bool    UsesZip64Size         (const SEntryOptions & options);
    static void    AppendLocalHeader     (vector<BYTE> & bytes, const SEntryOptions & options);
    static void    AppendCentralHeader   (vector<BYTE> & bytes, const SEntryOptions & options, ULONGLONG ullLocalOffset);
    static void    AppendExtraFields     (vector<BYTE> & extra, const SEntryOptions & options);
    static void    AppendZip64End        (vector<BYTE> & bytes, ULONGLONG cEntries, ULONGLONG ullSize, ULONGLONG ullOffset);
    static void    AppendEndRecord       (vector<BYTE> & bytes, ULONGLONG cEntries, ULONGLONG ullSize, ULONGLONG ullOffset, const string & comment, bool fZip64);

    static void    Append16              (vector<BYTE> & bytes, WORD w);
    static void    Append32              (vector<BYTE> & bytes, DWORD dw);
    static void    Append64              (vector<BYTE> & bytes, ULONGLONG ull);
    static void    AppendBytes           (vector<BYTE> & bytes, const string & text);
    static void    AppendBytes           (vector<BYTE> & bytes, const vector<BYTE> & data);

    static constexpr DWORD     s_kdwLocalSignature        = 0x04034B50;
    static constexpr DWORD     s_kdwCentralSignature      = 0x02014B50;
    static constexpr DWORD     s_kdwEndSignature          = 0x06054B50;
    static constexpr DWORD     s_kdwZip64EndSignature     = 0x06064B50;
    static constexpr DWORD     s_kdwZip64LocatorSignature = 0x07064B50;

    static constexpr WORD      s_kwSpecVersion            = 20;
    static constexpr WORD      s_kwSpecVersionZip64       = 45;
    static constexpr int       s_kcHostShift              = 8;
    static constexpr WORD      s_kwFlagEncrypted          = 0x0001;
    static constexpr WORD      s_kwFlagUtf8               = 0x0800;

    static constexpr WORD      s_kwExtraZip64             = 0x0001;
    static constexpr WORD      s_kwExtraNtfs              = 0x000A;
    static constexpr WORD      s_kwExtraUnicodePath       = 0x7075;
    static constexpr WORD      s_kwNtfsTimesTag           = 0x0001;
    static constexpr WORD      s_kcbNtfsTimes             = 24;
    static constexpr BYTE      s_kbUnicodePathVersion     = 1;

    static constexpr ULONGLONG s_kcbZip64EndRemainder     = 44;     // ZIP64 end record size minus its signature and size fields
    static constexpr DWORD     s_kdwMax32                 = 0xFFFFFFFF;
    static constexpr WORD      s_kwMax16                  = 0xFFFF;

    static constexpr WORD      s_kwDosYearBase            = 1980;
    static constexpr int       s_kcDosYearShift           = 9;
    static constexpr int       s_kcDosMonthShift          = 5;
    static constexpr int       s_kcDosHourShift           = 11;
    static constexpr int       s_kcDosMinuteShift         = 5;
    static constexpr WORD      s_kwDosSecondScale         = 2;

    vector<SEntryOptions>   m_vEntries;
    string                  m_strComment;
    bool                    m_fZip64EndRecord       = false;
    bool                    m_fReportedCountSet     = false;
    ULONGLONG               m_cReportedEntries      = 0;
};
