#pragma once
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

struct PriceInfo
{
	int32_t price;
	int32_t shares;
	char buy_sell;
	PriceInfo() :
		price(0),
		shares(0),
		buy_sell(0)
	{

	}
};

class AnalyzeTask : public CTask
{
public:
	AnalyzeTask();

public:
	void DoTask();

	void StopTask();

	void setParam(const std::string& path, LockFreeQueue<std::vector<int>>* queue);

	std::vector<int> toDayInfo(const std::string& path, const std::map<int, std::vector<PriceInfo>>& priceInfo);

	int32_t currentPrice(int32_t time, const std::map<int, std::vector<PriceInfo>>& priceInfo);

	std::pair<int32_t, int32_t> directPrice(int32_t time, const std::map<int32_t, std::vector<PriceInfo>>& priceInfo);

	std::pair<int32_t, int32_t> bestPrice(const std::map<int32_t, std::pair<int32_t, int32_t>>& timeInfo);

	std::map<int32_t, std::pair<int32_t, int32_t>> calcTimeInfo(int32_t timeBegin, int32_t timeEnd,
		const std::map<int, std::vector<PriceInfo>>& priceInfo);

private:
	std::string m_path;
	LockFreeQueue<std::vector<int>>* m_queue;
	std::atomic<bool> m_exit;
};