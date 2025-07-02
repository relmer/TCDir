// pch.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once



//#pragma warning (disable: 4127)		// Conditional expression is constant
//#pragma warning (disable: 4702)		// Unreachable code (STL list is not clean)



#ifdef _DEBUG
    #include <crtdbg.h>
#endif



#include <windows.h>

#include <assert.h>
#include <math.h>
#include <shlwapi.h>
#include <stdarg.h>
#include <stdio.h>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>



#include <algorithm>
#include <format>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>



using namespace std;



#define BOOLIFY(x)   (!!x)



#include "ehm.h"

