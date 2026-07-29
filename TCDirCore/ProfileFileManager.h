#pragma once

#include "ProfilePathResolver.h"
#include "ProfileFileAccess.h"





////////////////////////////////////////////////////////////////////////////////
//
//  SAliasBlock
//
//  A parsed alias block found in an existing profile file.
//
////////////////////////////////////////////////////////////////////////////////

struct SAliasBlock
{
    size_t          iStartLine  = 0;     // 0-based line index of opening marker
    size_t          iEndLine    = 0;     // 0-based line index of closing marker
    wstring         strRootAlias;        // Detected root alias name
    vector<wstring> rgAliasNames;        // All function names found in the block
    vector<wstring> rgFunctionLines;     // Full "function xxx { ... }" lines from the block
    wstring         strVersion;          // tcdir version that generated the block
    bool            fFound      = false; // Whether a block was found
};





////////////////////////////////////////////////////////////////////////////////
//
//  CProfileFileManager
//
//  Reads/writes profile files, finds/replaces/removes alias blocks.
//
////////////////////////////////////////////////////////////////////////////////

class CProfileFileManager
{
public:
    CProfileFileManager();
    explicit CProfileFileManager (IProfileFileAccess & fileAccess);

    HRESULT ReadProfileFile     (const wstring & strPath, vector<wstring> & rgLines, bool & fHasBom);
    HRESULT FindAliasBlock      (const vector<wstring> & rgLines, SAliasBlock & block);
    HRESULT WriteProfileFile    (const wstring & strPath, const vector<wstring> & rgLines, bool fPreserveBom);
    HRESULT CreateBackup        (const wstring & strPath);
    void    ReplaceAliasBlock   (vector<wstring> & rgLines, const SAliasBlock & block, const vector<wstring> & rgNewBlock);
    void    AppendAliasBlock    (vector<wstring> & rgLines, const vector<wstring> & rgNewBlock);
    void    RemoveAliasBlock    (vector<wstring> & rgLines, const SAliasBlock & block);

private:
    HRESULT ParseProfileBytes   (const string & strRaw, vector<wstring> & rgLines, bool & fHasBom);
    void    SplitIntoLines      (const wstring & strWide, vector<wstring> & rgLines);

    IProfileFileAccess & m_fileAccess;
};
