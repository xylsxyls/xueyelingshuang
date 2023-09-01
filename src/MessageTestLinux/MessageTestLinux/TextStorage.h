#pragma once
#include <memory>
#include <vector>
#include "Semaphore/SemaphoreAPI.h"
#include "FiniteDeque/FiniteDequeAPI.h"
#include "TextMessage.h"

class TextMessage;
class MessageTestLinux;

class TextStorage
{
public:
	TextStorage();

	~TextStorage();

public:
	void push(const std::shared_ptr<TextMessage>& textMessage);

	void clear();

	int32_t allToShow(FiniteDeque<std::shared_ptr<TextMessage>>& showStr, MessageTestLinux* messageTestLinux);

	std::vector<std::string> allToString(bool isShowParam, MessageTestLinux* messageTestLinux);

	std::shared_ptr<TextMessage> next(int32_t& currentIndex, int32_t& layer);

public:
	//threadId, semaphore, TextMessage
	std::vector<std::pair<std::pair<uint32_t, std::shared_ptr<Semaphore>>, std::vector<std::shared_ptr<TextMessage>>>> m_storage;
	int32_t m_currentIndex;
	int32_t m_threadCount;
};