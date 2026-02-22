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
        m_dirPath    (dirPath),
        m_vFileSpecs ({ fileSpec })
    {
    }

    CDirectoryInfo (const filesystem::path & dirPath, const vector<filesystem::path> & fileSpecs) :
        m_dirPath    (dirPath),
        m_vFileSpecs (fileSpecs)
    {
    }

    

    FileInfoVector                          m_vMatches;
    filesystem::path                        m_dirPath;
    vector<filesystem::path>                m_vFileSpecs;    // File specs to match (one or more)
    ULARGE_INTEGER                          m_uliLargestFileSize = {};
    size_t                                  m_cchLargestFileName = 0;
    UINT                                    m_cFiles             = 0;
    UINT                                    m_cSubDirectories    = 0;
    UINT                                    m_cStreams           = 0;
    ULARGE_INTEGER                          m_uliBytesUsed       = {};
    ULARGE_INTEGER                          m_uliStreamBytesUsed = {};

    //
    // Multithreading support members (unused in single-threaded mode)
    //

    Status                                  m_status = Status::Waiting;
    HRESULT                                 m_hr     = S_OK;
    vector<shared_ptr<CDirectoryInfo>>      m_vChildren;
    mutex                                   m_mutex;
    condition_variable                      m_cvStatusChanged;

    //
    // Tree-pruning support (used only when tree mode + file mask is active).
    // Producer threads propagate match/completion signals upward through the
    // parent chain; the display thread waits on m_cvStatusChanged until
    // visibility is determined.  See research.md R14.
    //

    weak_ptr<CDirectoryInfo>                m_wpParent;
    atomic<bool>                            m_fDescendantMatchFound { false };
    atomic<bool>                            m_fSubtreeComplete      { false };
};
