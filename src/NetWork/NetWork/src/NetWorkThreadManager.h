#pragma once
#include <stdint.h>
#include <vector>
#include <memory>
#include <atomic>

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

	void postWorkTaskToThreadPool(const std::shared_ptr<CTask>& spWorkTask);

	void postSendTaskToThread(uint32_t threadId, const std::shared_ptr<CTask>& spSendTask);

	uint32_t getWorkThreadId();

private:
	std::atomic<int32_t> m_workThreadIdCounter;
	std::vector<uint32_t> m_vecWorkThreadId;
	std::atomic<uint32_t> m_sendThreadId;
};