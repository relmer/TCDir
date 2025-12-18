#pragma once

#include "EnvironmentProviderBase.h"





class CEnvironmentProvider : public IEnvironmentProvider
{
public:
	virtual bool TryGetEnvironmentVariable (LPCWSTR pszName, wstring & value) const override;
};



