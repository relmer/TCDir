#pragma once
////////////////////////////////////////////////////////////////////////////////
//
//  Mock file system for testing directory enumeration without hitting disk.
//  Uses IAT patching to intercept FindFirstFileW/FindNextFileW/FindClose.
//
//  Usage:
//      MockFileTree tree;
//      tree.AddFile (L"C:\\Test\\file1.txt", 1024);
//      tree.AddFile (L"C:\\Test\\file2.txt", 2048);
//      tree.AddFile (L"C:\\Test\\sub\\file3.txt", 512);
//
//      ScopedFileSystemMock mock (tree);
//      // Now FindFirstFileW (L"C:\\Test\\*") returns mock data
//
////////////////////////////////////////////////////////////////////////////////

#include "../IatHook/ScopedIatPatch.h"





////////////////////////////////////////////////////////////////////////////////
//
//  MockFileEntry
//
//  Represents a single file or directory in the mock tree.
//
////////////////////////////////////////////////////////////////////////////////

struct MockFileEntry
{
    wstring         m_strName;          // Filename only (no path)
    DWORD           m_dwAttributes;     // FILE_ATTRIBUTE_* flags
    ULARGE_INTEGER  m_uliSize;          // File size (0 for directories)
    FILETIME        m_ftCreation;       // Creation time
    FILETIME        m_ftLastAccess;     // Last access time
    FILETIME        m_ftLastWrite;      // Last write time

    MockFileEntry() :
        m_dwAttributes (0),
        m_uliSize {},
        m_ftCreation {},
        m_ftLastAccess {},
        m_ftLastWrite {}
    {
    }
};





////////////////////////////////////////////////////////////////////////////////
//
//  MockDirectoryContents
//
//  Contents of a single directory (list of entries).
//
////////////////////////////////////////////////////////////////////////////////

struct MockDirectoryContents
{
    vector<MockFileEntry> m_vEntries;
};





////////////////////////////////////////////////////////////////////////////////
//
//  MockFileTree
//
//  Complete mock file system tree. Maps directory paths to their contents.
//
////////////////////////////////////////////////////////////////////////////////

class MockFileTree
{
public:
    MockFileTree();

    //
    // Builder pattern - add files and directories
    //

    MockFileTree& AddFile (LPCWSTR pszPath, ULONGLONG cbSize, DWORD dwAttributes = FILE_ATTRIBUTE_ARCHIVE);
    MockFileTree& AddDirectory (LPCWSTR pszPath, DWORD dwAttributes = FILE_ATTRIBUTE_DIRECTORY);

    //
    // Query mock data
    //

    const MockDirectoryContents * GetDirectoryContents (const wstring & strDirPath) const;
    bool                          PathExists (const wstring & strPath) const;

private:
    void        EnsureParentDirectories (const wstring & strPath);
    wstring     NormalizePath (const wstring & strPath) const;
    wstring     GetParentPath (const wstring & strPath) const;
    wstring     GetFileName (const wstring & strPath) const;
    FILETIME    GetCurrentFileTime() const;

    unordered_map<wstring, MockDirectoryContents> m_mapDirectories;  // path -> contents
};





////////////////////////////////////////////////////////////////////////////////
//
//  MockFindHandle
//
//  Tracks state for an active FindFirstFile/FindNextFile enumeration.
//
////////////////////////////////////////////////////////////////////////////////

struct MockFindHandle
{
    const MockDirectoryContents* m_pContents;
    size_t                       m_idxCurrent;
    wstring                      m_strPattern;   // For wildcard matching

    MockFindHandle() : m_pContents (nullptr), m_idxCurrent (0) {}
};





////////////////////////////////////////////////////////////////////////////////
//
//  FileSystemMockState
//
//  Global state for the mock. Thread-local to support parallel tests.
//
////////////////////////////////////////////////////////////////////////////////

class FileSystemMockState
{
public:
    static FileSystemMockState & Instance();

    void                  SetMockTree (const MockFileTree * pTree);
    const MockFileTree *  GetMockTree() const;

    HANDLE               CreateHandle (unique_ptr<MockFindHandle> pHandle);
    MockFindHandle *     GetHandle (HANDLE hFind);
    void                 CloseHandle (HANDLE hFind);

private:
    FileSystemMockState();

    const MockFileTree *                         m_pTree;
    unordered_map<HANDLE, unique_ptr<MockFindHandle>>  m_mapHandles;
    ULONG_PTR                                          m_nextHandle;
    mutex                                              m_mutex;
};





////////////////////////////////////////////////////////////////////////////////
//
//  ScopedFileSystemMock
//
//  RAII class that installs IAT hooks and sets up mock state.
//
////////////////////////////////////////////////////////////////////////////////

class ScopedFileSystemMock
{
public:
    explicit ScopedFileSystemMock (const MockFileTree & tree);
    ~ScopedFileSystemMock();

    bool IsActive() const;

    //
    // Access to original functions for pass-through
    //

    static HANDLE WINAPI OriginalFindFirstFileW (LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData);
    static BOOL   WINAPI OriginalFindNextFileW (HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData);
    static BOOL   WINAPI OriginalFindClose (HANDLE hFindFile);

private:
    unique_ptr<ScopedIatPatch<decltype(&FindFirstFileW)>> m_pPatchFindFirst;
    unique_ptr<ScopedIatPatch<decltype(&FindNextFileW)>>  m_pPatchFindNext;
    unique_ptr<ScopedIatPatch<decltype(&FindClose)>>      m_pPatchFindClose;

    static ScopedFileSystemMock * s_pInstance;
};





////////////////////////////////////////////////////////////////////////////////
//
//  Mock implementations (declared here, defined in .cpp)
//
////////////////////////////////////////////////////////////////////////////////

HANDLE WINAPI Mock_FindFirstFileW (LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData);
BOOL   WINAPI Mock_FindNextFileW (HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData);
BOOL   WINAPI Mock_FindClose (HANDLE hFindFile);

