#include "DialogManager.h"
#include "DialogBase.h"
#include "AskDialog.h"
#include "TipDialog.h"
#include "InputDialog.h"
#include "WaitDialog.h"
#include "TipShowDialog.h"
#include "AskShowDialog.h"
#include "LoginShowDialog.h"
#include "DownloadDialog.h"
#include "DownloadOperateDialog.h"
#include <vector>
#include <Windows.h>

DialogManager& DialogManager::instance()
{
	static DialogManager dialogHelper;
	return dialogHelper;
}

int32_t DialogManager::popAskDialog(int32_t& dialogId,
									const QString& title,
									const QString& tip,
									const QString& acceptText,
									int32_t acceptDone,
									const QString& ignoreText,
									int32_t ignoreDone,
									QWindow* parent,
									int32_t timeOut,
									bool isCountDownVisible)
{
	return AskDialog::popAskDialog(dialogId,
								   title,
								   tip,
								   acceptText,
								   acceptDone,
								   ignoreText,
								   ignoreDone,
								   parent,
								   timeOut,
								   isCountDownVisible);
}

int32_t DialogManager::popTipDialog(int32_t& dialogId,
									const QString& title,
									const QString& tip,
									const QString& buttonText,
									int32_t done,
									QWindow* parent,
									int32_t timeOut,
									bool isCountDownVisible)
{
	return TipDialog::popTipDialog(dialogId, title, tip, buttonText, done, parent, timeOut, isCountDownVisible);
}

int32_t DialogManager::popAskDialog(int32_t& dialogId,
									const QString& tip,
									const QString& title,
									QWindow* parent,
									const QString& acceptText,
									const QString& ignoreText,
									int32_t acceptDone,
									int32_t ignoreDone,
									int32_t timeOut,
									bool isCountDownVisible)
{
	return AskDialog::popAskDialog(dialogId,
								   title,
								   tip,
								   acceptText,
								   acceptDone,
								   ignoreText,
								   ignoreDone,
								   parent,
								   timeOut,
								   isCountDownVisible);
}

int32_t DialogManager::popAskDialog(int32_t& dialogId,
									const QString& tip,
									QWindow* parent,
									const QString& title,
									const QString& acceptText,
									const QString& ignoreText,
									int32_t acceptDone,
									int32_t ignoreDone,
									int32_t timeOut,
									bool isCountDownVisible)
{
	return AskDialog::popAskDialog(dialogId,
								   title,
								   tip,
								   acceptText,
								   acceptDone,
								   ignoreText,
								   ignoreDone,
								   parent,
								   timeOut,
								   isCountDownVisible);
}

int32_t DialogManager::popTipDialog(int32_t& dialogId,
									const QString& tip,
									const QString& title,
									QWindow* parent,
									const QString& buttonText,
									int32_t done,
									int32_t timeOut,
									bool isCountDownVisible)
{
	return TipDialog::popTipDialog(dialogId, title, tip, buttonText, done, parent, timeOut, isCountDownVisible);
}

int32_t DialogManager::popTipDialog(int32_t& dialogId,
									const QString& tip,
									QWindow* parent,
									const QString& title,
									const QString& buttonText,
									int32_t done,
									int32_t timeOut,
									bool isCountDownVisible)
{
	return TipDialog::popTipDialog(dialogId, title, tip, buttonText, done, parent, timeOut, isCountDownVisible);
}

int32_t DialogManager::popInputDialog(int32_t& dialogId,
									  const QString& title,
									  const QString& editTip,
									  const QString& buttonText,
									  int32_t done,
									  QString& editText,
									  int32_t maxLength,
									  QWindow* parent,
									  int32_t timeOut,
									  bool isCountDownVisible)
{
	return InputDialog::popInputDialog(dialogId,
									   title,
									   editTip,
									   buttonText,
									   done,
									   editText,
									   false,
									   maxLength,
									   parent,
									   timeOut,
									   isCountDownVisible);
}

int32_t DialogManager::popInputDialog(int32_t& dialogId,
									  QString& editText,
									  const QString& editTip,
									  bool isPassword,
									  const QString& title,
									  QWindow* parent,
									  int32_t maxLength,
									  const QString& buttonText,
									  int32_t done,
									  int32_t timeOut,
									  bool isCountDownVisible)
{
	return InputDialog::popInputDialog(dialogId,
									   title,
									   editTip,
									   buttonText,
									   done,
									   editText,
									   isPassword,
									   maxLength,
									   parent,
									   timeOut,
									   isCountDownVisible);
}

int32_t DialogManager::popWaitDialog(int32_t& dialogId,
									 const QString& title,
									 const QString& tip,
									 QWindow* parent,
									 int32_t timeOut,
									 bool isCountDownVisible)
{
	return WaitDialog::popWaitDialog(dialogId, title, tip, parent, timeOut, isCountDownVisible);
}

int32_t DialogManager::popDownloadDialog(int32_t& dialogId,
										 const QString& fileName,
										 const QString& tip,
										 QWindow* parent,
										 const QString& title,
										 const QString& buttonText,
										 int32_t done,
										 int32_t timeOut,
										 bool isCountDownVisible)
{
	return DownloadDialog::popDownloadDialog(dialogId, title, fileName, tip, buttonText, done, parent, timeOut, isCountDownVisible);
}

int32_t DialogManager::popDownloadOperateDialog(int32_t& dialogId,
												int32_t taskId,
												const QString& title,
												const QString& fileName,
												const QString& downloadAddr,
												const QString& path,
												QWindow* parent)
{
	return DownloadOperateDialog::popDownloadOperateDialog(dialogId, taskId, title, fileName, downloadAddr, path, parent);
}

void DialogManager::setDownloadRate(int32_t dialogId, int32_t persent)
{
	DownloadDialog* dialogPtr = (DownloadDialog*)DialogManager::instance().dialogPtr(dialogId);
	if (dialogPtr == nullptr)
	{
		return;
	}
	dialogPtr->setRate(persent);
}

void DialogManager::downloadComplete(int32_t dialogId)
{
	DownloadDialog* dialogPtr = (DownloadDialog*)DialogManager::instance().dialogPtr(dialogId);
	if (dialogPtr == nullptr)
	{
		return;
	}
	dialogPtr->complete();
}

void DialogManager::downloadError(int32_t dialogId)
{
	DownloadDialog* dialogPtr = (DownloadDialog*)DialogManager::instance().dialogPtr(dialogId);
	if (dialogPtr == nullptr)
	{
		return;
	}
	dialogPtr->error();
}

void DialogManager::closeDialog(int32_t dialogId)
{
	m_mutex.lock();
	if (m_mapDialog.empty())
	{
		m_mutex.unlock();
		return;
	}
	auto itDialog = m_mapDialog.find(dialogId);
	if (itDialog != m_mapDialog.end())
	{
		auto dialogPtr = itDialog->second;
		m_mutex.unlock();
		if (dialogPtr != nullptr)
		{
			dialogPtr->reject();
		}
		m_mutex.lock();
	}
	m_mutex.unlock();
}

void DialogManager::closeLastDialog()
{
	m_mutex.lock();
	if (m_mapDialog.empty())
	{
		m_mutex.unlock();
		return;
	}
	if (!m_mapDialog.empty())
	{
		auto itDialog = --m_mapDialog.end();
		auto dialogPtr = itDialog->second;
		m_mutex.unlock();
		if (dialogPtr != nullptr)
		{
			dialogPtr->reject();
		}
		m_mutex.lock();
	}
	m_mutex.unlock();
}

void DialogManager::destroyAll()
{
	m_mutex.lock();
	if (m_mapDialog.empty())
	{
		m_mutex.unlock();
		return;
	}

	int32_t count = m_mapDialog.size();
	while (count-- != 0)
	{
		auto itDialog = m_mapDialog.begin();
		if (itDialog != m_mapDialog.end())
		{
			auto dialogPtr = itDialog->second;
			m_mutex.unlock();
			if (dialogPtr != nullptr)
			{
				dialogPtr->done(DESTROY_ALL);
			}
			m_mutex.lock();
		}
	}
	m_mutex.unlock();
}

int32_t DialogManager::dialogCount()
{
	m_mutex.lock();
	int32_t size = m_mapDialog.size();
	m_mutex.unlock();
	return size;
}

int32_t DialogManager::DialogId(DialogBase* base)
{
	m_mutex.lock();
	if (m_mapDialog.empty())
	{
		m_mutex.unlock();
		return -1;
	}
	for (auto itDialog = m_mapDialog.begin(); itDialog != m_mapDialog.end(); ++itDialog)
	{
		if (itDialog->second == base)
		{
			int32_t dialogId = itDialog->first;
			m_mutex.unlock();
			return dialogId;
		}
	}
	m_mutex.unlock();
	return -1;
}

DownloadOperateDialog* DialogManager::downloadOperatePtr(int32_t dialogId)
{
	m_mutex.lock();
	if (m_mapDialog.empty())
	{
		m_mutex.unlock();
		return nullptr;
	}
	auto itDialog = m_mapDialog.find(dialogId);
	if (itDialog == m_mapDialog.end())
	{
		m_mutex.unlock();
		return nullptr;
	}
	DownloadOperateDialog* result = (DownloadOperateDialog*)itDialog->second;
	m_mutex.unlock();
	return result;
}

DownloadOperateDialog* DialogManager::downloadOperateTaskPtr(int32_t taskId)
{
	m_mutex.lock();
	if (m_mapDialog.empty())
	{
		m_mutex.unlock();
		return nullptr;
	}
	for (auto itDialog = m_mapDialog.begin(); itDialog != m_mapDialog.end(); ++itDialog)
	{
		auto dialogPtr = itDialog->second;
		if (dialogPtr != nullptr)
		{
			if (dialogPtr->dialogEnum() == DOWNLOAD_OPERATE_DIALOG)
			{
				if (((DownloadOperateDialog*)dialogPtr)->getTaskId() == taskId)
				{
					m_mutex.unlock();
					return (DownloadOperateDialog*)dialogPtr;
				}
			}
		}
	}
	m_mutex.unlock();
	return nullptr;
}

DialogManager::DialogManager() :
m_id(0)
{

}

int32_t DialogManager::setDialog(DialogBase* dialog)
{
	if (dialog == nullptr)
	{
		return -1;
	}
	int32_t id = getId();
	m_mutex.lock();
	m_mapDialog[id] = dialog;
	m_mutex.unlock();
	return id;
}

void DialogManager::removeDialog(int32_t dialogId)
{
	m_mutex.lock();
	if (m_mapDialog.empty())
	{
		m_mutex.unlock();
		return;
	}
	auto itDialog = m_mapDialog.find(dialogId);
	if (itDialog != m_mapDialog.end())
	{
		m_mapDialog.erase(itDialog);
	}
	m_mutex.unlock();
}

void DialogManager::removeDialog(DialogBase* base)
{
	m_mutex.lock();
	if (m_mapDialog.empty())
	{
		m_mutex.unlock();
		return;
	}
	for (auto itDialog = m_mapDialog.begin(); itDialog != m_mapDialog.end(); ++itDialog)
	{
		if (itDialog->second == base)
		{
			m_mapDialog.erase(itDialog);
			m_mutex.unlock();
			return;
		}
	}
	m_mutex.unlock();
}

DialogBase* DialogManager::dialogPtr(int32_t dialogId)
{
	m_mutex.lock();
	if (m_mapDialog.empty())
	{
		m_mutex.unlock();
		return nullptr;
	}

	auto itDialog = m_mapDialog.find(dialogId);
	if (itDialog != m_mapDialog.end())
	{
		return itDialog->second;
	}
	m_mutex.unlock();
	return nullptr;
}

int32_t DialogManager::getId()
{
	return ++m_id;
}