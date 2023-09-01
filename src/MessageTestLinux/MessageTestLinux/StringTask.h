#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <string>
#include <atomic>

class Semaphore;
class MessageTestLinux;
class TextStorage;

class StringTask : public CTask
{
public:
	StringTask();

	~StringTask();

public:
	void DoTask();

	void StopTask();

	void setParam(std::string* result, int32_t beginLayer, int32_t layerCount, int32_t rCurrentIndex, bool isShowParam,
		Semaphore* semaphore, MessageTestLinux* messageTestLinux, TextStorage* textStorage);

private:
	std::string* m_result;
	int32_t m_beginLayer;
	int32_t m_layerCount;
	int32_t m_rCurrentIndex;
	bool m_isShowParam;
	Semaphore* m_semaphore;
	MessageTestLinux* m_messageTestLinux;
	TextStorage* m_textStorage;
	std::atomic<bool> m_exit;
};