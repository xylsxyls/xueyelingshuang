#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"

class StockClient;

class GetFilterStockTask : public CTask
{
public:
	GetFilterStockTask();

public:
	void DoTask();

	void setParam(HWND hWnd,
		const IntDateTime& today,
		const std::string& searchStr,
		std::vector<std::string>* vecResult,
		const std::string& fileName,
		bool regain,
		StockClient* stockClient);

private:
	HWND m_hWnd;
	IntDateTime m_today;
	std::string m_searchStr;
	std::vector<std::string>* m_vecResult;
	std::string m_fileName;
	bool m_regain;
	StockClient* m_stockClient;
};