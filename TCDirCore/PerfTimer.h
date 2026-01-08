#pragma once





class PerfTimer
{
public:
    enum ReportMode
    {
        Msec = 0,
        Fps  = 1,
    };

    enum TimerMode
    {
        Automatic   = 0,    // Start/Stop controlled by instance lifetime
        Manual      = 1,    // Reusable timer instance with manual Start/Stop
        Accumulated = 2     // Tracks average duration across multiple Start/Stop events
    };


public:
    PerfTimer  (LPCWSTR pszName, TimerMode timerMode, ReportMode reportMode, std::function<void (const wchar_t *)> printFunc = RELEASEMSG);
    ~PerfTimer ();

    void Start ();
    void Stop  ();
    void Print ();

    bool IsRunning () const { return m_fRunning; }


protected:
    double GetReportingMetric (double duration) const
    {
        switch (m_reportMode)
        {
            case Msec:
                return duration * 1000.0;

            case Fps:
                return 1.0 / duration;

            default:
                ASSERT (false);
                return 0.0;
        }
    }


protected:
    LPCWSTR                                        m_pszName;
    TimerMode                                      m_timerMode;
    ReportMode                                     m_reportMode;

    bool                                           m_fRunning;
    bool                                           m_fPrinted;     

    std::chrono::high_resolution_clock::time_point m_start;
    std::chrono::high_resolution_clock::time_point m_end;
    std::chrono::duration<double>                  m_duration;

    std::chrono::duration<double>                  m_totalDuration;
    int                                            m_totalCalls;

    std::function<void (const wchar_t *)>          m_printFunc;
};

