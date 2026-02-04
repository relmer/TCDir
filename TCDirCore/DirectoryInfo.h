#pragma once





////////////////////////////////////////////////////////////////////////////////
//
//  SStreamInfo
//
//  Information about an alternate data stream
//
////////////////////////////////////////////////////////////////////////////////

struct SStreamInfo
{
    wstring        m_strName;       // Stream name (e.g., ":hidden", already stripped of :$DATA suffix)
    LARGE_INTEGER  m_liSize = {};   // Stream size
};





////////////////////////////////////////////////////////////////////////////////
//
//  FileInfo
//
//  Extended file information that includes WIN32_FIND_DATA plus streams.
//  Derives from WIN32_FIND_DATA so it can be used interchangeably.
//
////////////////////////////////////////////////////////////////////////////////

struct FileInfo : public WIN32_FIND_DATA
{
    // Default constructor
    FileInfo () : 
        WIN32_FIND_DATA {} 
    {
    }
    
    // Construct from WIN32_FIND_DATA
    FileInfo (const WIN32_FIND_DATA & wfd) : 
        WIN32_FIND_DATA (wfd) 
    {
    }

    vector<SStreamInfo> m_vStreams;   // Alternate data streams (empty if none or not collected)
};

typedef vector<FileInfo>         FileInfoVector;
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
        m_cStreams           (0),
        m_status             (Status::Waiting),
        m_hr                 (S_OK)
    {
        m_uliLargestFileSize.QuadPart = 0;
        m_uliBytesUsed.QuadPart       = 0;
        m_uliStreamBytesUsed.QuadPart = 0;
    }

    

    FileInfoVector                          m_vMatches;
    filesystem::path                        m_dirPath;
    filesystem::path                        m_fileSpec;
    ULARGE_INTEGER                          m_uliLargestFileSize;
    size_t                                  m_cchLargestFileName;
    UINT                                    m_cFiles;
    UINT                                    m_cSubDirectories;
    UINT                                    m_cStreams;
    ULARGE_INTEGER                          m_uliBytesUsed;
    ULARGE_INTEGER                          m_uliStreamBytesUsed;

    //
    // Multithreading support members (unused in single-threaded mode)
    //

    Status                                  m_status;
    HRESULT                                 m_hr;
    vector<shared_ptr<CDirectoryInfo>>      m_vChildren;
    mutex                                   m_mutex;
    condition_variable                      m_cvStatusChanged;
};
