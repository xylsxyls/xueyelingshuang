#pragma once
#include "StorageMacro.h"
#include "boost/any.hpp"
#include <stdint.h>
#include <unordered_map>

#define add(key, anyValue) insert(key, anyValue, typeid(anyValue).name())

class StorageAPI Storage
{
public:
	void insert(int32_t key, const boost::any& anyValue, const std::string& type = "");
	template <typename T>
	T find(int32_t key);
	std::string type(int32_t key);
	void remove(int32_t key);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::unordered_map<int32_t, boost::any> m_mapData;
	std::unordered_map<int32_t, std::string> m_mapType;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

#include "Storage.inl"