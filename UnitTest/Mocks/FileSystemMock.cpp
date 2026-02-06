#include "pch.h"
#include "FileSystemMock.h"





////////////////////////////////////////////////////////////////////////////////
//
//  MockFileTree::MockFileTree
//
////////////////////////////////////////////////////////////////////////////////

MockFileTree::MockFileTree()
{
}





////////////////////////////////////////////////////////////////////////////////
//
//  MockFileTree::AddFile
//
//  Adds a file to the mock tree. Parent directories are created automatically.
//
////////////////////////////////////////////////////////////////////////////////

MockFileTree& MockFileTree::AddFile (LPCWSTR pszPath, ULONGLONG cbSize, DWORD dwAttributes)
{
    wstring strOriginalPath = pszPath;
    wstring strPath         = NormalizePath (pszPath);
    wstring strParent       = GetParentPath (strPath);
    
    // Preserve original case for filename (Windows FindFirstFile returns actual case)
    wstring strName = GetFileName (strOriginalPath);

    EnsureParentDirectories (strPath);

    MockFileEntry entry;
    entry.m_strName          = strName;
    entry.m_dwAttributes     = dwAttributes;
    entry.m_uliSize.QuadPart = cbSize;
    entry.m_ftCreation       = GetCurrentFileTime();
    entry.m_ftLastAccess     = entry.m_ftCreation;
    entry.m_ftLastWrite      = entry.m_ftCreation;

    m_mapDirectories[strParent].m_vEntries.push_back (entry);

    return *this;
}





////////////////////////////////////////////////////////////////////////////////
//
//  MockFileTree::AddDirectory
//
//  Adds a directory to the mock tree.
//
////////////////////////////////////////////////////////////////////////////////

MockFileTree& MockFileTree::AddDirectory (LPCWSTR pszPath, DWORD dwAttributes)
{
    wstring strOriginalPath = pszPath;
    wstring strPath         = NormalizePath (pszPath);
    wstring strParent       = GetParentPath (strPath);
    
    // Preserve original case for directory name
    wstring strName = GetFileName (strOriginalPath);

    EnsureParentDirectories (strPath);

    //
    // Add directory entry to parent
    //

    MockFileEntry entry;
    entry.m_strName          = strName;
    entry.m_dwAttributes     = dwAttributes | FILE_ATTRIBUTE_DIRECTORY;
    entry.m_uliSize.QuadPart = 0;
    entry.m_ftCreation       = GetCurrentFileTime();
    entry.m_ftLastAccess     = entry.m_ftCreation;
    entry.m_ftLastWrite      = entry.m_ftCreation;

    m_mapDirectories[strParent].m_vEntries.push_back (entry);

    //
    // Ensure the directory itself exists in the map (even if empty)
    //

    if (m_mapDirectories.find (strPath) == m_mapDirectories.end())
    {
        m_mapDirectories[strPath] = MockDirectoryContents {};
    }

    return *this;
}





////////////////////////////////////////////////////////////////////////////////
//
//  MockFileTree::GetDirectoryContents
//
////////////////////////////////////////////////////////////////////////////////

const MockDirectoryContents * MockFileTree::GetDirectoryContents (const wstring & strDirPath) const
{
    wstring strNormalized = NormalizePath (strDirPath);
    auto    it = m_mapDirectories.find (strNormalized);

    return (it != m_mapDirectories.end()) ? &it->second : nullptr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  MockFileTree::PathExists
//
////////////////////////////////////////////////////////////////////////////////

bool MockFileTree::PathExists (const wstring & strPath) const
{
    wstring strNormalized = NormalizePath (strPath);

    //
    // Check if it's a directory
    //

    if (m_mapDirectories.find (strNormalized) != m_mapDirectories.end())
    {
        return true;
    }

    //
    // Check if it's a file in some directory
    //

    wstring strParent = GetParentPath (strNormalized);
    wstring strName   = GetFileName (strNormalized);

    auto it = m_mapDirectories.find (strParent);
    if (it != m_mapDirectories.end())
    {
        for (const auto & entry : it->second.m_vEntries)
        {
            if (_wcsicmp (entry.m_strName.c_str(), strName.c_str()) == 0)
            {
                return true;
            }
        }
    }

    return false;
}





////////////////////////////////////////////////////////////////////////////////
//
//  MockFileTree::EnsureParentDirectories
//
//  Creates all parent directories for a path if they don't exist.
//
////////////////////////////////////////////////////////////////////////////////

void MockFileTree::EnsureParentDirectories (const wstring & strPath)
{
    wstring strParent = GetParentPath (strPath);

    if (strParent.empty() || strParent == strPath)
    {
        return;
    }

    //
    // Recursively ensure parent's parents exist
    //

    wstring strGrandparent = GetParentPath (strParent);
    if (!strGrandparent.empty() && strGrandparent != strParent)
    {
        EnsureParentDirectories (strParent);
    }

    //
    // Create this directory if it doesn't exist
    //

    if (m_mapDirectories.find (strParent) == m_mapDirectories.end())
    {
        m_mapDirectories[strParent] = MockDirectoryContents {};

        //
        // Add directory entry to grandparent
        //

        wstring strDirName = GetFileName (strParent);
        if (!strGrandparent.empty() && !strDirName.empty())
        {
            MockFileEntry entry;
            entry.m_strName      = strDirName;
            entry.m_dwAttributes = FILE_ATTRIBUTE_DIRECTORY;
            entry.m_ftCreation   = GetCurrentFileTime();
            entry.m_ftLastAccess = entry.m_ftCreation;
            entry.m_ftLastWrite  = entry.m_ftCreation;

            m_mapDirectories[strGrandparent].m_vEntries.push_back (entry);
        }
    }
}





////////////////////////////////////////////////////////////////////////////////
//
//  MockFileTree::NormalizePath
//
//  Normalizes a path: uppercase, backslashes, no trailing backslash.
//
////////////////////////////////////////////////////////////////////////////////

wstring MockFileTree::NormalizePath (const wstring & strPath) const
{
    wstring strResult = strPath;

    //
    // Convert to uppercase for case-insensitive matching
    //

    transform (strResult.begin(), strResult.end(), strResult.begin(), towupper);

    //
    // Normalize slashes
    //

    replace (strResult.begin(), strResult.end(), L'/', L'\\');

    //
    // Remove trailing backslash (except for root like "C:\")
    //

    while (strResult.length() > 3 && strResult.back() == L'\\')
    {
        strResult.pop_back();
    }

    return strResult;
}





////////////////////////////////////////////////////////////////////////////////
//
//  MockFileTree::GetParentPath
//
////////////////////////////////////////////////////////////////////////////////

wstring MockFileTree::GetParentPath (const wstring & strPath) const
{
    size_t pos = strPath.rfind (L'\\');

    if (pos == wstring::npos || pos == 0)
    {
        return L"";
    }

    //
    // Handle root directory (e.g., "C:\")
    //

    if (pos == 2 && strPath.length() > 1 && strPath[1] == L':')
    {
        return strPath.substr (0, 3);
    }

    return strPath.substr (0, pos);
}





////////////////////////////////////////////////////////////////////////////////
//
//  MockFileTree::GetFileName
//
////////////////////////////////////////////////////////////////////////////////

wstring MockFileTree::GetFileName (const wstring & strPath) const
{
    size_t pos = strPath.rfind (L'\\');

    if (pos == wstring::npos)
    {
        return strPath;
    }

    return strPath.substr (pos + 1);
}





////////////////////////////////////////////////////////////////////////////////
//
//  MockFileTree::GetCurrentFileTime
//
////////////////////////////////////////////////////////////////////////////////

FILETIME MockFileTree::GetCurrentFileTime() const
{
    FILETIME   ft = {};
    SYSTEMTIME st = { 2026, 2, 0, 4, 12, 0, 0, 0 };  // Feb 4, 2026, 12:00:00

    SystemTimeToFileTime (&st, &ft);

    return ft;
}





////////////////////////////////////////////////////////////////////////////////
//
//  FileSystemMockState
//
////////////////////////////////////////////////////////////////////////////////

// Global mock tree pointer - shared across all threads
static atomic<const MockFileTree *> s_pGlobalMockTree = nullptr;

FileSystemMockState & FileSystemMockState::Instance()
{
    // Each thread has its own handle tracking, but shares the mock tree
    static thread_local FileSystemMockState s_instance;
    return s_instance;
}

FileSystemMockState::FileSystemMockState() :
    m_pTree (nullptr),
    m_nextHandle (0x10000000)
{
}

void FileSystemMockState::SetMockTree (const MockFileTree * pTree)
{
    // Store in global atomic for cross-thread access
    s_pGlobalMockTree.store (pTree, memory_order_release);
    m_pTree = pTree;
}

const MockFileTree * FileSystemMockState::GetMockTree() const
{
    // Read from global atomic - works across threads
    return s_pGlobalMockTree.load (memory_order_acquire);
}

HANDLE FileSystemMockState::CreateHandle (unique_ptr<MockFindHandle> pHandle)
{
    lock_guard<mutex> lock (m_mutex);

    HANDLE hResult = reinterpret_cast<HANDLE>(m_nextHandle++);
    m_mapHandles[hResult] = move (pHandle);

    return hResult;
}

MockFindHandle * FileSystemMockState::GetHandle (HANDLE hFind)
{
    lock_guard<mutex> lock (m_mutex);

    auto it = m_mapHandles.find (hFind);
    return (it != m_mapHandles.end()) ? it->second.get() : nullptr;
}

void FileSystemMockState::CloseHandle (HANDLE hFind)
{
    lock_guard<mutex> lock (m_mutex);
    m_mapHandles.erase (hFind);
}





////////////////////////////////////////////////////////////////////////////////
//
//  ScopedFileSystemMock
//
////////////////////////////////////////////////////////////////////////////////

ScopedFileSystemMock * ScopedFileSystemMock::s_pInstance = nullptr;

ScopedFileSystemMock::ScopedFileSystemMock (const MockFileTree & tree)
{
    FileSystemMockState::Instance().SetMockTree (&tree);
    s_pInstance = this;

    //
    // Install IAT hooks - patch the test module
    // Get the module handle for this DLL by using an address within it
    //

    HMODULE hThisModule = nullptr;
    GetModuleHandleEx (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCWSTR>(Mock_FindFirstFileW),
                       &hThisModule);

    m_pPatchFindFirst = make_unique<ScopedIatPatch<decltype(&FindFirstFileW)>> (
        hThisModule, "kernel32.dll", "FindFirstFileW", &Mock_FindFirstFileW);

    m_pPatchFindNext = make_unique<ScopedIatPatch<decltype(&FindNextFileW)>> (
        hThisModule, "kernel32.dll", "FindNextFileW", &Mock_FindNextFileW);

    m_pPatchFindClose = make_unique<ScopedIatPatch<decltype(&FindClose)>> (
        hThisModule, "kernel32.dll", "FindClose", &Mock_FindClose);
}

ScopedFileSystemMock::~ScopedFileSystemMock()
{
    s_pInstance = nullptr;
    FileSystemMockState::Instance().SetMockTree (nullptr);
}

bool ScopedFileSystemMock::IsActive() const
{
    return m_pPatchFindFirst && m_pPatchFindFirst->IsPatched();
}

HANDLE WINAPI ScopedFileSystemMock::OriginalFindFirstFileW (LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
    if (s_pInstance && s_pInstance->m_pPatchFindFirst)
    {
        return s_pInstance->m_pPatchFindFirst->Original() (lpFileName, lpFindFileData);
    }
    return FindFirstFileW (lpFileName, lpFindFileData);
}

BOOL WINAPI ScopedFileSystemMock::OriginalFindNextFileW (HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
    if (s_pInstance && s_pInstance->m_pPatchFindNext)
    {
        return s_pInstance->m_pPatchFindNext->Original() (hFindFile, lpFindFileData);
    }
    return FindNextFileW (hFindFile, lpFindFileData);
}

BOOL WINAPI ScopedFileSystemMock::OriginalFindClose (HANDLE hFindFile)
{
    if (s_pInstance && s_pInstance->m_pPatchFindClose)
    {
        return s_pInstance->m_pPatchFindClose->Original() (hFindFile);
    }
    return FindClose (hFindFile);
}





////////////////////////////////////////////////////////////////////////////////
//
//  WildcardMatch
//
//  Simple wildcard matching for * pattern.
//  Supports:
//    - "*" matches all
//    - "*.ext" matches files with extension .ext
//    - "prefix.*" matches files starting with prefix (any extension)
//    - "prefix.*" where prefix has no wildcard
//    - Exact match (case-insensitive)
//
////////////////////////////////////////////////////////////////////////////////

static bool WildcardMatch (const wstring & strPattern, const wstring & strName)
{
    //
    // For now, just handle "*" (match all) and exact match
    //

    if (strPattern == L"*")
    {
        return true;
    }

    //
    // Handle "*.ext" pattern (wildcard at start)
    //

    if (strPattern.length() > 1 && strPattern[0] == L'*' && strPattern[1] == L'.')
    {
        wstring strExt = strPattern.substr (1);  // ".ext"
        if (strName.length() >= strExt.length())
        {
            wstring strNameExt = strName.substr (strName.length() - strExt.length());
            return _wcsicmp (strNameExt.c_str(), strExt.c_str()) == 0;
        }
        return false;
    }

    //
    // Handle "prefix.*" pattern (wildcard at end)
    //

    size_t dotStarPos = strPattern.rfind (L".*");
    if (dotStarPos != wstring::npos && dotStarPos == strPattern.length() - 2)
    {
        //
        // Pattern ends with .* - check if name starts with the prefix
        // and has a dot somewhere after the prefix
        //

        wstring strPrefix = strPattern.substr (0, dotStarPos);
        
        if (strName.length() > strPrefix.length() && strName[strPrefix.length()] == L'.')
        {
            wstring strNamePrefix = strName.substr (0, strPrefix.length());
            return _wcsicmp (strPrefix.c_str(), strNamePrefix.c_str()) == 0;
        }
        return false;
    }

    //
    // Exact match (case-insensitive)
    //

    return _wcsicmp (strPattern.c_str(), strName.c_str()) == 0;
}





////////////////////////////////////////////////////////////////////////////////
//
//  FillFindData
//
//  Populates WIN32_FIND_DATA from MockFileEntry.
//
////////////////////////////////////////////////////////////////////////////////

static void FillFindData (LPWIN32_FIND_DATAW lpFindData, const MockFileEntry & entry)
{
    ZeroMemory (lpFindData, sizeof (WIN32_FIND_DATAW));

    lpFindData->dwFileAttributes = entry.m_dwAttributes;
    lpFindData->ftCreationTime   = entry.m_ftCreation;
    lpFindData->ftLastAccessTime = entry.m_ftLastAccess;
    lpFindData->ftLastWriteTime  = entry.m_ftLastWrite;
    lpFindData->nFileSizeHigh    = entry.m_uliSize.HighPart;
    lpFindData->nFileSizeLow     = entry.m_uliSize.LowPart;

    wcscpy_s (lpFindData->cFileName, entry.m_strName.c_str());
}





////////////////////////////////////////////////////////////////////////////////
//
//  Mock_FindFirstFileW
//
////////////////////////////////////////////////////////////////////////////////

HANDLE WINAPI Mock_FindFirstFileW (LPCWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
    const MockFileTree * pTree = FileSystemMockState::Instance().GetMockTree();

    if (!pTree)
    {
        //
        // No mock active - pass through
        //

        return ScopedFileSystemMock::OriginalFindFirstFileW (lpFileName, lpFindFileData);
    }

    //
    // Parse the path: "C:\Dir\*.txt" -> dir="C:\Dir", pattern="*.txt"
    //

    wstring strFullPath = lpFileName;
    wstring strDir;
    wstring strPattern;

    size_t pos = strFullPath.rfind (L'\\');
    if (pos != wstring::npos)
    {
        strDir     = strFullPath.substr (0, pos);
        strPattern = strFullPath.substr (pos + 1);
    }
    else
    {
        strDir     = L".";
        strPattern = strFullPath;
    }

    //
    // Normalize and look up directory
    //

    wstring strDirUpper = strDir;
    transform (strDirUpper.begin(), strDirUpper.end(), strDirUpper.begin(), towupper);

    const MockDirectoryContents * pContents = pTree->GetDirectoryContents (strDirUpper);

    if (!pContents)
    {
        //
        // Directory not in mock - pass through to real file system
        //

        return ScopedFileSystemMock::OriginalFindFirstFileW (lpFileName, lpFindFileData);
    }

    //
    // Create mock handle and find first matching entry
    //

    auto pHandle = make_unique<MockFindHandle> ();
    pHandle->m_pContents  = pContents;
    pHandle->m_idxCurrent = 0;
    pHandle->m_strPattern = strPattern;

    //
    // Find first matching entry
    //

    while (pHandle->m_idxCurrent < pContents->m_vEntries.size())
    {
        const MockFileEntry & entry = pContents->m_vEntries[pHandle->m_idxCurrent];

        if (WildcardMatch (strPattern, entry.m_strName))
        {
            FillFindData (lpFindFileData, entry);
            pHandle->m_idxCurrent++;

            return FileSystemMockState::Instance().CreateHandle (move (pHandle));
        }

        pHandle->m_idxCurrent++;
    }

    //
    // No matching files
    //

    SetLastError (ERROR_FILE_NOT_FOUND);
    return INVALID_HANDLE_VALUE;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Mock_FindNextFileW
//
////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI Mock_FindNextFileW (HANDLE hFindFile, LPWIN32_FIND_DATAW lpFindFileData)
{
    MockFindHandle * pHandle = FileSystemMockState::Instance().GetHandle (hFindFile);

    if (!pHandle)
    {
        //
        // Not a mock handle - pass through
        //

        return ScopedFileSystemMock::OriginalFindNextFileW (hFindFile, lpFindFileData);
    }

    //
    // Find next matching entry
    //

    while (pHandle->m_idxCurrent < pHandle->m_pContents->m_vEntries.size())
    {
        const MockFileEntry & entry = pHandle->m_pContents->m_vEntries[pHandle->m_idxCurrent];
        pHandle->m_idxCurrent++;

        if (WildcardMatch (pHandle->m_strPattern, entry.m_strName))
        {
            FillFindData (lpFindFileData, entry);
            return TRUE;
        }
    }

    //
    // No more files
    //

    SetLastError (ERROR_NO_MORE_FILES);
    return FALSE;
}





////////////////////////////////////////////////////////////////////////////////
//
//  Mock_FindClose
//
////////////////////////////////////////////////////////////////////////////////

BOOL WINAPI Mock_FindClose (HANDLE hFindFile)
{
    MockFindHandle * pHandle = FileSystemMockState::Instance().GetHandle (hFindFile);

    if (!pHandle)
    {
        //
        // Not a mock handle - pass through
        //

        return ScopedFileSystemMock::OriginalFindClose (hFindFile);
    }

    FileSystemMockState::Instance().CloseHandle (hFindFile);
    return TRUE;
}

