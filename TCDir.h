#pragma once





HANDLE hStdOut;





int  wmain         (int argc, WCHAR* argv[]);
int  ConsolePrintf (LPCWSTR pszFormat, ...);
void Usage         (void);






