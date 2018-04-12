#include "WaitingBoxManager.h"

WaitingBoxManager& WaitingBoxManager::instance()
{
	static WaitingBoxManager waitingBoxHelper;
	return waitingBoxHelper;
}

void WaitingBoxManager::showWaitingBox(int32_t key,
									   const QString& title,
									   const QString& tip,
									   QWindow* parent,
									   int32_t timeOut,
									   bool userExit)
{
	WaitingBox* waitingBox = nullptr;
	auto itBox = m_mapWaitingBox.find(key);
	if (itBox != m_mapWaitingBox.end())
	{
		waitingBox = itBox->second;
	}

	if (waitingBox == nullptr)
	{
		waitingBox = new WaitingBox;
		if (waitingBox == nullptr)
		{
			return;
		}
		waitingBox->initForWaitingBox(340, 165);
		QObject::connect(waitingBox, &WaitingBox::waitingBoxDone, this, &WaitingBoxManager::onWaitingBoxDone);
	}

	m_mapWaitingBox[key] = waitingBox;
	waitingBox->setCustomerTitleBarHeight(33);
	waitingBox->setParentWindow(parent);
	waitingBox->setTip(tip);
	waitingBox->setTitle(title);
	waitingBox->setTimeRest(timeOut);
	waitingBox->setUserExit(userExit);
	waitingBox->show();
}

void WaitingBoxManager::closeWaitingBox(int32_t key)
{
	auto itBox = m_mapWaitingBox.find(key);
	if (itBox == m_mapWaitingBox.end())
	{
		return;
	}

	auto& waitingBox = itBox->second;
	if (waitingBox == nullptr)
	{
		return;
	}

	waitingBox->exit();
}

void WaitingBoxManager::destroyAll()
{
	for (auto itBox = m_mapWaitingBox.begin(); itBox != m_mapWaitingBox.end(); ++itBox)
	{
		closeWaitingBox(itBox->first);
	}
}

void WaitingBoxManager::onWaitingBoxDone(WaitingBox* box, WaitingBox::ExitEnum exitEnum)
{
	if (box == nullptr)
	{
		return;
	}

	for (auto itBox = m_mapWaitingBox.begin(); itBox != m_mapWaitingBox.end(); ++itBox)
	{
		if (itBox->second == box)
		{
			emit waitingBoxDone(itBox->first, exitEnum);
			auto waitingBox = itBox->second;
			m_mapWaitingBox.erase(itBox);
			waitingBox->deleteLater();
			break;
		}
	}
}
