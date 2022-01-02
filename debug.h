#pragma once





#define ErrorLabel          Error

#define __EHM_ASSERT        TRUE
#define __EHM_NO_ASSERT     FALSE



//
// Forward declarations
//

int DbgPrintf (LPCWSTR pszFormat, ...);





//
// Core helper macros
//


#define __CHRAExHelper(__arg_hrTest, __arg_fAssert, __arg_fReplaceHr, __arg_hrReplaceHr)    \
{                                                                                           \
    HRESULT __hr = __arg_hrTest;                                                            \
                                                                                            \
    if (FAILED (__hr))                                                                      \
    {                                                                                       \
        if (__arg_fAssert)                                                                  \
        {                                                                                   \
            assert (FALSE);                                                                 \
        }                                                                                   \
                                                                                            \
        if (__arg_fReplaceHr)                                                               \
        {                                                                                   \
            hr = __arg_hrReplaceHr;                                                         \
        }                                                                                   \
        else                                                                                \
        {                                                                                   \
            hr = __hr;                                                                      \
        }                                                                                   \
                                                                                            \
        goto ErrorLabel;                                                                    \
    }                                                                                       \
}                             

#define __CWRAExHelper(__arg_fSuccess, __arg_fAssert, __arg_fReplaceHr, __arg_hrReplaceHr)  \
{                                                                                           \
    if (!fSuccess)                                                                          \
    {                                                                                       \
        if (__arg_fAssert)                                                                  \
        {                                                                                   \
            assert (FALSE);                                                                 \
        }                                                                                   \
                                                                                            \
        if (__arg_fReplaceHr)                                                               \
        {                                                                                   \
            hr = __arg_hrReplaceHr;                                                         \
        }                                                                                   \
        else                                                                                \
        {                                                                                   \
            hr = HRESULT_FROM_WIN32 (::GetLastError());                                     \
        }                                                                                   \
                                                                                            \
        goto ErrorLabel;                                                                    \
    }                                                                                       \
}  

#define __CBRAExHelper(__arg_brTest, __arg_fAssert, __arg_hrReplaceHr)                      \
{                                                                                           \
    BOOL __br = __arg_brTest;                                                               \
                                                                                            \
    if (!__br)                                                                              \
    {                                                                                       \
        if (__arg_fAssert)                                                                  \
        {                                                                                   \
            assert (FALSE);                                                                 \
        }                                                                                   \
                                                                                            \
        hr = __arg_hrReplaceHr;                                                             \
                                                                                            \
        goto ErrorLabel;                                                                    \
    }                                                                                       \
}                                                                                   

#define __CPRAExHelper(__arg_prTest, __arg_fAssert, __arg_hrReplaceHr)                      \
{                                                                                           \
    void * __pr = __arg_prTest;                                                             \
                                                                                            \
    if (__pr == NULL)                                                                       \
    {                                                                                       \
        if (__arg_fAssert)                                                                  \
        {                                                                                   \
            assert (FALSE);                                                                 \
        }                                                                                   \
                                                                                            \
        hr = __arg_hrReplaceHr;                                                             \
                                                                                            \
        goto ErrorLabel;                                                                    \
    }                                                                                       \
}




//
// CHR variants
//

#define CHRAEx(__arg_hrTest, __arg_hrReplaceHr)                                             \
{                                                                                           \
    __CHRAExHelper (__arg_hrTest, __EHM_ASSERT, TRUE, __arg_hrReplaceHr)                    \
}

#define CHREx(__arg_hrTest, __arg_hrReplaceHr)                                              \
{                                                                                           \
    __CHRAExHelper (__arg_hrTest, __EHM_NO_ASSERT, TRUE, __arg_hrReplaceHr)                 \
}                                                                                           \

#define CHRA(__arg_hrTest)                                                                  \
{                                                                                           \
    __CHRAExHelper (__arg_hrTest, __EHM_ASSERT, FALSE, 0)                                   \
}                                                                                   

#define CHR(__arg_hrTest)                                                                   \
{                                                                                           \
    __CHRAExHelper (__arg_hrTest, __EHM_NO_ASSERT, FALSE, 0)                                \
}                                                                                           





//
// CWR variants
//

#define CWRAEx(__arg_fSuccess, __arg_hrReplaceHr)                                           \
{                                                                                           \
    __CWRAExHelper (__arg_fSuccess, __EHM_ASSERT, TRUE, __arg_hrReplaceHr)                  \
}

#define CWREx(__arg_fSuccess, __arg_hrReplaceHr)                                            \
{                                                                                           \
    __CWRAExHelper (__arg_fSuccess, __EHM_NO_ASSERT, TRUE, __arg_hrReplaceHr)               \
}                                                                                           \

#define CWRA(__arg_fSuccess)                                                                \
{                                                                                           \
    __CWRAExHelper (__arg_fSuccess, __EHM_ASSERT, FALSE, 0)                                 \
}                                                                                   

#define CWR(__arg_fSuccess)                                                                 \
{                                                                                           \
    __CWRAExHelper (__arg_fSuccess, __EHM_NO_ASSERT, FALSE, 0)                              \
}                                                                                           





//
// CBR variants
//

#define CBRAEx(__arg_brTest, __arg_hrReplaceHr)                                             \
{                                                                                           \
    __CBRAExHelper (__arg_brTest, __EHM_ASSERT, __arg_hrReplaceHr)                          \
}                                                                                   


#define CBREx(__arg_brTest, __arg_hrReplaceHr)                                              \
{                                                                                           \
    __CBRAExHelper(__arg_brTest, __EHM_NO_ASSERT, __arg_hrReplaceHr);                       \
}                                                                                           

#define CBRA(__arg_brTest)                                                                  \
{                                                                                           \
    __CBRAExHelper (__arg_brTest, __EHM_ASSERT, E_FAIL)                                     \
}                                                                                           

#define CBR(__arg_brTest)                                                                   \
{                                                                                           \
    __CBRAExHelper (__arg_brTest, __EHM_NO_ASSERT, E_FAIL)                                  \
}                                                                                   

#define BAIL_OUT_IF(__arg_boolTest, __arg_hrReplaceHr)                                      \
{                                                                                           \
    __CBRAExHelper (!(__arg_boolTest), __EHM_NO_ASSERT, __arg_hrReplaceHr)                  \
}                                                                                           





//
// CPR variants
// 

#define CPRAEx(__arg_prTest, __arg_hrReplaceHr)                                             \
{                                                                                           \
    __CPRAExHelper (__arg_prTest, TRUE, TRUE, __arg_hrReplaceHr)                            \
}                                                                                   


#define CPREx(__arg_prTest, __arg_hrReplaceHr)                                              \
{                                                                                           \
    __CPRAExHelper (__arg_prTest, FALSE, TRUE, __arg_hrReplaceHr)                           \
}                                                                                   


#define CPRA(__arg_prTest)                                                                  \
{                                                                                           \
    __CPRAExHelper (__arg_prTest, TRUE, E_OUTOFMEMORY)                                      \
}                                                                                   


#define CPR(__arg_prTest)                                                                   \
{                                                                                           \
    __CPRAExHelper (__arg_prTest, FALSE, E_OUTOFMEMORY)                                     \
}                                                                                   

