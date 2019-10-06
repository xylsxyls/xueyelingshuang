#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "BigNumber/BigNumberAPI.h"

class StockEveryTestResultTask : public CTask
{
public:
	StockEveryTestResultTask();

public:
	void DoTask();

	void setParam(const std::string& stock, const BigNumber& fund, BigNumber* allFund, BigNumber* allStockCount);

private:
	std::string m_stock;
	BigNumber m_fund;
	BigNumber* m_allFund;
	BigNumber* m_allStockCount;
};