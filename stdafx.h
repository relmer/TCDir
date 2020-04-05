// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once





#pragma warning (disable: 4127)		// Conditional expression is constant
#pragma warning (disable: 4702)		// Unreachable code (STL list is not clean)




#include <windows.h>





#include <assert.h>
#include <math.h>
#include <shlwapi.h>
#include <stdarg.h>
#include <stdio.h>

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

#include <tchar.h>




#include <algorithm>
#include <unordered_map>
#include <list>
#include <string>
#include <vector>





using namespace std;
using namespace stdext;





typedef list<wstring>         StringList;
typedef StringList::iterator  StringListIter;

typedef list<LPCWSTR>         LPCWSTRList;
typedef LPCWSTRList::iterator LPCWSTRListIter;





//#define ARRAYSIZE(x) (sizeof(x)/sizeof (x[0]))
#define BOOLIFY(x)   (!!x)





extern const WCHAR g_kszDefaultMask[];





#include "Debug.h"
#include "Flag.h"







