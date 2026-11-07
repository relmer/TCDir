#pragma once

#include "FileEnumerator.h"





class CArchiveFileSystem;





////////////////////////////////////////////////////////////////////////////////
//
//  CArchiveInfoFinder
//
//  Finds the archives that one --Info target matches.  Archives are passed
//  to the report callback in output order, as they are found.
//
////////////////////////////////////////////////////////////////////////////////

class CArchiveInfoFinder
{
public:
    using ReportCallback = function<void (const filesystem::path & archivePath, ULONGLONG ullSize)>;
    using ErrorCallback  = function<void (const filesystem::path & archivePath, const wstring & strMessage)>;

    CArchiveInfoFinder (shared_ptr<CArchiveFileSystem> archiveFsPtr,
                        bool                           fRecurse,
                        ReportCallback                 reportCallback,
                        ErrorCallback                  errorCallback);

    bool    Find                 (const wstring & strTarget);

private:
    void    FindExactTarget      (const filesystem::path & targetPath);
    void    SearchFolder         (const filesystem::path & dirPath, const wstring & strSpec);
    void    SearchFolderMatches  (const filesystem::path & dirPath, const wstring & strSpec);
    HRESULT SearchFolderTree     (const filesystem::path & dirPath, const wstring & strSpec, SEnumerationError & error);
    void    SearchArchive        (const filesystem::path & archivePath, const wstring & strSpec, bool fReported);
    HRESULT ListFolder           (const filesystem::path    & dirPath,
                                  const wstring             & strSpec,
                                  vector<WIN32_FIND_DATA>   & entries,
                                  SEnumerationError         & error);
    void    Report               (const filesystem::path & archivePath, const WIN32_FIND_DATA & wfd);

    static bool      HasWildcards (const wstring & strName);
    static bool      IsFolder     (const WIN32_FIND_DATA & wfd);
    static ULONGLONG SizeOf       (const WIN32_FIND_DATA & wfd);

    static constexpr LPCWSTR s_kpszAll = L"*";

    shared_ptr<CArchiveFileSystem>   m_archiveFsPtr;
    CFileEnumerator                  m_enumerator;
    bool                             m_fRecurse;
    ReportCallback                   m_reportCallback;
    ErrorCallback                    m_errorCallback;
    ULONGLONG                        m_cMatches       = 0;
};
