#pragma once
#include <Windows.h>



#define USE_SCREEN_BUFFER



class CUtils
{
public:


    //
    // Public members
    //




protected:

    

    //
    // ctor, dtor
    //

    CUtils  (void); 
    ~CUtils (void); 
    
    //
    // Protected members
    //


#ifdef USE_SCREEN_BUFFER
    DWORD         m_cScreenBuffer;
    CHAR_INFO   * m_prgScreenBuffer;
#endif
    DWORD         m_consoleMode;
};               





extern CUtils & g_util;






