#ifndef _STORAGE_INL__
#define _STORAGE_INL__
#include "Storage.h"

template <typename T>
T Storage::find(int32_t key)
{
	auto itData = m_mapData.find(key);
	if (itData != m_mapData.end())
	{
		return boost::any_cast<T>(itData->second);
	}
	return T();
}

#endif