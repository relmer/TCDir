#include "pch.h"

#include "JsonValue.h"
#include "JsonParser.h"





////////////////////////////////////////////////////////////////////////////////
//
//  JsonValue::Find
//
////////////////////////////////////////////////////////////////////////////////

const JsonValue * JsonValue::Find (string_view key) const
{
    for (const auto & entry : m_object)
    {
        if (entry.first == key)
        {
            return &entry.second;
        }
    }

    return nullptr;
}





////////////////////////////////////////////////////////////////////////////////
//
//  JsonValue::FindObject
//
//  Convenience lookup that returns the nested object for a key, or nullptr if
//  the key is absent or its value is not an object.  Unlike GetObject it does
//  not surface an HRESULT, which suits walk-the-tree call sites.
//
////////////////////////////////////////////////////////////////////////////////

const JsonValue * JsonValue::FindObject (const string & key) const
{
    const JsonValue * value = Find (key);



    if (value == nullptr || value->m_type != JsonType::Object)
    {
        return nullptr;
    }

    return value;
}
