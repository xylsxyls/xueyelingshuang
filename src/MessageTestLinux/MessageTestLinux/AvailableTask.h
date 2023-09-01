#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <memory>
#include <vector>
#include <atomic>

class TextMessage;
class MessageTestLinux;
class Semaphore;

class AvailableTask : public CTask
{
public:
	AvailableTask();

	~AvailableTask();

public:
	void DoTask();

	void StopTask();

	void setParam(MessageTestLinux* messageTestLinux, std::vector<std::shared_ptr<TextMessage>>* vecTextMessage,
		int32_t layer, Semaphore* semaphore, int32_t* availCount);

private:
	MessageTestLinux* m_messageTestLinux;
	std::vector<std::shared_ptr<TextMessage>>* m_vecTextMessage;
	int32_t m_layer;
	Semaphore* m_semaphore;
	int32_t* m_availCount;
	std::atomic<bool> m_exit;
};