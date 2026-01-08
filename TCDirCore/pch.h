// pch.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once



#ifdef _DEBUG
    #include <crtdbg.h>
#endif



// Windows headers
#include <windows.h>

#include <lmcons.h>
#include <strsafe.h>



// C headers
#include <assert.h>
#include <math.h>
#include <shlwapi.h>
#include <stdarg.h>
#include <stdio.h>



// C++ headers
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <format>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <queue>
#include <ranges>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>



using namespace std;



#define BOOLIFY(x)   (!!x)



#include "ehm.h"
