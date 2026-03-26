#pragma once

struct SAliasConfig;





////////////////////////////////////////////////////////////////////////////////
//
//  CAliasBlockGenerator
//
//  Generates the PowerShell alias function block from an SAliasConfig.
//
////////////////////////////////////////////////////////////////////////////////

class CAliasBlockGenerator
{
public:
    static void Generate (const SAliasConfig & config, const wstring & strVersion, vector<wstring> & rgBlockLines);
};
