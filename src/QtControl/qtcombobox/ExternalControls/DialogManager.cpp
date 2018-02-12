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

void DialogManager::closeDialog(int32_t dialogId)
{
	if (m_mapDialog.empty())
	{
		return;
	}
	auto itDialog = m_mapDialog.find(dialogId);
	if (itDialog != m_mapDialog.end())
	{
		itDialog->second->reject();
	}
}

void DialogManager::closeLastDialog()
{
	if (m_mapDialog.empty())
	{
		return;
	}
	if (!m_mapDialog.empty())
	{
		auto itDialog = --m_mapDialog.end();
		itDialog->second->reject();
	}
}

void DialogManager::destroyAll()
{
	if (m_mapDialog.empty())
	{
		return;
	}

	int32_t count = m_mapDialog.size();
	while (count-- != 0)
	{
		auto itDialog = m_mapDialog.begin();
		if (itDialog != m_mapDialog.end())
		{
			itDialog->second->done(DESTROY_ALL);
		}
	}
}

int32_t DialogManager::dialogCount()
{
	return m_mapDialog.size();
}

int32_t DialogManager::DialogId(DialogBase* base)
{
	if (m_mapDialog.empty())
	{
		return -1;
	}
	for (auto itDialog = m_mapDialog.begin(); itDialog != m_mapDialog.end(); ++itDialog)
	{
		if (itDialog->second == base)
		{
			return itDialog->first;
		}
	}
	return -1;
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
	m_mapDialog[id] = dialog;
	return id;
}

void DialogManager::removeDialog(int32_t dialogId)
{
	if (m_mapDialog.empty())
	{
		return;
	}
	auto itDialog = m_mapDialog.find(dialogId);
	if (itDialog != m_mapDialog.end())
	{
		m_mapDialog.erase(itDialog);
	}
}

void DialogManager::removeDialog(DialogBase* base)
{
	if (m_mapDialog.empty())
	{
		return;
	}
	for (auto itDialog = m_mapDialog.begin(); itDialog != m_mapDialog.end(); ++itDialog)
	{
		if (itDialog->second == base)
		{
			m_mapDialog.erase(itDialog);
			return;
		}
	}
}

DialogBase* DialogManager::dialogPtr(int32_t dialogId)
{
	if (m_mapDialog.empty())
	{
		nullptr;
	}

	auto itDialog = m_mapDialog.find(dialogId);
	if (itDialog != m_mapDialog.end())
	{
		return itDialog->second;
	}
	return nullptr;
}

int32_t DialogManager::getId()
{
	return ++m_id;
}

