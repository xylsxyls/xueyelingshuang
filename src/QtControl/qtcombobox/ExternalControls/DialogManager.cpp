#include "DialogManager.h"
#include "DialogBase.h"
#include "AskDialog.h"
#include "TipDialog.h"
#include "InputDialog.h"
#include "WaitDialog.h"

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
									QDialog* parent,
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
									QDialog* parent,
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
									  QDialog* parent,
									  int32_t timeOut,
									  bool isCountDownVisible)
{
	return InputDialog::popInputDialog(dialogId,
									   title,
									   editTip,
									   buttonText,
									   done,
									   editText,
									   parent,
									   timeOut,
									   isCountDownVisible);
}

int32_t DialogManager::popWaitDialog(int32_t& dialogId,
									 const QString& title,
									 const QString& tip,
									 QDialog* parent,
									 int32_t timeOut,
									 bool isCountDownVisible)
{
	return WaitDialog::popWaitDialog(dialogId, title, tip, parent, timeOut, isCountDownVisible);
}

void DialogManager::destroyDialog(int32_t dialogId)
{
	auto itDialog = m_mapDialog.find(dialogId);
	if (itDialog != m_mapDialog.end())
	{
		itDialog->second->reject();
		m_mapDialog.erase(itDialog);
	}
}

void DialogManager::destroyLastDialog()
{
	if (!m_mapDialog.empty())
	{
		auto itDialog = --m_mapDialog.end();
		itDialog->second->reject();
		m_mapDialog.erase(itDialog);
	}
}

void DialogManager::destroyAll()
{
	for (auto itDialog = m_mapDialog.begin(); itDialog != m_mapDialog.end(); ++itDialog)
	{
		itDialog->second->reject();
	}
	m_mapDialog.clear();
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
	auto itDialog = m_mapDialog.find(dialogId);
	if (itDialog != m_mapDialog.end())
	{
		m_mapDialog.erase(itDialog);
	}
}

int32_t DialogManager::getId()
{
	return ++m_id;
}

