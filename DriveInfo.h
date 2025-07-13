#pragma once





class CDriveInfo
{
public:
    CDriveInfo (const filesystem::path & dirPath);

    const filesystem::path & GetUncPath()  const { return m_uncPath;                             }
    const filesystem::path & GetRootPath() const { return m_rootPath;                            }
    LPCWSTR         GetVolumeName()        const { return m_szVolumeName;                        }
    UINT            GetVolumeType()        const { return m_nVolumeType;                         }
    const wstring & GetVolumeDescription() const { return s_krgVolumeDescription[m_nVolumeType]; }
    LPCWSTR         GetFileSystemName()    const { return m_szFileSystemName;                    }
    bool            IsUncPath()            const { return m_fUncPath;                            }
    const wstring & GetRemoteName()        const { return m_remoteName;                          }



protected:
    void InitializeVolumeInfo (const filesystem::path & dirPath);
    void InitializeUncInfo    (void);



    filesystem::path m_uncPath;
    filesystem::path m_rootPath;
    WCHAR            m_szVolumeName[MAX_PATH + 1];
    WCHAR            m_szFileSystemName[MAX_PATH + 1];
    UINT             m_nVolumeType;
    bool             m_fUncPath;
    wstring          m_remoteName;



    inline static const wstring s_krgVolumeDescription[] =
    {
        L"an unknown type",
        L"an unknown type",
        L"a removable disk",
        L"a hard drive",
        L"a network drive",
        L"a CD/DVD",
        L"a RAM disk"
    };

};

