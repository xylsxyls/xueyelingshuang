#pragma once
#include <stdint.h>
#include <vector>
#include <memory>

class CTask;

class NetWorkThreadManager
{
private:
	NetWorkThreadManager();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static NetWorkThreadManager& instance();

public:
	void init(int32_t coreCount);

	void postSendTaskToThreadPool(std::shared_ptr<CTask> spSendTask);

	void postWorkTaskToThreadPool(std::shared_ptr<CTask> spWorkTask);

	uint32_t getSendThreadId();

	uint32_t getWorkThreadId();

private:
	int32_t m_sendThreadIdCounter;
	std::vector<uint32_t> m_vecSendThreadId;

	int32_t m_workThreadIdCounter;
	std::vector<uint32_t> m_vecWorkThreadId;
};