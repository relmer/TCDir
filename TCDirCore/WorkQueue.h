#pragma once

#include "pch.h"

template<typename T>
class CWorkQueue
{
public:
    CWorkQueue () : m_fDone (false) {}
    
    ~CWorkQueue () {}
    
    void Push (T item)
    {
        unique_lock<mutex> lock (m_mutex);
        
        if (!m_fDone)
        {
            m_queue.push (move (item));
            m_cv.notify_one ();
        }
    }
    
    bool Pop (T & item)
    {
        unique_lock<mutex> lock (m_mutex);
        
        // Wait for data or done signal
        m_cv.wait (lock, [this] { return !m_queue.empty () || m_fDone; });
        
        if (m_queue.empty ())
        {
            return false;  // Done and empty
        }
        
        item = move (m_queue.front ());
        m_queue.pop ();
        
        return true;
    }
    
    void SetDone ()
    {
        lock_guard<mutex> lock (m_mutex);
        m_fDone = true;
        m_cv.notify_all ();
    }
    
private:
    queue<T>            m_queue;
    mutex               m_mutex;
    condition_variable  m_cv;
    bool                m_fDone;
};
