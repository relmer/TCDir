#pragma once





struct FindHandleDeleter 
{
    void operator() (HANDLE h) const 
    {
        if (h && h != INVALID_HANDLE_VALUE) 
        {
            FindClose (h);
        }
    }
};





using UniqueFindHandle = std::unique_ptr<std::remove_pointer<HANDLE>::type, FindHandleDeleter>;
