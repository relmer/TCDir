#pragma once
////////////////////////////////////////////////////////////////////////////////
//
//  CTestConsole
//
//  A test-only subclass of CConsole that suppresses all output.
//  Overrides Flush() to silently discard the buffer instead of writing
//  to stdout, preventing spurious output during test runs.
//
////////////////////////////////////////////////////////////////////////////////

#include "../../TCDirCore/Console.h"





class CTestConsole : public CConsole
{
public:

    ~CTestConsole()
    {
        Flush();
    }




    
    HRESULT Flush (void) override
    {
        m_strBuffer.clear();
        return S_OK;
    }
};





////////////////////////////////////////////////////////////////////////////////
//
//  CCapturingConsole
//
//  A test-only subclass of CConsole that captures all output into
//  m_strCaptured instead of writing to stdout.  Each Flush() appends
//  the current buffer to m_strCaptured, then clears the buffer.
//
////////////////////////////////////////////////////////////////////////////////

class CCapturingConsole : public CConsole
{
public:

    ~CCapturingConsole()
    {
        Flush();
    }




    HRESULT Flush (void) override
    {
        m_strCaptured += m_strBuffer;
        m_strBuffer.clear();
        return S_OK;
    }



    wstring m_strCaptured;
};
