#pragma once





class CCommandLine;





class CDirectory
{
public:
    CDirectory  (CCommandLine * pCmdLine); 
    ~CDirectory (void); 

    void operator() (LPCWSTR pszMask);


protected:    

    class CFileInfo : public WIN32_FIND_DATA
    {
    public:
        CFileInfo (void);
        bool operator< (const CFileInfo & rhs) const;

        static CCommandLine * s_pCmdLine;
    };

    typedef vector<CFileInfo>        FileInfoVector;
    typedef FileInfoVector::iterator FileInfoVectorIter;



    class CDirectoryInfo
    {
    public:
        CDirectoryInfo (void);
        
        FileInfoVector m_vMatches;      
        LPCWSTR        m_pszPath;            
        LPCWSTR        m_pszFileSpec;        
        ULARGE_INTEGER m_uliLargestFileSize; 
        size_t         m_cchLargestFileName;
        UINT           m_cFiles;             
        UINT           m_cSubDirectories;    
        ULARGE_INTEGER m_uliBytesUsed;       
    };                                 



    HRESULT ProcessDirectory          (LPCWSTR pszPath, LPCWSTR pszFileSpec);
    HRESULT RecurseIntoSubdirectories (LPCWSTR pszPath, LPCWSTR pszFileSpec);
    HRESULT AddMatchToList            (CFileInfo * pwfd, CDirectoryInfo * pdi);
    void    Scroll                    (CDirectoryInfo * pdi);
    void    DisplayResults            (CDirectoryInfo * pdi);
    void    DisplayResultsWide        (CDirectoryInfo * pdi);
    HRESULT GetColumnInfo             (CDirectoryInfo * pdi, UINT * pcColumns, UINT * pcxColumnWidth);
    HRESULT AddEmptyMatches           (CDirectoryInfo * pdi, size_t cColumns);
    HRESULT GetWideFormattedName      (WIN32_FIND_DATA * pwfd, LPTSTR * ppszName);
    void    DisplayResultsNormal      (CDirectoryInfo * pdi);
    void    DisplayDriveHeader        (LPCWSTR pszPath);
    void    DisplayPathHeader         (LPCWSTR pszPath);
    void    DisplayFooter             (CDirectoryInfo * pdi);
    void    DisplayFooterQuotaInfo    (const ULARGE_INTEGER * puliFreeBytesAvailable);
    UINT    GetMaxSize                (ULARGE_INTEGER * puli);
    LPCWSTR FormatFileSize            (ULARGE_INTEGER uli);


    
    CCommandLine   * m_pCmdLine; 
    ULARGE_INTEGER   m_uliSizeOfAllFilesFound;
    UINT             m_cFilesFound;
    UINT             m_cDirectoriesFound;
};                           






