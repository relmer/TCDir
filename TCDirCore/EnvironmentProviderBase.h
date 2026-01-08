#pragma once





struct IEnvironmentProvider
{
    virtual ~IEnvironmentProvider () = default;
    
    virtual bool TryGetEnvironmentVariable (LPCWSTR pszName, wstring & value) const = 0;
};
