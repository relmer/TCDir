#pragma once





typedef vector<WIN32_FIND_DATA>  FileInfoVector;
typedef FileInfoVector::iterator FileInfoVectorIter;





class CDirectoryInfo
{
public:
    //
    // Multithreading status enum
    //

    enum class Status
    {
        Waiting,
        InProgress,
        Done,
        Error
    };


    
    CDirectoryInfo (const filesystem::path & dirPath, const filesystem::path & fileSpec) :
        m_dirPath            (dirPath),
        m_fileSpec           (fileSpec),
        m_cchLargestFileName (0),
        m_cFiles             (0),
        m_cSubDirectories    (0),
        m_status             (Status::Waiting),
        m_hr                 (S_OK)
    {
        m_uliLargestFileSize.QuadPart = 0;
        m_uliBytesUsed.QuadPart       = 0;
    }

    

    FileInfoVector                          m_vMatches;
    filesystem::path                        m_dirPath;
    filesystem::path                        m_fileSpec;
    ULARGE_INTEGER                          m_uliLargestFileSize;
    size_t                                  m_cchLargestFileName;
    UINT                                    m_cFiles;
    UINT                                    m_cSubDirectories;
    ULARGE_INTEGER                          m_uliBytesUsed;

    //
    // Multithreading support members (unused in single-threaded mode)
    //

    Status                                  m_status;
    HRESULT                                 m_hr;
    vector<shared_ptr<CDirectoryInfo>>      m_vChildren;
    mutex                                   m_mutex;
    condition_variable                      m_cvStatusChanged;
};
