#include "pch.h"

#include "PerfTimer.h"





using namespace std;
using namespace std::chrono;





// In Automatic mode, the lifetime of the PerfTimer object implicitly controls Start/Stop.
// In Manual mode, the user calls Start/Stop manually.  The timer may be reused as needed.
// In Accumulated mode, timing data is accumulated across multiple Start/Stop calls.
PerfTimer::PerfTimer (LPCWSTR pszName, TimerMode timerMode, ReportMode reportMode, std::function<void (const wchar_t *)> printFunc) :
    m_pszName       (pszName),
    m_timerMode     (timerMode),
    m_reportMode    (reportMode),
    m_fRunning      (false),
    m_fPrinted      (false),
    m_duration      { },
    m_totalDuration { },
    m_totalCalls    { },
    m_printFunc     (printFunc)
{
    if (timerMode == Automatic)
    {
        Start ();
    }
}





PerfTimer::~PerfTimer ()
{
    if (m_fRunning)
    {
        Stop ();
    }

    if (!m_fPrinted && m_duration != m_duration.zero())
    {
        Print ();
    }
}





void PerfTimer::Start ()
{
    ASSERT (!m_fRunning);

    m_fRunning = true;
    m_fPrinted = false;
    m_start    = high_resolution_clock::now ();
}





void PerfTimer::Stop ()
{
    ASSERT (m_fRunning);

    m_fRunning = false;
    m_end      = high_resolution_clock::now ();
    m_duration = m_end - m_start;
    
    m_totalCalls++;

    if (m_timerMode == Accumulated)
    {
        m_totalDuration += m_duration;
    }
    else
    {
        Print ();
    }
}





void PerfTimer::Print ()
{
    static const LPCWSTR s_kpszReportMode[] =
    {
        L"msec",
        L"fps"
    };

    wstring msg;



    ASSERT (!m_fRunning);
    ASSERT (m_duration != m_duration.zero());



    m_fPrinted = true;



    switch (m_timerMode)
    {
        case Automatic:
            msg = format (L"{0}:  {1:.2f} {2}\n",
                          m_pszName,
                          GetReportingMetric(m_duration.count()),
                          s_kpszReportMode[m_reportMode]);
            break;

        case Manual:
            msg = format (L"{0}:  {1:.2f} {2} (call {3})\n",
                          m_pszName,
                          GetReportingMetric(m_duration.count()),
                          s_kpszReportMode[m_reportMode],
                          m_totalCalls);
            break;

        case Accumulated:
            msg = format (L"{0} average:  {1:.2f} {2} ({3} calls), this call {4:.2f} {5}\n",
                          m_pszName,
                          GetReportingMetric(m_totalDuration.count() / m_totalCalls),
                          s_kpszReportMode[m_reportMode],
                          m_totalCalls,
                          GetReportingMetric(m_duration.count()),
                          s_kpszReportMode[m_reportMode]);
            break;

        default:
            ASSERT (false);
            break;
    }

    m_printFunc (msg.c_str());
}
