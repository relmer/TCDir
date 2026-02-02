#pragma once





////////////////////////////////////////////////////////////////////////////////
//
//  SFileAttributeMap
//
//  Maps a file attribute constant to its display character.
//  Shared between Config.cpp and ResultsDisplayerNormal.cpp.
//
////////////////////////////////////////////////////////////////////////////////

struct SFileAttributeMap
{
    DWORD   m_dwAttribute;
    wchar_t m_chKey;
};

static constexpr SFileAttributeMap k_rgFileAttributeMap[] =
{
    {  FILE_ATTRIBUTE_READONLY,      L'R' },
    {  FILE_ATTRIBUTE_HIDDEN,        L'H' },
    {  FILE_ATTRIBUTE_SYSTEM,        L'S' },
    {  FILE_ATTRIBUTE_ARCHIVE,       L'A' },
    {  FILE_ATTRIBUTE_TEMPORARY,     L'T' },
    {  FILE_ATTRIBUTE_ENCRYPTED,     L'E' },
    {  FILE_ATTRIBUTE_COMPRESSED,    L'C' },
    {  FILE_ATTRIBUTE_REPARSE_POINT, L'P' },
    {  FILE_ATTRIBUTE_SPARSE_FILE,   L'0' },
};

