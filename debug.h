#pragma once





#define ErrorLabel  Error





#define __CHRAExHelper(__arg_hrTest, __arg_fAssert, __arg_fReplaceHr, __arg_hrReplaceHr)    \
        {                                                                                   \
            HRESULT __hr = __arg_hrTest;                                                    \
                                                                                            \
            if (FAILED (__hr))                                                              \
            {                                                                               \
                if (__arg_fAssert)                                                          \
                {                                                                           \
                    assert (FALSE);                                                         \
                }                                                                           \
                                                                                            \
                if (__arg_fReplaceHr)                                                       \
                {                                                                           \
                    hr = __arg_hrReplaceHr;                                                 \
                }                                                                           \
                else                                                                        \
                {                                                                           \
                    hr = __hr;                                                              \
                }                                                                           \
                                                                                            \
                goto ErrorLabel;                                                            \
            }                                                                               \
        }                                                                                   \

 
#define CHRAEx(__arg_hrTest, __arg_hrReplaceHr)                                             \
        {                                                                                   \
            __CHRAExHelper (__arg_hrTest, TRUE, TRUE, __arg_hrReplaceHr)                    \
        }                                                                                   \


#define CHREx(__arg_hrTest, __arg_hrReplaceHr)                                              \
        {                                                                                   \
            __CHRAExHelper (__arg_hrTest, FALSE, TRUE, __arg_hrReplaceHr)                   \
        }                                                                                   \


#define CHRA(__arg_hrTest)                                                                  \
        {                                                                                   \
            __CHRAExHelper (__arg_hrTest, TRUE, FALSE, 0)                                   \
        }                                                                                   \


#define CHR(__arg_hrTest)                                                                   \
        {                                                                                   \
            __CHRAExHelper (__arg_hrTest, FALSE, FALSE, 0)                                  \
        }                                                                                   \





#define __CBRAExHelper(__arg_brTest, __arg_fAssert, __arg_hrReplaceHr)                      \
        {                                                                                   \
            BOOL __br = __arg_brTest;                                                       \
                                                                                            \
            if (!__br)                                                                      \
            {                                                                               \
                if (__arg_fAssert)                                                          \
                {                                                                           \
                    assert (FALSE);                                                         \
                }                                                                           \
                                                                                            \
                hr = __arg_hrReplaceHr;                                                     \
                                                                                            \
                goto ErrorLabel;                                                            \
            }                                                                               \
        }                                                                                   


#define CBRAEx(__arg_brTest, __arg_hrReplaceHr)                                             \
        {                                                                                   \
            __CBRAExHelper (__arg_brTest, TRUE, __arg_hrReplaceHr)                          \
        }                                                                                   \


#define CBREx(__arg_brTest, __arg_hrReplaceHr)                                              \
        {                                                                                   \
            __CBRAExHelper(__arg_brTest, FALSE, __arg_hrReplaceHr);                         \
        }                                                                                   \


#define CBRA(__arg_brTest)                                                                  \
        {                                                                                   \
            __CBRAExHelper (__arg_brTest, TRUE, E_FAIL)                                     \
        }                                                                                   \


#define CBR(__arg_brTest)                                                                   \
        {                                                                                   \
            __CBRAExHelper (__arg_brTest, FALSE, E_FAIL)                                    \
        }                                                                                   \






#define __CPRAExHelper(__arg_prTest, __arg_fAssert, __arg_hrReplaceHr)                      \
        {                                                                                   \
            void * __pr = __arg_prTest;                                                     \
                                                                                            \
            if (__pr == NULL)                                                               \
            {                                                                               \
                if (__arg_fAssert)                                                          \
                {                                                                           \
                    assert (FALSE);                                                         \
                }                                                                           \
                                                                                            \
                hr = __arg_hrReplaceHr;                                                     \
                                                                                            \
                goto ErrorLabel;                                                            \
            }                                                                               \
        }                                                                                   \

                
#define CPRAEx(__arg_prTest, __arg_hrReplaceHr)                                             \
        {                                                                                   \
            __CPRAExHelper (__arg_prTest, TRUE, TRUE, __arg_hrReplaceHr)                    \
        }                                                                                   \


#define CPREx(__arg_prTest, __arg_hrReplaceHr)                                              \
        {                                                                                   \
            __CPRAExHelper (__arg_prTest, FALSE, TRUE, __arg_hrReplaceHr)                   \
        }                                                                                   \


#define CPRA(__arg_prTest)                                                                  \
        {                                                                                   \
            __CPRAExHelper (__arg_prTest, TRUE, E_OUTOFMEMORY)                              \
        }                                                                                   \


#define CPR(__arg_prTest)                                                                   \
        {                                                                                   \
            __CPRAExHelper (__arg_prTest, FALSE, E_OUTOFMEMORY)                             \
        }                                                                                   \




#ifdef DEBUG
        

#else // DEBUG


#endif // DEBUG






