#pragma once
#define _StockConditionAPI
#include "StockCondition.h"
#include <stdint.h>

//#ifdef _DEBUG
//#pragma comment(lib,"StockConditiond.lib")
//#else
//#pragma comment(lib,"StockCondition.lib")
//#endif

typedef StockCondition* (*type_stockConditionInterface)();

class StockConditionInterface
{
private:
	StockConditionInterface():
		m_dllHinstance(nullptr),
		m_condition(nullptr)
	{

	}

public:
	static StockConditionInterface& instance()
	{
		static StockConditionInterface s_stockConditionInterface;
		return s_stockConditionInterface;
	}

	void load()
	{
		char szFilePath[1024] = {};
		::GetModuleFileNameA(NULL, szFilePath, 1024);
		std::string path = szFilePath;
		int32_t left = (int32_t)path.find_last_of("/\\");
		std::string name = path.substr(left + 1, path.length() - left - 1);
		int32_t point = (int32_t)name.find_last_of(".");
		std::string exeDllPath = path.substr(0, left + 1) + "StockCondition.dll";
		m_dllHinstance = ::LoadLibraryExA(exeDllPath.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
		type_stockConditionInterface stockConditionInterface = nullptr;
		if (m_dllHinstance != nullptr)
		{
			stockConditionInterface = (type_stockConditionInterface)::GetProcAddress(m_dllHinstance, "StockConditionInterface");
			m_condition = stockConditionInterface();
		}
	}

	StockCondition* condition()
	{
		return m_condition;
	}

	void unload()
	{
		if (m_dllHinstance == nullptr)
		{
			return;
		}
		if (!::FreeLibrary(m_dllHinstance))
		{
			return;
		}
		m_dllHinstance = nullptr;
		m_condition = nullptr;
	}

private:
	HINSTANCE m_dllHinstance;
	StockCondition* m_condition;
};