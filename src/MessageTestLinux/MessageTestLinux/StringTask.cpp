#include "StringTask.h"
#include "MessageTestLinux.h"
#include "TextStorage.h"

StringTask::StringTask():
m_result(nullptr),
m_beginLayer(0),
m_layerCount(0),
m_rCurrentIndex(0),
m_isShowParam(false),
m_messageTestLinux(nullptr),
m_textStorage(nullptr),
m_exit(false)
{

}

StringTask::~StringTask()
{
	m_semaphore->signal();
}

void StringTask::DoTask()
{
	if (m_layerCount == 0)
	{
		return;
	}

	int32_t rCurrentIndex = m_rCurrentIndex;
	int32_t layer = m_beginLayer + m_layerCount - 1;
	int32_t allSize = 0;
	std::vector<int32_t> vecId;
	std::shared_ptr<TextMessage> currentTextMessage;
	while ((currentTextMessage = m_textStorage->next(rCurrentIndex, layer)) != nullptr && !m_exit)
	{
		if (m_messageTestLinux->textAvailable(currentTextMessage))
		{
			allSize += (currentTextMessage->m_text.size() + (int32_t)m_isShowParam * 16);
			vecId.push_back(currentTextMessage->m_id);
		}
		
		if (layer < m_beginLayer)
		{
			break;
		}
	}

	if (allSize == 0 || m_exit)
	{
		return;
	}

	m_result->reserve(allSize);

	rCurrentIndex = m_rCurrentIndex;
	layer = m_beginLayer + m_layerCount - 1;
	int32_t idIndex = 0;
	while ((currentTextMessage = m_textStorage->next(rCurrentIndex, layer)) != nullptr && !m_exit)
	{
		if (currentTextMessage->m_id == vecId[idIndex])
		{
			++idIndex;
			m_result->append(m_messageTestLinux->textToString(currentTextMessage, m_isShowParam));
		}

		if (idIndex == vecId.size())
		{
			break;
		}
	}
}

void StringTask::StopTask()
{
	m_exit = true;
}

void StringTask::setParam(std::string* result, int32_t beginLayer, int32_t layerCount, int32_t rCurrentIndex, bool isShowParam,
	Semaphore* semaphore, MessageTestLinux* messageTestLinux, TextStorage* textStorage)
{
	m_result = result;
	m_beginLayer = beginLayer;
	m_layerCount = layerCount;
	m_rCurrentIndex = rCurrentIndex;
	m_isShowParam = isShowParam;
	m_semaphore = semaphore;
	m_messageTestLinux = messageTestLinux;
	m_textStorage = textStorage;
}
