#include "TextStorage.h"
#include "CSystem/CSystemAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "MessageTestLinux.h"
#include "AvailableTask.h"
#include "StringTask.h"
#include "ClearTask.h"

TextStorage::TextStorage():
m_currentIndex(0),
m_threadCount(0)
{
	m_threadCount = CSystem::GetCPUCoreCount() + 1;
	int32_t index = -1;
	while (index++ != m_threadCount - 1)
	{
		m_storage.emplace_back(std::pair<std::pair<uint32_t, std::shared_ptr<Semaphore>>, std::vector<std::shared_ptr<TextMessage>>>());
		m_storage.back().first.first = CTaskThreadManager::Instance().Init();
		m_storage.back().first.second.reset(new Semaphore);
	}
}

TextStorage::~TextStorage()
{
	int32_t index = -1;
	while (index++ != m_threadCount - 1)
	{
		CTaskThreadManager::Instance().Uninit(m_storage[index].first.first);
	}
}

void TextStorage::push(const std::shared_ptr<TextMessage>& textMessage)
{
	m_storage[m_currentIndex].second.push_back(textMessage);
	if (++m_currentIndex == m_threadCount)
	{
		m_currentIndex = 0;
	}
}

void TextStorage::clear()
{
	int32_t index = -1;
	while (index++ != m_threadCount - 1)
	{
		std::shared_ptr<ClearTask> spClearTask(new ClearTask);
		spClearTask->setParam(&(m_storage[index].second), m_storage[index].first.second.get());
		CTaskThreadManager::Instance().GetThreadInterface(m_storage[index].first.first)->PostTask(spClearTask);
	}

	index = -1;
	while (index++ != m_threadCount - 1)
	{
		m_storage[index].first.second->wait();
	}

	m_currentIndex = 0;
}

int32_t TextStorage::allToShow(FiniteDeque<std::shared_ptr<TextMessage>>& showStr, MessageTestLinux* messageTestLinux)
{
	showStr.clear();
	int32_t count = 0;
	int32_t rCurrentIndex = m_currentIndex - 1;
	int32_t layer = m_storage[0].second.size() - 1;

	if (rCurrentIndex == -1)
	{
		rCurrentIndex += m_threadCount;
	}

	std::shared_ptr<TextMessage> currentTextMessage;
	while ((currentTextMessage = next(rCurrentIndex, layer)) != nullptr)
	{
		if (messageTestLinux->textAvailable(currentTextMessage))
		{
			if (++count == showStr.finite())
			{
				showStr.push_back(currentTextMessage);
				break;
			}
			showStr.push_back(currentTextMessage);
		}
	}

	if (currentTextMessage == nullptr)
	{
		return count;
	}

	std::vector<int32_t> vecAvailCount(m_threadCount, 0);

	int32_t selectIndex = -1;
	while (selectIndex++ != rCurrentIndex)
	{
		std::shared_ptr<AvailableTask> spAvailableTask(new AvailableTask);
		spAvailableTask->setParam(messageTestLinux, &(m_storage[selectIndex].second), layer, m_storage[selectIndex].first.second.get(), &(vecAvailCount[selectIndex]));
		CTaskThreadManager::Instance().GetThreadInterface(m_storage[selectIndex].first.first)->PostTask(spAvailableTask);
	}

	selectIndex = rCurrentIndex;
	while (selectIndex++ != m_threadCount - 1)
	{
		std::shared_ptr<AvailableTask> spAvailableTask(new AvailableTask);
		spAvailableTask->setParam(messageTestLinux, &(m_storage[selectIndex].second), layer - 1, m_storage[selectIndex].first.second.get(), &(vecAvailCount[selectIndex]));
		CTaskThreadManager::Instance().GetThreadInterface(m_storage[selectIndex].first.first)->PostTask(spAvailableTask);
	}

	selectIndex = -1;
	while (selectIndex++ != m_threadCount - 1)
	{
		m_storage[selectIndex].first.second->wait();
	}

	int32_t allCount = 0;

	selectIndex = -1;
	while (selectIndex++ != m_threadCount - 1)
	{
		allCount += vecAvailCount[selectIndex];
	}

	return count + allCount;
}

std::vector<std::string> TextStorage::allToString(bool isShowParam, MessageTestLinux* messageTestLinux)
{
	std::vector<std::string> result(m_threadCount, std::string());
	int32_t maxLayer = m_storage[0].second.size();
	int32_t everyLayerCount = maxLayer / m_threadCount;
	int32_t beginLayer = 0;
	int32_t index = -1;
	while (index++ != m_threadCount - 1)
	{
		if (index == m_threadCount - 1)
		{
			std::shared_ptr<StringTask> spStringTask(new StringTask);
			spStringTask->setParam(&(result[index]), beginLayer, maxLayer - everyLayerCount * (m_threadCount - 1), m_currentIndex - 1, isShowParam,
				m_storage[index].first.second.get(), messageTestLinux, this);
			CTaskThreadManager::Instance().GetThreadInterface(m_storage[index].first.first)->PostTask(spStringTask);
			continue;
		}
		std::shared_ptr<StringTask> spStringTask(new StringTask);
		spStringTask->setParam(&(result[index]), beginLayer, everyLayerCount, m_threadCount - 1, isShowParam,
			m_storage[index].first.second.get(), messageTestLinux, this);
		CTaskThreadManager::Instance().GetThreadInterface(m_storage[index].first.first)->PostTask(spStringTask);

		beginLayer += everyLayerCount;
	}

	index = -1;
	while (index++ != m_threadCount - 1)
	{
		m_storage[index].first.second->wait();
	}
	return result;
}

std::shared_ptr<TextMessage> TextStorage::next(int32_t& currentIndex, int32_t& layer)
{
	if (currentIndex < 0)
	{
		currentIndex += m_threadCount;
	}
	if (layer == -1)
	{
		return nullptr;
	}
	std::shared_ptr<TextMessage>& result = m_storage[currentIndex].second[layer];
	--currentIndex;
	if (currentIndex < 0)
	{
		--layer;
		currentIndex += m_threadCount;
	}
	return result;
}
