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

	void postSendTaskToThreadPool(uint32_t threadId, const std::shared_ptr<CTask>& spSendTask, int32_t taskLevel);

	void postWorkTaskToThreadPool(const std::shared_ptr<CTask>& spWorkTask);

	uint32_t giveSendThreadId();

	uint32_t getWorkThreadId();

private:
	std::atomic<int32_t> m_sendThreadIdCounter;
	std::vector<uint32_t> m_vecSendThreadId;

	std::atomic<int32_t> m_workThreadIdCounter;
	std::vector<uint32_t> m_vecWorkThreadId;
};